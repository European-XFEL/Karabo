import os.path as op
from collections import OrderedDict
from datetime import datetime
from itertools import cycle

from PyQt4 import uic
from PyQt4.QtCore import QDateTime, pyqtSlot, QTimer
from PyQt4.QtGui import (
    QVBoxLayout, QWidget)
from traits.api import Bool, Dict, Instance, Set, String

from karabo.common.scenemodel.api import (
    build_graph_config, restore_graph_config, TrendGraphModel)
from karabogui.binding.api import (
    BoolBinding, FloatBinding, IntBinding)
from karabogui.const import MAX_NUMBER_LIMIT
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.globals import MAX_INT32
from karabogui.graph.common.colors import get_pen_cycler
from karabogui.graph.plots.base import KaraboPlotView
import karabogui.controllers.display.trendline as trendline

# NOTE: We limit ourselves to selected karabo actions!
ALLOWED_ACTIONS = ['x_grid', 'y_grid', 'y_invert', 'y_log', 'axes']


@register_binding_controller(
    ui_name='Trend Graph', klassname='DisplayTrendGraph',
    binding_type=(BoolBinding, FloatBinding, IntBinding),
    can_show_nothing=False)
class DisplayTrendGraph(BaseBindingController):
    # The scene model class used by this controller
    model = Instance(TrendGraphModel, args=())

    _karabo_plot_view = Instance(KaraboPlotView)
    _timer = Instance(QTimer)
    _initial_start_time = Instance(QDateTime)

    # Holds if the user is rescaling via mouse interaction
    _auto_scale = Bool(False)

    _curves = Dict
    _curves_start = Set

    _pens = Instance(cycle, factory=get_pen_cycler, args=())

    # Map time strs to QPushButtons
    _x_axis_str_btns = Instance(OrderedDict, args=())
    _x_detail = String(trendline.UPTIME)

    def create_widget(self, parent):
        widget = self._init_ui(parent)

        self._initial_start_time = QDateTime.currentDateTime()

        self._karabo_plot_view = KaraboPlotView(use_time_axis=True,
                                                actions=ALLOWED_ACTIONS,
                                                parent=widget.fr_trendline)
        self._karabo_plot_view.stateChanged.connect(self._change_model)
        self._karabo_plot_view.add_legend(visible=False)
        self._karabo_plot_view.add_cross_target()
        self._karabo_plot_view.add_toolbar()

        # Limit the panning zoom
        self._karabo_plot_view.plotItem.setLimits(
            xMin=datetime(1970, 12, 31).timestamp(),
            xMax=datetime(2038, 12, 31).timestamp())

        self._karabo_plot_view.plotItem.vb.sigRangeChangedManually.connect(
            self._on_range_manually_changed)

        self._karabo_plot_view.plotItem.vb.autoRangeTriggered.connect(
            self._uncheck_current_button)

        # Update datetime widgets everytime range changes
        self._karabo_plot_view.plotItem.sigXRangeChanged.connect(
            self._update_datetime_widgets)

        # Restore any past configurations
        self._karabo_plot_view.restore(build_graph_config(self.model))
        self._karabo_plot_view.enable_data_toggle()

        layout = QVBoxLayout()
        layout.addWidget(self._karabo_plot_view)
        widget.fr_trendline.setLayout(layout)

        # have a 1s timeout to request data, thus avoid frequent re-loading
        # while scaling
        self._timer = QTimer(parent)
        self._timer.setInterval(1000)
        self._timer.setSingleShot(True)
        self._timer.timeout.connect(self._update_intervals)

        # Add the first curve
        self.add_proxy(self.proxy)

        widget.addActions(self._karabo_plot_view.actions())

        return widget

    def _init_ui(self, parent):
        """Setup all widgets correctly"""
        widget = QWidget(parent)
        uic.loadUi(op.join(op.dirname(__file__), "ui_trendline.ui"), widget)

        current_date_time = QDateTime.currentDateTime()
        widget.dt_start.setDateTime(current_date_time)
        widget.dt_end.setDateTime(current_date_time)

        # Init x-axis buttons
        self._x_axis_str_btns[widget.bt_one_week] = trendline.ONE_WEEK
        self._x_axis_str_btns[widget.bt_one_day] = trendline.ONE_DAY
        self._x_axis_str_btns[widget.bt_one_hour] = trendline.ONE_HOUR
        self._x_axis_str_btns[widget.bt_ten_minutes] = trendline.TEN_MINUTES
        self._x_axis_str_btns[widget.bt_uptime] = trendline.UPTIME
        widget.bg_x_axis.buttonClicked.connect(self._x_axis_btns_toggled)

        return widget

    def add_proxy(self, proxy):
        if proxy in self._curves:
            return

        curve = self._karabo_plot_view.add_curve_item(name=proxy.key,
                                                      pen=next(self._pens))
        curve.sigPlotChanged.connect(self._update_ranges)

        self._curves[proxy] = trendline.Curve(proxy=proxy, curve=curve)
        self._curves_start.add(proxy)

        if len(self._curves) > 1:
            self._karabo_plot_view.set_legend(True)

        return True

    def value_update(self, proxy):
        if self.widget is None or abs(proxy.value) >= MAX_NUMBER_LIMIT:
            return

        timestamp = proxy.binding.timestamp
        t = timestamp.toTimestamp()

        self._curves[proxy].add_point(proxy.value, t)
        if proxy in self._curves_start:
            self._curves[proxy].add_point(proxy.value,
                                          self._initial_start_time.toTime_t())
            self._curves_start.remove(proxy)

    def set_interval(self, t0, t1):
        """Update lower and upper bound of curve intervals"""
        for v in self._curves.values():
            v.changeInterval(t0, t1)

    @pyqtSlot()
    def _on_range_manually_changed(self):
        """When the range changes manually, we stop automatically
        updating the ranges and start the timer"""
        self._timer.start()
        self._auto_scale = False
        self._uncheck_current_button()

    def _uncheck_current_button(self):
        """Uncheck any checked button"""
        button_group = self.widget.bg_x_axis
        checked_button = button_group.checkedButton()
        if checked_button is not None:
            # Little workaround as we can't uncheck exclusive button group
            button_group.setExclusive(False)
            checked_button.setChecked(False)
            button_group.setExclusive(True)

    @pyqtSlot()
    def _update_intervals(self):
        """This slot is called whenever the timer timed out and previously the
        x axis scale was changed"""
        x_axis = self._karabo_plot_view.plotItem.getAxis("bottom")
        self.set_interval(*x_axis.range)

    def _update_datetime_widgets(self, vb, x_range):
        # Note that the time comes in seconds
        x_min, x_max = x_range
        start = QDateTime.fromMSecsSinceEpoch(x_min * 1000)
        end = QDateTime.fromMSecsSinceEpoch(x_max * 1000)

        # We change our QDateTime widgets
        self.widget.dt_start.setDateTime(start)
        self.widget.dt_end.setDateTime(end)

    @pyqtSlot(object)
    def _x_axis_btns_toggled(self, button):
        """Update the x axis scale when a time button is clicked"""
        self._x_detail = self._x_axis_str_btns[button]
        # We're updating the ranges via the buttons now
        self._auto_scale = True
        if self._update_axis_scale():
            self.update_later()

    def _update_axis_scale(self):
        """The start and end time date for the x axis is updated here
        depending on the selected time button"""
        if not self._x_detail:
            return False

        if self._x_detail == trendline.UPTIME:
            # We don't need to wait to update
            self._update_ranges()
        else:
            # Schedule an update for the intervals
            start_secs, end_secs = self._get_start_end_date_secs()
            self.set_interval(start_secs, end_secs)

        return True

    def _update_ranges(self):
        """Updates both x and y axes ranges but only in case the ranges
        are not regulated via mouse"""
        if not self._auto_scale:
            return

        ymin = MAX_INT32
        ymax = -ymin
        for curve in self._curves.values():
            has_historic_data = self._x_detail != trendline.UPTIME

            min_value = curve.get_min_y_value(full_range=has_historic_data)
            max_value = curve.get_max_y_value(full_range=has_historic_data)

            if min_value < ymin:
                ymin = min_value
            if max_value > ymax:
                ymax = max_value

        y_range = (ymin, ymax)
        x_range = self._get_start_end_date_secs()

        self._karabo_plot_view.plotItem.setRange(xRange=x_range,
                                                 yRange=y_range)

    def _get_start_end_date_secs(self):
        """Returns the start and end date based on the selected button"""
        if self._x_detail == trendline.UPTIME:
            start = self._initial_start_time
            end = self._get_last_timestamp()
        else:
            start, end = trendline.get_start_end_date_time(self._x_detail)

        return start.toMSecsSinceEpoch() / 1000, end.toMSecsSinceEpoch() / 1000

    def _get_last_timestamp(self):
        """Returns the last timestamp set on the curves"""
        timestamps = []
        for curve in self._curves.values():
            if not curve.fill:
                continue
            timestamps.append(curve.get_max_x_value())

        if not timestamps:
            return QDateTime.currentDateTime()
        else:
            return QDateTime.fromTime_t(max(timestamps))

    @pyqtSlot(object)
    def _change_model(self, content):
        self.model.trait_set(**restore_graph_config(content))
