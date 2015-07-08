
__all__ = ["DisplayTrendline"]

import time
import datetime
from bisect import bisect
import pickle
import base64
from xml.etree.ElementTree import Element

from const import ns_karabo
from manager import Manager, getDevice
from util import SignalBlocker
from widget import DisplayWidget

from PyQt4.QtCore import Qt, QObject, QTimer, pyqtSlot

import numpy

from PyQt4.Qwt5.Qwt import (QwtPlot, QwtScaleDraw, QwtText,
                            QwtLinearScaleEngine, QwtScaleDiv)
from guiqwt.plot import CurveDialog, PlotManager
from guiqwt.tools import SelectPointTool
from guiqwt.builder import make
from guiqwt import signals

from karabo.hashtypes import Simple
from karabo.timestamp import Timestamp


class Curve(QObject):
    """This holds the data for one curve

    The data is contained in one member variable, data, which is
    an n-by-2 array, so that we have pairs of value and timestamp.

    The array is split in two: the first part, up to self.past, is the
    data that we got by inquiring from the past. The range that we acutally
    did inquire is self.startpast to self.endpast.

    Starting at self.past, we have current data. self.startcurrent is the time
    starting at which we have current data.
    The current data reaches up to self.fill. If less than
    self.spare slots are left over, we discard older current data,
    moving the current start time forward.

    When getting values from the past, we always ask for self.maxHistory
    values, and start trying to get more data if the user zooms in if
    less than self.minHistory points are visible. self.sparse is True
    if there simply are no more data to zoom in."""
    ini = 1000  # Initial size of data container
    spare = 200 # Buffer until historic data is requested
    maxHistory = 800 # Limits amount of data from past
    minHistory = 100  # minimum number of points shown (if possible)

    def __init__(self, box, curve, parent):
        QObject.__init__(self, parent)
        self.curve = curve
        self.box = box
        self.data = numpy.empty((self.ini, 2), dtype=float) # Data container 
        self.fill = 0 # Actual fill size (historic + current)
        self.past = 0 # How much belongs to past
        self.startcurrent = time.time()
        self.sparse = False
        self.startpast = self.endpast = 0
        self.t0 = self.t1 = 0
        box.signalHistoricData.connect(self.onHistoricData)
        box.visibilityChanged.connect(self.onVisibilityChanged)

    def addPoint(self, value, timestamp):
        if self.fill >= self.data.shape[0]: # Have to get rid of data
            # Emergency case, should only happen if historic data is slow
            self.data[self.past:-self.spare, :] = (
                self.data[self.past + self.spare:, :])
            self.breaktime = self.data[self.past, 1]
            self.fill -= self.spare
        # Spare is touched, ask for historic data
        elif self.fill == self.data.shape[0] - self.spare:
            self.getPropertyHistory(self.data[0, 1],
                self.data[(self.past + self.data.shape[0]) // 2, 1])
        self.data[self.fill, :] = value, timestamp
        if self.fill == self.past:
            self.breaktime = timestamp
        self.fill += 1
        self.update()


    def getPropertyHistory(self, t0, t1):
        self.startpast = t0
        self.endpast = t1
        t0 = str(datetime.datetime.utcfromtimestamp(t0).isoformat())
        t1 = str(datetime.datetime.utcfromtimestamp(t1).isoformat())
        self.box.getPropertyHistory(t0, t1, self.maxHistory)


    def changeInterval(self, t0, t1):
        # t0 represent the oldest value displayed in the widget 
        if t0 < self.startpast or (self.endpast < self.startcurrent and
                t1 > self.endpast and t0 < self.startcurrent):
            self.getPropertyHistory(t0, min(t1, self.startcurrent))
        if t1 < self.startcurrent and not self.sparse:
            p0 = self.data[:self.fill, 1].searchsorted(t0)
            p1 = self.data[:self.fill, 1].searchsorted(t1)
            if p1 - p0 < self.minHistory:
                self.getPropertyHistory(t0, t1)
        self.t0 = t0
        self.t1 = t1


    def update(self):
        self.curve.set_data(self.data[:self.fill, 1], self.data[:self.fill, 0])

    @pyqtSlot(bool)
    def onVisibilityChanged(self, visible):
        if visible and self.t1 >= self.startcurrent:
            self.getPropertyHistory(self.t0, self.t1)

    @pyqtSlot(object, object)
    def onHistoricData(self, box, data):
        if not data:
            return
        l = [(e['v'], Timestamp.fromHashAttributes(e.getAttributes('v')).
             toTimestamp()) for e in data]
        self.sparse = len(l) < self.maxHistory / 2
        pos = self.data[self.past:self.fill, 1].searchsorted(l[-1][1])
        newsize = max(self.ini, self.fill - pos - self.past + self.spare)
        data = numpy.empty((len(l) + newsize, 2), dtype=float)
        data[:len(l), :] = l
        newfill = len(l) + self.fill - pos - self.past
        data[len(l):newfill, :] = self.data[pos + self.past:self.fill, :]
        self.fill = newfill
        self.data = data
        self.past = len(l)
        if pos > 0:
            self.startcurrent = self.endpast
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
