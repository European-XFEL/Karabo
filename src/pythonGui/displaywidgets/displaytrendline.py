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


import datetime
import sys
import time

from displaywidget import DisplayWidget
from randomcolor import RandomColor

from PyQt4.QtCore import *
from PyQt4.QtGui import *

try:
    from guiqwt.baseplot import BasePlot
    from guiqwt.plot import CurveDialog
    from guiqwt.builder import make
    useGuiQwt = True
except:
    from PyQt4.Qwt5 import *
    useGuiQwt = False


def getCategoryAliasClassName():
    return ["Digit","Trendline","DisplayTrendline"]


class DisplayTrendline(DisplayWidget):
    
    def __init__(self, **params):
        super(DisplayTrendline, self).__init__(**params)
        
        # Minimum and maximum number of associated keys, 1 by default for each
        self.__minMaxAssociatedKeys = (1,10) # tuple<min,max>
        
        if useGuiQwt:
            self.__plot = CurveDialog(edit=True, toolbar=True)
        else:        
            self.__plot = QwtPlot()
            self.__plot.setMinimumSize(QSize(200,200))

            # Attach grid to plot
            grid = QwtPlotGrid()
            grid.enableXMin(True)
            grid.enableYMin(True)
            grid.setMajPen(QPen(Qt.black, 0, Qt.DotLine))
            grid.setMinPen(QPen(Qt.gray, 0, Qt.DotLine))
            grid.attach(self.__plot)
            
        key = params.get(QString('key'))
        if key is None:
            key = params.get('key')
        # Stores key and list of x/y valuePairs in dictionary
        self.__keys = {str(key):[]}
        
        # Set value
        value = params.get(QString('value'))
        if value is None:
            value = params.get('value')
        if value is not None:
            self.valueChanged(key, value)


    def _getCategory(self):
        category, alias, className = getCategoryAliasClassName()
        return category
    category = property(fget=_getCategory)


    # Returns the actual widget which is part of the composition
    def _getWidget(self):
        return self.__plot
    widget = property(fget=_getWidget)


    # Returns a tuple of min and max number of associated keys with this component
    def _getMinMaxAssociatedKeys(self):
        return self.__minMaxAssociatedKeys
    minMaxAssociatedKeys = property(fget=_getMinMaxAssociatedKeys)


    def _getKeys(self):
        return self.__keys.keys()
    keys = property(fget=_getKeys)


    def _value(self):
        print "DisplayTrendline.value"
        return None
    value = property(fget=_value)


    def addKeyValue(self, key, value):
        self.valueChanged(key, value)


    def removeKey(self, key):
        if key in self.__keys:
            self.__keys.pop(key)


    def valueChanged(self, key, value, timestamp=None):
        if value is None:
            return
        
        if timestamp is None:
            # Generate timestamp here...
            timestamp = time.mktime(datetime.datetime.now().timetuple())

        key = str(key)
        nbValues = 10
        # Store value in list associated with key
        if key in self.__keys:
            valuePair = self.__keys[key]
            if valuePair:
                if len(valuePair) is nbValues:
                    # Only show 10 values at once
                    valuePair.remove(valuePair[0])
                    if useGuiQwt:
                        plot = self.__plot.get_plot()
                        plot.del_all_items()
                    else:
                        self.__plot.clear()
                valuePair.append((timestamp,value))
                self.__keys[key] = valuePair
            else:
                self.__keys[key] = [(timestamp,value)]
        else:
            self.__keys[key] = [(timestamp,value)]
        
        xMinValue = float(sys.maxint)
        xMaxValue = float(-sys.maxint)
        yMinValue = float(sys.maxint)
        yMaxValue = float(-sys.maxint)
        
        for k in self.__keys.keys():
            valuePair = self.__keys[k]
            if valuePair is None: continue

            xValues = []
            yValues = []
            for i in xrange(len(valuePair)):
                xValue = valuePair[i][0]
                yValue = valuePair[i][1]
                
                xValues.append(xValue)
                yValues.append(yValue)
            
                if xMinValue > xValue:
                    xMinValue = xValue
                if xMaxValue < xValue:
                    xMaxValue = xValue
                if yMinValue > yValue:
                    yMinValue = yValue
                if yMaxValue < yValue:
                    yMaxValue = yValue
            
            if useGuiQwt:
                plot = self.__plot.get_plot()
                #plot.del_all_items()
                
                # Remove only legend item
                #plot.del_items(legendItem)
                
                curve = make.curve(xValues, yValues, title="Curve", color="g") # TODO: associate curve data with one color
                #legend = make.legend("TR")

                plot.add_item(curve)
                #plot.add_item(legend)
                
                # TODO: add legend
                
                plot.set_axis_limits(BasePlot.Y_LEFT, yMinValue, yMaxValue)
                plot.set_axis_limits(BasePlot.X_BOTTOM, xMinValue, xMaxValue)
                plot.replot()
            else:
                curve = QwtPlotCurve() # TODO: name of attribute
                curve.setPen(QPen(RandomColor())) # TODO: associate curve data with one color
                curve.setData(xValues, yValues)
                curve.attach(self.__plot)
                
                # TODO: add legend
                #self.__plot.insertLegend(legend)
                
                self.__plot.setAxisScale(QwtPlot.yLeft, yMinValue, yMaxValue)
                self.__plot.setAxisScale(QwtPlot.xBottom, xMinValue, xMaxValue)
                self.__plot.replot()


    class Maker:
        def make(self, **params):
            return DisplayTrendline(**params)
