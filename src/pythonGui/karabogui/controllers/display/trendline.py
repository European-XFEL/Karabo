import time
import datetime
from bisect import bisect
from collections import OrderedDict
from itertools import cycle, product
import os.path as op

from guiqwt.plot import CurveWidget, PlotManager
from guiqwt.tools import SelectPointTool
from guiqwt.builder import make
import numpy
from PyQt4 import uic
from PyQt4.QtCore import Qt, QDateTime, QTimer, pyqtSignal, pyqtSlot
from PyQt4.QtGui import (QButtonGroup, QDateTimeEdit, QDialog, QHBoxLayout,
                         QIntValidator, QLabel, QLineEdit, QPushButton,
                         QVBoxLayout, QWidget)
from PyQt4.Qwt5.Qwt import (QwtPlot, QwtScaleDraw, QwtText,
                            QwtLinearScaleEngine, QwtScaleDiv)
from traits.api import (
    HasStrictTraits, Array, Constant, Dict, Float, Instance, Int, List,
    on_trait_change
)

from karabo.common.scenemodel.api import LinePlotModel
from karabo.middlelayer import Timestamp
from karabogui import globals as krb_globals
from karabogui.binding.api import (
    BoolBinding, FloatBinding, IntBinding, PropertyProxy
)
from karabogui.controllers.api import (
    BaseBindingController, axis_label, register_binding_controller)
from karabogui.util import SignalBlocker

X_AXIS = "_x_axis_btns"
Y_AXIS = "_y_axis_btns"

ONE_WEEK = "One Week"
ONE_DAY = "One Day"
ONE_HOUR = "One Hour"
TEN_MINUTES = "Ten Minutes"
RESET = "Uptime"
HIDDEN = "Hidden"

FULL_RANGE = "Full Range"
DETAIL_RANGE = "Detail"

STYLE = "{text-align: center; font-size: 9px; padding: 0}"
BUTTON_STYLE_SHEET = ("QPushButton {}".format(STYLE))
DATETIMEEDIT_STYLE_SHEET = ("QDateTimeEdit {}".format(STYLE))
LINEEDIT_STYLE_SHEET = ("QLineEdit {}".format(STYLE))
CURVE_COLORS = ['b', 'orange', 'k', 'g', 'pink', 'brown']
CURVE_STYLES = ['-', '--', ':', '-.']

PLOTTABLE_TYPES = (BoolBinding, FloatBinding, IntBinding)


def get_start_end_date_time(selected_time_span):
    """ Return beginning and end date time for given ``selected_time_span``.
        If the ``selected_time_span`` is not supported ``None`` is returned.
    """
    current_date_time = QDateTime.currentDateTime()
    if selected_time_span == ONE_WEEK:
        # One week
        start_date_time = current_date_time.addDays(-7)
    elif selected_time_span == ONE_DAY:
        # One day
        start_date_time = current_date_time.addDays(-1)
    elif selected_time_span == ONE_HOUR:
        # One hour
        start_date_time = current_date_time.addSecs(-3600)
    elif selected_time_span == TEN_MINUTES:
        # Ten minutes
        start_date_time = current_date_time.addSecs(-600)
    else:
        return None, None

    return start_date_time, current_date_time


class _Generation(object):
    """ This holds a single generation of a Curve's data.
    """
    size = 200
    base = 10  # number of points to combine per generation

    def __init__(self):
        self.fill = 0
        self.xs = numpy.empty(self.size, dtype=numpy.float)
        self.ys = numpy.empty(self.size, dtype=numpy.float)

    def add_point(self, x, y):
        self.xs[self.fill] = x
        self.ys[self.fill] = y
        self.fill += 1

        if self.fill == self.size:
            return self.reduce_data()
        return None

    def reduce_data(self):
        x = self.xs[:self.base].mean()
        y = self.ys[:self.base].mean()
        self.xs[:-self.base] = self.xs[self.base:]
        self.ys[:-self.base] = self.ys[self.base:]
        self.fill -= self.base
        return x, y


class Curve(HasStrictTraits):
    """This holds the data for one curve

    The currently to be shown data is in self.x and self.y.
    Up to self.histsize it is filled with historical data, up to self.fill
    it is then filled with "current" data, meaning data that has been
    accumulated from the changes coming in.

    There is a second data structure, self.generations, which is a list of
    _Generation objects containing averaged data, always `base` points are
    averaged over and put into the next higher aggregated storage.

    Once the basic storage in self.x and self.y flows over, it gets replaced
    by the averaged data in self.generations.
    """
    curve = Instance(object)  # some Qwt bullshit
    proxy = Instance(PropertyProxy)
    generations = List(Instance(_Generation))

    histsize = Int(0)
    fill = Int(0)

    x = Array(dtype=numpy.float)
    y = Array(dtype=numpy.float)

    t0 = Float(0)
    t1 = Float(0)

    spare = Constant(100)
    # Limits amount of data from past
    maxHistory = Constant(500)
    sparsesize = Constant(400)
    # minimum number of points shown (if possible)
    minHistory = Constant(100)

    def _generations_default(self):
        return [_Generation() for i in range(4)]

    def _x_default(self):
        arraysize = self.spare + sum([g.size for g in self.generations], 0)
        return numpy.empty(arraysize, dtype=numpy.float)

    def _y_default(self):
        arraysize = self.spare + sum([g.size for g in self.generations], 0)
        return numpy.empty(arraysize, dtype=numpy.float)

    def add_point(self, value, timestamp):
        # Fill the generations data, possibly propagating averaged values
        point = (timestamp, value)
        for gen in reversed(self.generations):
            point = gen.add_point(*point)
            if point is None:
                break

        # Fill the main data buffer
        self.x[self.fill] = timestamp
        self.y[self.fill] = value
        self.fill += 1
        # When the main buffer fills up, copy the generations data
        if self.fill == len(self.x):
            self.fill_current()
        self.update()

    def fill_current(self):
        pos = self.histsize
        for gen in self.generations:
            fill = gen.fill
            if fill == 0:
                continue
            self.x[pos:pos + fill] = gen.xs[:fill]
            self.y[pos:pos + fill] = gen.ys[:fill]
            pos += fill
        self.fill = pos

    def get_property_history(self, t0, t1):
        # Avoid if not currently visible
        if not self.proxy.visible:
            return
        t0 = str(datetime.datetime.utcfromtimestamp(t0).isoformat())
        t1 = str(datetime.datetime.utcfromtimestamp(t1).isoformat())
        self.proxy.get_history(t0, t1, max_value_count=self.maxHistory)

    def changeInterval(self, t0, t1):
        p0 = self.x[:self.fill].searchsorted(t0)
        p1 = self.x[:self.fill].searchsorted(t1)

        not_enough_data = (p1 - p0) < self.minHistory
        no_data = self.histsize == 0
        zoomed_out = self.histsize > self.sparsesize
        nearly_left_border = 0.9 * self.t0 + 0.1 * self.t1
        nearly_right_border = 0.1 * self.t0 + 0.9 * self.t1

        # Request history only needs to be done under certain circumstances
        if (no_data or (not_enough_data and zoomed_out) or
                (self.x[p0] > nearly_left_border) or
                (p1 < self.histsize and self.x[p1 - 1] < nearly_right_border)):
            self.get_property_history(t0, t1)
        self.t0 = t0
        self.t1 = t1

    def update(self):
        """ Show the new data on screen """
        self.curve.set_data(self.x[:self.fill], self.y[:self.fill])

    @on_trait_change('proxy:visible')
    def _visibility_update(self, visible):
        if visible and self.t1 >= self.x[self.histsize]:
            self.get_property_history(self.t0, self.t1)

    @on_trait_change('proxy:binding:historic_data')
    def _historic_data_arrival(self, data):
        if not data:
            return

        datasize = len(data)
        gensize = sum([g.size for g in self.generations], 0)
        arraysize = datasize + gensize + self.spare
        x = numpy.empty(arraysize, dtype=numpy.float)
        y = numpy.empty(arraysize, dtype=numpy.float)

        for i, d in enumerate(data):
            x[i] = Timestamp.fromHashAttributes(d['v', ...]).toTimestamp()
            y[i] = d["v"]

        p0 = self.x[:self.fill].searchsorted(self.t0)
        p1 = self.x[:self.fill].searchsorted(self.t1)
        np0 = x[:datasize].searchsorted(self.t0)
        np1 = x[:datasize].searchsorted(self.t1)

        if self.t1 == self.t0:
            # Prevent division by 0, otherwise self.curve.plot() is None
            return

        span = (self.x[p1 - 1] - self.x[p0]) / (self.t1 - self.t0)
        nspan = (x[np1 - 1] - x[np0]) / (self.t1 - self.t0)

        if (np1 - np0 < p1 - p0) and not nspan > span < 0.9:
            return

        # If the history overlaps generation data, favor the history data.
        end = x[datasize - 1]
        for gen in self.generations:
            fill = gen.fill
            if fill == 0:
                continue
            pos = gen.xs[:fill].searchsorted(end)
            if pos == 0:
                break
            gen.xs[:fill - pos] = gen.xs[pos:fill]
            gen.fill = fill - pos

        self.histsize = datasize
        self.x = x
        self.y = y
        self.fill_current()
        self.update()
        self.curve.plot().replot()

    def get_mean_y_value(self, count=10):
        """ Return mean value for last ``count`` of y values."""
        if count > len(self.y):
            count = len(self.y)
        return numpy.mean(self.y[max(self.fill-count, 0):self.fill])

    def get_min_y_value(self):
        """ Return min value of all y values."""
        return min(self.y[:self.fill])

    def get_max_y_value(self):
        """ Return max value for all y values"""
        return max(self.y[:self.fill])


class DateTimeScaleDraw(QwtScaleDraw):
        '''Class used to draw a datetime axis on our plot. '''
        formats = ((60, "%Y-%m-%d %H:%M", "%Ss"),
                   (60 * 60, "%Y-%m-%d", "%H:%M"),
                   (60 * 60 * 24, "%Y-%m-%d", "%Hh"),
                   (60 * 60 * 24 * 7, "%b %Y", "%d"),
                   (60 * 60 * 24 * 30, "%Y", "%d/%m"))

        def setFormat(self, start, ss):
            self.start = start
            for s, maj, min in self.formats:
                if ss < s:
                    break
            self.major = maj
            self.minor = min

        def label(self, value):
            '''create the text of each label to draw the axis. '''
            if value == self.start:
                fmt = self.minor + "\n" + self.major
            else:
                fmt = self.minor
            try:
                dt = datetime.datetime.fromtimestamp(value)
            except (TypeError, ValueError):
                dt = datetime.datetime.fromtimestamp(0)
            return QwtText(dt.strftime(fmt))


class ScaleEngine(QwtLinearScaleEngine):
    def __init__(self, drawer):
        super().__init__()
        self.drawer = drawer

    def divideScale(self, x1, x2, maxMajorSteps, maxMinorSteps, stepSize):
        ss = (x2 - x1) / maxMajorSteps
        a = [1, 2, 5, 10, 20, 60,
             60 * 2, 60 * 5, 60 * 10, 60 * 30, 60 * 60,
             3600 * 2, 3600 * 4, 3600 * 8, 3600 * 12, 3600 * 24,
             3600 * 24 * 2, 3600 * 24 * 4, 3600 * 24 * 7]
        pos = bisect(a, ss)
        if pos == len(a):
            v = a[-1]
            while v < ss:
                v *= 2
        else:
            v = a[pos]
        start = int(x1 // v + 1) * v
        self.drawer.setFormat(start, v)
        return QwtScaleDiv(x1, x2, [], [], list(range(start, int(x2), v)))


class Timespan(QDialog):
    def __init__(self, parent):
        super(Timespan, self).__init__(parent)
        uic.loadUi(op.join(op.dirname(__file__), "timespan.ui"), self)

    def _set_beginning_end_date_time(self, selected_time_span):
        start_dt, end_dt = get_start_end_date_time(selected_time_span)
        self.dt_beginning.setDateTime(start_dt)
        self._dt_end.setDateTime(end_dt)

    def on_pb_one_week_clicked(self):
        self._set_beginning_end_date_time(ONE_WEEK)

    def on_pb_one_day_clicked(self):
        self._set_beginning_end_date_time(ONE_DAY)

    def on_pb_one_hour_clicked(self):
        self._set_beginning_end_date_time(ONE_HOUR)

    def on_pb_ten_minutes_clicked(self):
        self._set_beginning_end_date_time(TEN_MINUTES)


class _KaraboCurveWidget(CurveWidget):
    signal_mouse_event = pyqtSignal()

    def __init__(self, **kwargs):
        """Possible key arguments:
            * parent: parent widget
            * title: plot title
            * xlabel: (bottom axis title, top axis title) or bottom axis
                      title only
            * ylabel: (left axis title, right axis title) or left axis
                      title only
            * xunit: (bottom axis unit, top axis unit) or bottom axis unit only
            * yunit: (left axis unit, right axis unit) or left axis unit only
            * panels (optional): additionnal panels (list, tuple)
        """
        super(_KaraboCurveWidget, self).__init__(**kwargs)

    def mousePressEvent(self, event):
        if event.button() in (Qt.LeftButton, Qt.MidButton, Qt.RightButton):
            self.signal_mouse_event.emit()
        super(_KaraboCurveWidget, self).mousePressEvent(event)


@register_binding_controller(ui_name='Trendline', klassname='DisplayTrendline',
                             binding_type=PLOTTABLE_TYPES,
                             can_show_nothing=False)
class DisplayTrendline(BaseBindingController):
    # The scene model class used by this controller
    model = Instance(LinePlotModel)

    _initial_start_time = Instance(QDateTime)
    _plot = Instance(object)  # some Qwt bullshit
    _curve_widget = Instance(_KaraboCurveWidget)
    _dt_start = Instance(QDateTimeEdit)
    _dt_end = Instance(QDateTimeEdit)
    _le_detail_range = Instance(QLineEdit)

    _manager = Instance(PlotManager)
    _timer = Instance(QTimer)
    _curves = Dict
    _lasttime = Float
    _curve_count = Int(0)

    # Map time strs to QPushButtons
    _x_axis_str_btns = Instance(OrderedDict, args=())
    _y_axis_str_btns = Instance(OrderedDict, args=())
    _sel_x_axis_btn = Instance(QPushButton)
    _sel_y_axis_btn = Instance(QPushButton)
    _x_button_group = Instance(QButtonGroup)
    _y_button_group = Instance(QButtonGroup)

    _curve_styles = Instance(cycle, allow_none=False)

    def create_widget(self, parent):
        widget = self._initUI(parent)

        # Keep the initial start time to recover trendline for 'Reset'
        self._initial_start_time = QDateTime.currentDateTime()

        self._plot = self._curve_widget.get_plot()
        self._plot.set_antialiasing(True)
        self._plot.setAxisTitle(QwtPlot.xBottom, 'Time')

        # We are using CurveWidget, which internally creates a BaseCurveWidget,
        # which internally creates a CurvePlot, which contains the method
        # edit_axis_parameters, which creates the widget to set the time axis.
        # It would be a nightmare to overwrite three classes, so we just do
        # a little monkey patching here.
        self._plot.edit_axis_parameters = self.edit_axis_parameters

        # have a 1 s timeout to request data, thus avoid
        # frequent re-loading while scaling
        self._timer = QTimer(parent)
        self._timer.setInterval(1000)
        self._timer.setSingleShot(True)
        self._plot.axisWidget(QwtPlot.xBottom).scaleDivChanged.connect(
            self._timer.start)
        self._timer.timeout.connect(self._x_axis_scale_changed)

        self._manager = PlotManager(widget)
        self._manager.add_plot(self._plot)

        self._manager.register_all_curve_tools()
        self._manager.register_other_tools()
        self._manager.add_tool(SelectPointTool, title='Test',
                               on_active_item=True, mode='create')

        drawer = DateTimeScaleDraw()
        self._plot.setAxisScaleEngine(QwtPlot.xBottom, ScaleEngine(drawer))
        self._plot.setAxisScaleDraw(QwtPlot.xBottom, drawer)
        self._plot.setAxisAutoScale(QwtPlot.yLeft)
        self._lasttime = time.time()
        self._plot.setAxisScale(QwtPlot.xBottom,
                                round(time.time() - 1),
                                round(time.time() + 10))
        self._plot.setAxisLabelAlignment(QwtPlot.xBottom,
                                         Qt.AlignRight | Qt.AlignBottom)

        # Add the first curve
        self.add_proxy(self.proxy)
        return widget

    def _initUI(self, parent):
        """ Setup all widgets correctly.
        """
        self._curve_widget = _KaraboCurveWidget(parent=parent)
        # Make connection to update time buttons when mouse event in QwtWidget
        # happened
        self._curve_widget.signal_mouse_event.connect(
            self._uncheck_axis_buttons)

        # Init x-axis buttons
        # Create widget for beginning and end date time
        date_time_widget = QWidget()
        date_time_layout = QHBoxLayout(date_time_widget)
        date_time_layout.setContentsMargins(0, 0, 0, 0)

        current_date_time = QDateTime.currentDateTime()
        self._dt_start = QDateTimeEdit(current_date_time)
        self._dt_start.setDisplayFormat("yyyy-MM-dd hh:mm:ss")
        self._dt_start.setStyleSheet(DATETIMEEDIT_STYLE_SHEET)
        self._dt_end = QDateTimeEdit(current_date_time)
        self._dt_end.setDisplayFormat("yyyy-MM-dd hh:mm:ss")
        self._dt_end.setStyleSheet(DATETIMEEDIT_STYLE_SHEET)
        date_time_layout.addWidget(self._dt_start)
        date_time_layout.addWidget(self._dt_end)

        x_axis_btns_widget = QWidget()
        x_axis_btns_layout = QHBoxLayout(x_axis_btns_widget)
        x_axis_btns_layout.setContentsMargins(0, 0, 0, 0)

        self._x_axis_str_btns = OrderedDict()
        self._x_axis_str_btns[ONE_WEEK] = None
        self._x_axis_str_btns[ONE_DAY] = None
        self._x_axis_str_btns[ONE_HOUR] = None
        self._x_axis_str_btns[TEN_MINUTES] = None
        self._x_axis_str_btns[RESET] = None
        self._x_button_group = self._create_button_group(
            X_AXIS, self._x_axis_str_btns, x_axis_btns_layout)

        # Init y-axis buttons
        y_axis_buttons_widget = QWidget()
        y_axis_btns_layout = QHBoxLayout(y_axis_buttons_widget)
        y_axis_btns_layout.setContentsMargins(0, 0, 0, 0)

        self._y_axis_str_btns[FULL_RANGE] = None
        self._y_axis_str_btns[DETAIL_RANGE] = None
        self._y_button_group = self._create_button_group(
            Y_AXIS, self._y_axis_str_btns, y_axis_btns_layout)

        self._le_detail_range = QLineEdit("10")
        self._le_detail_range.setMinimumWidth(30)
        self._le_detail_range.setMaximumWidth(80)
        self._le_detail_range.setEnabled(False)
        self._le_detail_range.setStyleSheet(LINEEDIT_STYLE_SHEET)
        validator = QIntValidator(1, krb_globals.MAX_INT32)
        self._le_detail_range.setValidator(validator)
        self._le_detail_range.textChanged.connect(self._detail_range_changed)
        laDetailRange = QLabel("%")

        y_axis_btns_layout.addWidget(self._y_axis_str_btns[FULL_RANGE])
        y_axis_btns_layout.addWidget(self._y_axis_str_btns[DETAIL_RANGE])
        y_axis_btns_layout.addWidget(self._le_detail_range)
        y_axis_btns_layout.addWidget(laDetailRange)

        xLayout = QVBoxLayout()
        xLayout.addWidget(y_axis_buttons_widget)
        xLayout.addWidget(self._curve_widget)
        xLayout.addWidget(date_time_widget)
        xLayout.addWidget(x_axis_btns_widget)

        widget = QWidget()
        layout = QHBoxLayout(widget)
        layout.addLayout(xLayout)
        return widget

    def edit_axis_parameters(self, axis_id):
        if axis_id != QwtPlot.xBottom:
            # call the original method that we monkey-patched over
            type(self._plot).edit_axis_parameters(self._plot, axis_id)
        else:
            dialog = Timespan(self._curve_widget)
            sd = self._plot.axisScaleDiv(QwtPlot.xBottom)
            dialog.dt_beginning.setDateTime(
                QDateTime.fromMSecsSinceEpoch(sd.lowerBound() * 1000))
            dialog.dt_end.setDateTime(
                QDateTime.fromMSecsSinceEpoch(sd.upperBound() * 1000))
            if dialog.exec_() != dialog.Accepted:
                return
            # Reset time buttons
            self._uncheck_axis_buttons()
            start_date_time = dialog.dt_beginning.dateTime()
            end_date_time = dialog.dt_end.dateTime()
            start_secs = start_date_time.toMSecsSinceEpoch() / 1000
            end_secs = end_date_time.toMSecsSinceEpoch() / 1000
            self._plot.setAxisScale(QwtPlot.xBottom, start_secs, end_secs)
            self.update_later()

    def binding_update(self, proxy):
        # We only care about binding updates after the widget is initialized.
        if self.widget is None:
            return

        # XXX: This is a bad idea with multiple curves!
        self._plot.setAxisTitle(QwtPlot.yLeft, axis_label(proxy))

    def add_proxy(self, proxy):
        style, color = next(self._curve_styles)
        curve = make.curve([], [], proxy.path, color=color, linestyle=style)
        self._addCurve(proxy, curve)
        if self._curve_count == 2:
            # show the item panel if we have more than one curve
            self._curve_widget.get_itemlist_panel().show()
            # also show a legend
            legend = make.legend("TL")
            self._plot.add_item(legend)

        return True

    def value_update(self, proxy):
        if self.widget is None:
            return

        timestamp = proxy.binding.timestamp
        t = timestamp.toTimestamp()
        self._curves[proxy].add_point(proxy.value, t)

        t0 = self._plot.axisScaleDiv(QwtPlot.xBottom).lowerBound()
        t1 = self._plot.axisScaleDiv(QwtPlot.xBottom).upperBound()
        if self._lasttime < t1 < t:
            if not self._update_x_axis_scale():
                aw = self._plot.axisWidget(QwtPlot.xBottom)
                with SignalBlocker(aw):
                    self._plot.setAxisScale(QwtPlot.xBottom, t0, t + 10)

        # Make sure to keep the y axis updated as well
        self._update_y_axis_scale()

        self._lasttime = timestamp.toTimestamp()
        self.update_later()

    def deferred_update(self):
        self._plot.replot()
        asd = self._plot.axisScaleDiv(QwtPlot.xBottom)
        t0, t1 = asd.lowerBound(), asd.upperBound()
        # Update date time widgets
        start = QDateTime.fromMSecsSinceEpoch(t0 * 1000)
        end = QDateTime.fromMSecsSinceEpoch(t1 * 1000)
        self._update_date_time_widgets(start, end)

    @pyqtSlot()
    def _x_axis_scale_changed(self):
        """ This slot is called whenever the timer timed out and previously the
            x axis scale was changed. """
        asd = self._plot.axisScaleDiv(QwtPlot.xBottom)
        t0, t1 = asd.lowerBound(), asd.upperBound()
        self._update_x_axis_interval(t0, t1)
        self._update_y_axis_scale()

    @pyqtSlot(str)
    def _detail_range_changed(self, text):
        if self._update_y_axis_scale():
            self.update_later()

    @pyqtSlot(object)
    def _y_axis_btns_toggled(self, button):
        self._sel_y_axis_btn = button
        if self._update_y_axis_scale():
            self.update_later()

    @pyqtSlot(object)
    def _x_axis_btns_toggled(self, button):
        """ A time button was clicked which needs to update the axis scale. """
        self._sel_x_axis_btn = button
        if self._update_x_axis_scale():
            self.update_later()

    @pyqtSlot()
    def _reset_button_clicked(self):
        """ Reset the x axis scale to the initial start time."""
        self._uncheck_axis_buttons()

        start_date_time = self._initial_start_time
        end_date_time = QDateTime.currentDateTime()

        start_secs = start_date_time.toMSecsSinceEpoch() / 1000
        end_secs = end_date_time.toMSecsSinceEpoch() / 1000
        # Rescale x axis
        self._plot.setAxisScale(QwtPlot.xBottom, start_secs, end_secs)
        self.update_later()

    @pyqtSlot()
    def _uncheck_axis_buttons(self):
        """ No axis button should be selected.
            To realize that a ``HIDDEN`` button of the button groups is clicked
            and the selected x/y axis button is set to ``None``.
        """
        if self._sel_x_axis_btn is None and self._sel_y_axis_btn is None:
            return

        # x axis related buttons
        self._x_axis_str_btns[HIDDEN].click()
        self._sel_x_axis_btn = None

        # y axis related buttons
        self._y_axis_str_btns[HIDDEN].click()
        self._sel_y_axis_btn = None

    # ----------------------------
    # Private methods

    def _addCurve(self, proxy, curve):
        """ Give derived classes a place to respond to changes. """
        if proxy in self._curves:
            old_curve = self._curves[proxy]
            self._plot.del_item(old_curve.curve)
        else:
            self._curve_count += 1
        if not isinstance(curve, Curve):
            curve = Curve(proxy=proxy, curve=curve)
        self._curves[proxy] = curve
        self._plot.add_item(curve.curve)
        curve.update()

    def _create_button_group(self, button_type, string_btn_dict, layout):
        """ The buttons for time/range scaling are created.
        """
        _handlers = {
            X_AXIS: self._x_axis_btns_toggled,
            Y_AXIS: self._y_axis_btns_toggled,
        }
        button_group = QButtonGroup()
        button_group.buttonClicked.connect(_handlers[button_type])

        for btn_text in string_btn_dict.keys():
            button = QPushButton(btn_text)
            button.setStyleSheet(BUTTON_STYLE_SHEET)
            string_btn_dict[btn_text] = button
            if btn_text == RESET:
                button.clicked.connect(self._reset_button_clicked)
            else:
                # Do not add reset button to button group
                button.setCheckable(True)
                button_group.addButton(button)

            # Different layout used for y axis buttons
            if button_type == X_AXIS:
                layout.addWidget(button)

        # Add invisible button to get the state that no buttons are visible
        hidden_btn = QPushButton()
        hidden_btn.setCheckable(True)
        hidden_btn.setVisible(False)
        string_btn_dict[HIDDEN] = hidden_btn
        button_group.addButton(hidden_btn, 0)
        layout.addWidget(hidden_btn)

        return button_group

    def __curve_styles_default(self):
        return cycle(product(CURVE_STYLES, CURVE_COLORS))

    def _update_x_axis_interval(self, t0, t1):
        """ Update lower and upper bound of curve intervals. """
        for v in self._curves.values():
            v.changeInterval(t0, t1)

    def _update_date_time_widgets(self, start, end):
        """ The date time widgets get updated to the corresponding x axis
            scale.
        """
        self._dt_start.setDateTime(start)
        self._dt_end.setDateTime(end)

    def _update_x_axis_scale(self):
        """ The start and end time date for the x axis is updated here
            depending on the selected time button.
            In case of success ``True`` is returned, else ``False``."""
        if self._sel_x_axis_btn is None:
            return False

        start, end = get_start_end_date_time(self._sel_x_axis_btn.text())

        if start is None or end is None:
            return False

        start_secs = start.toMSecsSinceEpoch() / 1000
        end_secs = end.toMSecsSinceEpoch() / 1000

        # Rescale x axis
        aw = self._plot.axisWidget(QwtPlot.xBottom)
        with SignalBlocker(aw):
            # Use blocker to prevent timer start
            self._plot.setAxisScale(QwtPlot.xBottom, start_secs, end_secs)
        self._update_x_axis_interval(start_secs, end_secs)
        return True

    def _update_y_axis_scale(self):
        """ The minimum and maximum value for the y axis is updated depending
            on the selected range button.
            In case of success ``True`` is returned, else ``False``.
        """
        if self._sel_y_axis_btn is None:
            return False

        if self._sel_y_axis_btn.text() == FULL_RANGE:
            # Reset
            self._le_detail_range.setEnabled(False)
            ymin = krb_globals.MAX_INT32
            ymax = -ymin
            for curve in self._curves.values():
                min_y = curve.get_min_y_value()
                if ymin > min_y:
                    ymin = min_y
                max_y = curve.get_max_y_value()
                if ymax < max_y:
                    ymax = max_y
        elif self._sel_y_axis_btn.text() == DETAIL_RANGE:
            # Calculate mean of last 10 values and use detail range value to
            # get ymin/ymax
            self._le_detail_range.setEnabled(True)
            y_mean_values = []
            for curve in self._curves.values():
                y_mean = curve.get_mean_y_value()
                y_mean_values.append(y_mean)
            y_mean = numpy.mean(y_mean_values)
            y_range = y_mean * int(self._le_detail_range.text()) * 0.01
            ymin = y_mean - y_range
            ymax = y_mean + y_range
        else:
            return False

        # Rescale y axis
        aw = self._plot.axisWidget(QwtPlot.yLeft)
        with SignalBlocker(aw):
            self._plot.setAxisScale(QwtPlot.yLeft, ymin, ymax)
        return True
