#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on March 12, 2012
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

from qtpy.QtCore import Qt, Slot
from qtpy.QtGui import QKeySequence
from qtpy.QtWidgets import (
    QDialog, QHBoxLayout, QInputDialog, QListWidget, QListWidgetItem,
    QPushButton, QVBoxLayout)

from karabogui import icons, messagebox
from karabogui.binding.api import (
    VectorBoolBinding, get_min_max_size, is_vector_floating, is_vector_integer)

USER_ROLE = Qt.UserRole


class ListEditDialog(QDialog):
    def __init__(self, binding, duplicates_ok=True, parent=None):
        super().__init__(parent)
        self.setWindowTitle("Edit list")
        self.binding = binding
        self.duplicates_ok = duplicates_ok

        self._allowed_choices = {}
        if isinstance(self.binding, VectorBoolBinding):
            self._allowed_choices["False"] = False
            self._allowed_choices["True"] = True

        # Check for possible size limitations!
        self.minSize, self.maxSize = get_min_max_size(binding)

        h_layout = QHBoxLayout(self)
        list_widget = QListWidget(self)
        slot = list_widget.currentItemChanged[QListWidgetItem, QListWidgetItem]
        slot.connect(self._update_buttons)
        list_widget.itemDoubleClicked.connect(self.itemDoubleClicked)
        h_layout.addWidget(list_widget)
        self.list_widget = list_widget
        self.list_widget.setDragDropMode(QListWidget.InternalMove)

        # Button Actions
        v_layout = QVBoxLayout()
        button = QPushButton(icons.add, "", self)
        button.clicked.connect(self._add_clicked)
        v_layout.addWidget(button)

        self.edit_button = QPushButton(icons.edit, "", self)
        self.edit_button.clicked.connect(self._edit_clicked)
        v_layout.addWidget(self.edit_button)

        self.remove_button = QPushButton(icons.delete, "", self)
        self.remove_button.clicked.connect(self._remove_clicked)
        v_layout.addWidget(self.remove_button)

        self.move_up_button = QPushButton(icons.arrowFancyUp, "", self)
        self.move_up_button.clicked.connect(self._move_up_clicked)
        v_layout.addWidget(self.move_up_button)

        self.move_down_button = QPushButton(icons.arrowFancyDown, "", self)
        self.move_down_button.clicked.connect(self._move_down_clicked)
        v_layout.addWidget(self.move_down_button)
        v_layout.addStretch(1)

        button = QPushButton("OK", self)
        button.clicked.connect(self.accept)
        v_layout.addWidget(button)

        button = QPushButton("Cancel", self)
        button.clicked.connect(self.reject)
        v_layout.addWidget(button)
        h_layout.addLayout(v_layout)

    # ----------------------------------------------------------------------
    # Public interface

    def set_list(self, values):
        self.list_widget.clear()
        for value in values:
            self._add_item(value)

        self._update_buttons()

    @property
    def values(self):
        return [self.list_widget.item(index).data(USER_ROLE)
                for index in range(self.list_widget.count())]

    @property
    def string_values(self):
        return ",".join(self.list_widget.item(index).text()
                        for index in range(self.list_widget.count()))

    # ----------------------------------------------------------------------

    def keyPressEvent(self, event):
        """Reimplemented method of `QDialog`"""
        if event.matches(QKeySequence.New):
            item = self.list_widget.currentItem()
            if item is None:
                event.accept()
                return
            index = self.list_widget.currentRow()
            value = item.data(USER_ROLE)
            self._insert_item(index + 1, value)
            event.accept()
            return
        elif event.matches(QKeySequence.Delete):
            self._remove_clicked()
            event.accept()
            return
        return super().keyPressEvent(event)

    # ----------------------------------------------------------------------
    # Private interface

    def _insert_item(self, index, value):
        """Insert and item at `index` with `value`"""
        item = QListWidgetItem(str(value))
        item.setData(USER_ROLE, value)
        self.list_widget.insertItem(index, item)
        self.list_widget.setCurrentItem(item)

    def _add_item(self, value):
        """Add an item to the list widget with `value`"""
        item = QListWidgetItem(str(value))
        item.setData(USER_ROLE, value)
        self.list_widget.addItem(item)

    def _retrieve_value(self, mode=None):
        item = self.list_widget.currentItem()
        value = item.data(USER_ROLE) if item is not None else None

        dialog = QInputDialog.getText
        title = "Text"
        if is_vector_floating(self.binding):
            dialog = QInputDialog.getDouble
            title = "Float"
        elif is_vector_integer(self.binding):
            dialog = QInputDialog.getInt
            title = "Integer"

        if value is None:
            value, ok = dialog(self, title, "Add")
        elif dialog == QInputDialog.getText:
            value, ok = dialog(self, title, mode, text=value)
        elif dialog == QInputDialog.getInt:
            value = int(value)
            value, ok = dialog(self, title, mode, value)
        elif dialog == QInputDialog.getDouble:
            value = float(value)
            value, ok = dialog(self, title, mode, value, decimals=3)

        if ok:
            return value

    def _retrieve_choice(self, mode):
        text = ""
        if self.list_widget.currentItem() is not None:
            text = self.list_widget.currentItem().text()

        keys = list(self._allowed_choices.keys())
        index = 0
        if text in keys:
            index = keys.index(text)

        text, ok = QInputDialog.getItem(self, "Bool", mode, keys, index, False)
        if ok:
            return self._allowed_choices[text]

    # ----------------------------------------------------------------------
    # Slots

    @Slot(QListWidgetItem)
    def itemDoubleClicked(self, item):
        """If an item is double clicked on the widget, relay editing"""
        self._edit_clicked()

    @Slot()
    def _add_clicked(self):
        if (self.maxSize is not None
                and self.list_widget.count() == self.maxSize):
            messagebox.show_error("The vector size cannot be greater than {}!"
                                  .format(self.maxSize), parent=self)
            return

        if len(self._allowed_choices) < 1:
            value = self._retrieve_value("Add")
        else:
            value = self._retrieve_choice("Add")

        if (value is None or not self.duplicates_ok and
                self.list_widget.findItems(str(value),
                                           Qt.MatchCaseSensitive)):
            return

        self._add_item(value)
        self.list_widget.setCurrentRow(self.list_widget.count() - 1)
        self._update_buttons()

    @Slot()
    def _edit_clicked(self):
        if len(self._allowed_choices) < 1:
            value = self._retrieve_value("Edit")
        else:
            value = self._retrieve_choice("Edit")

        if (value is None or not self.duplicates_ok and
                self.list_widget.findItems(str(value),
                                           Qt.MatchCaseSensitive)):
            return

        item = self.list_widget.currentItem()
        item.setData(USER_ROLE, value)
        item.setText(str(value))
        self._update_buttons()

    @Slot()
    def _remove_clicked(self):
        if (self.minSize is not None
                and self.list_widget.count() == self.minSize):
            messagebox.show_error("The vector size cannot be smaller than {}!"
                                  .format(self.minSize), parent=self)
            return
        self.list_widget.takeItem(self.list_widget.currentRow())
        self._update_buttons()

    @Slot()
    def _move_up_clicked(self):
        row = self.list_widget.currentRow()
        widget = self.list_widget
        if row > 0:
            item = widget.takeItem(row)
            widget.insertItem(row - 1, item)
            widget.setCurrentRow(row - 1)
            self._update_buttons()

    @Slot()
    def _move_down_clicked(self):
        row = self.list_widget.currentRow()
        widget = self.list_widget
        if row < widget.count() - 1:
            item = widget.takeItem(row)
            widget.insertItem(row + 1, item)
            widget.setCurrentRow(row + 1)
            self._update_buttons()

    @Slot()
    def _update_buttons(self):
        """Enable or disable the `Edit`, `Delete` and `Move` buttons"""
        has_items = self.list_widget.count() > 0
        self.edit_button.setEnabled(has_items)
        self.remove_button.setEnabled(has_items)

        row = self.list_widget.currentRow()
        enable_up = has_items and row > 0
        self.move_up_button.setEnabled(enable_up)
        enable_down = has_items and row < self.list_widget.count() - 1
        self.move_down_button.setEnabled(enable_down)
