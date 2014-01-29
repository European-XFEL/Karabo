#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 30, 2012
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

__all__ = ["DisplayLabel"]


from widget import DisplayWidget
from karabo.karathon import *
import decimal
import re

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class DisplayLabel(DisplayWidget):
    category = "Value"
    alias = "Value Field"
  
    def __init__(self, **params):
        super(DisplayLabel, self).__init__(**params)
        
        self.value = None
        
        self.__label = QLabel()
        self.__label.setAutoFillBackground(True)
        self.__label.setAlignment(Qt.AlignCenter)
        self.__label.setMinimumWidth(160)
        self.__label.setMinimumHeight(32)
        self.__label.setWordWrap(True)
        self.setErrorState(False)
        

    @property
    def widget(self):
        return self.__label


    def setErrorState(self, isError):
        if isError is True:
            self.__label.setStyleSheet("QLabel { background-color : rgba(255,155,155,128); }") # light red
        else:
            self.__label.setStyleSheet("QLabel { background-color : rgba(225,242,225,128); }") # light green


    def valueChanged(self, key, value, timestamp=None):
        if isinstance(value, Hash):# or isinstance(value, Hash[]):
            # No visualization of Hash objects in this widget
            return
        if isinstance(value, list) and len(value) > 0:
            if isinstance(value[0], Hash):
                # No visualization of vector of Hash in this widget
                return
        
        if value is None:
            return
        
        # Store original value with type
        self.value = value
        
        if isinstance(value, list):    
            listLen = len(value)
            maxLen = 4
            valueAsString = "["
            
            for i in range(listLen):
                if maxLen < 1:
                    valueAsString += ".."
                    break

                index = value[i]                
                if self.valueType is Types.VECTOR_FLOAT:
                    index = float(decimal.Decimal(str(index)).quantize(decimal.Decimal('.0000000')))
                elif self.valueType is Types.VECTOR_DOUBLE:
                    index = float(decimal.Decimal(str(index)))
                valueAsString += str(index)

                if i != (listLen-1):
                    valueAsString += ", "
                maxLen -= 1
            valueAsString += "]"
            value = valueAsString
        elif self.valueType is Types.FLOAT:
            value = float(decimal.Decimal(str(value)).quantize(decimal.Decimal('.0000000')))
            value = self.toScientificNotation(value)
            
        elif self.valueType is Types.DOUBLE:
            value = float(decimal.Decimal(str(value)))

        self.__label.setText(str(value))
        
    def toScientificNotation(self, value):
        strvalue = str(value)
        if re.match('^[^\.]*?\d{6,}\.0+$', strvalue):
            locale = self.__label.locale()
            strvalue = locale.toString(value, 'e', 6)
            if abs(value) >= 1000.0:
                strvalue.remove(locale.groupSeparator())
            return strvalue
        else: return strvalue
