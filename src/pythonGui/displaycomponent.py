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
from vacuumwidget import VacuumWidget

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class DisplayComponent(BaseComponent):


    def __init__(self, classAlias, **params):
        super(DisplayComponent, self).__init__(classAlias)
        
        self.__initParams = params

        widgetFactory = params.get('widgetFactory')
        
        if widgetFactory is None or (widgetFactory == "DisplayWidget"):
            self.__displayWidget = DisplayWidget.create(classAlias, **params)
        elif widgetFactory == "VacuumWidget":
            self.__displayWidget = VacuumWidget.create(classAlias, **params)
        
        # Use path to register component to manager
        key = params.get('key')
            
        #print "### Registering key: ", key
        Manager().registerDisplayComponent(key, self)


### getter and setter functions ###
    def _getWidgetCategory(self):
        return self.__displayWidget.category
    widgetCategory = property(fget=_getWidgetCategory)


    def _getWidgetFactory(self):
        return self.__displayWidget
    widgetFactory = property(fget=_getWidgetFactory)


    # Returns the actual widget which is part of the composition
    def _getWidget(self):
        return self.__displayWidget.widget
    widget = property(fget=_getWidget)


    def _getDisplayWidget(self):
        return self.__displayWidget
    displayWidget = property(fget=_getDisplayWidget)


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


    def changeWidget(self, proxyWidget, classAlias):
        self.classAlias = classAlias
        self.__initParams['value'] = self.value
        
        oldWidget = self.__displayWidget.widget
        oldWidget.deleteLater()
        self.__displayWidget = DisplayWidget.create(classAlias, **self.__initParams)
        self.__displayWidget.widget.setWindowFlags(Qt.BypassGraphicsProxyWidget)
        self.__displayWidget.widget.setAttribute(Qt.WA_NoSystemBackground, True)
        proxyWidget.setWidget(self.__displayWidget.widget)
        self.__displayWidget.widget.show()
        
        # Refresh new widget...
        for key in self.__displayWidget.keys:
            Manager().onRefreshInstance(key)


    def changeToVacuumWidget(self, classAlias):
        self.classAlias = classAlias
        self.__initParams['value'] = self.value
        
        oldWidget = self.__displayWidget.widget
        oldWidget.deleteLater()
        self.__displayWidget = VacuumWidget.create(classAlias, **self.__initParams)
        self.__displayWidget.widget.setWindowFlags(Qt.BypassGraphicsProxyWidget)
        self.__displayWidget.widget.setAttribute(Qt.WA_NoSystemBackground, True)
        proxyWidget.setWidget(self.__displayWidget.widget)
        
        # Refresh new widget...
        for key in self.__displayWidget.keys:
            Manager().onRefreshInstance(key)


### slots ###
    def onValueChanged(self, key, value, timestamp=None):
        self.__displayWidget.valueChanged(key, value, timestamp)


    def onDisplayValueChanged(self, key, value):
        pass

