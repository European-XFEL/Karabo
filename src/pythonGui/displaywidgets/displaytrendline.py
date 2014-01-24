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

from widget import DisplayWidget

from PyQt4.QtCore import Qt
from PyQt4.QtGui import QColor

import numpy

useGuiQwt = True
try:
    from PyQt4.Qwt5 import Qwt
    from guiqwt.plot import CurveDialog, PlotManager
    from guiqwt.tools import SelectPointTool
    from guiqwt.builder import make
except:
    print "Missing package guiqwt (this is normal under MacOSX and will come later)"
    useGuiQwt = False

from karabo.karathon import Timestamp


class DateTimeScaleDraw(Qwt.QwtScaleDraw):
        '''Class used to draw a datetime axis on our plot. '''
        def __init__(self, *args):
            Qwt.QwtScaleDraw.__init__( self, *args )


        def label(self, value):
            '''create the text of each label to draw the axis. '''
            try:
                dt = datetime.datetime.fromtimestamp(value)
            except:
                dt = datetime.datetime.fromtimestamp(0)
            return Qwt.QwtText('%s' % dt.strftime( '%H:%M:%S' ))
        

class DisplayTrendline(DisplayWidget):
    category = "Digit"
    alias = "Trendline"

 
    def __init__(self, **kwargs):
        super(DisplayTrendline, self).__init__(**kwargs)
        
        if not useGuiQwt:
            self.dialog = None
            return

        self.data = [ ]
        
        self.dialog = CurveDialog(edit=False, toolbar=True,
                                  wintitle="Trendline")
        self.plot = self.dialog.get_plot()
        self.plot.set_antialiasing(True)
        
        self.plot.setAxisTitle(Qwt.QwtPlot.xBottom, 'Time')
        self.plot.setAxisTitle(Qwt.QwtPlot.yLeft, 'Value')
        
        self.curve = make.curve([ ], [ ], 'Random values', QColor(255, 0, 0))
        self.plot.add_item(self.curve)

        self.manager = PlotManager(self)
        self.manager.add_plot(self.plot)

        self.manager.register_all_curve_tools( )
        self.manager.register_other_tools()
        self.manager.add_tool(SelectPointTool, title='Test',
                              on_active_item=True, mode='create')
        
        make.legend('TL')

        self.plot.setAxisScaleDraw(Qwt.QwtPlot.xBottom, DateTimeScaleDraw())
        self.plot.setAxisAutoScale(Qwt.QwtPlot.yLeft)


    @property
    def widget(self):
        return self.dialog


    value = None


    def valueChanged(self, key, value, timestamp=None):
        if value is None or not useGuiQwt:
            return
        
        if timestamp is None:
            # Generate timestamp here...
            timestamp = Timestamp()
            
        self.data.append((value, timestamp.getSeconds()))
        data = numpy.array(self.data)

        self.plot.setAxisLabelRotation(Qwt.QwtPlot.xBottom, -45.0)
        self.plot.setAxisLabelAlignment(Qwt.QwtPlot.xBottom,
                                        Qt.AlignLeft | Qt.AlignBottom)
                
        self.curve.set_data(data[:, 1], data[:, 0])
        self.plot.setAxisAutoScale(Qwt.QwtPlot.xBottom)
        self.plot.setAxisAutoScale(Qwt.QwtPlot.yLeft)
        self.plot.replot()
