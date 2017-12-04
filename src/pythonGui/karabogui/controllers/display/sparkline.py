import datetime
from functools import partial
import time

import numpy as np
from PyQt4.QtCore import QPoint, Qt, pyqtSlot
from PyQt4.QtGui import (QAction, QActionGroup, QColor, QHBoxLayout,
                         QInputDialog, QLineEdit, QPainter, QPainterPath, QPen,
                         QWidget)
from traits.api import Instance, on_trait_change

from karabo.common.scenemodel.api import SparklineModel
from karabo.middlelayer import Timestamp
from karabogui.alarms.api import (
    ALARM_COLOR, ALARM_HIGH, ALARM_LOW, WARN_COLOR, WARN_GLOBAL, WARN_HIGH,
    WARN_LOW)
from karabogui.binding.api import FloatBinding, IntBinding
from karabogui.controllers.base import BaseBindingController
from karabogui.controllers.registry import register_binding_controller
from karabogui.util import SignalBlocker

# this widget will always have a fixed size in pixels
# we do not want to tempt people to scale it and misuse it as a trendline
WIDGET_HEIGHT = 50
WIDGET_WIDTH = 200
# timebases available for the widget, values in seconds
TIMEBASES = (('60s', 60), ('10m', 600), ('10h', 36000))


class SparkRenderer(QWidget):
    """A renderer for sparklines using QPainter
    """
    # the widget has a fixed time base
    time_base = 600  # seconds
    # maximum number of points to use, determines the binning of data
    _p_max = 120
    # plot fraction in y-direction to use
    _plot_frac = 0.9
    # size in pixels of the min max indication area
    _min_max_indic_size = 60
    # relative epsilon within within the tendency will be considered stable
    _tendency_eps = 0.05
    # size of the window in bins which is used for evaluating trend indication
    _indicator_window_size = 5

    def __init__(self, parent):
        super(SparkRenderer, self).__init__(parent)

        self.pen = QPen(Qt.black, 2, Qt.SolidLine)
        self.no_pen = QPen(Qt.blue, 1, Qt.SolidLine)

        self.yrange = None
        self.ystep = None
        self.tvals = np.arange(self._p_max)

        self.yvals = np.zeros(self._p_max, np.double)

        self.finfo = np.finfo(np.float32)
        self.ymax = np.ones(self._p_max, np.float32) * self.finfo.min
        self.ymin = np.ones(self._p_max, np.float32) * self.finfo.max
        self.ycnts = np.zeros(self._p_max, np.int32)

        self.painter_path = None
        self.painter_path_min = None
        self.painter_path_max = None

        self.then = time.time()
        self.alarms = None

        self.tendency = 0
        self.tendency_indicators = {}
        self.tendency_indicators[0] = {'x': np.array([0., 10., 0., 0.]),
                                       'y': np.array([-5., 0., 5., -5.])}

        self.tendency_indicators[1] = {'x': np.array([-5., 0., 5., -5.]),
                                       'y': np.array([10., 0., 10., 10.])}

        self.tendency_indicators[-1] = {'x': np.array([-5., 0., 5., -5.]),
                                        'y': np.array([-7., 3., -7., -7.])}

    def _scale_y(self, val, height):
        if self.yrange is None:
            return

        dy = self.yrange[1] - self.yrange[0]
        if dy == 0:
            return

        scaled = (val - self.yrange[0]) / dy
        scaled /= self.ystep
        scaled = (height - scaled + self.offset).astype(np.int64)
        return scaled

    def paintEvent(self, event):
        if self.painter_path is None:
            return

        width = self.width() - self._min_max_indic_size
        height = self._plot_frac * self.height()

        with QPainter(self) as painter:
            painter.setPen(self.pen)

            painter.setPen(self.pen)
            painter.drawPath(self.painter_path)

            pen = QPen(Qt.gray, 1, Qt.SolidLine)
            painter.setPen(pen)

            painter.drawPath(self.painter_path_min)
            painter.drawPath(self.painter_path_max)

            # draw available alarm indicators
            if self.alarms is not None:
                for alarm_type, val in self.alarms.items():
                    if val is None:
                        continue
                    if WARN_GLOBAL in alarm_type:
                        pen = QPen(QColor(*WARN_COLOR), 1, Qt.DashLine)
                    else:
                        pen = QPen(QColor(*ALARM_COLOR), 1, Qt.SolidLine)

                    painter.setPen(pen)
                    val = self._scale_y(val, height)
                    painter.drawLine(0, val, width, val)

            # add min and max value indicators
            pen = QPen(Qt.black, 1, Qt.SolidLine)
            painter.setPen(pen)
            x_points = [width, width]
            y_points = [0, self.height()]
            painter.drawPath(self._createPainterPath(np.array(x_points),
                                                     np.array(y_points)))

            # add min max indicator
            font = painter.font()
            font.setPointSize(8)
            font.setFamily("Monospace")
            painter.setFont(font)
            painter.drawText(QPoint(width+5, 10),
                             self._returnFormatted(self.yrange[1]))
            painter.drawText(QPoint(width+5, height+2),
                             self._returnFormatted(self.yrange[0]))

            #  add tendency indicator
            x_points = self.tendency_indicators[self.tendency]['x'] + width
            y_points = self.tendency_indicators[self.tendency]['y']
            dy = height/2 + 2.5
            if self.tendency != 0:
                dy = height/2 + (-1)*self.tendency*height/2
            path = self._createPainterPath(np.array(x_points),
                                           np.array(y_points)+dy)
            painter.fillPath(path, Qt.magenta)
            painter.drawPath(path)

            # add timebase info
            tb = self.time_base
            if tb >= 3600:
                tb = "{:d}h".format(tb//3600)
            elif tb >= 600:
                tb = "{:d}m".format(tb//60)
            else:
                tb = "{:d}s".format(tb)

            dy = self.height()-13
            path = self._createPainterPath(np.array([3., 28., 28., 3., 3.]),
                                           np.array([0., 0., 12., 12., 0.])+dy)
            painter.fillPath(path, QColor(255, 255, 255, 200))
            painter.drawPath(path)
            painter.drawText(QPoint(5, self.height()-2), tb)

    def _returnFormatted(self, v):
        if v == 0:
            return "{:+0.2f}".format(v)

        exp = np.log10(abs(v))
        if abs(exp) <= 2:
            return "{:+0.2f}".format(v)
        else:
            return "{:+0.1e}".format(v)

    def _createPainterPath(self, x, y):
        path = QPainterPath()
        path.moveTo(x[0], y[0])
        for i in range(1, x.size):
            path.lineTo(x[i], y[i])
        return path

    def setData(self, value, timestamp, alarms):
        """Set a new data point with associated timestamp to the spark line

        Alarms should be a dict indicating alarm bounds if they exists.
        The expected keys are "warnLow", "warnHigh", "alarmLow" and "alarmHigh"
        Set the value to `None` if no bound is set.
        """
        t = timestamp.toTimestamp()

        # check if we need to roll to the next bin
        dt = int((t-self.then)/(self.time_base / self._p_max))
        if dt >= 1:
            # the sparkline might need to cover "gaps" in its data series
            # e.g. when for a given bin, no data was pushed to the widget
            # in this case we fill with the value of the previous bin.
            # If the sparkline does not receive any value updates from a
            # device for dt > time_base, indexing by [-dt:] would result
            # in an IndexError. In these cases we set all values in the
            # sparkline to the value of the last update, consitent with the
            # gap filling behaviour for valid "dt"s.
            if dt < self.yvals.size:
                self.yvals = np.roll(self.yvals, -dt)
                self.yvals[-dt:-1] = self.yvals[-dt-1]
                self.yvals[-1] = 0.
                self.ycnts = np.roll(self.ycnts, -dt)
                self.ycnts[-dt:-1] = self.ycnts[-dt-1]
                self.ycnts[-1] = 0.
                self.ymax = np.roll(self.ymax, -dt)
                self.ymax[-dt:-1] = self.ymax[-dt-1]
                self.ymin = np.roll(self.ymin, -dt)
                self.ymin[-dt:-1] = self.ymin[-dt-1]
            else:
                self.yvals[:] = self.yvals[-1]
                self.ycnts[:] = self.ycnts[-1]
                self.ymax[:] = self.ymax[-1]
                self.ymin[:] = self.ymin[-1]

            self.then = time.time()

        self.yvals[-1] += value
        self.ycnts[-1] += 1
        self.ymax[-1] = max(self.ymax[-1], value)
        self.ymin[-1] = min(self.ymin[-1], value)
        self._update_ranges(value, alarms)
        self.alarms = alarms

        width = self.width() - self._min_max_indic_size
        height = self._plot_frac * self.height()

        step = width / self._p_max

        nonzero = np.nonzero(self.ycnts)
        x = (self.tvals[nonzero] * step).astype(np.int32)
        y = self._scale_y(self.yvals[nonzero] / self.ycnts[nonzero], height)

        if y is None:
            return

        ymin = self._scale_y(self.ymin[nonzero], height)
        ymax = self._scale_y(self.ymax[nonzero], height)

        # evaluate tendency for last 5 point intervals
        iws = self._indicator_window_size
        c_idx_nz = np.nonzero(self.ycnts[-iws:])
        t_idx_nz = np.nonzero(self.ycnts[-2*iws:-iws])
        if np.any(c_idx_nz) and np.any(t_idx_nz):
            current_avg = np.mean(self.yvals[-iws:][c_idx_nz]
                                  / self.ycnts[-iws:][c_idx_nz])
            test_avg = np.mean(self.yvals[-2*iws:-iws][t_idx_nz]
                               / self.ycnts[-2*iws:-iws][t_idx_nz])
            delta = (current_avg-test_avg)/test_avg
            if abs(delta) <= self._tendency_eps or not np.isfinite(delta):
                self.tendency = 0
            else:
                self.tendency = np.sign(delta)

        self.painter_path = self._createPainterPath(x, y)
        self.painter_path_min = self._createPainterPath(x, ymin)
        self.painter_path_max = self._createPainterPath(x, ymax)

    def setSeries(self, x, y):
        """Set a series of data to the sparkline, e.g. historic data

        Note that this will not redraw the sparkline until the next
        setData update.
        """
        assert x.size == y.size

        if y.size == 0:
            return

        now = time.time()
        x = now-x
        binned, bins = np.histogram(x, bins=self._p_max,
                                    range=[0, self.time_base], weights=y)

        counts, _ = np.histogram(x, bins=self._p_max,
                                 range=[0, self.time_base])

        self.yvals = binned[::-1]
        self.ycnts = counts[::-1]

        bin_idx = np.digitize(x, bins)

        self.ymin = np.zeros(self.yvals.shape, np.float32)
        self.ymax = np.zeros(self.yvals.shape, np.float32)

        for i in range(self._p_max):
            idx = bin_idx == i
            if idx.any():
                self.ymin[i] = np.min(y[idx])
                self.ymax[i] = np.max(y[idx])

        self.ymin = self.ymin[::-1]
        self.ymax = self.ymax[::-1]

        self.yrange = (np.min(y), np.max(y))

    def _update_ranges(self, value, alarms):
        """Update data ranges as needed, depending on values and alarms.

        If alarms are present, the range will be always be set such that
        they are indicated.
        """
        # the y range which simply needs to scale to new maxima and minima
        if self.yrange is None:
            # initially only single value
            self.yrange = (min(0.9*value, 1.1*value),
                           max(0.9*value, 1.1*value))
        else:
            self.yrange = min(value, *self.yrange), max(value, *self.yrange)

        alarmlst = [a for a in alarms.values() if a is not None]
        if alarmlst:
            self.yrange = (min(self.yrange[0], *alarmlst),
                           max(self.yrange[1], *alarmlst))

        # scale to plot fraction
        pheight = self._plot_frac * self.height()
        self.offset = (self.height() - pheight)//2
        self.ystep = 1./pheight

    def update_time_base(self, time_base):
        self.time_base = time_base


@register_binding_controller(ui_name='Sparkline', klassname='DisplaySparkline',
                             binding_type=(FloatBinding, IntBinding))
class DisplaySparkline(BaseBindingController):
    """Displays a spark line with a fixed time basis and coarsing of data

    On initialization and visibility changes historic data is requested.
    Rendering is performed via the SparkRenderer class
    """
    # The scene model class used by this controller
    model = Instance(SparklineModel)

    line_edit = Instance(QLineEdit)
    render_area = Instance(SparkRenderer)
    action_show_format = Instance(QAction)

    def create_widget(self, parent):
        widget = QWidget(parent)
        layout = QHBoxLayout(self.widget)
        widget.setFixedHeight(WIDGET_HEIGHT)

        self.line_edit = QLineEdit()
        self.line_edit.setReadOnly(True)
        self.render_area = SparkRenderer(widget)
        self.render_area.setFixedWidth(WIDGET_WIDTH)
        self.render_area.time_base = self.model.time_base

        layout.addWidget(self.line_edit)
        self.line_edit.setVisible(self.model.show_value)
        layout.addWidget(self.render_area)

        # display options
        @pyqtSlot(bool)
        def toggle_show_value(stat):
            self.model.show_value = stat

        action_show_value = QAction("Show value", widget, checkable=True)
        action_show_value.toggled.connect(toggle_show_value)
        action_show_value.setChecked(self.model.show_value)
        widget.addAction(action_show_value)

        self.action_show_format = QAction("Set value format", widget)
        self.action_show_format.triggered.connect(self._change_show_format)
        self.action_show_format.setEnabled(self.model.show_value)
        widget.addAction(self.action_show_format)

        def change_time_base(tb):
            self.model.time_base = tb

        time_base_separator = QAction("Timebase", widget)
        time_base_separator.setSeparator(True)
        widget.addAction(time_base_separator)
        time_base_group = QActionGroup(widget)
        for label, base in TIMEBASES:
            tbAction = QAction("{} time base".format(label),
                               time_base_group, checkable=True)
            tbAction.toggled.connect(partial(change_time_base, base))

            if self.model.time_base == base:
                tbAction.setChecked(True)

            time_base_group.addAction(tbAction)

        widget.addActions(time_base_group.actions())
        return widget

    def _widget_changed(self):
        """Called after the widget is done being created."""
        now = time.time()
        self._fetch_property_history(now - self.model.time_base, now)

    @on_trait_change('proxy:binding:config_update')
    def _value_update(self, value):
        timestamp = self.proxy.binding.timestamp
        attrs = self.proxy.binding.attributes
        # extract alarms if present, if not the sparkrenderer expects "None"
        alarms = {k: attrs.get(k, None)
                  for k in (ALARM_LOW, WARN_LOW, WARN_HIGH, ALARM_HIGH)}

        self.render_area.setData(value, timestamp, alarms)
        self._set_text()

    @on_trait_change('proxy.visible')
    def _visibility_update(self, visible):
        if visible:
            now = time.time()
            self._fetch_property_history(now - self.model.time_base, now)

    @on_trait_change('proxy:binding:historic_data')
    def _historic_data_arrived(self, data):
        if not data:
            return

        datasize = len(data)
        x = np.empty(datasize, dtype=float)
        y = np.empty(datasize, dtype=float)

        for i, d in enumerate(data):
            x[i] = Timestamp.fromHashAttributes(d['v', ...]).toTimestamp()
            y[i] = d["v"]

        self.render_area.setSeries(x[:-1], y[:-1])
        # use the last data point to trigger a redraw
        last = data[-1]
        self.proxy.binding.timestamp = Timestamp.fromHashAttributes(last['v',
                                                                         ...])
        self._value_update(last["v"])

    def _fetch_property_history(self, t0, t1):
        if not self.proxy.visible:
            return
        t0 = str(datetime.datetime.utcfromtimestamp(t0).isoformat())
        t1 = str(datetime.datetime.utcfromtimestamp(t1).isoformat())
        self.proxy.get_history(t0, t1)

    @on_trait_change('model:time_base')
    def _update_time_base(self, timebase):
        self.render_area.update_time_base(timebase)
        now = time.time()
        self._fetch_property_history(now - timebase, now)

    @on_trait_change('model:show_value')
    def _show_value_update(self, value):
        self.line_edit.setVisible(value)
        self.action_show_format.setEnabled(value)

    @on_trait_change('model:show_format')
    def _show_format_update(self):
        self._set_text()

    @pyqtSlot()
    def _change_show_format(self):
        form, ok = QInputDialog.getText(self.widget, "Enter Format",
                                        "", text=self.model.show_format)
        if ok:
            self.model.show_format = form

    def _set_text(self):
        if self.proxy.binding is None or self.line_edit is None:
            return
        template = "{:" + self.model.show_format + "}"
        with SignalBlocker(self.line_edit):
            self.line_edit.setText(template.format(self.proxy.value))
