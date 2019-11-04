import os.path as op
from collections import OrderedDict
from datetime import datetime
from itertools import cycle

from PyQt5 import uic
from PyQt5.QtCore import QDateTime, QTimer
from PyQt5.QtWidgets import QVBoxLayout, QWidget
from traits.api import Bool, Dict, Instance, Set, String

from karabo.common.scenemodel.api import (
    build_graph_config, restore_graph_config, StateGraphModel)
from karabogui.binding.api import StringBinding
from karabogui.graph.common.const import (
    STATE_INTEGER_MAP, MIN_STATE_INT, MAX_STATE_INT)
from karabogui.graph.common.enums import AxisType
from karabogui.controllers.api import (
    BaseBindingController, Curve, get_start_end_date_time, ONE_DAY, ONE_HOUR,
    ONE_WEEK, register_binding_controller, with_display_type,
    TEN_MINUTES, UPTIME)
from karabogui.globals import MAX_INT32
from karabogui.graph.common.colors import get_pen_cycler
from karabogui.graph.plots.base import KaraboPlotView

# NOTE: We limit ourselves to selected karabo actions!
ALLOWED_ACTIONS = ['x_grid', 'y_grid', 'y_invert', 'y_log', 'axes']


@register_binding_controller(
    ui_name='State Graph', klassname='DisplayStateGraph',
    binding_type=StringBinding,
    is_compatible=with_display_type('State'),
    can_show_nothing=False)
class DisplayStateGraph(BaseBindingController):
    # The scene model class used by this controller
    model = Instance(StateGraphModel, args=())

    _plot = Instance(KaraboPlotView)
    _timer = Instance(QTimer)
    _start_time = Instance(QDateTime)

    # Holds if the user is rescaling via mouse interaction
    _auto_scale = Bool(False)

    _curves = Dict
    _curves_start = Set

    _pens = Instance(cycle, factory=get_pen_cycler, args=())

    # Map time strs to QPushButtons
    _x_axis_str_btns = Instance(OrderedDict, args=())
    _x_detail = String(UPTIME)

    def create_widget(self, parent):
        widget = self._init_ui(parent)

        self._start_time = QDateTime.currentDateTime()

        self._plot = KaraboPlotView(axis=AxisType.State,
                                    actions=ALLOWED_ACTIONS,
                                    parent=widget.time_frame)
        self._plot.stateChanged.connect(self._change_model)
        self._plot.add_legend(visible=False)
        self._plot.add_cross_target()
        self._plot.add_toolbar()
        self._plot.enable_data_toggle()

        # Limit the panning zoom
        self._plot.plotItem.setLimits(
            xMin=datetime(1970, 12, 31).timestamp(),
            xMax=datetime(2038, 12, 31).timestamp())

        # Limit the panning zoom
        self._plot.plotItem.setLimits(yMin=MIN_STATE_INT, yMax=MAX_STATE_INT)

        self._plot.plotItem.vb.sigRangeChangedManually.connect(
            self._on_range_manually_changed)

        self._plot.plotItem.vb.autoRangeTriggered.connect(
            self._uncheck_current_button)

        # Update datetime widgets everytime range changes
        self._plot.plotItem.sigXRangeChanged.connect(
            self._update_datetime_widgets)

        self._plot.restore(build_graph_config(self.model))

        layout = QVBoxLayout()
        layout.addWidget(self._plot)
        widget.time_frame.setLayout(layout)

        # have a 1s timeout to request data, thus avoid frequent re-loading
        # while scaling
        self._timer = QTimer(parent)
        self._timer.setInterval(1000)
        self._timer.setSingleShot(True)
        self._timer.timeout.connect(self._update_intervals)

        # Add the first curve
        self.add_proxy(self.proxy)

        widget.addActions(self._plot.actions())

        return widget

    def _init_ui(self, parent):
        """Setup all widgets correctly"""
        widget = QWidget(parent)
        uic.loadUi(op.join(op.dirname(__file__), "ui_trendline.ui"), widget)

        current_date_time = QDateTime.currentDateTime()
        widget.dt_start.setDateTime(current_date_time)
        widget.dt_end.setDateTime(current_date_time)

        # Init x-axis buttons
        self._x_axis_str_btns[widget.bt_one_week] = ONE_WEEK
        self._x_axis_str_btns[widget.bt_one_day] = ONE_DAY
        self._x_axis_str_btns[widget.bt_one_hour] = ONE_HOUR
        self._x_axis_str_btns[widget.bt_ten_minutes] = TEN_MINUTES
        self._x_axis_str_btns[widget.bt_uptime] = UPTIME
        widget.bg_x_axis.buttonClicked.connect(self._x_axis_btns_toggled)

        return widget

    def add_proxy(self, proxy):
        if proxy in self._curves:
            return

        curve = self._plot.add_curve_item(name=proxy.key, pen=next(self._pens))
        curve.sigPlotChanged.connect(self._update_ranges)

        self._curves[proxy] = Curve(proxy=proxy, curve=curve)
        self._curves_start.add(proxy)

        if len(self._curves) > 1:
            self._plot.set_legend(True)

        return True

    def value_update(self, proxy):
        if self.widget is None:
            return

        timestamp = proxy.binding.timestamp
        t = timestamp.toTimestamp()

        value = STATE_INTEGER_MAP[proxy.value]
        self._curves[proxy].add_point(value, t)
        if proxy in self._curves_start:
            self._curves[proxy].add_point(value,
                                          self._start_time.toTime_t())
            self._curves_start.remove(proxy)

    def set_time_interval(self, t0, t1):
        """Update the x axis scale interval of the curves"""
        for v in self._curves.values():
            v.changeInterval(t0, t1)

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

    def _change_model(self, content):
        self.model.trait_set(**restore_graph_config(content))

    def _update_intervals(self):
        """This slot is called whenever the timer timed out and previously the
        x axis scale was changed"""
        x_axis = self._plot.plotItem.getAxis("bottom")
        self.set_time_interval(*x_axis.range)

    def _update_datetime_widgets(self, vb, x_range):
        """This slot is called whenever the xRange changes"""
        # Note that the time comes in seconds
        x_min, x_max = x_range
        start = QDateTime.fromMSecsSinceEpoch(x_min * 1000)
        end = QDateTime.fromMSecsSinceEpoch(x_max * 1000)

        # We change our QDateTime widgets
        self.widget.dt_start.setDateTime(start)
        self.widget.dt_end.setDateTime(end)

    def _x_axis_btns_toggled(self, button):
        """Update the x axis scale when a time button is clicked"""
        self._x_detail = self._x_axis_str_btns[button]
        # We're updating the ranges via the buttons now
        self._auto_scale = True
        if self._update_axis_scale():
            self.update_later()

    def deferred_update(self):
        self._update_ranges()

    def _update_axis_scale(self):
        """The start and end time date for the x axis is updated here
        depending on the selected time button"""
        if not self._x_detail:
            return False

        if self._x_detail != UPTIME:
            # Schedule an update for the intervals
            start_secs, end_secs = self._get_start_end_date_secs()
            self.set_time_interval(start_secs, end_secs)

        return True

    def _update_ranges(self):
        """Updates both x and y axes ranges but only in case the ranges
        are not regulated via mouse"""
        if not self._auto_scale:
            return

        ymin = MAX_INT32
        ymax = -ymin
        for curve in self._curves.values():
            min_value = curve.get_min_y_value()
            max_value = curve.get_max_y_value()

            if min_value < ymin:
                ymin = min_value
            if max_value > ymax:
                ymax = max_value

        y_range = (ymin, ymax)
        x_range = self._get_start_end_date_secs()

        self._plot.plotItem.setRange(xRange=x_range, yRange=y_range)

    def _get_start_end_date_secs(self):
        """Returns the start and end date based on the selected button"""
        if self._x_detail == UPTIME:
            start = self._start_time
            end = self._get_last_timestamp()
        else:
            start, end = get_start_end_date_time(self._x_detail)

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
