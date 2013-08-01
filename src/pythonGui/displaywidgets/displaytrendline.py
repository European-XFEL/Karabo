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

useGuiQwt = True
try:
    from PyQt4 import Qwt5
    from guiqwt.plot import CurveDialog, CurvePlot, PlotManager
    from guiqwt.tools import SelectPointTool
    from guiqwt.builder import make    
except:
    print "Missing package guiqwt (this is normal under MacOSX and will come later)"
    useGuiQwt = False

from karabo.karathon import Timestamp


def getCategoryAliasClassName():
    return ["Digit","Trendline","DisplayTrendline"]


class DateTimeScaleDraw( Qwt5.Qwt.QwtScaleDraw ):
        '''Class used to draw a datetime axis on our plot.
        '''
        def __init__( self, *args ):
            Qwt5.Qwt.QwtScaleDraw.__init__( self, *args )


        def label( self, value ):
            '''Function used to create the text of each label
            used to draw the axis.
            '''
            try:
                dt = datetime.datetime.fromtimestamp( value )
            except:
                dt = datetime.datetime.fromtimestamp( 0 )
            #return Qwt5.Qwt.QwtText( '%s' % dt.strftime( '%d/%m%Y %H:%M:%S' ) )
            return Qwt5.Qwt.QwtText( '%s' % dt.strftime( '%H:%M:%S' ) )
        

class DisplayTrendline(DisplayWidget):
    
    def __init__(self, **params):
        super(DisplayTrendline, self).__init__(**params)
        
        if useGuiQwt == False:
            self.__plot = None
            return

        self.__data = []
        
        self.__dialog = CurveDialog(edit=False, toolbar=True)
        self.__dialog.resize(400, 600)
        
        self.__plot = self.__dialog.get_plot()
        self.__plot.set_antialiasing( True )
        
         # Set axis's labels
        self.__plot.setAxisTitle( Qwt5.Qwt.QwtPlot.xBottom, 'Time' )
        self.__plot.setAxisTitle( Qwt5.Qwt.QwtPlot.yLeft, 'Value' )
        
          # Create the curves
        self.__curve = make.curve( [ ], [ ], 'Random values', QColor( 255, 0, 0 ) )
        self.__plot.add_item( self.__curve )

        # Crate the PlotManager
        self.__manager = PlotManager( self )
        self.__manager.add_plot( self.__plot )

        # Create Toolbar
        #toolbar = self.addToolBar( 'tools' )
        #self.__manager.add_toolbar( toolbar, id( toolbar ) )

        # Register the ToolBar's type
        self.__manager.register_all_curve_tools( )
        self.__manager.register_other_tools()

        # Register a custom tool
        self.__manager.add_tool( SelectPointTool, title = 'Test', on_active_item = True, mode = 'create' )
        
        # Create the Legend
        legend = make.legend( 'TL' )
        #self.__plot.add_item( legend )

        # Setup the plot's scale
        self.__plot.setAxisScaleDraw( Qwt5.Qwt.QwtPlot.xBottom, DateTimeScaleDraw() )
        self.__plot.setAxisAutoScale( Qwt5.Qwt.QwtPlot.yLeft )
        
        # Set value
#        value = params.get(QString('value'))
#        if value is None:
#            value = params.get('value')
#        if value is not None:
#            self.valueChanged(key, value)


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
        if value is None or useGuiQwt is False:
            return
        
        if timestamp is None:
            # Generate timestamp here...
            timestamp = Timestamp()
            
        self.__data.append( (value, timestamp.getSeconds()) )

        key = str(key)
        
         # Set x-axis's label rotation
        self.__plot.setAxisLabelRotation( Qwt5.Qwt.QwtPlot.xBottom, -45.0 )
        self.__plot.setAxisLabelAlignment( Qwt5.Qwt.QwtPlot.xBottom, Qt.AlignLeft | Qt.AlignBottom )
                
        # Set the data to the curve and update the plot
        self.__curve.set_data( map( lambda x: x[ 1 ], self.__data ), map( lambda x: x[ 0 ], self.__data ) )
        self.__plot.replot()        


    

    class Maker:
        def make(self, **params):
            return DisplayTrendline(**params)
