#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on March 12, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from collections import OrderedDict

import numpy as np
from qtpy.QtCore import Qt, Slot
from qtpy.QtGui import QFontMetrics
from qtpy.QtWidgets import (
    QApplication, QDialog, QHBoxLayout, QInputDialog, QListWidget,
    QListWidgetItem, QPushButton, QVBoxLayout)

from karabo.common.api import KARABO_SCHEMA_MAX_SIZE, KARABO_SCHEMA_MIN_SIZE
from karabogui import messagebox
from karabogui.binding.api import (
    VectorBoolBinding, VectorDoubleBinding, VectorFloatBinding,
    VectorInt8Binding, VectorInt16Binding, VectorInt32Binding,
    VectorInt64Binding, VectorUint8Binding, VectorUint16Binding,
    VectorUint32Binding, VectorUint64Binding, get_editor_value)

FLOAT_BINDINGS = (VectorDoubleBinding, VectorFloatBinding)
INT_BINDINGS = (VectorInt8Binding, VectorInt16Binding, VectorInt32Binding,
                VectorInt64Binding, VectorUint8Binding, VectorUint16Binding,
                VectorUint32Binding, VectorUint64Binding)


class ListEditDialog(QDialog):
    def __init__(self, proxy, duplicates_ok=True, parent=None):
        super(ListEditDialog, self).__init__(parent)

        self._proxy = proxy
        self._duplicates_ok = duplicates_ok

        self._allowed_choices = OrderedDict()
        if isinstance(self._proxy.binding, VectorBoolBinding):
            self._allowed_choices['0'] = 0
            self._allowed_choices['1'] = 1

        # Check for possible size limitations!
        attrs = proxy.binding.attributes
        self._minSize = attrs.get(KARABO_SCHEMA_MIN_SIZE, None)
        self._maxSize = attrs.get(KARABO_SCHEMA_MAX_SIZE, None)

        self.setWindowTitle('Edit list')

        self._add_caption = 'Add String'
        self._add_label = 'String:'
        self._edit_caption = 'Edit String'
        self._edit_label = self._add_label

        hbox = QHBoxLayout(self)
        self._list_widget = QListWidget(self)
        lw = self._list_widget
        slot = lw.currentItemChanged[QListWidgetItem, QListWidgetItem]
        slot.connect(self._on_update_buttons)
        hbox.addWidget(self._list_widget)

        vbox = QVBoxLayout()
        button = QPushButton('&Add...', self)
        button.clicked.connect(self._on_add_clicked)
        vbox.addWidget(button)

        self._edit_button = QPushButton('&Edit...', self)
        self._edit_button.clicked.connect(self._on_edit_clicked)
        vbox.addWidget(self._edit_button)

        self._remove_button = QPushButton('&Delete', self)
        self._remove_button.clicked.connect(self._on_remove_clicked)
        vbox.addWidget(self._remove_button)

        self._up_button = QPushButton('&Up', self)
        self._up_button.clicked.connect(self._on_moveup_clicked)
        vbox.addWidget(self._up_button)

        self._down_button = QPushButton('&Down', self)
        self._down_button.clicked.connect(self._on_movedown_clicked)
        vbox.addWidget(self._down_button)
        vbox.addStretch(1)

        button = QPushButton('OK', self)
        button.clicked.connect(self.accept)
        vbox.addWidget(button)

        button = QPushButton('Cancel', self)
        button.clicked.connect(self.reject)
        vbox.addWidget(button)
        hbox.addLayout(vbox)

        values = get_editor_value(proxy, [])
        self.set_list(values)

    # ----------------------------------------------------------------------
    # Public interface

    def set_list(self, values):
        self._list_widget.clear()

        fm = QFontMetrics(self._list_widget.font())
        width = 0
        for index in values:
            # Insert item
            if isinstance(index, np.bool_):
                index = int(index)
            self._add_item(index)

            w = fm.width(str(index))
            if w > width:
                width = w

        if self._list_widget.verticalScrollBar() is not None:
            width += self._list_widget.verticalScrollBar().width()

        desktop = QApplication.desktop()
        min_width = min(width, desktop.screenGeometry().width() * 4 / 5)
        self._list_widget.setMinimumWidth(min_width)
        self._on_update_buttons()

    def set_texts(self, add_caption, add_label, edit_caption, edit_label=''):
        self._add_caption = add_caption
        self._add_label = add_label
        self._edit_caption = edit_caption
        self._edit_label = edit_label if edit_label else add_label

    @property
    def values(self):
        return [self._list_widget.item(index).editable_value
                for index in range(self._list_widget.count())]

    # ----------------------------------------------------------------------
    # Private interface

    def _add_item(self, value):
        item = QListWidgetItem(str(value))
        item.editable_value = value
        self._list_widget.addItem(item)

    def _retrieve_any_string(self, caption, label):
        currentItem = self._list_widget.currentItem()
        if currentItem is None:
            currentValue = None
        else:
            currentValue = currentItem.editable_value

        dialog = QInputDialog.getText
        if isinstance(self._proxy.binding, FLOAT_BINDINGS):
            dialog = QInputDialog.getDouble
        elif isinstance(self._proxy.binding, INT_BINDINGS):
            dialog = QInputDialog.getInt

        if currentValue is None:
            currentValue, ok = dialog(self, caption, label)
        elif dialog == QInputDialog.getText:
            currentValue, ok = dialog(self, caption, label, text=currentValue)
        elif dialog == QInputDialog.getInt:
            value = int(currentValue)
            currentValue, ok = dialog(self, caption, label, value)
        elif dialog == QInputDialog.getDouble:
            value = float(currentValue)
            currentValue, ok = dialog(self, caption, label, value)

        if ok:
            return currentValue
        else:
            return None

    def _retrieve_choice(self, title, label):
        ok = False
        currentText = ''
        if self._list_widget.currentItem() is not None:
            currentText = self._list_widget.currentItem().text()

        keys = list(self._allowed_choices.keys())
        if currentText in keys:
            index = keys.index(currentText)
        else:
            index = 0
        text, ok = QInputDialog.getItem(self, title, label, keys, index, False)

        if ok:
            return self._allowed_choices[text]

    # ----------------------------------------------------------------------
    # Slots

    @Slot()
    def _on_add_clicked(self):
        if (self._maxSize is not None
                and self._list_widget.count() == self._maxSize):
            messagebox.show_error("The vector size cannot be greater than {}!"
                                  .format(self._maxSize), parent=self)
            return

        if len(self._allowed_choices) < 1:
            value = self._retrieve_any_string(
                self._add_caption, self._add_label)
        else:
            value = self._retrieve_choice(self._add_caption, self._add_label)

        if (value is None or not self._duplicates_ok and
                self._list_widget.findItems(str(value),
                                            Qt.MatchCaseSensitive)):
            return

        self._add_item(value)
        self._list_widget.setCurrentRow(self._list_widget.count() - 1)
        self._on_update_buttons()

    @Slot()
    def _on_edit_clicked(self):
        if len(self._allowed_choices) < 1:
            value = self._retrieve_any_string(
                self._edit_caption, self._edit_label)
        else:
            value = self._retrieve_choice(self._edit_caption, self._edit_label)

        if (value is None or not self._duplicates_ok and
                self._list_widget.findItems(str(value),
                                            Qt.MatchCaseSensitive)):
            return

        currentItem = self._list_widget.currentItem()
        currentItem.editable_value = value
        currentItem.setText(str(value))
        self._on_update_buttons()

    @Slot()
    def _on_remove_clicked(self):
        if (self._minSize is not None
                and self._list_widget.count() == self._minSize):
            messagebox.show_error("The vector size cannot be smaller than {}!"
                                  .format(self._minSize), parent=self)
            return
        self._list_widget.takeItem(self._list_widget.currentRow())
        self._on_update_buttons()

    @Slot()
    def _on_moveup_clicked(self):
        row = self._list_widget.currentRow()
        widget = self._list_widget
        if row > 0:
            from_item, to_item = widget.item(row), widget.item(row - 1)
            tmp_text = to_item.text()
            to_item.setText(from_item.text())
            from_item.setText(tmp_text)

            from_item.editable_value, to_item.editable_value = (
                to_item.editable_value, from_item.editable_value
            )
            widget.setCurrentRow(row - 1)
            self._on_update_buttons()

    @Slot()
    def _on_movedown_clicked(self):
        row = self._list_widget.currentRow()
        widget = self._list_widget
        if row < widget.count() - 1:
            from_item, to_item = widget.item(row), widget.item(row + 1)
            tmp_text = to_item.text()
            to_item.setText(from_item.text())
            from_item.setText(tmp_text)
            from_item.editable_value, to_item.editable_value = (
                to_item.editable_value, from_item.editable_value
            )
            widget.setCurrentRow(row + 1)
            self._on_update_buttons()

    @Slot()
    def _on_update_buttons(self):
        hasItems = self._list_widget.count() > 0
        self._edit_button.setEnabled(hasItems)
        self._remove_button.setEnabled(hasItems)
        i = self._list_widget.currentRow()
        self._up_button.setEnabled(hasItems and i > 0)
        self._down_button.setEnabled(hasItems and
                                     i < self._list_widget.count() - 1)
