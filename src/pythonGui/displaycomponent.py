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
import manager
from widget import DisplayWidget

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class DisplayComponent(BaseComponent):
    factories = DisplayWidget.factories


    def __init__(self, classAlias, box, widgetFactory="DisplayWidget"):
        super(DisplayComponent, self).__init__(classAlias)
        
        self.__displayWidget = DisplayWidget.factories[widgetFactory].get_class(
            classAlias)(box)
        box.addComponent(self)


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


    def addKey(self, key):
        if self.__displayWidget.addKey(key):
            manager.Manager().registerDisplayComponent(key, self)
            return True
        return False


    def removeKey(self, key):
        manager.Manager().unregisterDisplayComponent(key, self)
        self.__displayWidget.removeKey(key)


    def destroy(self):
        for key in self.__displayWidget.keys:
            self.removeKey(key)


    def changeWidget(self, factory, alias):
        self.classAlias = alias
        self.__initParams['value'] = self.value
        
        oldWidget = self.__displayWidget.widget
        self.__displayWidget = factory.get_class(alias)(**self.__initParams)
        self.__displayWidget.widget.setWindowFlags(Qt.BypassGraphicsProxyWidget)
        self.__displayWidget.widget.setAttribute(Qt.WA_NoSystemBackground, True)
        oldWidget.parent().addWidget(self.__displayWidget.widget)
        oldWidget.parent().removeWidget(oldWidget)
        oldWidget.parent().setCurrentWidget(self.__displayWidget.widget)
        oldWidget.setParent(None)
        self.__displayWidget.widget.show()
        
        # Refresh new widget...
        for key in self.__displayWidget.keys:
            manager.Manager().onRefreshInstance(key)


    def onValueChanged(self, key, value, timestamp=None):
        self.__displayWidget.valueChanged(key, value, timestamp)
        # Emit signal to update possible tooltips in ParameterTreeWidget
        self.signalValueChanged.emit(key, value)
