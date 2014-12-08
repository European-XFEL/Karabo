#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 2, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a base class for all inherited
   factory classes for creation of certain widgets and bundles main functionalities.
"""

__all__ = ["BaseComponent"]


import manager
from network import Network
import icons

from layouts import ProxyWidget
from registry import Loadable
from const import ns_karabo
from messagebox import MessageBox
from widget import EditableWidget, DisplayWidget, Widget

from PyQt4.QtCore import QObject, pyqtSignal, pyqtSlot, QSize, QTimer
from PyQt4.QtGui import (QAction, QHBoxLayout, QLabel, QMessageBox,
                         QToolButton, QWidget)

import numpy
import numbers

class BaseComponent(Loadable, QObject):
    factories = EditableWidget.factories

    signalValueChanged = pyqtSignal(object, object) # key, value


    def __init__(self, classAlias, parent):
        super(BaseComponent, self).__init__(parent)
        self.classAlias = classAlias


    def connectWidget(self, box):
        box.signalNewDescriptor.connect(self.widgetFactory.typeChangedSlot)
        self.widgetFactory.setParent(self)
        if box.descriptor is not None:
            self.widgetFactory.typeChanged(box)


    def save(self, e):
        """saves this component into the ElementTree.Element e"""
        d = { }
        e.set(ns_karabo + "class", self.__class__.__name__)
        e.set(ns_karabo + "widget", type(self.widgetFactory).__name__)
        e.set(ns_karabo + "keys", ",".join(b.key()
                                           for b in self.widgetFactory.boxes))
        self.widgetFactory.save(e)


    @classmethod
    def load(cls, elem, layout):
        boxes = []
        for k in elem.get(ns_karabo + 'keys').split(","):
            deviceId, path = k.split('.', 1)
            conf = manager.getDevice(deviceId)
            boxes.append(conf.getBox(path.split(".")))
        parent = ProxyWidget(layout.parentWidget())
        wn = elem.get(ns_karabo + "widget")
        try:
            component = cls(wn, boxes[0], parent)
        except KeyError:
            QMessageBox.warning(layout.parentWidget(), "Widget not found",
                                "Could not find widget '{}'.".format(wn))
            return
        parent.setComponent(component)
        parent.setWidget(component.widget)
        layout.loadPosition(elem, parent)
        for b in boxes[1:]:
            component.addBox(b)
        for b in boxes:
            b.configuration.addVisible()
        component.widgetFactory.load(elem)
        elem.clear()
        return component


    @property
    def boxes(self):
        return self.widgetFactory.boxes


class DisplayComponent(BaseComponent):
    factories = DisplayWidget.factories


    def __init__(self, classAlias, box, parent, widgetFactory="DisplayWidget"):

        W = Widget.widgets.get(classAlias)
        if W is None:
            self.widgetFactory = DisplayWidget.factories[widgetFactory].\
                                   getClass(classAlias)(box, parent)
            super(DisplayComponent, self).__init__(classAlias, parent)
        else:
            self.widgetFactory = W(box, parent)
            super(DisplayComponent, self).__init__(W.alias, parent)
        self.widgetFactory.setReadOnly(True)
        self.connectWidget(box)


    def connectWidget(self, box):
        BaseComponent.connectWidget(self, box)
        box.signalUpdateComponent.connect(self.widgetFactory.valueChangedSlot)
        if box.hasValue():
            self.widgetFactory.valueChanged(box, box.value, box.timestamp)


    def addBox(self, box):
        if self.widgetFactory.addBox(box):
            self.connectWidget(box)
            return True
        return False


    def _getWidgetCategory(self):
        return self.widgetFactory.category
    widgetCategory = property(fget=_getWidgetCategory)


    # Returns the actual widget which is part of the composition
    def _getWidget(self):
        return self.widgetFactory.widget
    widget = property(fget=_getWidget)


    def _getDisplayWidget(self):
        return self.widgetFactory
    displayWidget = property(fget=_getDisplayWidget)


    def _getKeys(self):
        return self.widgetFactory.keys
    keys = property(fget=_getKeys)


    def _getValue(self):
        return self.widgetFactory.value
    def _setValue(self, value):
        self.widgetFactory.value = value
    value = property(fget=_getValue, fset=_setValue)


    def setErrorState(self, isError):
        self.widgetFactory.setErrorState(isError)


    def removeKey(self, key):
        self.widgetFactory.removeKey(key)


    def destroy(self):
        for key in self.widgetFactory.keys:
            self.removeKey(key)


    def changeWidget(self, factory, alias):
        self.classAlias = alias
        oldWidget = self.widgetFactory.widget
        oldFactory = self.widgetFactory
        self.widgetFactory.setParent(None)
        self.widgetFactory = factory.getClass(alias)(
            oldFactory.boxes[0], oldWidget.parent())
        self.widgetFactory.setReadOnly(True)
        self.connectWidget(self.boxes[0])
        for b in oldFactory.boxes[1:]:
            self.widgetFactory.addBox(b)
            self.connectWidget(b)
        oldWidget.parent().setWidget(self.widgetFactory.widget)
        oldWidget.setParent(None)
        self.widgetFactory.widget.show()


class EditableNoApplyComponent(BaseComponent):
    """ These components are used while editing the initial parameters
    of a class. """
    def __init__(self, classAlias, box, parent, widgetFactory=None):
        self.__compositeWidget = QWidget(parent)
        hLayout = QHBoxLayout(self.__compositeWidget)
        hLayout.setContentsMargins(0,0,0,0)

        W = Widget.widgets.get(classAlias)
        if W is None:
            self.widgetFactory = EditableWidget.getClass(classAlias)(
                                        box, self.__compositeWidget)
            super(EditableNoApplyComponent, self).__init__(classAlias, parent)
        else:
            self.widgetFactory = W(box, self.__compositeWidget)
            super(EditableNoApplyComponent, self).__init__(W.alias, parent)
        self.connectWidget(box)
        self.widgetFactory.setReadOnly(False)
        hLayout.addWidget(self.widgetFactory.widget)

        unitLabel = (box.descriptor.metricPrefixSymbol +
                     box.descriptor.unitSymbol)

        if unitLabel:
            hLayout.addWidget(QLabel(unitLabel))


    def connectWidget(self, box):
        BaseComponent.connectWidget(self, box)
        if box.hasValue():
            self.widgetFactory.valueChanged(box, box.value, box.timestamp)
        box.signalUpdateComponent.connect(self.widgetFactory.valueChangedSlot)
        self.widgetFactory.signalEditingFinished.connect(self.onEditingFinished)
        box.signalUserChanged.connect(self.widgetFactory.valueChangedSlot)


    def _getWidgetCategory(self):
        return self.widgetFactory.category
    widgetCategory = property(fget=_getWidgetCategory)


    # Returns the actual widget which is part of the composition
    def _getWidget(self):
        return self.__compositeWidget
    widget = property(fget=_getWidget)


    def _getKeys(self):
        return self.widgetFactory.keys
    keys = property(fget=_getKeys)


    def _getValue(self):
        return self.widgetFactory.value
    value = property(fget=_getValue)


    def setEnabled(self, enable):
        self.widget.setEnabled(enable)


    def addKeyValue(self, key, value):
        self.widgetFactory.addKeyValue(key, value)


    def removeKey(self, key):
        pass


    def destroy(self):
        for key in self.widgetFactory.keys:
            manager.Manager().unregisterEditableComponent(key, self)


    def changeWidget(self, factory, proxyWidget, alias):
        self.classAlias = alias
        self.__initParams['value'] = self.value

        oldWidget = self.widgetFactory.widget
        oldWidget.deleteLater()
        self.widgetFactory = factory.getClass(alias)(**self.__initParams)
        self.widgetFactory.setReadOnly(False)
        proxyWidget.setWidget(self.widgetFactory.widget)
        self.widgetFactory.widget.show()

        # Refresh new widget...
        for key in self.widgetFactory.keys:
            Network().onGetDeviceConfiguration(key)


    def onEditingFinished(self, box, value):
        box.set(value, None)

        # Configuration changed - so project needs to be informed to show it
        if box.configuration.type == 'projectClass':
            box.configuration.signalDeviceModified.emit(True)


class EditableApplyLaterComponent(BaseComponent):
    # signals
    signalConflictStateChanged = pyqtSignal(str, bool) # key, hasConflict
    signalApplyChanged = pyqtSignal(object, bool) # key, state of apply button


    def __init__(self, classAlias, box, parent, widgetFactory=None):
        self.__currentDisplayValue = None

        self.__compositeWidget = QWidget(parent)
        hLayout = QHBoxLayout(self.__compositeWidget)
        hLayout.setContentsMargins(0,0,0,0)

        W = Widget.widgets.get(classAlias)
        if W is None:
            self.widgetFactory = EditableWidget.getClass(classAlias)(
                                            box, self.__compositeWidget)
            super(EditableApplyLaterComponent, self).__init__(classAlias,
                                                              parent)
        else:
            self.widgetFactory = W(box, self.__compositeWidget)
            super(EditableApplyLaterComponent, self).__init__(W.alias, parent)

        hLayout.addWidget(self.widgetFactory.widget)

        self.box = box
        d = box.descriptor
        if d is not None:
            unitLabel = d.metricPrefixSymbol + d.unitSymbol
            if unitLabel:
                hLayout.addWidget(QLabel(unitLabel))

        self.hasConflict = False

        text = "Apply"
        self.acApply = QAction(icons.applyGrey, text, self)
        self.acApply.setStatusTip(text)
        self.acApply.setToolTip(text)
        self.acApply.triggered.connect(self.onApplyClicked)
        tb = QToolButton()
        tb.setDefaultAction(self.acApply)
        tb.setPopupMode(QToolButton.InstantPopup)
        tb.setIconSize(QSize(24, 24))
        hLayout.addWidget(tb)

        text = "Reset"
        self.acReset = QAction(icons.no, text, self)
        self.acReset.setStatusTip(text)
        self.acReset.setToolTip(text)
        self.acReset.triggered.connect(self.onApplyRemoteChanges)
        tb = QToolButton()
        tb.setDefaultAction(self.acReset)
        tb.setIconSize(QSize(24, 24))
        hLayout.addWidget(tb)

        self.__busyTimer = QTimer(self)
        self.__busyTimer.setSingleShot(True)
        self.__busyTimer.timeout.connect(self.onTimeOut)
        self._applyEnabled = False

        self.connectWidget(box)
        self.widgetFactory.setReadOnly(False)
        # In case of attributes (Hash-V2) connect another function here
        self.signalConflictStateChanged.connect(
            manager.Manager().onConflictStateChanged)


    def connectWidget(self, box):
        BaseComponent.connectWidget(self, box)
        self.widgetFactory.signalEditingFinished.connect(self.onEditingFinished)
        box.signalUserChanged.connect(self.onUserChanged)
        box.signalUpdateComponent.connect(self.onDisplayValueChanged)
        if box.hasValue():
            self.onDisplayValueChanged(box, box.value)
        box.configuration.boxvalue.state.signalUpdateComponent.connect(
            self.updateButtons)


    def _getWidgetCategory(self):
        return self.widgetFactory.category
    widgetCategory = property(fget=_getWidgetCategory)


    # Returns the actual widget which is part of the composition
    def _getWidget(self):
        return self.__compositeWidget
    widget = property(fget=_getWidget)


    def _getValue(self):
        return self.widgetFactory.value
    value = property(fget=_getValue)


    def setEnabled(self, enable):
        self.widget.setEnabled(enable)


    @property
    def applyEnabled(self):
        return self._applyEnabled


    @applyEnabled.setter
    def applyEnabled(self, value):
        emit = value != self._applyEnabled
        self._applyEnabled = value
        if emit:
            self.signalApplyChanged.emit(self.boxes[0], value)


    def addKeyValue(self, key, value):
        self.widgetFactory.addKeyValue(key, value)


    def removeKey(self, key):
        pass


    def destroy(self):
        for key in self.widgetFactory.keys:
            manager.Manager().unregisterEditableComponent(key, self)


    def changeWidget(self, factory, alias):
        self.classAlias = alias

        oldWidget = self.widgetFactory.widget
        self.widgetFactory = factory.getClass(alias)(
            self.box, oldWidget.parent())
        self.__currentDisplayValue = None
        self.widgetFactory.setReadOnly(False)
        self.connectWidget(self.box)
        oldWidget.parent().layout().insertWidget(0, self.widgetFactory.widget)
        oldWidget.setParent(None)
        self.widgetFactory.widget.show()

        for c in {b.configuration for b in self.widgetFactory.boxes}:
            c.refresh()


    # Slot called when changes need to be sent to Manager
    def onApplyClicked(self):
        network = []
        for b in self.boxes:
            b.signalUserChanged.emit(b, self.widgetFactory.value, None)
            # If this box belongs to a deviceGroup configuration, no need to
            # broadcast to Network
            if b.configuration.type == "macro":
                b.set(self.widgetFactory.value)
            elif (b.configuration.type != "deviceGroup" and
                  b.descriptor is not None):
                network.append((b, self.widgetFactory.value))

        if network:
            self.__busyTimer.start(5000)
            Network().onReconfigure(network)


    def onApplyRemoteChanges(self):
        for b in self.boxes:
            self.widgetFactory.valueChanged(b, self.__currentDisplayValue)
        self.updateButtons()


    def onTimeOut(self):
        MessageBox.showWarning(
            "The attribute couldn't be set in the current state.")


    @pyqtSlot(object, object)
    def onDisplayValueChanged(self, box, value):
        if self.__currentDisplayValue is None:
            self.widgetFactory.valueChanged(box, value)
        self.__currentDisplayValue = value
        self.__busyTimer.stop()
        self.hasConflict = True
        self.updateButtons()


    @pyqtSlot(object, object)
    def onUserChanged(self, box, value):
        self.widgetFactory.valueChangedSlot(box, value)
        self.updateButtons()


    @pyqtSlot()
    def updateButtons(self):
        """ update the buttons to reflect the current state of affairs """
        allowed = self.boxes[0].isAllowed()
        self.acApply.setEnabled(allowed)

        EPSILON = 1e-4
        value = self.__currentDisplayValue
        if value is None:
            return

        if (isinstance(value, (numbers.Complex, numpy.inexact))
                and not isinstance(value, numbers.Integral)):
            diff = abs(value - self.widgetFactory.value)
            isEqualEditable = diff < EPSILON
        elif isinstance(value, list):
            if len(value) != len(self.widgetFactory.value):
                isEqualEditable = False
            else:
                isEqualEditable = True
                for i in range(len(value)):
                    if value[i] != self.widgetFactory.value[i]:
                        isEqualEditable = False
                        break
        else:
            isEqualEditable = (str(value) == str(self.widgetFactory.value))

        if isEqualEditable:
            self.acApply.setIcon(icons.applyGrey)
            self.hasConflict = False
            text = None
        elif self.hasConflict:
            self.acApply.setIcon(icons.applyConflict)
            text = "Apply mine"
        else:
            text = "Apply"
            self.acApply.setIcon(icons.apply)
        self.acApply.setStatusTip(text)
        self.acApply.setToolTip(text)
        self.applyEnabled = allowed and not isEqualEditable
        self.acReset.setEnabled(self.applyEnabled)


    def onEditingFinished(self, box, value):
        if self.__currentDisplayValue is None:
            return
        self.updateButtons()


    def addBox(self, box):
        """we cannot add several boxes onto one editable thing yet"""
        pass


class ChoiceComponent(BaseComponent):
    def __init__(self, classAlias, box, parent, widgetFactory=None):
        W = Widget.widgets.get(classAlias)
        if W is None:
            self.widgetFactory = EditableWidget.getClass(classAlias)(
                                                    box, parent)
            super(ChoiceComponent, self).__init__(classAlias, parent)
        else:
            self.widgetFactory = W(box, parent)
            super(ChoiceComponent, self).__init__(W.alias, parent)
        self.widget.setEnabled(False)
        self.connectWidget(box)


    def connectWidget(self, box):
        BaseComponent.connectWidget(self, box)
        box.signalUpdateComponent.connect(self.widgetFactory.valueChangedSlot)
        box.signalUserChanged.connect(self.widgetFactory.valueChangedSlot)
        if box.hasValue():
            self.widgetFactory.valueChanged(box, box.value, box.timestamp)


    def _getWidgetCategory(self):
        return self.widgetFactory.category
    widgetCategory = property(fget=_getWidgetCategory)


    # Returns the actual widget which is part of the composition
    def _getWidget(self):
        return self.widgetFactory.widget
    widget = property(fget=_getWidget)


    def _getValue(self):
        return self.widgetFactory.value
    def _setValue(self, value):
        self.widgetFactory.value = value
    value = property(fget=_getValue, fset=_setValue)


    def setEnabled(self, enable):
        # Is not processed due to self.widget should always stay disabled
        pass


    # Triggered by DataNotifier signalAddKey
    def addKeyValue(self, key, value):
        pass


    # Triggered by DataNotifier signalRemoveKey
    def removeKey(self, key):
        pass
