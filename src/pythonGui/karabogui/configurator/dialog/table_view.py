#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 9, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from pathlib import Path

from qtpy import uic
from qtpy.QtCore import QModelIndex
from qtpy.QtWidgets import (
    QDialog, QDialogButtonBox, QHBoxLayout, QPushButton, QVBoxLayout, QWidget)

import karabogui.icons as icons
from karabogui.controllers.edit.table import EditableTableElement

MAX_WIDTH = 1024
COLUMN_FACTOR = 120


class TableButtonToolbar(QWidget):
    def __init__(self, table_controller, parent=None):
        super().__init__(parent=parent)

        self.table_controller = table_controller
        self.table_model = self.table_controller.sourceModel()

        self._move_up_bt = QPushButton(icons.arrowFancyUp, "", self)
        self._move_down_bt = QPushButton(icons.arrowFancyDown, "", self)
        self._add_bt = QPushButton(icons.add, "", self)
        self._du_bt = QPushButton(icons.editCopy, "", self)
        self._remove_bt = QPushButton(icons.delete, "", self)

        self._move_up_bt.clicked.connect(self._move_row_up)
        self._move_down_bt.clicked.connect(self._move_row_down)
        self._add_bt.clicked.connect(self._add_row)
        self._du_bt.clicked.connect(self._duplicate_row)
        self._remove_bt.clicked.connect(self._remove_row)

        widget_layout = QVBoxLayout()
        widget_layout.setContentsMargins(0, 0, 0, 0)
        widget_layout.addWidget(self._move_up_bt)
        widget_layout.addWidget(self._move_down_bt)
        widget_layout.addWidget(self._add_bt)
        widget_layout.addWidget(self._du_bt)
        widget_layout.addWidget(self._remove_bt)
        widget_layout.addStretch(10)
        widget_layout.setContentsMargins(0, 0, 0, 0)
        self.setLayout(widget_layout)

        selection_model = self.table_controller.tableWidget().selectionModel()
        selection_model.selectionChanged.connect(self._table_selection_changed)
        self.set_button_enabled()

    def _add_row(self):
        row = self.table_controller.currentIndex().row()
        self.table_model.insertRows(row + 1, 1, QModelIndex())
        self.set_button_enabled()

    def _duplicate_row(self):
        row = self.table_controller.currentIndex().row()
        self.table_model.duplicate_row(row)
        self.set_button_enabled()

    def _move_row_up(self):
        row = self.table_controller.currentIndex().row()
        self.table_model.move_row_up(row)
        self.table_controller.tableWidget().selectRow(row - 1)
        self.set_button_enabled()

    def _move_row_down(self):
        row = self.table_controller.currentIndex().row()
        self.table_model.move_row_down(row)
        self.table_controller.tableWidget().selectRow(row + 1)
        self.set_button_enabled()

    def _remove_row(self):
        index = self.table_controller.currentIndex()
        self.table_model.removeRows(index.row(), 1, QModelIndex())
        self.set_button_enabled()

    def set_button_enabled(self):
        selection_model = self.table_controller.tableWidget().selectionModel()
        has_selection = selection_model.hasSelection()
        current_row = self.table_controller.currentIndex().row()
        self._move_up_bt.setEnabled(has_selection and current_row > 0)
        self._move_down_bt.setEnabled(has_selection and current_row <
                                      self.table_model.rowCount() - 1)
        self._du_bt.setEnabled(has_selection)
        self._remove_bt.setEnabled(has_selection)

    def _table_selection_changed(self, selected, deselected):
        self.set_button_enabled()


class TableDialog(QDialog):
    def __init__(self, proxy, editable, parent=None):
        """A dialog to configure a VectorHash

        :param proxy: The `PropertyProxy` object representing the VectorHash
        :param editable: If the controller inside the dialog is editable
        :param parent: The parent of the dialog
        """
        super().__init__(parent)
        filepath = Path(__file__).parent / 'table_view.ui'
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

        self.toolbar_controller = TableButtonToolbar(
            table_controller=self.controller)

        layout = QHBoxLayout()
        layout.setContentsMargins(2, 2, 2, 2)
        layout.addWidget(self.controller.widget)
        layout.addWidget(self.toolbar_controller)
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
        return super().done(result)
