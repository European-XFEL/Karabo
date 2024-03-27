#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 29, 2016
# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
#############################################################################
from qtpy import uic
from qtpy.QtCore import Slot
from qtpy.QtGui import QValidator
from qtpy.QtWidgets import QDialog, QDialogButtonBox, QFormLayout, QSpinBox

from karabogui.util import InputValidator, SignalBlocker

from .utils import get_dialog_ui


class ObjectDuplicateDialog(QDialog):
    def __init__(self, simple_name, parent=None):
        """ The dialog expects a ``simple_name`` string

        :param simple_name: A string which could be used for creating the new
                            duplicate simple Name
        :param parent: A parent object
        """
        super().__init__(parent)
        uic.loadUi(get_dialog_ui('object_duplicate.ui'), self)
        self.setWindowTitle(f'Duplicate object {simple_name}')

        # Add spinboxes with validator
        self.sbStart = UIntSpinbox(self)
        self.sbStart.valueChanged.connect(self._indexChanged)
        self.formLayout.setWidget(1, QFormLayout.FieldRole, self.sbStart)
        self.sbEnd = UIntSpinbox(self)
        self.formLayout.setWidget(2, QFormLayout.FieldRole, self.sbEnd)
        self.sbEnd.valueChanged.connect(self._indexChanged)
        self.cbNoIndex.stateChanged.connect(self._updateIndex)

        validator = InputValidator(parent=self)
        self.leTitle.setValidator(validator)
        self.leTitle.setText(simple_name)
        self._update_text()

    def _update_text(self):
        """Update the text for duplicated devices"""
        if not self.cbNoIndex.isChecked():
            start_index = self.sbStart.value()
            end_index = self.sbEnd.value()
            nb_dupe = end_index - start_index + 1
        else:
            nb_dupe = 1

        text = f'You are about to create <b>{nb_dupe}</b> duplicate(s)'
        self.laText.setText(text)

    @Slot(int)
    def _updateIndex(self, value):
        enabled = True if not value else False
        self.sbStart.setEnabled(enabled)
        self.sbEnd.setEnabled(enabled)
        self._update_text()

    @Slot(int)
    def _indexChanged(self, value):
        start_index = self.sbStart.value()
        end_index = self.sbEnd.value()

        while start_index > end_index:
            end_index += 1
        with SignalBlocker(self.sbEnd):
            self.sbEnd.setValue(end_index)
        self._update_text()

    @property
    def duplicate_names(self):
        """Create duplicated names for the project devices

        If the `noIndex` checkbox is enabled, we only consider the `title`!
        """
        simple_names = []
        if not self.cbNoIndex.isChecked():
            start_index = self.sbStart.value()
            end_index = self.sbEnd.value()
            for index in range(start_index, end_index + 1):
                dupe_name = f'{self.leTitle.text()}{index}'
                simple_names.append(dupe_name)
        else:
            simple_names.append(self.leTitle.text())

        return simple_names


class UIntSpinbox(QSpinBox):

    def __init__(self, parent):
        super().__init__(parent)
        self.setMinimum(0)
        self.setMaximum(1000)

    def validate(self, value, pos):
        if not value.isdigit() and value != '':
            return QValidator.Invalid, value, pos

        return super().validate(value, pos)


class ObjectEditDialog(QDialog):
    def __init__(self, object_type='object', model=None, parent=None):
        super().__init__(parent)
        uic.loadUi(get_dialog_ui('object_edit.ui'), self)

        if model is None:
            title = f'Add {object_type}'
        else:
            title = f'Edit {object_type}'
            self.leTitle.setText(model.simple_name)

        validator = InputValidator(object_type, parent=self)
        self.leTitle.setValidator(validator)
        self.leTitle.textChanged.connect(self.validate)
        self.setWindowTitle(title)
        self.validate()

    @Slot()
    def validate(self):
        enabled = self.leTitle.hasAcceptableInput()
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(enabled)

    @property
    def simple_name(self):
        return self.leTitle.text()
