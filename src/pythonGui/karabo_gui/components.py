#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 2, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a base class for all inherited
   factory classes for creation of certain widgets and bundles main
   functionalities.
"""

__all__ = ["BaseComponent"]

import numbers

import numpy
from PyQt4.QtCore import QObject, pyqtSignal, pyqtSlot, QSize, QTimer
from PyQt4.QtGui import (QAction, QHBoxLayout, QLabel, QMessageBox,
                         QToolButton, QWidget)

from .const import ns_karabo
from . import icons
from .layouts import ProxyWidget
from .messagebox import MessageBox
from .network import Network
from .registry import Loadable
from .topology import getDevice, Manager
from .widget import EditableWidget, DisplayWidget, Widget


class BaseComponent(Loadable, QObject):
    Widget = EditableWidget
    signalValueChanged = pyqtSignal(object, object)  # key, value

    def __init__(self, parent):
        super(BaseComponent, self).__init__(parent)

    def connectWidget(self, box):
        box.signalNewDescriptor.connect(self.widgetFactory.typeChangedSlot)
        self.widgetFactory.setParent(self)
        if box.descriptor is not None:
            self.widgetFactory.typeChangedSlot(box)

    def save(self, e):
        """saves this component into the ElementTree.Element e"""
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
            conf = getDevice(deviceId)
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
        component.widgetFactory.load(elem)
        elem.clear()
        return component

    @property
    def boxes(self):
        return self.widgetFactory.boxes


class DisplayComponent(BaseComponent):
    Widget = DisplayWidget

    def __init__(self, classAlias, box, parent):
        W = Widget.widgets.get(classAlias)
        if W is None:
            self.widgetFactory = classAlias
        else:
            self.widgetFactory = W(box, parent)
        super(DisplayComponent, self).__init__(parent)
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

    @property
    def widgetCategory(self):
        return self.widgetFactory.category

    # Returns the actual widget which is part of the composition
    @property
    def _getWidget(self):
        return self.widgetFactory.widget

    @property
    def displayWidget(self):
        return self.widgetFactory

    @property
    def keys(self):
        return self.widgetFactory.keys

    @property
    def value(self):
        return self.widgetFactory.value

    @value.setter
    def value(self, value):
        self.widgetFactory.value = value

    def setErrorState(self, isError):
        self.widgetFactory.setErrorState(isError)

    def removeKey(self, key):
        self.widgetFactory.removeKey(key)

    def changeWidget(self, factory):
        oldWidget = self.widgetFactory.widget
        oldFactory = self.widgetFactory
        self.widgetFactory.setParent(None)
        self.widgetFactory = factory(oldFactory.boxes[0], oldWidget.parent())
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
    def __init__(self, classAlias, box, parent):
        self.__compositeWidget = QWidget(parent)
        hLayout = QHBoxLayout(self.__compositeWidget)
        hLayout.setContentsMargins(0, 0, 0, 0)

        W = Widget.widgets.get(classAlias)
        if W is None:
            self.widgetFactory = classAlias
        else:
            self.widgetFactory = W(box, self.__compositeWidget)
        super(EditableNoApplyComponent, self).__init__(parent)
        self.connectWidget(box)
        self.widgetFactory.setReadOnly(False)
        hLayout.addWidget(self.widgetFactory.widget)

        unitLabel = box.unitLabel()
        if unitLabel:
            hLayout.addWidget(QLabel(unitLabel))

    def connectWidget(self, box):
        BaseComponent.connectWidget(self, box)
        if box.hasValue():
            self.widgetFactory.valueChanged(box, box.value, box.timestamp)
        box.signalUpdateComponent.connect(self.widgetFactory.valueChangedSlot)
        self.widgetFactory.signalEditingFinished.connect(self.onEditingFinished)
        box.signalUserChanged.connect(self.widgetFactory.valueChangedSlot)

    @property
    def widgetCategory(self):
        return self.widgetFactory.category

    # Returns the actual widget which is part of the composition
    @property
    def widget(self):
        return self.__compositeWidget

    @property
    def keys(self):
        return self.widgetFactory.keys

    @property
    def value(self):
        return self.widgetFactory.value

    def setEnabled(self, enable):
        self.widget.setEnabled(enable)

    def addKeyValue(self, key, value):
        self.widgetFactory.addKeyValue(key, value)

    def removeKey(self, key):
        pass

    def onEditingFinished(self, box, value):
        box.set(value, None)

        # Configuration changed - so project needs to be informed to show it
        if box.configuration.type in ('projectClass', 'deviceGroupClass'):
            box.configuration.signalConfigurationModified.emit(True)


class EditableApplyLaterComponent(BaseComponent):
    # signals
    signalConflictStateChanged = pyqtSignal(str, bool)  # key, hasConflict
    signalApplyChanged = pyqtSignal(object, bool)  # key, state of apply button

    def __init__(self, classAlias, box, parent):
        self.__currentDisplayValue = None

        self.__compositeWidget = QWidget(parent)
        hLayout = QHBoxLayout(self.__compositeWidget)
        hLayout.setContentsMargins(0, 0, 0, 0)

        W = Widget.widgets.get(classAlias)
        if W is None:
            self.widgetFactory = classAlias
        else:
            self.widgetFactory = W(box, self.__compositeWidget)
        super(EditableApplyLaterComponent, self).__init__(parent)

        hLayout.addWidget(self.widgetFactory.widget)

        self.box = box
        unitLabel = box.unitLabel()
        if unitLabel:
            hLayout.addWidget(QLabel(unitLabel))

        self.hasConflict = False

        text = "Apply"
        description = "Apply property changes"
        self.acApply = QAction(icons.applyGrey, text, self)
        self.acApply.setStatusTip(text)
        self.acApply.setToolTip(text)
        self.acApply.triggered.connect(self.onApplyClicked)
        tb = QToolButton()
        tb.setDefaultAction(self.acApply)
        tb.setPopupMode(QToolButton.InstantPopup)
        tb.setIconSize(QSize(24, 24))
        hLayout.addWidget(tb)

        text = "Decline"
        description = "Decline property changes and reset them to value on device"
        self.acReset = QAction(icons.no, text, self)
        self.acReset.setStatusTip(description)
        self.acReset.setToolTip(description)
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
        self.signalConflictStateChanged.connect(Manager().onConflictStateChanged)

    def connectWidget(self, box):
        BaseComponent.connectWidget(self, box)
        self.widgetFactory.signalEditingFinished.connect(self.onEditingFinished)
        box.signalUserChanged.connect(self.onUserChanged)
        box.signalUpdateComponent.connect(self.onDisplayValueChanged)
        if box.hasValue():
            self.onDisplayValueChanged(box, box.value)
        box.configuration.boxvalue.state.signalUpdateComponent.connect(
            self.updateButtons)

    @property
    def widgetCategory(self):
        return self.widgetFactory.category

    # Returns the actual widget which is part of the composition
    @property
    def widget(self):
        return self.__compositeWidget

    @property
    def value(self):
        return self.widgetFactory.value

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

    def changeWidget(self, factory):
        oldWidget = self.widgetFactory.widget
        oldFactory = self.widgetFactory
        self.widgetFactory.setParent(None)
        self.widgetFactory = factory(oldFactory.boxes[0], oldWidget.parent())
        self.widgetFactory.setReadOnly(False)
        self.connectWidget(self.boxes[0])
        oldWidget.parent().layout().insertWidget(0, self.widgetFactory.widget)
        oldWidget.setParent(None)
        self.widgetFactory.widget.show()
        if self.boxes[0].hasValue():
            self.widgetFactory.valueChanged(self.boxes[0], self.boxes[0].value)

    # Slot called when changes need to be sent to Manager
    def onApplyClicked(self):
        network = []
        for b in self.boxes:
            b.signalUserChanged.emit(b, self.widgetFactory.value, None)
            if b.configuration.type == "macro":
                b.set(self.widgetFactory.value)
            elif b.configuration.type == "deviceGroup":
                b.set(self.widgetFactory.value)
                # Broadcast changes for all devices belonging to this group
                for d in b.configuration.devices:
                    deviceBox = d.getBox(b.path)
                    deviceBox.set(self.widgetFactory.value)
                    # Send to network per device
                    Network().onReconfigure([(deviceBox, self.widgetFactory.value)])
            elif b.descriptor is not None:
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

    @pyqtSlot(object, object, object)
    def onUserChanged(self, box, value, timestamp=None):
        self.widgetFactory.valueChangedSlot(box, value, timestamp)
        self.updateButtons()

    @pyqtSlot()
    def updateButtons(self):
        """ update the buttons to reflect the current state of affairs """
        box = self.boxes[0]
        allowed = box.isAllowed()
        self.acApply.setEnabled(allowed)

        value = self.__currentDisplayValue

        if value is None:
            isEqualEditable = False
        elif (isinstance(value, (numbers.Complex, numpy.inexact))
                and not isinstance(value, numbers.Integral)):
            diff = abs(value - self.widgetFactory.value)
            absErr = box.descriptor.absoluteError
            relErr = box.descriptor.relativeError
            if absErr is not None:
                isEqualEditable = diff < absErr
            elif relErr is not None:
                isEqualEditable = diff < abs(value * relErr)
            else:
                isEqualEditable = diff < 1e-4
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
            description = None
        elif self.hasConflict:
            self.acApply.setIcon(icons.applyConflict)
            description = "Apply my property changes"
        else:
            description = "Apply property changes"
            self.acApply.setIcon(icons.apply)
        self.acApply.setStatusTip(description)
        self.acApply.setToolTip(description)
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
    def __init__(self, classAlias, box, parent):
        W = Widget.widgets.get(classAlias)
        if W is None:
            self.widgetFactory = classAlias
        else:
            self.widgetFactory = W(box, parent)
        super(ChoiceComponent, self).__init__(parent)
        self.widget.setEnabled(False)
        self.connectWidget(box)

    def connectWidget(self, box):
        BaseComponent.connectWidget(self, box)
        box.signalUpdateComponent.connect(self.widgetFactory.valueChangedSlot)
        box.signalUserChanged.connect(self.widgetFactory.valueChangedSlot)
        if box.hasValue():
            self.widgetFactory.valueChanged(box, box.value, box.timestamp)

    @property
    def widgetCategory(self):
        return self.widgetFactory.category

    # Returns the actual widget which is part of the composition
    @property
    def widget(self):
        return self.widgetFactory.widget

    @property
    def value(self):
        return self.widgetFactory.value

    @value.setter
    def value(self, value):
        self.widgetFactory.value = value

    def setEnabled(self, enable):
        # Is not processed due to self.widget should always stay disabled
        pass

    # Triggered by DataNotifier signalAddKey
    def addKeyValue(self, key, value):
        pass

    # Triggered by DataNotifier signalRemoveKey
    def removeKey(self, key):
        pass
