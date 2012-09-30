#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 27, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a class which inherits from BaseComponent
   and will just show values.
"""

__all__ = ["DisplayComponent"]


from basecomponent import BaseComponent
from displaywidget import DisplayWidget
from manager import Manager

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class DisplayComponent(BaseComponent):


    def __init__(self, classAlias, **params):
        super(DisplayComponent, self).__init__()
        
        self.__classAlias = classAlias
        self.__initParams = params
        
        # Use key to register component to manager
        key = params.get(QString('key'))
        if key is None:
            key = params.get('key')
        Manager().registerDisplayComponent(key, self)
        
        self.__displayWidget = DisplayWidget.create(classAlias, **params)


### getter and setter functions ###
    def _getWidgetCategory(self):
        return self.__displayWidget.category
    widgetCategory = property(fget=_getWidgetCategory)


    # Returns the actual widget which is part of the composition
    def _getWidget(self):
        return self.__displayWidget.widget
    widget = property(fget=_getWidget)


    def _getKeys(self):
        return self.__displayWidget.keys
    keys = property(fget=_getKeys)


    def _getValue(self):
        return self.__displayWidget.value
    def _setValue(self, value):
        self.__displayWidget.value = value
    value = property(fget=_getValue, fset=_setValue)


    def setErrorState(self, isError):
        self.__displayWidget.setErrorState(isError)


    def addKeyValue(self, key, value):
        if self.__displayWidget.minMaxAssociatedKeys[1] == 1:
            existingKeys = self.__displayWidget.keys
            for k in existingKeys:
                self.removeKey(k)
        Manager().registerDisplayComponent(key, self)
        self.__displayWidget.addKeyValue(key, value)


    def removeKey(self, key):
        Manager().unregisterDisplayComponent(key, self)
        self.__displayWidget.removeKey(key)


    def destroy(self):
        for key in self.__displayWidget.keys:
            self.removeKey(key)


    def changeWidget(self, classAlias):
        self.__initParams['value'] = self.value
        self.__displayWidget = DisplayWidget.create(classAlias, **self.__initParams)


### slots ###
    def onValueChanged(self, key, value, timestamp=None):
        self.__displayWidget.valueChanged(key, value, timestamp)


    def onDisplayValueChanged(self, key, value):
        pass

