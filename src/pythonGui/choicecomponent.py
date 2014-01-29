#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on August 6, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a class which inherits from BaseComponent
   and will just show values.
"""

__all__ = ["ChoiceComponent"]


from basecomponent import BaseComponent
from widget import EditableWidget
from manager import Manager

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class ChoiceComponent(BaseComponent):


    def __init__(self, classAlias, **params):
        super(ChoiceComponent, self).__init__(classAlias)
        
        # Use key to register component to manager
        key = params.get('key')
        Manager().registerEditableComponent(key, self)
        
        self.__classAlias = classAlias
        self.__initParams = params
        
        self.__choiceWidget = EditableWidget.get_class(classAlias)(**params)
        self.widget.setEnabled(False)


    def copy(self):
        copyComponent = ChoiceComponent(self.__classAlias, **self.__initParams)
        return copyComponent


### getter and setter functions ###
    def _getWidgetCategory(self):
        return self.__choiceWidget.category
    widgetCategory = property(fget=_getWidgetCategory)


    def _getWidgetFactory(self):
        return self.__choiceWidget
    widgetFactory = property(fget=_getWidgetFactory)


    # Returns the actual widget which is part of the composition
    def _getWidget(self):
        return self.__choiceWidget.widget
    widget = property(fget=_getWidget)


    def _getValue(self):
        return self.__choiceWidget.value
    def _setValue(self, value):
        self.__choiceWidget.value = value
    value = property(fget=_getValue, fset=_setValue)


    def setEnabled(self, enable):
        # Is not processed due to self.widget should always stay disabled
        pass


    def addParameters(self, **params):
        self.__choiceWidget.addParameters(**params)


    # Triggered by DataNotifier signalAddKey
    def addKeyValue(self, key, value):
        pass


    # Triggered by DataNotifier signalRemoveKey
    def removeKey(self, key):
        pass


### slots ###
    def onValueChanged(self, key, value, timestamp=None):
        self.__choiceWidget.valueChanged(key, value, timestamp)


    def onDisplayValueChanged(self, key, value, timestamp=None):
        self.__choiceWidget.valueChanged(key, value, timestamp)

