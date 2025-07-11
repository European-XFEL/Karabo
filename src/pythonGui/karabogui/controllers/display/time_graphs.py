# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
import datetime
from collections import OrderedDict
from itertools import cycle
from weakref import WeakValueDictionary

from qtpy import uic
from qtpy.QtCore import QDateTime, QTimer
from qtpy.QtGui import QColor
from qtpy.QtWidgets import QAction, QDialog, QVBoxLayout, QWidget
from traits.api import Bool, Dict, Instance, Int, Set, String, WeakRef

from karabo.common.scenemodel.api import (
    KARABO_CURVE_OPTIONS, AlarmGraphModel, CurveType, StateGraphModel,
    TrendGraphModel, build_graph_config, extract_graph_curve_option,
    restore_graph_config)
from karabogui import icons
from karabogui.binding.api import (
    BoolBinding, FloatBinding, IntBinding, StringBinding)
from karabogui.controllers.api import (
    HIDDEN, ONE_DAY, ONE_HOUR, ONE_WEEK, TEN_MINUTES, UPTIME,
    BaseBindingController, Curve, get_start_end_date_time,
    register_binding_controller, with_display_type)
from karabogui.dialogs.api import RequestTimeDialog
from karabogui.graph.common.api import AxisType, create_button, get_pen_cycler
from karabogui.graph.common.const import ALARM_INTEGER_MAP, STATE_INTEGER_MAP
from karabogui.graph.plots.base import KaraboPlotView
from karabogui.graph.plots.dialogs import CurveOptionsDialog
from karabogui.graph.plots.utils import create_curve_options

from .util import get_ui_file

# NOTE: We limit ourselves to selected karabo actions!
BASE_ACTIONS = ["x_grid", "y_grid", "y_invert", "y_log", "axes", "view"]
TIME_ACTIONS = BASE_ACTIONS + ["y_range"]

MIN_TIMESTAMP = datetime.datetime(1970, 1, 31).timestamp()
MAX_TIMESTAMP = datetime.datetime(2038, 12, 31).timestamp()


def curve_to_actions(axis_type):
    """Return the allowed actions for the curve type"""
    return TIME_ACTIONS if axis_type is AxisType.Time else BASE_ACTIONS


def curve_to_axis(value):
    """Return the axis type of the current curve configuration"""
    return [AxisType.Time, AxisType.State, AxisType.Alarm][value]


class BaseSeriesGraph(BaseBindingController):
    """The BaseClass for all trendline controllers"""
    _plot = WeakRef(KaraboPlotView)
    _timer = Instance(QTimer)
    _start_time = Instance(QDateTime)

    # the plots from pyqtgraph
    _curves = Instance(WeakValueDictionary, args=())

    # Holds if the user is rescaling via mouse interaction
    _button_scale = Bool(False)
    _curve_points = Dict
    _curves_start = Set
    _pens = Instance(cycle, factory=get_pen_cycler, args=())

    # Map time strs to QPushButtons
    _x_axis_buttons = Instance(OrderedDict, args=())
    _x_detail = String(UPTIME)

    # Our type of curve
    curve_type = Int(0)

    _window_action = Instance(QAction)
    _window = Int(1)

    def create_widget(self, parent):
        """Setup all widgets correctly"""
        widget = QWidget(parent)
        uic.loadUi(get_ui_file("ui_trendline.ui"), widget)
        self._start_time = QDateTime.currentDateTime()

        widget.dt_start.setDateTime(self._start_time)
        widget.dt_end.setDateTime(self._start_time)

        # Init x-axis buttons
        self._x_axis_buttons[widget.bt_one_week] = ONE_WEEK
        self._x_axis_buttons[widget.bt_one_day] = ONE_DAY
        self._x_axis_buttons[widget.bt_one_hour] = ONE_HOUR
        self._x_axis_buttons[widget.bt_ten_minutes] = TEN_MINUTES
        self._x_axis_buttons[widget.bt_window] = HIDDEN
        self._x_axis_buttons[widget.bt_uptime] = UPTIME
        # Hide the hidden button for window mode
        widget.bt_window.setVisible(False)
        widget.bg_x_axis.buttonClicked.connect(self._x_axis_btns_toggled)

        axis_type = curve_to_axis(self.curve_type)
        actions = curve_to_actions(axis_type)
        self._plot = KaraboPlotView(axis=axis_type,
                                    actions=actions,
                                    parent=widget.time_frame)
        self._plot.stateChanged.connect(self._change_model)
        self._plot.add_legend(visible=False)
        self._plot.add_cross_target()
        toolbar = self._plot.add_toolbar()
        self._plot.enable_export()
        self._plot.enable_data_toggle(activate=True)

        _btn_reset = create_button(
            checkable=False,
            icon=icons.reset,
            tooltip="Reload the view and request historic data",
            on_clicked=self._purge_curves)
        toolbar.add_button(_btn_reset)

        # Restore previous configuration!
        self._plot.restore(build_graph_config(self.model))

        viewbox = self._plot.plotItem.vb
        viewbox.autorange_enabled = False
        viewbox.sigRangeChangedManually.connect(self._range_change_manually)
        viewbox.middleButtonClicked.connect(self._autorange_requested)

        dialog_ac = QAction(icons.clock, "Request Time", parent=widget)
        dialog_ac.triggered.connect(self._request_dialog)
        viewbox.add_action(dialog_ac, separator=False)

        window_ac = QAction("Window Mode", parent=widget)
        window_ac.triggered.connect(widget.bt_window.click)
        window_ac.setCheckable(True)
        viewbox.add_action(window_ac, separator=False)
        self._window_action = window_ac

        # Update datetime widgets everytime range changes and limit zoom
        self._plot.plotItem.setLimits(xMin=MIN_TIMESTAMP, xMax=MAX_TIMESTAMP)
        self._plot.plotItem.sigXRangeChanged.connect(self._update_date_widgets)

        layout = QVBoxLayout()
        layout.setSizeConstraint(QVBoxLayout.SetNoConstraint)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(self._plot)
        widget.time_frame.setLayout(layout)

        # XXX: Have a 1s timeout to request data, thus avoid frequent
        # re-loading while scaling
        self._timer = QTimer(parent)
        self._timer.setInterval(1000)
        self._timer.setSingleShot(True)
        self._timer.timeout.connect(self._update_intervals)

        # Add the first curve
        self.add_proxy(self.proxy)

        # Add the plot actions to the general widget
        widget.addActions(self._plot.actions())

        # Finally, add the plot options
        curve_options = QAction("Curve Options", widget)
        curve_options.setIcon(icons.settings)
        curve_options.triggered.connect(self.show_options_dialog)
        widget.addAction(curve_options)

        return widget

    def add_proxy(self, proxy):
        if proxy in self._curve_points:
            return

        name = proxy.key
        pen = next(self._pens)
        options = extract_graph_curve_option(self.model, name)
        if options:
            pen_color = options["pen_color"]
            pen.setColor(QColor(pen_color))
            name = options["name"]

        curve = self._plot.add_curve_item(
            name=name, pen=pen, connect="all")
        curve.sigPlotChanged.connect(self._update_x_range)

        self._curves[proxy] = curve
        self._curve_points[proxy] = Curve(
            proxy=proxy, curve=curve,
            curve_type=self.curve_type)
        self._curves_start.add(proxy)

        if len(self._curves) > 1:
            self._plot.set_legend(True)

        return True

    def remove_proxy(self, proxy):
        """Remove an additional proxy curve from the controller"""
        options = [option for option in self.model.curve_options
                   if option.key != proxy.key]
        self.model.trait_set(curve_options=options)

        self._curves.pop(proxy)
        self._curves_start.discard(proxy)
        curve = self._curve_points.pop(proxy)
        item = curve.curve
        self._plot.remove_item(item)
        item.sigPlotChanged.disconnect()
        item.deleteLater()
        legend_visible = len(self._curves) > 1

        self._plot.set_legend(legend_visible)

        return True

    def value_update(self, proxy):
        raise NotImplementedError

    def destroy_widget(self):
        """Remove all curves and their trait handlers"""
        self._curve_points = {}
        self._curves.clear()

    # ----------------------------------------------------------------
    # Plot Options

    def show_options_dialog(self):
        config = build_graph_config(self.model)
        options = config.get(KARABO_CURVE_OPTIONS, {})

        # Use model information to create a new dict[curve, option]
        curve_options = create_curve_options(
            curves=self._curves, options=options,
            curve_type=CurveType.Trend)

        dialog = CurveOptionsDialog(curve_options, parent=self.widget)
        if dialog.exec() != QDialog.Accepted:
            return

        curve_options = dialog.get_curve_options()
        if curve_options is None:
            self.reset_curve_options()
            return

        if curve_options:
            self.set_curve_options(curve_options)

    def set_curve_options(self, curve_options: dict):
        """Method to set the curve options on the model and plot"""
        self._change_model(
            {KARABO_CURVE_OPTIONS: list(curve_options.values())})
        self._plot.apply_curve_options(curve_options)

    def reset_curve_options(self):
        """Restore the default curvce options."""
        self.reset_traits(["_pens"])
        options = {}
        for proxy, curve in self._curves.items():
            pen = next(self._pens)
            options[curve] = {
                "name": proxy.key,
                "curve_type": CurveType.Trend,
                "pen_color": pen.color().name()}

        self.model.curve_options = []
        self._plot.apply_curve_options(options)

    # ----------------------------------------------------------------
    # PyQt Slots

    def _request_dialog(self):
        """Set a new time interval and request historic data"""
        x_axis = self._plot.plotItem.getAxis("bottom")
        x_min, x_max = x_axis.range
        start = QDateTime.fromSecsSinceEpoch(int(x_min))
        end = QDateTime.fromSecsSinceEpoch(int(x_max))

        dialog = RequestTimeDialog(start=start, end=end, parent=self.widget)
        if dialog.exec() == QDialog.Accepted:
            # Convert again to set the interval
            self._button_scale = False
            self._uncheck_button()
            start, end = dialog.get_start_and_end_time()
            self.set_time_interval(start, end, force=True)
            # Remove padding to avoid showing many values outside the
            # selected time-range on X-axis
            self._plot.plotItem.setRange(xRange=(start, end), padding=0)

    def _purge_curves(self):
        """Purge all curves and request historic again"""
        for v in self._curve_points.values():
            v.purge()
        x_axis = self._plot.plotItem.getAxis("bottom")
        self.set_time_interval(*x_axis.range, force=True)

    def _range_change_manually(self):
        """When the range changes manually, we stop automatically
        updating the ranges and start the timer"""
        self._timer.start()
        self._button_scale = False
        self._uncheck_button()

    def _autorange_requested(self):
        self._button_scale = False
        self._uncheck_button()
        self._plot.reset_range()

    def _change_model(self, content):
        self.model.trait_set(**restore_graph_config(content))

    def _update_intervals(self):
        """This slot is called whenever the timer timed out and previously the
        x axis scale was changed"""
        x_axis = self._plot.plotItem.getAxis("bottom")
        self.set_time_interval(*x_axis.range)

    def _update_date_widgets(self, vb, x_range):
        """This slot is called whenever the xRange changes"""
        # Note that the range comes in seconds (float)
        x_min, x_max = x_range
        start = QDateTime.fromSecsSinceEpoch(int(x_min))
        end = QDateTime.fromSecsSinceEpoch(int(x_max))

        # We change our QDateTime widgets
        self.widget.dt_start.setDateTime(start)
        self.widget.dt_end.setDateTime(end)

    def _x_axis_btns_toggled(self, button):
        """Update the x axis scale when a time button is clicked"""
        self._x_detail = self._x_axis_buttons[button]
        window_mode = self._x_detail == HIDDEN
        if window_mode:
            x_axis = self._plot.plotItem.getAxis("bottom")
            self._window = round(max(1, (x_axis.range[1] - x_axis.range[0])))
        self._window_action.setChecked(window_mode)
        # We're updating the ranges via the buttons now
        self._button_scale = True
        if self._update_axis_scale():
            self.update_later()

    # ----------------------------------------------------------------

    def deferred_update(self):
        self._update_x_range()

    def _uncheck_button(self):
        """Uncheck any checked button and the window mode"""
        self._window_action.setChecked(False)
        button_group = self.widget.bg_x_axis
        checked_button = button_group.checkedButton()
        if checked_button is not None:
            # Little workaround as we can't uncheck exclusive button group
            button_group.setExclusive(False)
            checked_button.setChecked(False)
            button_group.setExclusive(True)

    def _update_axis_scale(self):
        """The start and end time date for the x axis is updated here
        depending on the selected time button"""
        if not self._x_detail:
            return False

        if self._x_detail not in (UPTIME, HIDDEN):
            # Schedule an update for the intervals
            start_secs, end_secs = self._get_start_end_date_secs()
            # On button update we always trigger!
            self.set_time_interval(start_secs, end_secs, force=True)

        return True

    def _update_x_range(self):
        """Update x-range only in case the ranges are not regulated via mouse
        """
        if not self._button_scale:
            return

        x_range = self._get_start_end_date_secs()

        # From a deferred call the plot might have been deleted!
        if self._plot is not None:
            self._plot.plotItem.setRange(xRange=x_range)

    def _get_start_end_date_secs(self):
        """Returns the start and end date based on the selected button"""
        if self._x_detail == UPTIME:
            start = self._start_time
            end = self._get_last_timestamp()
        elif self._x_detail == HIDDEN:
            end = self._get_last_timestamp()
            start = end.addSecs(-self._window)
        else:
            start, end = get_start_end_date_time(self._x_detail)

        return start.toMSecsSinceEpoch() / 1000, end.toMSecsSinceEpoch() / 1000

    def _get_last_timestamp(self):
        """Returns the last timestamp set on the curves"""
        timestamps = []
        for curve in self._curve_points.values():
            if not curve.fill:
                continue
            timestamps.append(curve.get_max_timepoint())

        if not timestamps:
            return QDateTime.currentDateTime()
        else:
            # Preserve resolution with `fromMSecsSinceEpoch`
            # Qt has deprecated `QDateTime.fromTime_t`
            return QDateTime.fromMSecsSinceEpoch(int(max(timestamps) * 1000))

    def _draw_start_time(self, proxy, value, timestamp):
        if proxy in self._curves_start:
            self._curves_start.remove(proxy)
            start = self._start_time.toTime_t()
            if start > timestamp:
                self._curve_points[proxy].add_point(value, start)

    def set_time_interval(self, t0, t1, force=False):
        """Update the x axis scale interval of the curves"""
        for v in self._curve_points.values():
            v.changeInterval(t0, t1, force=force)


@register_binding_controller(
    ui_name="Trend Graph", klassname="DisplayTrendGraph",
    binding_type=(BoolBinding, FloatBinding, IntBinding),
    can_show_nothing=False)
class DisplayTrendGraph(BaseSeriesGraph):
    model = Instance(TrendGraphModel, args=())
    curve_type = Int(0)

    def value_update(self, proxy):
        if self.widget is None:
            return

        timestamp = proxy.binding.timestamp
        ts = timestamp.toTimestamp()
        value = proxy.value
        self._curve_points[proxy].add_point(proxy.value, ts)
        self._draw_start_time(proxy, value, timestamp=ts)


@register_binding_controller(
    ui_name="State Graph", klassname="DisplayStateGraph",
    binding_type=StringBinding,
    is_compatible=with_display_type("State"),
    can_show_nothing=False)
class DisplayStateGraph(BaseSeriesGraph):
    model = Instance(StateGraphModel, args=())
    curve_type = Int(1)

    def value_update(self, proxy):
        if self.widget is None:
            return

        timestamp = proxy.binding.timestamp
        ts = timestamp.toTimestamp()

        value = STATE_INTEGER_MAP[proxy.value]
        self._curve_points[proxy].add_point(value, ts)
        self._draw_start_time(proxy, value, timestamp=ts)


@register_binding_controller(
    ui_name="Alarm Graph", klassname="DisplayAlarmGraph",
    binding_type=StringBinding,
    is_compatible=with_display_type("AlarmCondition"),
    can_show_nothing=False)
class DisplayAlarmGraph(BaseSeriesGraph):
    model = Instance(AlarmGraphModel, args=())
    curve_type = Int(2)

    def value_update(self, proxy):
        if self.widget is None:
            return

        timestamp = proxy.binding.timestamp
        ts = timestamp.toTimestamp()

        value = ALARM_INTEGER_MAP[proxy.value]
        self._curve_points[proxy].add_point(value, ts)
        self._draw_start_time(proxy, value, timestamp=ts)
