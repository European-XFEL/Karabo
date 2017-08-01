#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on April 27, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from collections import OrderedDict, defaultdict
import numbers

import numpy
from PyQt4.QtCore import pyqtSlot, Qt
from PyQt4.QtGui import QCursor, QHBoxLayout, QTreeWidgetItem, QWidget

from karabo.common.api import State
from karabo.middlelayer import Type
from karabo_gui.alarms.api import ALARM_LOW, ALARM_HIGH, WARN_LOW, WARN_HIGH
from karabo_gui.indicators import STATE_COLORS
from karabo_gui.popupwidget import PopupWidget
from karabo_gui.schema import ChoiceOfNodes
from karabo_gui.util import generateObjectName


def _build_array_cmp(dtype):
    """Builds a comparison function for numpy arrays"""
    coerce = dtype.type
    if coerce is numpy.bool_:
        coerce = int

    def _array_cmp(a, b):
        try:
            return coerce(a) == coerce(b)
        except ValueError:
            return False

    return _array_cmp


def _cmp(a, b):
    """Normal comparison"""
    return a == b


class BaseTreeWidgetItem(QTreeWidgetItem):
    isChoiceElement = False
    isListElement = False
    description = None

    def __init__(self, box, parent, parentItem=None):
        if parentItem:
            super(BaseTreeWidgetItem, self).__init__(parentItem)
        else:
            super(BaseTreeWidgetItem, self).__init__(parent)

        self.box = box
        self._connected_signals = defaultdict(list)
        self.is_editable = False
        self.has_conflict = False
        self.apply_enabled = False
        self._editor_value_initialized = False

        self.display_widget = None
        self.editable_widget = None
        self._contextMenu = None

        # Popup widget for tooltip info
        self.popupWidget = None

    def connect_signal(self, signal, receiver):
        self._connected_signals[signal].append(receiver)
        signal.connect(receiver)

    def create_display_widget(self, klass, box):
        tree_widget = self.treeWidget()
        # Create a display widget
        self.display_widget = klass(box, tree_widget)
        self.connect_signal(box.signalUpdateComponent,
                            self.display_widget.valueChangedSlot)
        if box.hasValue():
            self.display_widget.valueChanged(box, box.value, box.timestamp)

        self.display_widget.setReadOnly(True)
        tree_widget.setItemWidget(self, 1, self.display_widget.widget)
        tree_widget.resizeColumnToContents(1)

    def create_editable_widget(self, klass, box):
        tree_widget = self.treeWidget()
        self.editable_widget = klass(box, tree_widget)
        # Introduce layout to have some border to show
        self.qt_edit_widget = QWidget()
        layout = QHBoxLayout(self.qt_edit_widget)
        layout.setContentsMargins(2, 2, 2, 2)
        layout.addWidget(self.editable_widget.widget)
        self.qt_edit_widget.setObjectName(
            generateObjectName(self.qt_edit_widget))
        tree_widget.setItemWidget(self, 2, self.qt_edit_widget)
        tree_widget.resizeColumnToContents(2)
        # Setup context menu
        self._contextMenu = self.setupContextMenu()

    def make_class_connections(self, box):
        """Connect to Box signals needed when editing a class Configuration
        before instantiation.
        """
        self.connect_signal(box.signalNewDescriptor,
                            self.editable_widget.typeChangedSlot)
        if box.descriptor is not None:
            self.editable_widget.typeChangedSlot(box)
        if box.hasValue():
            self.editable_widget.valueChanged(box, box.value, box.timestamp)
        self.connect_signal(box.signalUpdateComponent,
                            self.editable_widget.valueChangedSlot)
        self.connect_signal(self.editable_widget.signalEditingFinished,
                            self._on_editing_finished)
        self.connect_signal(box.signalUserChanged,
                            self._on_user_edit)
        self.editable_widget.setReadOnly(False)

    def make_device_connections(self, box):
        """Connect to Box signals needed when editing a Configuration for a
        running device instance.
        """
        self.connect_signal(box.signalNewDescriptor,
                            self.editable_widget.typeChangedSlot)
        if box.descriptor is not None:
            self.editable_widget.typeChangedSlot(box)
        self.connect_signal(self.editable_widget.signalEditingFinished,
                            self._on_editing_finished)
        self.connect_signal(box.signalUserChanged, self._on_user_edit)
        self.connect_signal(box.signalUpdateComponent,
                            self._on_display_value_change)
        if box.hasValue():
            self._on_display_value_change(box, box.value)
        self.connect_signal(
            box.configuration.boxvalue.state.signalUpdateComponent,
            self._update_edit_widget)
        self.editable_widget.setReadOnly(False)
        self.is_editable = True

    def destroy(self):
        """Disconnect all signal connections made by this item
        """
        for signal, receivers in self._connected_signals.items():
            for recvr in receivers:
                signal.disconnect(recvr)
        self._connected_signals.clear()

    def setupContextMenu(self):
        raise NotImplementedError("BaseTreeWidgetItem.setupContextMenu")

    def showContextMenu(self):
        if self._contextMenu is None:
            return
        self._contextMenu.exec_(QCursor.pos())

    def setErrorState(self, isError):
        if self.display_widget:
            self.display_widget.setErrorState(isError)

    def setReadOnly(self, readOnly):
        if readOnly is True:
            self.setFlags(self.flags() & ~Qt.ItemIsEnabled)
        else:
            self.setFlags(self.flags() | Qt.ItemIsEnabled)

    def setToolTipDialogVisible(self, show):
        if not self.popupWidget:
            self.popupWidget = PopupWidget(self.treeWidget())

        if show:
            self.updateToolTipDialog()
            pos = QCursor.pos()
            pos.setX(pos.x() + 10)
            pos.setY(pos.y() + 10)
            self.popupWidget.move(pos)
            self.popupWidget.show()
        else:
            self.popupWidget.hide()

    def updateToolTipDialog(self):
            info = OrderedDict()

            if len(self.text(0)) > 0:
                info["Property"] = self.text(0)
            d = self.box.descriptor
            if d.description is not None:
                info["Description"] = d.description

            info["Key"] = '.'.join(self.box.path)
            if isinstance(d, Type):
                info["Value Type"] = d.hashname()
            if d.defaultValue is not None:
                info["Default Value"] = d.defaultValue
            if d.alias is not None:
                info["Alias"] = d.alias
            if d.tags is not None:
                info["Tags"] = ", ".join(d.tags)
            if self.box.timestamp is not None:
                info["Timestamp"] = self.box.timestamp.toLocal()
            if d.displayType and d.displayType.startswith('bin|'):
                info["Bits"] = d.displayType[4:]
            if self.box.configuration.type == "device":
                info["Value on device"] = self.box.value

            alarms = [('Warn low', WARN_LOW), ('Warn high', WARN_HIGH),
                      ('Alarm low', ALARM_LOW), ('Alarm high', ALARM_HIGH)]
            for label, alarm in alarms:
                this_alarm = getattr(d, alarm)
                info[label] = 'n/a' if this_alarm is None else this_alarm

            self.popupWidget.setInfo(info)

    # ---------------------------------------------------------------------
    # Edit widget related code

    def _update_edit_widget(self):
        """Update the editable widget to reflect the current state of affairs
        """
        box = self.box
        value = box.value if box.hasValue() else None
        if isinstance(box.descriptor, ChoiceOfNodes):
            value = box.current  # ChoiceOfNodes is "special"
        widget_value = self.editable_widget.value

        if value is None:
            no_changes = False
        elif (isinstance(value, (numbers.Complex, numpy.inexact))
                and not isinstance(value, numbers.Integral)):
            diff = abs(value - widget_value)
            absErr = box.descriptor.absoluteError
            relErr = box.descriptor.relativeError
            if absErr is not None:
                no_changes = diff < absErr
            elif relErr is not None:
                no_changes = diff < abs(value * relErr)
            else:
                no_changes = diff < 1e-4
        elif isinstance(value, (list, numpy.ndarray)):
            if len(value) != len(widget_value):
                no_changes = False
            else:
                no_changes = True
                cmp = _cmp
                if isinstance(value, numpy.ndarray):
                    cmp = _build_array_cmp(value.dtype)
                for i in range(len(value)):
                    if not cmp(value[i], widget_value[i]):
                        no_changes = False
                        break
        else:
            no_changes = (str(value) == str(widget_value))

        object_name = self.qt_edit_widget.objectName()
        if no_changes:
            # No changes
            self.has_conflict = False
            style_sheet = ("QWidget#{} ".format(object_name) +
                           "{{ border: 0px }}")
        elif self.has_conflict:
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
        apply_changed = allowed and not no_changes
        if apply_changed:
            # Store the widget value in the Configuration object
            box.configuration.setUserValue(box, widget_value)
        else:
            # Otherwise, clear the user value!
            box.configuration.clearUserValue(box)

        if self.apply_enabled != apply_changed:
            self.apply_enabled = apply_changed
            # Let the tree widget know...
            self.treeWidget().onApplyChanged(self.box, self.apply_enabled)
        # Show colored border around the wrapper widget
        self.qt_edit_widget.setStyleSheet(style_sheet)

    def apply_changes(self):
        """All changes of this component need to be send to the GuiServerDevice
        """
        if not self.is_editable:
            # Does not make sense for classes
            return

        box = self.box
        box.signalUserChanged.emit(box, self.editable_widget.value, None)
        if box.configuration.type == "macro":
            box.set(self.editable_widget.value)
        elif box.descriptor is not None:
            box.configuration.setUserValue(box, self.editable_widget.value)
            box.configuration.sendUserValue(box)

    def decline_changes(self):
        """All changes of this component are declined and reset to the current
        value on device
        """
        if not self.is_editable:
            # Does not make sense for classes
            return

        box = self.box
        box.configuration.clearUserValue(box)
        if box.hasValue():
            self.editable_widget.valueChanged(box, box.value, box.timestamp)
        self._update_edit_widget()

    @pyqtSlot(object, object)
    def _on_display_value_change(self, box, value):
        if not self._editor_value_initialized:
            self.editable_widget.valueChanged(box, value)
            self._editor_value_initialized = True
        self.has_conflict = True
        self._update_edit_widget()

    @pyqtSlot(object, object)
    def _on_editing_finished(self, box, value):
        if not self.is_editable:
            # Class related editable_widget
            self.box.set(value, None)
            # Configuration changed - so project needs to be informed
            if self.box.configuration.type == 'projectClass':
                self.box.configuration.signalBoxChanged.emit()
        else:
            # Device related editable_widget
            self._update_edit_widget()

    @pyqtSlot(object, object, object)
    def _on_user_edit(self, box, value, timestamp=None):
        self.editable_widget.valueChangedSlot(box, value, timestamp)
        self._update_edit_widget()
