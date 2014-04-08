#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on August 14, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a widget plugin for attributes
   and is created by the factory class DisplayWidget.
   
   Each plugin needs to implement the following interface:
   
   def getCategoryAliasClassName():
       pass
   
    class Maker:
        def make(self, **params):
            return Attribute*(**params)
"""

__all__ = ["DisplayTrendline"]


import time
import datetime

from manager import Manager
from widget import DisplayWidget

from PyQt4.QtCore import Qt, QObject, QTimer
from PyQt4.QtGui import QColor

import numpy

useGuiQwt = True
try:
    from PyQt4.Qwt5.Qwt import QwtPlot, QwtScaleDraw, QwtText
    from guiqwt.plot import CurveDialog, PlotManager
    from guiqwt.tools import SelectPointTool
    from guiqwt.builder import make
    from guiqwt import signals
except:
    print "Missing package guiqwt (this is normal under MacOSX and will come later)"
    useGuiQwt = False

from timestamp import Timestamp


class Curve(QObject):
    ini = 1000
    spare = 100

    def __init__(self, box, curve):
        self.curve = curve
        self.box = box
        self.data = numpy.empty((self.ini, 2), dtype=float)
        self.fill = 0
        self.past = 0
        box.signalHistoricData.connect(self.onHistoricData)


    def addPoint(self, value, timestamp):
        if self.fill >= self.data.shape[0]:
            self.data[self.past:-self.spare, :] = (
                self.data[self.past + self.spare:, :])
            self.fill -= self.spare
        elif self.fill >= self.data.shape[0] - self.spare:
            self.getFromPast(self.data[0, 1],
                self.data[(self.past + self.data.shape[0]) // 2, 1])
        self.data[self.fill, :] = value, timestamp
        self.fill += 1
        self.update()


    def getFromPast(self, t0, t1):
        t0 = datetime.datetime.utcfromtimestamp(t0)
        t1 = datetime.datetime.utcfromtimestamp(t1)
        self.box.getFromPast(t0.isoformat(), t1.isoformat())


    def changeInterval(self, t0, t1):
        if t0 < self.data[0, 1] or (
                self.past != 0 and t0 < self.data[self.past - 1, 1] and
                t1 > self.data[self.past, 1]):
            self.getFromPast(t0, min(t1, self.data[self.past, 1]))


    def update(self):
        self.curve.set_data(self.data[:self.fill, 1], self.data[:self.fill, 0])


    def onHistoricData(self, box, data):
        l = [(e['v'], Timestamp.fromHashAttributes(e.getAttributes('v')).
             toTimestamp()) for e in data]
        pos = self.data[:self.fill, 1].searchsorted(l[0][1])
        data = numpy.empty((len(l) + self.fill - pos + self.ini, 2),
                           dtype=float)
        data[:len(l), :] = l[::-1]
        data[len(l):-self.ini, :] = self.data[pos:self.fill, :]
        self.fill = data.shape[0] - self.ini
        self.data = data
        self.update()


class DateTimeScaleDraw(QwtScaleDraw):
        '''Class used to draw a datetime axis on our plot. '''
        def __init__(self, *args):
            QwtScaleDraw.__init__( self, *args )


        def label(self, value):
            '''create the text of each label to draw the axis. '''
            try:
                dt = datetime.datetime.fromtimestamp(value)
            except:
                dt = datetime.datetime.fromtimestamp(0)
            return QwtText(dt.isoformat())
        

class DisplayTrendline(DisplayWidget):
    category = "Digit"
    alias = "Trendline"

 
    def __init__(self, box, parent):
        super(DisplayTrendline, self).__init__(None)
        
        if not useGuiQwt:
            self.dialog = None
            return

        self.dialog = CurveDialog(edit=False, toolbar=True,
                                  wintitle="Trendline")
        self.plot = self.dialog.get_plot()
        self.plot.set_antialiasing(True)
        self.timer = QTimer(self)
        self.timer.setInterval(1000)
        self.timer.setSingleShot(True)
        self.plot.axisWidget(QwtPlot.xBottom).scaleDivChanged.connect(
            self.timer.start)
        self.timer.timeout.connect(self.scaleChanged)
        
        self.plot.setAxisTitle(QwtPlot.xBottom, 'Time')
        self.plot.setAxisTitle(QwtPlot.yLeft, 'Value')
        
        self.curves = { }

        self.manager = PlotManager(self)
        self.manager.add_plot(self.plot)

        self.manager.register_all_curve_tools( )
        self.manager.register_other_tools()
        self.manager.add_tool(SelectPointTool, title='Test',
                              on_active_item=True, mode='create')
        
        make.legend('TL')

        self.plot.setAxisScaleDraw(QwtPlot.xBottom, DateTimeScaleDraw())
        self.plot.setAxisAutoScale(QwtPlot.yLeft)
        self.lasttime = time.time()
        self.plot.setAxisScale(QwtPlot.xBottom,
                               round(time.time() - 1), round(time.time() + 10))
        self.plot.setAxisLabelRotation(QwtPlot.xBottom, -45.0)
        self.plot.setAxisLabelAlignment(QwtPlot.xBottom,
                                        Qt.AlignLeft | Qt.AlignBottom)
        self.addBox(box)


    def addBox(self, box):
        curve = make.curve([ ], [ ], 'Random values', QColor(255, 0, 0))
        self.curves[box] = Curve(box, curve)
        self.plot.add_item(curve)
        box.addWidget(self)
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
        return self.curves.keys()


    def valueChanged(self, box, value, timestamp=None):
        if timestamp is None:
            timestamp = Timestamp()

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
        for v in self.curves.itervalues():
            v.changeInterval(t0 + 0.001, t1 + 0.001)
