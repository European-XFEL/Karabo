from qtpy.QtCore import Qt, Slot
from qtpy.QtWidgets import QDialog, QHBoxLayout, QToolButton, QWidget

from karabogui import icons
from karabogui.dialogs.api import ListEditDialog
from karabogui.widgets.edits import LineEditEditor

from .utils import string2list


class TableVectorEdit(QWidget):

    def __init__(self, binding, parent=None):
        super().__init__(parent)
        layout = QHBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)

        self.lineedit = LineEditEditor(parent)
        layout.addWidget(self.lineedit)
        self.setFocusProxy(self.lineedit)
        self.lineedit.setFocusPolicy(Qt.StrongFocus)
        self.list_edit = ListEditDialog(
            binding, duplicates_ok=True, parent=self)

        edit_button = QToolButton()
        edit_button.setText("Edit")
        edit_button.setIcon(icons.edit)
        edit_button.setMaximumSize(25, 25)
        edit_button.setFocusPolicy(Qt.NoFocus)
        edit_button.clicked.connect(self.on_open_dialog)
        layout.addWidget(edit_button)

    def setValidator(self, validator):
        self.lineedit.setValidator(validator)

    def setText(self, value):
        self.lineedit.setText(value)
        self.list_edit.set_list(string2list(value))

    def text(self):
        return self.lineedit.text()

    @Slot()
    def on_open_dialog(self):
        if self.list_edit.exec() == QDialog.Accepted:
            self.lineedit.setText(self.list_edit.string_values)
        self.lineedit.setFocus(Qt.PopupFocusReason)
