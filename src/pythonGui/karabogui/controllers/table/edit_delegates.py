#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on March 6, 2021
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

import numpy as np
from PyQt5.QtCore import pyqtSlot, Qt
from PyQt5.QtGui import QPalette, QValidator
from PyQt5.QtWidgets import QComboBox, QItemDelegate, QLineEdit

from karabo.common.api import KARABO_SCHEMA_OPTIONS
from karabogui.binding.api import (
    FloatBinding, get_default_value, get_native_min_max, IntBinding,
    validate_value)


def get_table_delegate(binding, parent):
    options = binding.attributes.get(KARABO_SCHEMA_OPTIONS, None)
    if options is not None:
        return ComboBoxDelegate(options, parent)
    elif isinstance(binding, (FloatBinding, IntBinding)):
        return NumberDelegate(binding, parent)
    return None


class ComboBoxDelegate(QItemDelegate):
    def __init__(self, options, parent=None):
        super(ComboBoxDelegate, self).__init__(parent)
        # XXX: We might have options from number values, they serialize as
        # ndarray! Once the value casting is back, the string cast has
        # to be removed!
        if isinstance(options, np.ndarray):
            options = options.astype(np.str).tolist()
        self._options = options

    def createEditor(self, parent, option, index):
        """Reimplemented function of QItemDelegate"""
        combo = QComboBox(parent)
        combo.addItems([str(o) for o in self._options])
        combo.currentIndexChanged.connect(self._on_editor_changed)
        return combo

    def setEditorData(self, editor, index):
        """Reimplemented function of QItemDelegate"""
        selection = index.model().data(index, Qt.DisplayRole)
        try:
            selection_index = self._options.index(selection)
            editor.blockSignals(True)
            editor.setCurrentIndex(selection_index)
            editor.blockSignals(False)
        except ValueError:
            # XXX: Due to schema injection, a property value might not be in
            # allowed options and thus not available. This is very rare
            # and unlikely and we just continue!
            raise RuntimeError("The value {} is not in the following "
                               "options: {}".format(selection, self._options))

    def setModelData(self, editor, model, index):
        """Reimplemented function of QItemDelegate"""
        model.setData(index, self._options[editor.currentIndex()], Qt.EditRole)

    @pyqtSlot()
    def _on_editor_changed(self):
        """The current index of the combobox changed. Notify the model.

        This signal MUST be emitted when the editor widget has completed
        editing the data, and wants to write it back into the model.
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


class NumberDelegate(QItemDelegate):
    def __init__(self, binding, parent=None):
        super(NumberDelegate, self).__init__(parent)
        self._binding = binding

    def createEditor(self, parent, option, index):
        """Reimplemented function of QItemDelegate"""
        editor = LineEditEditor(parent)
        value = index.model().data(index, Qt.DisplayRole)
        validator = NumberValidator(self._binding, value, parent=editor)
        editor.setValidator(validator)
        editor.textChanged.connect(self._on_editor_changed)
        return editor

    @pyqtSlot()
    def _on_editor_changed(self):
        self.commitData.emit(self.sender())


class NumberValidator(QValidator):
    def __init__(self, binding, value, parent=None):
        QValidator.__init__(self, parent)
        self._set_value = value
        self._binding = binding
        self.low, self.high = get_native_min_max(binding)

    def validate(self, input, pos):
        """Reimplemented function of QValidator"""
        if input in ('+', '-', ''):
            return self.Intermediate, input, pos
        elif input[-1] in (' '):
            return self.Invalid, input, pos
        elif input[-1] in ('+', '-', 'e'):
            return self.Intermediate, input, pos

        value = validate_value(self._binding, input)
        if value is None:
            return self.Invalid, input, pos
        if self.low <= value and value <= self.high:
            return self.Acceptable, input, pos

        return self.Intermediate, input, pos

    def fixup(self, input):
        """Reimplemented function of QValidator

        If the value input has not been validated properly return the previous
        value before editing.
        """
        if self._set_value is None or self._set_value == "":
            # Note: Ideally a table value should always be there. We make sure
            # it stays like this
            return str(get_default_value(self._binding, force=True))

        return str(self._set_value)