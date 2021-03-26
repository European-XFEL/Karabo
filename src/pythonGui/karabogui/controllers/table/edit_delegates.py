#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on March 6, 2021
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt5.QtCore import pyqtSlot, Qt
from PyQt5.QtGui import QPalette, QValidator
from PyQt5.QtWidgets import QComboBox, QStyledItemDelegate, QLineEdit

from karabo.common.api import KARABO_SCHEMA_OPTIONS
from karabogui.binding.api import (
    FloatBinding, get_default_value, get_min_max, is_equal,
    IntBinding, validate_value)
from karabogui.logger import get_logger
from karabogui.util import SignalBlocker


def get_table_delegate(binding, parent):
    options = binding.attributes.get(KARABO_SCHEMA_OPTIONS, None)
    if options is not None:
        return ComboBoxDelegate(options, parent)
    elif isinstance(binding, (FloatBinding, IntBinding)):
        return NumberDelegate(binding, parent)
    return None


class ComboBoxDelegate(QStyledItemDelegate):
    def __init__(self, options, parent=None):
        super(ComboBoxDelegate, self).__init__(parent)
        # Note: We make sure that the options are a list of strings. For simple
        # types in Karabo they are a coercing array on the binding
        self._options = [str(value) for value in options]

    def createEditor(self, parent, option, index):
        """Reimplemented function of QStyledItemDelegate"""
        editor = QComboBox(parent)
        editor.addItems(self._options)
        editor.currentIndexChanged.connect(self._on_editor_changed)
        return editor

    def setEditorData(self, editor, index):
        """Reimplemented function of QStyledItemDelegate"""
        selection = index.model().data(index, Qt.DisplayRole)
        if selection in self._options:
            selection_index = self._options.index(selection)
            with SignalBlocker(editor):
                editor.setCurrentIndex(selection_index)
        else:
            get_logger().error(
                f"The value {selection} is not in the following "
                f"options: {self._options}")

    def setModelData(self, editor, model, index):
        """Reimplemented function of QStyledItemDelegate"""
        old = index.model().data(index, Qt.DisplayRole)
        new = self._options[editor.currentIndex()]
        if not is_equal(old, new):
            model.setData(index, new, Qt.EditRole)

    @pyqtSlot()
    def _on_editor_changed(self):
        """The current index of the combobox changed. Notify the model.

        This signal MUST be emitted when the editor widget has completed
        editing the data, and wants to write it back into the model.

        XXX: This is in principle a wrong implementation, as it should be
        only emitted when the editor finished.
        """
        self.commitData.emit(self.sender())


class LineEditEditor(QLineEdit):

    def __init__(self, parent=None):
        super(LineEditEditor, self).__init__(parent)
        self._normal_palette = self.palette()
        self._error_palette = QPalette(self._normal_palette)
        self._error_palette.setColor(QPalette.Text, Qt.red)
        self.textChanged.connect(self._check_background)

    @pyqtSlot(str)
    def _check_background(self, text):
        acceptable_input = self.hasAcceptableInput()
        palette = (self._normal_palette if acceptable_input
                   else self._error_palette)
        self.setPalette(palette)


class NumberDelegate(QStyledItemDelegate):
    def __init__(self, binding, parent=None):
        super(NumberDelegate, self).__init__(parent)
        self._binding = binding

    def createEditor(self, parent, option, index):
        """Reimplemented function of QStyledItemDelegate"""
        editor = LineEditEditor(parent)
        old = index.model().data(index, Qt.DisplayRole)
        validator = NumberValidator(self._binding, old, parent=editor)
        editor.setValidator(validator)
        return editor

    def setModelData(self, editor, model, index):
        """Reimplemented function of QStyledItemDelegate

        We only recreate the table if we have changes in our value! We can do
        this since the delegate will call `setModelData` once its out of focus.
        """
        old = index.model().data(index, Qt.DisplayRole)
        new = editor.text()
        if not is_equal(old, new):
            model.setData(index, new, Qt.EditRole)
            self.commitData.emit(self.sender())


class NumberValidator(QValidator):
    def __init__(self, binding, old, parent=None):
        super().__init__(parent=parent)
        self._old_value = old
        self._binding = binding
        self.low, self.high = get_min_max(binding)

    def validate(self, input, pos):
        """Reimplemented function of QValidator to validate numeric input"""
        if input in ('+', '-', ''):
            return self.Intermediate, input, pos
        elif input[-1] in (' '):
            return self.Invalid, input, pos
        elif input[-1] in ('+', '-', 'e'):
            return self.Intermediate, input, pos

        value = validate_value(self._binding, input)
        if value is None:
            return self.Invalid, input, pos

        if self.inside_limits(value):
            return self.Acceptable, input, pos

        return self.Intermediate, input, pos

    def inside_limits(self, value):
        """Check if a value is within limits"""
        if self.low is not None and value < self.low:
            return False
        if self.high is not None and value > self.high:
            return False
        return True

    def fixup(self, input):
        """Reimplemented function of QValidator

        If the value input has not been validated properly return the previous
        value before editing.
        """
        if self._old_value is None or self._old_value == "":
            # Note: Ideally a table value should always be there. We make sure
            # it stays like this
            return str(get_default_value(self._binding, force=True))

        return str(self._old_value)
