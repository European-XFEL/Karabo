import datetime
import time

import numpy as np
from PyQt4.QtGui import (QColor, QHBoxLayout, QLineEdit, QPainter,
                         QPainterPath, QPen, QWidget)
from PyQt4.QtCore import Qt, pyqtSlot

from karabo.middlelayer import Simple, Timestamp
from karabo_gui.const import ALARM_COLOR, WARN_COLOR
from karabo_gui.indicators import (ALARM_LOW, ALARM_HIGH, WARN_GLOBAL,
                                   WARN_LOW, WARN_HIGH)
from karabo_gui.util import SignalBlocker
from karabo_gui.widget import DisplayWidget


class SparkRenderer(QWidget):
    """
    A renderer for sparklines using QPainter
    """

    # the widget has a fixed time base
    time_range = 600  # seconds

    # maximum number of points to use, determines the binning off data
    _p_max = 120

    # plot fraction in y-direction to use
    _plot_frac = 0.9

    def __init__(self, parent):
        super(SparkRenderer, self).__init__(parent)

        self.pen = QPen(Qt.black, 2, Qt.SolidLine)
        self.no_pen = QPen(Qt.blue, 1, Qt.SolidLine)

        self.yrange = None
        self.ystep = None
        self.dt = self.time_range/self._p_max
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

        width = self.width()
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
        dt = int(np.floor(t-self.then)//self.dt)
        if dt >= 1:
            self.yvals = np.roll(self.yvals, -dt)
            self.yvals[-dt:] = 0.
            self.ycnts = np.roll(self.ycnts, -dt)
            self.ycnts[-dt:] = 0.
            self.ymax = np.roll(self.ymax, -dt)
            self.ymin = np.roll(self.ymin, -dt)
            self.then = time.time()

        self.yvals[-1] += value
        self.ycnts[-1] += 1
        self.ymax[-1] = max(self.ymax[-1], value)
        self.ymin[-1] = min(self.ymin[-1], value)
        self._update_ranges(value, alarms)
        self.alarms = alarms

        width = self.width()
        height = self._plot_frac * self.height()

        step = width / self._p_max

        nonzero = np.nonzero(self.ycnts)
        x = (self.tvals[nonzero] * step).astype(np.int32)
        y = self._scale_y(self.yvals[nonzero] / self.ycnts[nonzero], height)

        if y is None:
            return

        ymin = self._scale_y(self.ymin[nonzero], height)
        ymax = self._scale_y(self.ymax[nonzero], height)

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
                                    range=[0, self.time_range], weights=y)

        counts, _ = np.histogram(x, bins=self._p_max,
                                 range=[0, self.time_range])

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

    def __init__(self, box, parent):
        super(DisplaySparkline, self).__init__(None)

        self.widget = QWidget(parent)
        self.layout = QHBoxLayout(self.widget)
        self.widget.setFixedHeight(self._height)

        self.lineedit = QLineEdit()
        self.lineedit.setReadOnly(True)
        self.renderarea = SparkRenderer(self.widget)
        self.renderarea.setFixedWidth(self._width)
        self.tspan = self.renderarea.time_range

        self.layout.addWidget(self.lineedit)
        self.layout.addWidget(self.renderarea)

        self._value = None
        self.box = box

        self.box.signalHistoricData.connect(self.onHistoricData)
        self.box.visibilityChanged.connect(self.onVisibilityChanged)

        now = time.time()
        self.getPropertyHistory(now - self.tspan, now, True)

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
                self.lineedit.setText(str(value))

        self._value = value

    @pyqtSlot(bool)
    def onVisibilityChanged(self, visible):
        if visible:
            now = time.time()
            self.getPropertyHistory(now - self.tspan, now)

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
