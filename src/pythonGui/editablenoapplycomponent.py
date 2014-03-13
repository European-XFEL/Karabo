#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 27, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a class which inherits from BaseComponent
   and will edit values.
"""

__all__ = ["EditableNoApplyComponent"]


from basecomponent import BaseComponent
import manager
from widget import EditableWidget

from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import QHBoxLayout, QLabel, QWidget


class EditableNoApplyComponent(BaseComponent):


    def __init__(self, classAlias, **params):
        super(EditableNoApplyComponent, self).__init__(classAlias)
        
        self.__initParams = params
        
        self.__compositeWidget = QWidget()
        hLayout = QHBoxLayout(self.__compositeWidget)
        hLayout.setContentsMargins(0,0,0,0)
        
        self.__editableWidget = EditableWidget.get_class(classAlias)(**params)
        self.__editableWidget.signalEditingFinished.connect(self.onEditingFinished)
        hLayout.addWidget(self.__editableWidget.widget)
        
        metricPrefixSymbol = params.get('metricPrefixSymbol')
        unitSymbol = params.get('unitSymbol')
        
        # Append unit label, if available
        unitLabel = str()
        if metricPrefixSymbol:
            unitLabel += metricPrefixSymbol
        if unitSymbol:
            unitLabel += unitSymbol
        if len(unitLabel) > 0:
            hLayout.addWidget(QLabel(unitLabel))
        
        # In case of attributes (Hash-V2) connect another function here
        self.signalValueChanged.connect(
            manager.Manager().onDeviceClassValueChanged)

        params['key'].addComponent(self)


    def copy(self):
        copyComponent = EditableNoApplyComponent(self.classAlias, **self.__initParams)
        return copyComponent


### getter and setter functions ###
    def _getWidgetCategory(self):
        return self.__editableWidget.category
    widgetCategory = property(fget=_getWidgetCategory)


    def _getWidgetFactory(self):
        return self.__editableWidget
    widgetFactory = property(fget=_getWidgetFactory)


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
            manager.Manager().unregisterEditableComponent(key, self)


    def changeWidget(self, factory, proxyWidget, alias):
        self.classAlias = alias
        self.__initParams['value'] = self.value

        oldWidget = self.__editableWidget.widget
        oldWidget.deleteLater()
        self.__editableWidget = factory.get_class(alias)(**self.__initParams)
        self.__editableWidget.widget.setWindowFlags(Qt.BypassGraphicsProxyWidget)
        self.__editableWidget.widget.setAttribute(Qt.WA_NoSystemBackground, True)
        proxyWidget.setWidget(self.__editableWidget.widget)
        self.__editableWidget.widget.show()

        # Refresh new widget...
        for key in self.__editableWidget.keys:
            manager.Manager().onRefreshInstance(key)


    @pyqtSlot(str, object)
    def onValueChanged(self, key, value, timestamp=None):
        self.__editableWidget.valueChanged(key, value, timestamp)


    # Triggered from self.__editableWidget when value was edited
    def onEditingFinished(self, key, value):
        self.signalValueChanged.emit(key, value)

