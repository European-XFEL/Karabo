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
from manager import Manager
from widget import EditableWidget


from PyQt4.QtGui import QHBoxLayout, QLabel, QWidget


class EditableApplyNowComponent(BaseComponent):


    def __init__(self, classAlias, box, parent, widgetFactory=None):
        super(EditableApplyNowComponent, self).__init__(classAlias)

        self.__compositeWidget = QWidget(parent)
        hLayout = QHBoxLayout(self.__compositeWidget)
        hLayout.setContentsMargins(0,0,0,0)

        self.__editableWidget = EditableWidget.get_class(classAlias)(
            box, self.__compositeWidget)
        self.__editableWidget.signalEditingFinished.connect(self.onEditingFinished)
        hLayout.addWidget(self.__editableWidget.widget)
        
        metricPrefixSymbol = params.get('metricPrefixSymbol')
        unitSymbol = params.get('unitSymbol')
        
        # Append unit label, if available
        unitLabel = ""
        if metricPrefixSymbol:
            unitLabel += metricPrefixSymbol
        if unitSymbol:
            unitLabel += unitSymbol
        if len(unitLabel) > 0:
            hLayout.addWidget(QLabel(unitLabel))
        

    def copy(self):
        copyComponent = EditableApplyNowComponent(self.classAlias, **self.__initParams)
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


    @property
    def boxes(self):
        return self.__editableWidget.boxes


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
            Manager().onRefreshInstance(key)


    # Triggered from self.__editableWidget when value was edited
    def onEditingFinished(self, key, value):
        manager.Manager().onDeviceInstanceValuesChanged([key])

