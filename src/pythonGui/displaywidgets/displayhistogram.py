#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on March 2, 2012
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

__all__ = ["DisplayHistogram", "HistogramItem"]


import sys
from widget import DisplayWidget
from randomcolor import RandomColor

from PyQt4.QtCore import *
from PyQt4.QtGui import *

import numpy as np
try:
    from guiqwt.plot import CurveDialog
    from guiqwt.builder import make
    from PyQt4.Qwt5 import *
    useGuiQwt = True
except:
    from PyQt4.Qwt5 import *
    useGuiQwt = False


class DisplayHistogram(DisplayWidget):
    category = "List"
    alias = "Histogram"
    minMaxAssociatedKeys = 1, 10

    def __init__(self, key, **params):
        super(DisplayHistogram, self).__init__(**params)
        
        if useGuiQwt:
            self.__histogramWidget = CurveDialog(edit=False, toolbar=True, wintitle="Histogram")
            self.__plot = self.__histogramWidget.get_plot()
        else:        
            self.__histogramWidget = QwtPlot()
            self.__histogramWidget.setMinimumSize(QSize(200,200))
            # Attach grid to plot
            grid = QwtPlotGrid()
            grid.enableXMin(True)
            grid.enableYMin(True)
            grid.setMajPen(QPen(Qt.black, 0, Qt.DotLine))
            grid.setMinPen(QPen(Qt.gray, 0, Qt.DotLine))
            grid.attach(self.__histogramWidget)
            
        self.__plotCurves = []
            
        # Default colors
        self.__colorList = ["red", "green", "blue", "gray", "violet", "orange", "lightgreen", "black"]
            
        # Stores key/value pair
        self.__keys = {key: None}


    @property
    def widget(self):
        return self.__histogramWidget


    @property
    def keys(self):
        return self.__keys.keys()


    value = None

    def addKeyValue(self, key, value):
        self.valueChanged(key, value)


    def removeKey(self, key):
        if key in self.__keys:
            self.__keys.pop(key)


    def valueChanged(self, key, value, timestamp=None):
        key = str(key)
        self.__keys[key] = value
        
        # Update plot
        if useGuiQwt:
            while len(self.__plotCurves) > 0:
                self.__plot.del_item(self.__plotCurves.pop())

            index = -1
            for key in self.__keys:
                value = self.__keys.get(key)
                if value is None:
                    continue
                
                index += 1
                curveItem = make.curve(range(0, len(value), 1), value, self.__colorList[index])
                
                self.__plot.add_item(curveItem)
                
                self.__plotCurves.append(curveItem)
            
            self.__plot.replot()
        else:
            while len(self.__plotCurves) > 0:
                self.__plotCurves.pop().detach()

            index = -1
            for key in self.__keys:
                value = self.__keys.get(key)
                if value is None:
                    continue

                index += 1
                
                i = 0
                width = 1
                intervals = []
                values = QwtArrayDouble(len(value))
                
                for element in value:
                    if isinstance(element, str):
                        return # TODO: what happens if string in list?
                    xValue = i+width
                    intervals.append(QwtDoubleInterval(i, xValue))
                    yValue = float(element)
                    values[i] = yValue
                    
                    i += width
                
                # Attach histogram item to plot
                histogram = HistogramItem()
                histogram.setColor(QColor(self.__colorList[index]))
                histogram.setData(QwtIntervalData(intervals, values))
                histogram.attach(self.__histogramWidget)
                
                self.__plotCurves.append(histogram)
            
            self.__histogramWidget.replot()


class HistogramItem(QwtPlotItem):

    Auto = 0
    Xfy = 1
    
    def __init__(self, *args):
        QwtPlotItem.__init__(self, *args)
        
        self.__attributes = HistogramItem.Auto
        self.__data = QwtIntervalData()
        self.__color = QColor()
        self.__reference = 0.0
        self.setItemAttribute(QwtPlotItem.AutoScale, True)
        self.setItemAttribute(QwtPlotItem.Legend, True)
        self.setZ(20.0)


    def setData(self, data):
        self.__data = data
        self.itemChanged()


    def data(self):
        return self.__data


    def setColor(self, color):
        if self.__color != color:
            self.__color = color
            self.itemChanged()


    def color(self):
        return self.__color


    def boundingRect(self):
        result = self.__data.boundingRect()
        if not result.isValid():
            return result
        if self.testHistogramAttribute(HistogramItem.Xfy):
            result = QwtDoubleRect(result.y(), result.x(),
                                       result.height(), result.width())
            if result.left() > self.baseline():
                result.setLeft(self.baseline())
            elif result.right() < self.baseline():
                result.setRight(self.baseline())
        else:
            if result.bottom() < self.baseline():
                result.setBottom(self.baseline())
            elif result.top() > self.baseline():
                result.setTop(self.baseline())
        return result


    def draw(self, painter, xMap, yMap, rect):
        iData = self.data()
        painter.setPen(self.color())
        x0 = xMap.transform(self.baseline())
        y0 = yMap.transform(self.baseline())
        for i in range(iData.size()):
            if self.testHistogramAttribute(HistogramItem.Xfy):
                x2 = xMap.transform(iData.value(i))
                if x2 == x0:
                    continue

                y1 = yMap.transform(iData.interval(i).minValue())
                y2 = yMap.transform(iData.interval(i).maxValue())

                if y1 > y2:
                    y1, y2 = y2, y1
                    
                if  i < iData.size()-2:
                    yy1 = yMap.transform(iData.interval(i+1).minValue())
                    yy2 = yMap.transform(iData.interval(i+1).maxValue())

                    if y2 == min(yy1, yy2):
                        xx2 = xMap.transform(iData.interval(i+1).minValue())
                        if xx2 != x0 and ((xx2 < x0 and x2 < x0)
                                          or (xx2 > x0 and x2 > x0)):
                            # One pixel distance between neighboured bars
                            y2 += 1

                self.drawBar(
                    painter, Qt.Horizontal, QRect(x0, y1, x2-x0, y2-y1))
            else:
                y2 = yMap.transform(iData.value(i))
                if y2 == y0:
                    continue

                x1 = xMap.transform(iData.interval(i).minValue())
                x2 = xMap.transform(iData.interval(i).maxValue())

                if x1 > x2:
                    x1, x2 = x2, x1

                if i < iData.size()-2:
                    xx1 = xMap.transform(iData.interval(i+1).minValue())
                    xx2 = xMap.transform(iData.interval(i+1).maxValue())
                    x2 = min(xx1, xx2)
                    yy2 = yMap.transform(iData.value(i+1))
                    if x2 == min(xx1, xx2):
                        if yy2 != 0 and (( yy2 < y0 and y2 < y0)
                                         or (yy2 > y0 and y2 > y0)):
                            # One pixel distance between neighboured bars
                            x2 -= 1
                
                self.drawBar(painter, Qt.Vertical, QRect(x1, y0, x2-x1, y2-y0))


    def setBaseline(self, reference):
        if self.baseline() != reference:
            self.__reference = reference
            self.itemChanged()

    
    def baseline(self,):
        return self.__reference

    def setHistogramAttribute(self, attribute, on = True):
        if self.testHistogramAttribute(attribute):
            return

        if on:
            self.__attributes |= attribute
        else:
            self.__attributes &= ~attribute

        self.itemChanged()


    def testHistogramAttribute(self, attribute):
        return bool(self.__attributes & attribute) 


    def drawBar(self, painter, orientation, rect):
        painter.save()
        color = painter.pen().color()
        r = rect.normalized()
        factor = 125;
        light = color.light(factor)
        dark = color.dark(factor)

        painter.setBrush(color)
        painter.setPen(Qt.NoPen)
        QwtPainter.drawRect(painter, r.x()+1, r.y()+1, r.width()-2, r.height()-2)

        painter.setBrush(Qt.NoBrush)

        painter.setPen(QPen(light, 2))
        QwtPainter.drawLine(painter, r.left()+1, r.top()+2, r.right()+1, r.top()+2)

        painter.setPen(QPen(dark, 2))
        QwtPainter.drawLine(painter, r.left()+1, r.bottom(), r.right()+1, r.bottom())

        painter.setPen(QPen(light, 1))
        QwtPainter.drawLine(painter, r.left(), r.top() + 1, r.left(), r.bottom())
        QwtPainter.drawLine(painter, r.left()+1, r.top()+2, r.left()+1, r.bottom()-1)

        painter.setPen(QPen(dark, 1))
        QwtPainter.drawLine(painter, r.right()+1, r.top()+1, r.right()+1, r.bottom())
        QwtPainter.drawLine(painter, r.right(), r.top()+2, r.right(), r.bottom()-1)

        painter.restore()

