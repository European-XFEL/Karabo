#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 9, 2017
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
import weakref
from pathlib import Path

from qtpy import uic
from qtpy.QtCore import QItemSelection, Qt, Slot
from qtpy.QtWidgets import (
    QDialog, QDialogButtonBox, QHBoxLayout, QPushButton, QVBoxLayout, QWidget)

import karabogui.icons as icons
from karabogui.controllers.edit.table import EditableTableElement

MAX_WIDTH = 1024
COLUMN_FACTOR = 120


class TableToolBar(QWidget):
    def __init__(self, controller, parent=None):
        super().__init__(parent=parent)
        self.setAttribute(Qt.WA_DeleteOnClose)
        self._controller = weakref.ref(controller)
        selection_model = controller.tableWidget().selectionModel()
        selection_model.selectionChanged.connect(self._table_selection_changed)

        self._move_up_button = QPushButton(icons.arrowFancyUp, "", self)
        self._move_down_button = QPushButton(icons.arrowFancyDown, "", self)
        self._add_button = QPushButton(icons.add, "", self)
        self._du_button = QPushButton(icons.editCopy, "", self)
        self._remove_button = QPushButton(icons.delete, "", self)

        self._move_up_button.clicked.connect(self._move_row_up)
        self._move_down_button.clicked.connect(self._move_row_down)
        self._add_button.clicked.connect(self._add_row_below)
        self._du_button.clicked.connect(self._duplicate_row)
        self._remove_button.clicked.connect(self._remove_row)

        layout = QVBoxLayout()
        layout.addWidget(self._move_up_button)
        layout.addWidget(self._move_down_button)
        layout.addWidget(self._add_button)
        layout.addWidget(self._du_button)
        layout.addWidget(self._remove_button)
        layout.addStretch(10)
        layout.setContentsMargins(0, 0, 0, 0)
        self.setLayout(layout)

        self._set_button_states()

    @property
    def controller(self):
        """Return the weakly stored controller variable"""
        return self._controller()

    @Slot()
    def _add_row_below(self):
        self.controller.add_row_below()
        self._set_button_states()

    @Slot()
    def _duplicate_row(self):
        self.controller.duplicate_row()
        self._set_button_states()

    @Slot()
    def _move_row_up(self):
        self.controller.move_row_up()
        self._set_button_states()

    @Slot()
    def _move_row_down(self):
        self.controller.move_row_down()

    @Slot()
    def _remove_row(self):
        self.controller.remove_row()
        self._set_button_states()

    @Slot(QItemSelection, QItemSelection)
    def _table_selection_changed(self, selected, deselected):
        self._set_button_states()

    def _set_button_states(self):
        model = self.controller.sourceModel()
        if model is None:
            # Note: This happens when the dialog is closed.
            return

        selection_model = self.controller.tableWidget().selectionModel()
        has_selection = selection_model.hasSelection()
        current_row = self.controller.currentIndex().row()
        self._move_up_button.setEnabled(has_selection and current_row > 0)
        self._move_down_button.setEnabled(has_selection and current_row <
                                          model.rowCount() - 1)
        self._du_button.setEnabled(has_selection)
        self._remove_button.setEnabled(has_selection)


class TableDialog(QDialog):
    def __init__(self, proxy, editable, parent=None):
        """A dialog to configure a VectorHash

        :param proxy: The `PropertyProxy` object representing the VectorHash
        :param editable: If the controller inside the dialog is editable
        :param parent: The parent of the dialog
        """
        super().__init__(parent)
        filepath = Path(__file__).parent / "table_view.ui"
        uic.loadUi(filepath, self)
        # XXX: Do not change the name of this variable since the
        # `EditDelegate` method `setModelData` refers to this dialog as the
        # passed `editor` and fetches the data of the `controller`
        self.controller = EditableTableElement(proxy=proxy)
        # This is parented by the `EditDelegate`, which means it will stay
        # alive very long!
        self.controller.create(parent)
        self.controller.set_read_only(not editable)
        self.controller.binding_update(proxy)
        self.controller.value_update(proxy)
        self.toolbar = None

        layout = QHBoxLayout()
        layout.setContentsMargins(2, 2, 2, 2)
        layout.addWidget(self.controller.widget)
        if editable:
            self.toolbar = TableToolBar(controller=self.controller)
            layout.addWidget(self.toolbar)
        # `empty_widget` is defined in table_edit.ui
        self.empty_widget.setLayout(layout)
        cancel_button = self.buttonBox.button(QDialogButtonBox.Cancel)
        cancel_button.setVisible(editable)

        width = 450
        height = 350

        binding = proxy.binding
        if binding is not None:
            columns = len(binding.bindings)
            # minimum width of 450, but can go up to `MAX_WIDTH`
            width = min(max(columns * COLUMN_FACTOR, width), MAX_WIDTH)
        self.resize(width, height)

    def done(self, result):
        self.controller.destroy_widget()
        self.toolbar = None
        return super().done(result)
