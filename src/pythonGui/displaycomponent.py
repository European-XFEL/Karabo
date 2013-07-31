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
        
        self.__compositeWidget = QWidget()
        hLayout = QHBoxLayout(self.__compositeWidget)
        hLayout.setContentsMargins(0,0,0,0)

        widgetFactory = params.get(QString('widgetFactory'))
        if widgetFactory is None:
            widgetFactory = params.get('widgetFactory')
        
        if widgetFactory is None or (widgetFactory == "DisplayWidget"):
            self.__displayWidget = DisplayWidget.create(classAlias, **params)
        elif widgetFactory == "VacuumWidget":
            self.__displayWidget = VacuumWidget.create(classAlias, **params)
        
        hLayout.addWidget(self.__displayWidget.widget)

        metricPrefixSymbol = params.get(QString('metricPrefixSymbol'))
        if metricPrefixSymbol is None:
            metricPrefixSymbol = params.get('metricPrefixSymbol')
        unitSymbol = params.get(QString('unitSymbol'))
        if unitSymbol is None:
            unitSymbol = params.get('unitSymbol')
        
        # Append unit label, if available
        unitLabel = str()
        if metricPrefixSymbol:
            unitLabel += metricPrefixSymbol
        if unitSymbol:
            unitLabel += unitSymbol
        if len(unitLabel) > 0:
            hLayout.addWidget(QLabel(unitLabel))
        
        # Use path to register component to manager
        key = params.get(QString('key'))
        if key is None:
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
        return self.__compositeWidget
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


    def changeWidget(self, classAlias):
        self.classAlias = classAlias
        self.__initParams['value'] = self.value
        
        layout = self.__compositeWidget.layout()
        oldWidget = self.__displayWidget.widget
        oldWidget.deleteLater()
        layout.removeWidget(oldWidget)
        
        self.__displayWidget = DisplayWidget.create(classAlias, **self.__initParams)
        layout.addWidget(self.__displayWidget.widget)
        self.__compositeWidget.adjustSize()
        
        # Refresh new widget...
        for key in self.__displayWidget.keys:
            Manager().onRefreshInstance(key)


    def changeToVacuumWidget(self, classAlias):
        self.classAlias = classAlias
        self.__initParams['value'] = self.value
        
        layout = self.__compositeWidget.layout()
        oldWidget = self.__displayWidget.widget
        oldWidget.deleteLater()
        layout.removeWidget(oldWidget)
        
        self.__displayWidget = VacuumWidget.create(classAlias, **self.__initParams)
        layout.addWidget(self.__displayWidget.widget)
        self.__compositeWidget.adjustSize()
        
        # Refresh new widget...
        for key in self.__displayWidget.keys:
            Manager().onRefreshInstance(key)


### slots ###
    def onValueChanged(self, key, value, timestamp=None):
        self.__displayWidget.valueChanged(key, value, timestamp)


    def onDisplayValueChanged(self, key, value):
        pass

