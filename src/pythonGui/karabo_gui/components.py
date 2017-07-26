#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 2, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import numbers

import numpy
from PyQt4.QtCore import QObject, pyqtSignal, pyqtSlot, QTimer
from PyQt4.QtGui import QHBoxLayout, QWidget

from karabo.common.api import State
from . import messagebox
from .indicators import STATE_COLORS
from .singletons.api import get_network
from .util import generateObjectName
from .widget import EditableWidget, DisplayWidget, Widget


class BaseComponent(QObject):
    Widget = EditableWidget
    signalValueChanged = pyqtSignal(object, object)  # key, value

    def __init__(self, parent):
        super(BaseComponent, self).__init__(parent)

    def connectWidget(self, box):
        box.signalNewDescriptor.connect(self.widgetFactory.typeChangedSlot)
        self.widgetFactory.setParent(self)
        if box.descriptor is not None:
            self.widgetFactory.typeChangedSlot(box)

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
    def widget(self):
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

    def connectWidget(self, box):
        BaseComponent.connectWidget(self, box)
        if box.hasValue():
            self.widgetFactory.valueChanged(box, box.value, box.timestamp)
        box.signalUpdateComponent.connect(self.widgetFactory.valueChangedSlot)
        self.widgetFactory.signalEditingFinished.connect(self.onEditingFinished)
        box.signalUserChanged.connect(self.onUserChanged)

    @pyqtSlot(object, object, object)
    def onUserChanged(self, box, value, timestamp=None):
        box.set(value, None)

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

        # Configuration changed - so project needs to be informed
        if box.configuration.type == 'projectClass':
            box.configuration.signalBoxChanged.emit()


class EditableApplyLaterComponent(BaseComponent):
    # signals
    signalApplyChanged = pyqtSignal(object, bool)  # key, state of apply button

    def __init__(self, classAlias, box, parent):
        self.__currentDisplayValue = None

        self.__compositeWidget = QWidget(parent)
        self.__compositeWidget.setObjectName(generateObjectName(
            self.__compositeWidget))
        hLayout = QHBoxLayout(self.__compositeWidget)
        hLayout.setContentsMargins(2, 2, 2, 2)

        W = Widget.widgets.get(classAlias)
        if W is None:
            self.widgetFactory = classAlias
        else:
            self.widgetFactory = W(box, self.__compositeWidget)
        super(EditableApplyLaterComponent, self).__init__(parent)

        hLayout.addWidget(self.widgetFactory.widget)

        self.box = box
        self.hasConflict = False

        self.__busyTimer = QTimer(self)
        self.__busyTimer.setSingleShot(True)
        self.__busyTimer.timeout.connect(self.onTimeOut)
        self._applyEnabled = False

        self.connectWidget(box)
        self.widgetFactory.setReadOnly(False)

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

    def apply_changes(self):
        """All changes of this component need to be send to the GuiServerDevice
        """
        network = get_network()
        changes = []
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
                    # XXX: This can probably be batched for the device group
                    dev_changes = [(deviceBox, self.widgetFactory.value)]
                    network.onReconfigure(dev_changes)
            elif b.descriptor is not None:
                changes.append((b, self.widgetFactory.value))

        if changes:
            self.__busyTimer.start(5000)
            network.onReconfigure(changes)

    def decline_changes(self):
        """All changes of this component are declined and reset to the current
        value on device
        """
        for b in self.boxes:
            self.widgetFactory.valueChanged(b, self.__currentDisplayValue)
        self.updateButtons()

    def onTimeOut(self):
        messagebox.show_warning(
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

        object_name = self.__compositeWidget.objectName()
        if isEqualEditable:
            # No changes
            self.hasConflict = False
            style_sheet = ("QWidget#{} ".format(object_name) +
                           "{ border: 0px }")
        elif self.hasConflict:
            # Conflict - the value on device got changed
            color = STATE_COLORS[State.UNKNOWN]
            style_sheet = ("QWidget#{} ".format(object_name) +
                           "{{ border: 2px solid rgb{} }}".format(color))
        else:
            # Something which can be changed
            color = STATE_COLORS[State.CHANGING]
            style_sheet = ("QWidget#{} ".format(object_name) +
                           "{{ border: 2px solid rgb{} }}".format(color))

        allowed = box.isAllowed()
        self.applyEnabled = allowed and not isEqualEditable
        # Use different borders to show status
        self.__compositeWidget.setStyleSheet(style_sheet)

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
