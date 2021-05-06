from qtpy import uic
from qtpy.QtCore import Qt, Slot
from qtpy.QtGui import QColor, QFont, QIcon, QPixmap
from qtpy.QtWidgets import QColorDialog, QDialog

from karabo.common.scenemodel.api import LabelModel
from karabogui.dialogs.font_dialog import FontDialog
from karabogui.fonts import (
    get_alias_from_font, get_font_size_from_dpi, get_qfont)

from .utils import get_dialog_ui


class TextDialog(QDialog):

    def __init__(self, label_model=None, parent=None):
        super(TextDialog, self).__init__(parent)
        uic.loadUi(get_dialog_ui('textdialog.ui'), self)

        if label_model is None:
            # NOTE: Fonts similar on all OS are Arial, Helvetica, sans-serif!
            self.label_model = LabelModel()
        else:
            self.label_model = label_model.clone_traits()
        self.text_font = get_qfont(self.label_model.font, adjust_size=False)
        # Save font string after corrections
        self.label_model.font = self.text_font.toString()
        self.leText.setText(self.label_model.text)

        self.set_text_font_button()
        self.set_text_color_button()
        self.set_text_background_button()

        if self.label_model.frame_width > 0:
            self.cbFrameWidth.setChecked(True)
            self.sbFrameWidth.setValue(self.label_model.frame_width)

        self.cbBackground.setChecked(
            self.label_model.background != 'transparent')

    def set_text_font_button(self):
        qfont = QFont(self.text_font)
        qfont.setPointSize(get_font_size_from_dpi(qfont.pointSize()))
        self.pbFont.setFont(qfont)
        self.pbFont.setText(get_alias_from_font((self.text_font.family())))

    def set_text_color_button(self):
        pixmap = QPixmap(24, 16)
        pixmap.fill(QColor(self.label_model.foreground))
        self.pbTextColor.setIcon(QIcon(pixmap))

    def set_text_background_button(self):
        pixmap = QPixmap(24, 16)
        pixmap.fill(QColor(self.label_model.background))
        self.pbBackground.setIcon(QIcon(pixmap))

    @Slot(int)
    def on_cbBackground_stateChanged(self, state):
        if state != Qt.Checked:
            self.label_model.background = 'transparent'
            self.set_text_background_button()

    @Slot(str)
    def on_leText_textChanged(self, text):
        self.label_model.text = text

    @Slot()
    def on_pbFont_clicked(self):
        dialog = FontDialog(self.text_font, parent=self)
        if dialog.exec_() == QDialog.Accepted:
            self.text_font = dialog.qfont
            self.label_model.font = self.text_font.toString()
            self.set_text_font_button()

    @Slot()
    def on_pbTextColor_clicked(self):
        color = QColorDialog.getColor(QColor(self.label_model.foreground))
        if color.isValid():
            self.label_model.foreground = color.name()
            self.set_text_color_button()

    @Slot(bool)
    def on_cbFrameWidth_toggled(self, checked):
        self.sbFrameWidth.setEnabled(checked)
        if not checked:
            self.label_model.frame_width = 0

    @Slot(int)
    def on_sbFrameWidth_valueChanged(self, value):
        self.label_model.frame_width = value

    @Slot(bool)
    def on_cbBackground_toggled(self, checked):
        self.pbBackground.setEnabled(checked)
        if not checked:
            self.label_model.background = ''

    @Slot()
    def on_pbBackground_clicked(self):
        color = QColorDialog.getColor(QColor(self.label_model.background))
        if color.isValid():
            self.label_model.background = color.name()
            self.set_text_background_button()
