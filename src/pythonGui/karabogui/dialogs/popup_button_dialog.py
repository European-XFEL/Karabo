from qtpy import uic
from qtpy.QtCore import QSize, Qt, Slot
from qtpy.QtGui import QColor, QTextFormat
from qtpy.QtWidgets import QDialog, QTextEdit

from .utils import get_dialog_ui

HIGHLIGHT_COLOR = QColor(Qt.yellow).lighter(180)
MIN_SIZE = 0
MAX_SIZE = 16777215


class PopupButtonDialog(QDialog):
    """The dialog provides a text editor. The size of the editor is stored
    in the model."""
    def __init__(self, model, parent=None):
        super().__init__(parent=parent)
        uic.loadUi(get_dialog_ui("popup_button_dialog.ui"), self)
        self.model = model.clone_traits()
        self.update_plain_widget()

    def update_plain_widget(self):
        model = self.model
        self.leText.setPlainText(model.text)
        self.leText.setFixedSize(
            QSize(int(model.popup_width), int(model.popup_height)))

    def resizeEvent(self, event):
        """Store the size of the QPlainTextEdit in the model when dialog is
        resized. The popup widget will have the same size"""
        super().resizeEvent(event)
        if self.model.popup_width != self.leText.width():
            self.model.popup_width = self.leText.width()
        if self.model.popup_height != self.leText.height():
            self.model.popup_height = self.leText.height()

    def _make_text_edit_resizable(self):
        """Allow the QPlainTextEdit resizable"""
        min_size = QSize(MIN_SIZE, MIN_SIZE)
        max_size = QSize(MAX_SIZE, MAX_SIZE)
        self.leText.setMinimumSize(min_size)
        self.leText.setMaximumSize(max_size)

    def showEvent(self, event):
        # Before showing the dialog remove the fixed size constraint from
        # the leText QPlainTextEdit.
        self._make_text_edit_resizable()
        super().showEvent(event)

    # ----------------------------------------------------------------------
    # Qt Slots

    @Slot()
    def on_leText_cursorPositionChanged(self):
        extra_selections = []
        selection = QTextEdit.ExtraSelection()
        selection.format.setBackground(HIGHLIGHT_COLOR)
        selection.format.setProperty(QTextFormat.FullWidthSelection, True)
        selection.cursor = self.leText.textCursor()
        selection.cursor.clearSelection()
        extra_selections.append(selection)
        self.leText.setExtraSelections(extra_selections)

    @Slot()
    def on_leText_textChanged(self):
        self.model.text = self.leText.toPlainText()
