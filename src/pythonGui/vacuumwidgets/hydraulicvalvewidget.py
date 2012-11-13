#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on October 25, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a widget plugin for attributes
   and is created by the factory class VacuumWidget.
   
   Each plugin needs to implement the following interface:
   
   def getCategoryAliasClassName():
       pass
   
    class Maker:
        def make(self, **params):
            return Attribute*(**params)
"""

__all__ = ["HydraulicValveWidget"]


from vacuumwidget import VacuumWidget

from PyQt4.QtCore import *
from PyQt4.QtGui import *


def getCategoryAliasClassName():
    return ["State","Hydraulic valve","HydraulicValveWidget"]


class HydraulicValveWidget(VacuumWidget):
  
    def __init__(self, **params):
        super(HydraulicValveWidget, self).__init__(**params)
        
        # Minimum and maximum number of associated keys, 1 by default for each
        self.__minMaxAssociatedKeys = (1,1) # tuple<min,max>
        
        self.__value = None
        
        self.__label = QLabel()
        # Get image representation for value
        self._setPixmap(QPixmap(":hydraulicvalve"))
        self.setErrorState(False)
        
        self.__key = params.get(QString('key'))
        if self.__key is None:
            self.__key = params.get('key')
        
        # Set value
        value = params.get(QString('value'))
        if value is None:
            value = params.get('value')
        if value is not None:
            self.valueChanged(self.__key, value)


    def _getCategory(self):
        category, alias, className = getCategoryAliasClassName()
        return category
    category = property(fget=_getCategory)


    # Returns the actual widget which is part of the composition
    def _getWidget(self):
        return self.__label
    widget = property(fget=_getWidget)


    # Returns a tuple of min and max number of associated keys with this component
    def _getMinMaxAssociatedKeys(self):
        return self.__minMaxAssociatedKeys
    minMaxAssociatedKeys = property(fget=_getMinMaxAssociatedKeys)


    def _getKeys(self):
        return [self.__key]
    keys = property(fget=_getKeys)


    def _value(self):
        return self.__value
    value = property(fget=_value)


    def _setPixmap(self, pixmap):
        self.__label.setPixmap(pixmap)
        self.__label.setMaximumWidth(pixmap.width())
        self.__label.setMaximumHeight(pixmap.height())


    def setErrorState(self, isError):
        if isError is True:
            self.__label.setStyleSheet("QLabel { background-color : rgba(255,155,155,128); }") # light red
        else:
            self.__label.setStyleSheet("QLabel { background-color : rgba(225,242,225,128); }") # light green


    def addKeyValue(self, key, value):
        self.__key = key # TODO: Overwritten - unregistering in Manager...
        self.valueChanged(key, value)


    def removeKey(self, key):
        self.__key = None


    def valueChanged(self, key, value, timestamp=None):
        #print "HydraulicValveWidget.valueChanged", key, value
        if value == "Changing...":
            self._setPixmap(QPixmap(":hydraulicvalve-orange"))
        elif ("On" in value) or ("on" in value):
            self._setPixmap(QPixmap(":hydraulicvalve-green"))
        elif ("Off" in value) or ("off" in value):
            self._setPixmap(QPixmap(":hydraulicvalve-yellow"))
        elif ("Error" in value) or ("error" in value):
            self._setPixmap(QPixmap(":hydraulicvalve-red"))


    class Maker:
        def make(self, **params):
            return HydraulicValveWidget(**params)

