
__all__ = ["DisplayTrendline"]

import time
import datetime
from bisect import bisect
import pickle
import base64
from xml.etree.ElementTree import Element

from const import ns_karabo
from manager import getDevice
from network import Network
from util import SignalBlocker
from widget import DisplayWidget

from PyQt4.QtCore import Qt, QObject, QTimer, pyqtSlot

import numpy

from PyQt4.Qwt5.Qwt import (QwtPlot, QwtScaleDraw, QwtText,
                            QwtLinearScaleEngine, QwtScaleDiv)
from guiqwt.plot import CurveDialog, PlotManager
from guiqwt.tools import SelectPointTool
from guiqwt.builder import make

from karabo.hash import Simple
from karabo.timestamp import Timestamp


class Curve(QObject):
    """This holds the data for one curve

    the currently to be shown data is in self.x and self.y.
    Up to self.histsize it is filled with historical data, up to self.fill
    it is then filled with "current" data, meaning data that has been
    accumulated from the changes coming in.

    There is a second data structure, self.datas, which is a list of arrays
    with averaged data, always self.base points are averaged over and
    put into the next higher aggregated storage.

    Once the basic storage in self.x and self.y flows over, it gets replaced
    by the averaged data in self.datas. """
    generations = 4
    base = 10  # number of points to combine per generation
    size = 200
    spare = 100
    maxHistory = 500  # Limits amount of data from past
    sparsesize = 400
    minHistory = 100  # minimum number of points shown (if possible)

    dtype = numpy.dtype([("avgx", "f8"), ("avgy", "f8")])

    def __init__(self, box, curve, parent):
        QObject.__init__(self, parent)
        self.curve = curve
        self.box = box
        self.datas = [numpy.empty(self.size, dtype=self.dtype)
                      for i in range(self.generations)]

        self.histsize = 0
        self.fill = 0
        rsize = self.size * self.generations + self.spare
        self.x = numpy.empty(rsize, dtype=float)
        self.y = numpy.empty(rsize, dtype=float)
        self.dx = numpy.empty(rsize, dtype=float)
        self.dy = numpy.empty(rsize, dtype=float)

        self.fills = [0] * self.generations
        self.startpast = self.endpast = 0
        self.t0 = self.t1 = 0
        box.signalHistoricData.connect(self.onHistoricData)
        box.visibilityChanged.connect(self.onVisibilityChanged)
        self.timer = None

    def addPoint(self, value, timestamp):
        next = (timestamp, value)
        n = 1
        for i, data in reversed(list(enumerate(self.datas))):
            data[self.fills[i]] = next
            self.fills[i] += 1
            if self.fills[i] < len(data):
                break
            next = (data[:self.base]["avgx"].mean(),
                    data[:self.base]["avgy"].mean())
            data[:-self.base] = data[self.base:]
            self.fills[i] -= self.base
            n *= self.base
        else:
            self.datas[0][1:] = self.datas[0][:-1]
            self.fills[0] -= 1

        self.x[self.fill] = timestamp
        self.y[self.fill] = value
        self.dx[self.fill] = self.dy[self.fill] = 0
        self.fill += 1

        if self.fill == len(self.x):
            self.fill_current()
        self.update()

    def fill_current(self):
        pos = self.histsize
        for fill, data in zip(self.fills, self.datas):
            d = data[:fill]
            self.x[pos:pos + fill] = d["avgx"]
            self.y[pos:pos + fill] = d["avgy"]
            pos += fill
        self.fill = pos

    def getPropertyHistory(self, t0, t1):
        t0 = str(datetime.datetime.utcfromtimestamp(t0).isoformat())
        t1 = str(datetime.datetime.utcfromtimestamp(t1).isoformat())
        self.box.getPropertyHistory(t0, t1, self.maxHistory)

    def changeInterval(self, t0, t1):
        p0 = self.x[:self.fill].searchsorted(t0)
        p1 = self.x[:self.fill].searchsorted(t1)
        if (p1 - p0 < self.minHistory and self.histsize > self.sparsesize or
                self.x[p0] > 0.9 * self.t0 + 0.1 * self.t1 or
                p1 < self.histsize and
                self.x[p1 - 1] < 0.1 * self.t0 + 0.9 * self.t1):
            self.getPropertyHistory(t0, t1)
        self.t0 = t0
        self.t1 = t1

    def update(self):
        """show the new data to screen

        As showing the data is actually a time consuming task, it is only
        done if the event loop has nothing else in it. """
        if self.timer is None:
            self.timer = self.startTimer(0)

    def timerEvent(self, event):
        if Network().isDataPending():
            return
        self.killTimer(self.timer)
        self.timer = None
        self.curve.set_data(self.x[:self.fill], self.y[:self.fill])

    @pyqtSlot(bool)
    def onVisibilityChanged(self, visible):
        if visible and self.t1 >= self.x[self.histsize]:
            self.getPropertyHistory(self.t0, self.t1)

    @pyqtSlot(object, object)
    def onHistoricData(self, box, data):
        if not data:
            return

        rsize = self.size * self.generations + len(data) + self.spare
        x = numpy.empty(rsize, dtype=float)
        y = numpy.empty(rsize, dtype=float)

        for i, d in enumerate(data):
            x[i] = Timestamp.fromHashAttributes(d['v', ...]).toTimestamp()
            y[i] = d["v"]

        p0 = self.x[:self.fill].searchsorted(self.t0)
        p1 = self.x[:self.fill].searchsorted(self.t1)
        np0 = x[:len(data)].searchsorted(self.t0)
        np1 = x[:len(data)].searchsorted(self.t1)

        span = (self.x[p1 - 1] - self.x[p0]) / (self.t1 - self.t0)
        nspan = (x[np1 - 1] - x[np0]) / (self.t1 - self.t0)

        if (np1 - np0 < p1 - p0) and not nspan > span < 0.9:
            return

        end = x[len(data) - 1]
        for i, d in enumerate(self.datas):
            fill = self.fills[i]
            if fill == 0:
                continue
            pos = d[:fill]["avgx"].searchsorted(end)
            if pos == 0:
                break
            d[:fill - pos] = d[pos:fill]
            self.fills[i] = fill - pos

        self.histsize = len(data)
        self.x = x
        self.y = y
        self.dx = numpy.zeros(rsize, dtype=float)
        self.dy = numpy.zeros(rsize, dtype=float)
        self.fill_current()
        self.update()


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
            except:
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

class DisplayTrendline(DisplayWidget):
    category = Simple
    alias = "Trendline"

 
    def __init__(self, box, parent):
        super(DisplayTrendline, self).__init__(None)
        self.dialog = CurveDialog(edit=False, toolbar=True,
                                  wintitle="Trendline")
        self.plot = self.dialog.get_plot()
        self.plot.set_antialiasing(True)
        self.plot.setAxisTitle(QwtPlot.xBottom, 'Time')

        # have a 1 s timeout to request data, thus avoid
        # frequent re-loading while scaling
        self.timer = QTimer(self)
        self.timer.setInterval(1000)
        self.timer.setSingleShot(True)
        self.plot.axisWidget(QwtPlot.xBottom).scaleDivChanged.connect(
            self.timer.start)
        self.timer.timeout.connect(self.scaleChanged)

        self.curves = { }

        self.manager = PlotManager(self)
        self.manager.add_plot(self.plot)

        self.manager.register_all_curve_tools( )
        self.manager.register_other_tools()
        self.manager.add_tool(SelectPointTool, title='Test',
                              on_active_item=True, mode='create')

        drawer = DateTimeScaleDraw()
        self.plot.setAxisScaleEngine(QwtPlot.xBottom, ScaleEngine(drawer))
        self.plot.setAxisScaleDraw(QwtPlot.xBottom, drawer)
        self.plot.setAxisAutoScale(QwtPlot.yLeft)
        self.lasttime = time.time()
        self.wasVisible = False
        self.plot.setAxisScale(QwtPlot.xBottom,
                               round(time.time() - 1), round(time.time() + 10))
        self.plot.setAxisLabelAlignment(QwtPlot.xBottom,
                                        Qt.AlignRight | Qt.AlignBottom)
        self.destroyed.connect(self.destroy)
        self.addBox(box)


    def typeChanged(self, box):
        self.plot.setAxisTitle(QwtPlot.yLeft, box.descriptor.displayedName)


    def addBox(self, box):
        curve = make.curve([], [], box.key(), "r")
        self.curves[box] = Curve(box, curve, self.dialog)
        box.addVisible()
        self.plot.add_item(curve)
        return True

    @pyqtSlot(object)
    def destroy(self):
        for box in self.curves:
            box.removeVisible()

    @property
    def widget(self):
        return self.dialog


    value = None


    def removeKey(self, key):
        self.plot.remove_item(self.curves[key])
        del self.curves[key]
        key.removeVisible()


    @property
    def boxes(self):
        return list(self.curves)


    def valueChanged(self, box, value, timestamp=None):
        if timestamp is None:
            return

        t = timestamp.toTimestamp()
        self.curves[box].addPoint(value, t)
        t1 = self.plot.axisScaleDiv(QwtPlot.xBottom).upperBound()
        if self.lasttime < t1 < t:
            aw = self.plot.axisWidget(QwtPlot.xBottom)
            with SignalBlocker(aw):
                self.plot.setAxisScale(
                    QwtPlot.xBottom,
                    self.plot.axisScaleDiv(QwtPlot.xBottom).lowerBound(),
                    t + 10)

        self.lasttime = timestamp.toTimestamp()
        self.wasVisible = True
        self.plot.replot()

    def scaleChanged(self):
        asd = self.plot.axisScaleDiv(QwtPlot.xBottom)
        t0, t1 = asd.lowerBound(), asd.upperBound()
        for v in self.curves.values():
            v.changeInterval(t0, t1)

    def save(self, e):
        for k, v in self.curves.items():
            ee = Element(ns_karabo + "box")
            ee.set("device", k.configuration.id)
            ee.set("path", ".".join(k.path))
            ee.text = base64.b64encode(pickle.dumps(v.curve)).decode("ascii")
            e.append(ee)

    def load(self, e):
        for ee in e:
            box = getDevice(ee.get("device")).getBox(ee.get("path").split("."))
            curve = self.curves.get(box)
            self.plot.del_item(curve.curve)
            curve.curve = pickle.loads(base64.b64decode(ee.text))
            self.plot.add_item(curve.curve)
            curve.update()
