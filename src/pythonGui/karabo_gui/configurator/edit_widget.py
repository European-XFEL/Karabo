#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on August 7, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from collections import defaultdict
import numbers

import numpy
from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import QHBoxLayout, QWidget

from karabo_gui.schema import ChoiceOfNodes


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


class EditWidgetWrapper(QWidget):
    def __init__(self, box, parent=None):
        super(EditWidgetWrapper, self).__init__(parent)

        self.box = box
        self._connected_signals = defaultdict(list)
        self._editor_value_initialized = False

        self.is_class_editor = False
        self.editable_widget = None

    def connect_signal(self, signal, receiver):
        self._connected_signals[signal].append(receiver)
        signal.connect(receiver)

    def disconnect_signals(self):
        """Disconnect all signal connections made by this widget
        """
        for signal, receivers in self._connected_signals.items():
            for recvr in receivers:
                signal.disconnect(recvr)
        self._connected_signals.clear()

    def create_editable_widget(self, klass):
        self.editable_widget = klass(self.box, self)
        # Introduce layout to have some border to show
        layout = QHBoxLayout(self)
        layout.setContentsMargins(2, 2, 2, 2)
        layout.addWidget(self.editable_widget.widget)

    def make_class_connections(self):
        """Connect to Box signals needed when editing a class Configuration
        before instantiation.
        """
        box = self.box
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
        self.is_class_editor = True

    def make_device_connections(self):
        """Connect to Box signals needed when editing a Configuration for a
        running device instance.
        """
        box = self.box
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
        self.is_class_editor = False

    # ---------------------------------------------------------------------
    # Edit widget related code

    @pyqtSlot()
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

        allowed = box.isAllowed()
        apply_changed = allowed and not no_changes
        if apply_changed:
            # Store the widget value in the Configuration object
            box.configuration.setUserValue(box, widget_value)
        else:
            # Otherwise, clear the user value!
            box.configuration.clearUserValue(box)

        # Make sure to keep enabling in sync
        self.editable_widget.widget.setEnabled(allowed)

    def apply_changes(self):
        """All changes of this component need to be send to the GuiServerDevice
        """
        if self.is_class_editor:
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
        if self.is_class_editor:
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
        self._update_edit_widget()

    @pyqtSlot(object, object)
    def _on_editing_finished(self, box, value):
        if self.is_class_editor:
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
