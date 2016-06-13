import os.path as op

from PyQt4 import uic
from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import (QColor, QColorDialog, QDialog, QFont, QFontDialog,
                         QIcon, QPixmap)

from karabo_gui.scenemodel.simple_widgets import LabelModel


class TextDialog(QDialog):

    def __init__(self, label_model=None):
        super(TextDialog, self).__init__()
        uic.loadUi(op.join(op.dirname(__file__), 'textdialog.ui'), self)

        if label_model is None:
            self.label_model = LabelModel()
        else:
            self.label_model = label_model

        self.leText.setText(self.label_model.text)
        self.label_model.font = QFont(self.label_model.font).toString()

        self.set_colors()

        if self.label_model.frame_width > 0:
            self.cbFrameWidth.setChecked(True)
            self.sbFrameWidth.setValue(self.label_model.frame_width)

        self.cbBackground.setChecked(
            True if self.label_model.background else False)

    def set_colors(self):
        pixmap = QPixmap(24, 16)
        pixmap.fill(QColor(self.label_model.foreground))
        self.pbTextColor.setIcon(QIcon(pixmap))
        pixmap.fill(QColor(self.label_model.background))
        self.pbBackground.setIcon(QIcon(pixmap))

    @pyqtSlot(str)
    def on_leText_textChanged(self, text):
        self.label_model.text = text

    @pyqtSlot()
    def on_pbFont_clicked(self):
        font, ok = QFontDialog.getFont(QFont(self.label_model.font), self)
        if ok:
            self.label_model.font = font.toString()

    @pyqtSlot()
    def on_pbTextColor_clicked(self):
        color = QColorDialog.getColor(QColor(self.label_model.foreground))
        self.label_model.foreground = color.name()
        self.set_colors()

    @pyqtSlot(bool)
    def on_cbFrameWidth_toggled(self, checked):
        self.sbFrameWidth.setEnabled(checked)

    @pyqtSlot(int)
    def on_sbFrameWidth_valueChanged(self, value):
        self.label_model.frame_width = value

    @pyqtSlot(bool)
    def on_cbBackground_toggled(self, checked):
        self.pbBackground.setEnabled(checked)

    @pyqtSlot()
    def on_pbBackground_clicked(self):
        color = QColorDialog.getColor(QColor(self.label_model.background))
        self.label_model.background = color.name()
        self.set_colors()
