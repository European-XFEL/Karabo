#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on March 6, 2021
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt5.QtCore import pyqtSlot, Qt
from PyQt5.QtGui import QPalette
from PyQt5.QtWidgets import QComboBox, QStyledItemDelegate, QLineEdit

from karabo.common.api import KARABO_SCHEMA_OPTIONS
from karabogui.binding.api import (
    FloatBinding, get_default_value, IntBinding, VectorBinding)
from karabogui.controllers.validators import (
    BindingValidator as GenericValidator, ListValidator, SimpleValidator)
from karabogui.logger import get_logger
from karabogui.util import SignalBlocker

from karabo.native import is_equal


def get_table_delegate(binding, parent):
    options = binding.attributes.get(KARABO_SCHEMA_OPTIONS, None)
    if options is not None:
        return ComboBoxDelegate(options, parent)
    elif isinstance(binding, (FloatBinding, IntBinding)):
        return NumberDelegate(binding, parent)
    elif isinstance(binding, VectorBinding):
        return VectorDelegate(binding, parent)
    else:
        return BindingDelegate(binding, parent)


class ComboBoxDelegate(QStyledItemDelegate):
    def __init__(self, options, parent=None):
        super().__init__(parent)
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
        super().__init__(parent)
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
        super().__init__(parent)
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


class NumberValidator(SimpleValidator):

    def __init__(self, binding, old, parent=None):
        super().__init__(binding=binding, parent=parent)
        self._old_value = old

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


class VectorDelegate(QStyledItemDelegate):
    def __init__(self, binding, parent=None):
        super().__init__(parent=parent)
        self._binding = binding

    def createEditor(self, parent, option, index):
        """Reimplemented function of QStyledItemDelegate"""
        editor = LineEditEditor(parent)
        old = index.model().data(index, Qt.DisplayRole)
        old = _create_string_list(old)
        validator = VectorValidator(self._binding, old, parent=editor)
        editor.setValidator(validator)
        return editor

    def setModelData(self, editor, model, index):
        """Reimplemented function of QStyledItemDelegate"""
        old = index.model().data(index, Qt.DisplayRole)
        old = _create_string_list(old)
        new = editor.text()
        if not is_equal(old, new):
            model.setData(index, new, Qt.EditRole)
            self.commitData.emit(self.sender())

    def setEditorData(self, editor, index):
        """Reimplemented function of QStyledItemDelegate"""
        value = index.model().data(index, Qt.DisplayRole)
        value = _create_string_list(value)
        editor.setText(value)


def _create_string_list(value):
    """Return a string list with stripped white spaces if present"""
    return value if not value else ",".join(
        [v.rstrip().lstrip() for v in value.split(",")])


class VectorValidator(ListValidator):

    def __init__(self, binding, old, parent=None):
        super().__init__(binding=binding, parent=parent)
        self._old_value = old

    def fixup(self, input):
        """Reimplemented function of QValidator"""
        if self._old_value is None:
            # The model will account for an empty string and use a list
            return ""

        return str(self._old_value)


class BindingDelegate(QStyledItemDelegate):
    def __init__(self, binding, parent=None):
        super().__init__(parent)
        self._binding = binding

    def createEditor(self, parent, option, index):
        """Reimplemented function of QStyledItemDelegate"""
        editor = LineEditEditor(parent)
        old = index.model().data(index, Qt.DisplayRole)
        validator = BindingValidator(self._binding, old, parent=editor)
        editor.setValidator(validator)
        return editor

    def setModelData(self, editor, model, index):
        """Reimplemented function of QStyledItemDelegate"""
        old = index.model().data(index, Qt.DisplayRole)
        new = editor.text()
        if not is_equal(old, new):
            model.setData(index, new, Qt.EditRole)
            self.commitData.emit(self.sender())


class BindingValidator(GenericValidator):

    def __init__(self, binding, old, parent=None):
        super().__init__(binding=binding, parent=parent)
        self._old_value = old

    def fixup(self, input):
        """Reimplemented function of QValidator"""
        if self._old_value is None:
            return str(get_default_value(self._binding, force=True))

        return str(self._old_value)
