#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 27, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a class which inherits from BaseComponent
   and will edit values and apply them later to the system.
"""

__all__ = ["EditableApplyNowComponent"]


from basecomponent import BaseComponent
from editablewidget import EditableWidget
from manager import Manager

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class EditableApplyNowComponent(BaseComponent):


    def __init__(self, classAlias, **params):
        super(EditableApplyNowComponent, self).__init__()

        # Use key to register component to manager
        key = params.get(QString('key'))
        if key is None:
            key = params.get('key')
        Manager().registerEditableComponent(key, self)

        self.__classAlias = classAlias
        self.__initParams = params

        self.__compositeWidget = QWidget()
        self.__layout = QHBoxLayout(self.__compositeWidget)
        self.__layout.setContentsMargins(0,0,0,0)

        self.__editableWidget = EditableWidget.create(classAlias, **params)
        self.__editableWidget.signalEditingFinished.connect(self.onEditingFinished)
        self.__layout.addWidget(self.__editableWidget.widget)
        
        unitSymbol = params.get(QString('unitSymbol'))
        if unitSymbol is None:
            unitSymbol = params.get('unitSymbol')
        if unitSymbol is not None and len(unitSymbol) > 0:
            self.__layout.addWidget(QLabel(unitSymbol))

        self.signalValueChanged.connect(Manager().onDeviceInstanceValueChanged)


    def copy(self):
        copyComponent = EditableApplyNowComponent(self.__classAlias, **self.__initParams)
        return copyComponent


### getter and setter functions ###
    def _getWidgetCategory(self):
        return self.__editableWidget.category
    widgetCategory = property(fget=_getWidgetCategory)


    # Returns the actual widget which is part of the composition
    def _getWidget(self):
        return self.__compositeWidget
    widget = property(fget=_getWidget)


    def _getKeys(self):
        return self.__editableWidget.keys
    keys = property(fget=_getKeys)


    def _getValue(self):
        return self.__editableWidget.value
    value = property(fget=_getValue)


    def setEnabled(self, enable):
        self.widget.setEnabled(enable)


    def addParameters(self, **params):
        self.__editableWidget.addParameters(**params)


    def addKeyValue(self, key, value):
        self.__editableWidget.addKeyValue(key, value)


    def removeKey(self, key):
        pass


    def destroy(self):
        for key in self.__editableWidget.keys:
            Manager().unregisterEditableComponent(key, self)


    def changeWidget(self, classAlias):
        layout = self.__compositeWidget.layout()
        
        oldWidget = self.__editableWidget.widget
        index = layout.indexOf(oldWidget)
        oldWidget.deleteLater()
        layout.removeWidget(oldWidget)
        
        self.__initParams['value'] = self.value
        # Disconnect signal from old widget
        self.__editableWidget.signalEditingFinished.disconnect(self.onEditingFinished)
        self.__editableWidget = EditableWidget.create(classAlias, **self.__initParams)
        # Connect signal to new widget
        self.__editableWidget.signalEditingFinished.connect(self.onEditingFinished)
        layout.insertWidget(index, self.__editableWidget.widget)


### slots ###
    def onValueChanged(self, key, value, timestamp=None):
        self.__editableWidget.valueChanged(key, value, timestamp)


    def onDisplayValueChanged(self, key, value):
        pass


    # Triggered from self.__editableWidget when value was edited
    def onEditingFinished(self, key, value):
        self.signalValueChanged.emit(key, value)

