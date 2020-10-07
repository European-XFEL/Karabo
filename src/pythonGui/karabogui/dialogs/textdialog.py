import os.path as op

from PyQt5 import uic
from PyQt5.QtCore import pyqtSlot, Qt
from PyQt5.QtGui import QColor, QPixmap, QIcon
from PyQt5.QtWidgets import (
    QApplication, QColorDialog, QDialog, QFontDialog)

from karabo.common.scenemodel.api import LabelModel, SCENE_FONT_SIZE
from karabogui.fonts import get_font_from_string


class TextDialog(QDialog):

    def __init__(self, label_model=None, parent=None):
        super(TextDialog, self).__init__(parent)
        uic.loadUi(op.join(op.dirname(__file__), 'textdialog.ui'), self)

        if label_model is None:
            # NOTE: Fonts similar on all OS are Arial, Helvetica, sans-serif!
            self.label_model = LabelModel()
            self.text_font = QApplication.font()
            self.text_font.setStyleName("Normal")
            self.text_font.setPointSize(SCENE_FONT_SIZE)
            self.label_model.font = self.text_font.toString()
        else:
            self.label_model = label_model.clone_traits()
            self.text_font = get_font_from_string(self.label_model.font)
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
        self.pbFont.setFont(self.text_font)
        self.pbFont.setText(self.text_font.family())

    def set_text_color_button(self):
        pixmap = QPixmap(24, 16)
        pixmap.fill(QColor(self.label_model.foreground))
        self.pbTextColor.setIcon(QIcon(pixmap))

    def set_text_background_button(self):
        pixmap = QPixmap(24, 16)
        pixmap.fill(QColor(self.label_model.background))
        self.pbBackground.setIcon(QIcon(pixmap))

    @pyqtSlot(int)
    def on_cbBackground_stateChanged(self, state):
        if state != Qt.Checked:
            self.label_model.background = 'transparent'
            self.set_text_background_button()

    @pyqtSlot(str)
    def on_leText_textChanged(self, text):
        self.label_model.text = text

    @pyqtSlot()
    def on_pbFont_clicked(self):
        self.text_font, ok = QFontDialog.getFont(self.text_font, self)
        if ok:
            self.label_model.font = self.text_font.toString()
            self.set_text_font_button()

    @pyqtSlot()
    def on_pbTextColor_clicked(self):
        color = QColorDialog.getColor(QColor(self.label_model.foreground))
        if color.isValid():
            self.label_model.foreground = color.name()
            self.set_text_color_button()

    @pyqtSlot(bool)
    def on_cbFrameWidth_toggled(self, checked):
        self.sbFrameWidth.setEnabled(checked)
        if not checked:
            self.label_model.frame_width = 0

    @pyqtSlot(int)
    def on_sbFrameWidth_valueChanged(self, value):
        self.label_model.frame_width = value

    @pyqtSlot(bool)
    def on_cbBackground_toggled(self, checked):
        self.pbBackground.setEnabled(checked)
        if not checked:
            self.label_model.background = ''

    @pyqtSlot()
    def on_pbBackground_clicked(self):
        color = QColorDialog.getColor(QColor(self.label_model.background))
        if color.isValid():
            self.label_model.background = color.name()
            self.set_text_background_button()
