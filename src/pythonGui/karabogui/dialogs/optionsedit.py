from PyQt5 import uic
from PyQt5.QtCore import pyqtSlot, QCoreApplication, Qt
from PyQt5.QtGui import QFontMetrics
from PyQt5.QtWidgets import QDialog, QInputDialog, QListWidgetItem

from .utils import get_dialog_ui


class OptionsEditDialog(QDialog):
    def __init__(self, options, parent=None):
        super(OptionsEditDialog, self).__init__(parent)

        file_path = get_dialog_ui('listedit.ui')
        uic.loadUi(file_path, self)

        self._duplicates_ok = False

        self._add_caption = 'Add String'
        self._add_label = 'String:'
        self._edit_caption = 'Edit String'
        self._edit_label = self._add_label

        self.list_widget.currentItemChanged.connect(self._on_update_buttons)

        self.ok_button.clicked.connect(self.accept)
        self.cancel_button.clicked.connect(self.reject)

        self.set_list(options)

    # ----------------------------------------------------------------------
    # Public interface

    def set_list(self, values):
        self.list_widget.clear()

        fm = QFontMetrics(self.list_widget.font())
        width = 0
        for index in values:
            self._add_item(index)

            w = fm.width(str(index))
            if w > width:
                width = w

        if self.list_widget.verticalScrollBar() is not None:
            width += self.list_widget.verticalScrollBar().width()

        desktop = QCoreApplication.instance().desktop()
        min_width = min(width, desktop.screenGeometry().width() * 4 / 5)
        self.list_widget.setMinimumWidth(min_width)
        self._on_update_buttons()

    def set_texts(self, add_caption, add_label, edit_caption, edit_label=''):
        self._add_caption = add_caption
        self._add_label = add_label
        self._edit_caption = edit_caption
        self._edit_label = edit_label if edit_label else add_label

    @property
    def values(self):
        return [self.list_widget.item(index).editable_value
                for index in range(self.list_widget.count())]

    # ----------------------------------------------------------------------
    # Private interface

    def _add_item(self, value):
        item = QListWidgetItem(str(value))
        item.editable_value = value
        self.list_widget.addItem(item)

    def _retrieve_any_string(self, caption, label):
        current_item = self.list_widget.currentItem()
        if current_item is None:
            current_value = None
        else:
            current_value = current_item.editable_value

        current_value, ok = QInputDialog.getText(self, caption,
                                                 label, text=current_value)

        if ok:
            return current_value
        else:
            return None

    def _swap_items(self, from_item, to_item):
        temp_text = to_item.text()
        to_item.setText(from_item.text())
        from_item.setText(temp_text)
        from_item.editable_value, to_item.editable_value = (
            to_item.editable_value, from_item.editable_value)

    def _is_invalid(self, value):
        in_list = self.list_widget.findItems(str(value), Qt.MatchCaseSensitive)
        return value in [None, ""] or not self._duplicates_ok and in_list

    # ----------------------------------------------------------------------
    # Slots

    @pyqtSlot()
    def on_add_button_clicked(self):
        value = self._retrieve_any_string(self._add_caption, self._add_label)

        if self._is_invalid(value):
            return

        self._add_item(value)
        self.list_widget.setCurrentRow(self.list_widget.count() - 1)
        self._on_update_buttons()

    @pyqtSlot()
    def on_edit_button_clicked(self):
        value = self._retrieve_any_string(self._edit_caption, self._edit_label)

        if self._is_invalid(value):
            return

        current_item = self.list_widget.currentItem()
        current_item.editable_value = value
        current_item.setText(str(value))
        self._on_update_buttons()

    @pyqtSlot()
    def on_delete_button_clicked(self):
        self.list_widget.takeItem(self.list_widget.currentRow())
        self._on_update_buttons()

    @pyqtSlot()
    def on_up_button_clicked(self):
        row = self.list_widget.currentRow()
        widget = self.list_widget
        if row > 0:
            self._swap_items(widget.item(row), widget.item(row - 1))
            widget.setCurrentRow(row - 1)
            self._on_update_buttons()

    @pyqtSlot()
    def on_down_button_clicked(self):
        row = self.list_widget.currentRow()
        widget = self.list_widget
        if row < widget.count() - 1:
            self._swap_items(widget.item(row), widget.item(row + 1))
            widget.setCurrentRow(row + 1)
            self._on_update_buttons()

    @pyqtSlot()
    def _on_update_buttons(self):
        has_items = self.list_widget.count() > 0
        self.edit_button.setEnabled(has_items)
        self.delete_button.setEnabled(has_items)
        i = self.list_widget.currentRow()
        self.up_button.setEnabled(has_items and i > 0)
        self.down_button.setEnabled(has_items and
                                    i < self.list_widget.count() - 1)
