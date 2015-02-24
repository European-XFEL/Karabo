
__all__ = ["DisplayTrendline"]

import time
import datetime
from bisect import bisect

from manager import Manager
from widget import DisplayWidget

from PyQt4.QtCore import Qt, QObject, QTimer, pyqtSlot

import numpy

from PyQt4.Qwt5.Qwt import (QwtPlot, QwtScaleDraw, QwtText,
                            QwtLinearScaleEngine, QwtScaleDiv)
from guiqwt.plot import CurveDialog, PlotManager
from guiqwt.tools import SelectPointTool
from guiqwt.builder import make
from guiqwt import signals

from karabo.timestamp import Timestamp


class Curve(QObject):
    ini = 1000 # Maximum size of data container
    spare = 200 # Buffer until historic data is requested
    maxHistory = 400 # Limits amount of data from past

    def __init__(self, box, curve, parent):
        QObject.__init__(self, parent)
        self.curve = curve
        self.box = box
        self.data = numpy.empty((self.ini, 2), dtype=float) # Data container 
        self.fill = 0 # Actual fill size (historic + current)
        self.past = 0 # How much belongs to past
        box.signalHistoricData.connect(self.onHistoricData)


    def addPoint(self, value, timestamp):
        if self.fill >= self.data.shape[0]: # Have to get rid of data
            # Emergency case, should only happen if historic data is slow
            self.data[self.past:-self.spare, :] = (
                self.data[self.past + self.spare:, :])
            self.fill -= self.spare
        # Spare is touched, ask for historic data
        elif self.fill == self.data.shape[0] - self.spare:
            self.getPropertyHistory(self.data[0, 1],
                self.data[(self.past + self.data.shape[0]) // 2, 1])
        self.data[self.fill, :] = value, timestamp
        self.fill += 1
        self.update()


    def getPropertyHistory(self, t0, t1):
        t0 = str(datetime.datetime.utcfromtimestamp(t0).isoformat())
        t1 = str(datetime.datetime.utcfromtimestamp(t1).isoformat())
        self.box.getPropertyHistory(t0, t1, self.maxHistory)


    def changeInterval(self, t0, t1):
        # t0 represent the oldest value displayed in the widget 
        if t0 < self.data[0, 1] or t1 < self.data[self.past, 1]:
            self.getPropertyHistory(t0, min(t1, self.data[self.past, 1]))


    def update(self):
        self.curve.set_data(self.data[:self.fill, 1], self.data[:self.fill, 0])


    @pyqtSlot(object, object)
    def onHistoricData(self, box, data):
        if not data:
            return
        l = [(e['v'], Timestamp.fromHashAttributes(e.getAttributes('v')).
             toTimestamp()) for e in data]
        pos = self.data[:self.fill, 1].searchsorted(l[-1][1])
        newsize = max(self.ini, self.fill - pos + self.spare)
        data = numpy.empty((len(l) + newsize, 2), dtype=float)
        data[:len(l), :] = l
        data[len(l):len(l) + self.fill - pos, :] = self.data[pos:self.fill, :]
        self.fill = len(l) + self.fill - pos
        self.data = data
        self.past = len(l)
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
    category = "Digit"
    alias = "Trendline"

 
    def __init__(self, box, parent):
        super(DisplayTrendline, self).__init__(None)
        self.dialog = CurveDialog(edit=False, toolbar=True,
                                  wintitle="Trendline")
        self.plot = self.dialog.get_plot()
        self.plot.set_antialiasing(True)
        self.plot.setAxisTitle(QwtPlot.xBottom, 'Time')
        self.plot.setAxisTitle(QwtPlot.yLeft, box.descriptor.displayedName)

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
        self.plot.setAxisScale(QwtPlot.xBottom,
                               round(time.time() - 1), round(time.time() + 10))
        self.plot.setAxisLabelAlignment(QwtPlot.xBottom,
                                        Qt.AlignRight | Qt.AlignBottom)
        self.addBox(box)


    def addBox(self, box):
        curve = make.curve([ ], [ ], 'Random values', "r")
        self.curves[box] = Curve(box, curve, self.dialog)
        self.plot.add_item(curve)
        return True


    @property
    def widget(self):
        return self.dialog


    value = None


    def removeKey(self, key):
        self.plot.remove_item(self.curves[key])
        del self.curves[key]


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
            blocked = aw.blockSignals(True)
            self.plot.setAxisScale(
                QwtPlot.xBottom,
                self.plot.axisScaleDiv(QwtPlot.xBottom).lowerBound(),
                t + 10)
            aw.blockSignals(blocked)

        self.lasttime = timestamp.toTimestamp()
        self.plot.replot()


    def scaleChanged(self):
        asd = self.plot.axisScaleDiv(QwtPlot.xBottom)
        t0, t1 = asd.lowerBound(), asd.upperBound()
        for v in self.curves.values():
            v.changeInterval(t0, t1)
