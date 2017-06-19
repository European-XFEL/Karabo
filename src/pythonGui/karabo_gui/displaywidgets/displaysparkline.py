from collections import OrderedDict
import datetime
from functools import partial
import time

import numpy as np
from PyQt4.QtCore import QPoint, Qt, pyqtSlot
from PyQt4.QtGui import (QAction, QActionGroup, QColor, QHBoxLayout,
                         QInputDialog, QLineEdit, QPainter, QPainterPath, QPen,
                         QWidget)

from karabo.middlelayer import Simple, Timestamp
from karabo_gui.alarms.api import (ALARM_COLOR, ALARM_HIGH, ALARM_LOW,
                                   WARN_COLOR, WARN_GLOBAL, WARN_HIGH,
                                   WARN_LOW)
from karabo_gui.util import SignalBlocker
from karabo_gui.widget import DisplayWidget


class SparkRenderer(QWidget):
    """
    A renderer for sparklines using QPainter
    """

    # the widget has a fixed time base
    time_base = 600  # seconds

    # maximum number of points to use, determines the binning off data
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
        """
        Set a new data point with associated timestamp to the spark line

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
        """
        Set a series of data to the sparkline, e.g. historic data

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
        """
        Update data ranges as needed, depending on values and alarms.

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


class DisplaySparkline(DisplayWidget):
    """
    Displays a spark line with a fixed time basis and coarsing of data

    On initialization and visibility changes historic data is requested.
    Rendering is performed via the SparkRenderer class
    """
    category = Simple
    alias = "Sparkline"

    # this widget will always have a fixed size in pixels
    # we do not want to tempt people to scale it and misuse it as a trendline
    _height = 50
    _width = 200

    # timebases available for the widget, values in seconds
    _timebases = OrderedDict()
    _timebases['60s'] = 60
    _timebases['10m'] = 600
    _timebases['10h'] = 36000

    def __init__(self, model, box, parent):
        super(DisplaySparkline, self).__init__(None)

        self.model = model
        self.widget = QWidget(parent)
        self.layout = QHBoxLayout(self.widget)
        self.widget.setFixedHeight(self._height)

        self.lineedit = QLineEdit()
        self.lineedit.setReadOnly(True)
        self.renderarea = SparkRenderer(self.widget)
        self.renderarea.setFixedWidth(self._width)
        self.renderarea.time_base = model.time_base

        self.layout.addWidget(self.lineedit)
        self.lineedit.setVisible(model.show_value)
        self.layout.addWidget(self.renderarea)

        self._value = None
        self.box = box

        # display options
        showValueAction = QAction("Show value", self.widget, checkable=True)

        def showValFun():
            self._setShowValue(not self.model.show_value)

        showValueAction.triggered.connect(showValFun)
        showValueAction.setChecked(self.model.show_value)
        self.widget.addAction(showValueAction)

        self.showFormatAction = QAction("Set value format", self.widget)
        self.showFormatAction.triggered.connect(self._queryShowFormat)
        self.showFormatAction.setEnabled(self.model.show_value)
        self.widget.addAction(self.showFormatAction)

        timeBaseSeparator = QAction("Timebase", self.widget)
        timeBaseSeparator.setSeparator(True)
        self.widget.addAction(timeBaseSeparator)

        timebaseGroup = QActionGroup(self.widget)
        for label, base in self._timebases.items():
            tbAction = QAction("{} time base".format(label),
                               timebaseGroup, checkable=True)

            tbAction.triggered.connect(partial(self._setTimeBase, base))

            if self.model.time_base == base:
                tbAction.setChecked(True)

            timebaseGroup.addAction(tbAction)

        self.widget.addActions(timebaseGroup.actions())

        self.box.signalHistoricData.connect(self.onHistoricData)
        self.box.visibilityChanged.connect(self.onVisibilityChanged)

        now = time.time()
        self.getPropertyHistory(now - self.model.time_base, now, True)

        self.widget.destroyed.connect(self.destroy)

    def valueChanged(self, box, value, timestamp=None):

        if timestamp is None:
            return

        # extract alarms if present, if not the sparkrenderer expects "None"
        alarms = {ALARM_LOW: getattr(box.descriptor, ALARM_LOW, None),
                  WARN_LOW: getattr(box.descriptor, WARN_LOW, None),
                  WARN_HIGH: getattr(box.descriptor, WARN_HIGH, None),
                  ALARM_HIGH: getattr(box.descriptor, ALARM_HIGH, None),
                  }

        self.renderarea.setData(value, timestamp, alarms)

        if value != self._value:
            with SignalBlocker(self.widget):
                self.lineedit.setText(str("{:"+self.model.show_format+"}")
                                      .format(value))

        self._value = value

    @pyqtSlot(bool)
    def onVisibilityChanged(self, visible):
        if visible:
            now = time.time()
            self.getPropertyHistory(now - self.model.time_base, now)

    @pyqtSlot(object, object)
    def onHistoricData(self, box, data):
        if not data:
            return

        datasize = len(data)
        x = np.empty(datasize, dtype=float)
        y = np.empty(datasize, dtype=float)

        for i, d in enumerate(data):
            x[i] = Timestamp.fromHashAttributes(d['v', ...]).toTimestamp()
            y[i] = d["v"]

        self.renderarea.setSeries(x[:-1], y[:-1])
        tstamp = Timestamp.fromHashAttributes(data[-1]['v', ...])
        self.valueChanged(self.box, y[-1], timestamp=tstamp)

    def getPropertyHistory(self, t0, t1, init=False):
        # Avoid if not currently visible
        if self.box.visible < 1 and not init:
            return
        t0 = str(datetime.datetime.utcfromtimestamp(t0).isoformat())
        t1 = str(datetime.datetime.utcfromtimestamp(t1).isoformat())
        self.box.getPropertyHistory(t0, t1, -1)

    @pyqtSlot(object)
    def _setTimeBase(self, timebase):
        self.model.time_base = timebase
        self.renderarea.update_time_base(timebase)
        now = time.time()
        self.getPropertyHistory(now - self.model.time_base, now, True)

    @pyqtSlot(object)
    def _setShowValue(self, value):
        self.model.show_value = value
        self.lineedit.setVisible(value)
        self.showFormatAction.setEnabled(value)

    @pyqtSlot(object)
    def _queryShowFormat(self):
        form, ok = QInputDialog.getText(self.widget, "Enter Format",
                                        "", text=self.model.show_format)
        if ok:
            self._setShowFormat(form)

    def _setShowFormat(self, format):
        self.model.show_format = format

    @pyqtSlot(object)
    def destroy(self):
        self.box.signalHistoricData.disconnect(self.onHistoricData)
        self.box.visibilityChanged.disconnect(self.onVisibilityChanged)
