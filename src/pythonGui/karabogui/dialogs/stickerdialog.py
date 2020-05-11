#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on April 28, 2020
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import os.path as op

from PyQt5 import uic
from PyQt5.QtCore import pyqtSlot, QSize, Qt
from PyQt5.QtGui import QColor, QFont, QIcon, QPixmap, QTextFormat
from PyQt5.QtWidgets import (
    QApplication, QColorDialog, QDialog, QFontDialog, QTextEdit)

HIGHLIGHT_COLOR = QColor(Qt.yellow).lighter(180)
GREY = "#d3d3d3"


class StickerDialog(QDialog):

    def __init__(self, model=None, parent=None):
        super(StickerDialog, self).__init__(parent)
        uic.loadUi(op.join(op.dirname(__file__), 'stickerdialog.ui'), self)

        self.model = model.clone_traits()

        # Set default font if the model font is empty
        if self.model.font:
            self.text_font = QFont()
            self.text_font.fromString(self.model.font)
        else:
            self.text_font = QApplication.font()
            self.model.font = self.text_font.toString()

        self.set_text_font_button()
        self.set_text_color_button()
        self.set_text_background_button()
        self.cbBackground.setChecked(self.model.background != 'transparent')

        self.update_plain_widget()

    def update_plain_widget(self):
        model = self.model
        self.leText.setFixedSize(QSize(model.width, model.height))
        self.leText.setPlainText(model.text)
        sheet = []
        sheet.append('qproperty-font: "{}";'.format(model.font))
        sheet.append('color: "{}";'.format(model.foreground))
        sheet.append('background-color: "{}";'.format(model.background))

        # Add borders
        bg = model.background
        if bg == 'transparent':
            bg = GREY
        color = QColor(bg).darker(120).name()
        sheet.append('border: 1px solid;')
        sheet.append('border-top: 5px solid;')
        sheet.append('border-color: {};'.format(color))

        self.leText.setStyleSheet("QPlainTextEdit {{ {} }}".format("".join(
            sheet)))

    def set_text_font_button(self):
        self.pbFont.setFont(self.text_font)
        self.pbFont.setText(self.text_font.family())

    def set_text_color_button(self):
        pixmap = QPixmap(24, 16)
        pixmap.fill(QColor(self.model.foreground))
        self.pbTextColor.setIcon(QIcon(pixmap))

    def set_text_background_button(self):
        pixmap = QPixmap(24, 16)
        pixmap.fill(QColor(self.model.background))
        self.pbBackground.setIcon(QIcon(pixmap))

    # ----------------------------------------------------------------------
    # Qt Slots

    @pyqtSlot()
    def on_leText_cursorPositionChanged(self):
        extra_selections = []
        selection = QTextEdit.ExtraSelection()
        selection.format.setBackground(HIGHLIGHT_COLOR)
        selection.format.setProperty(QTextFormat.FullWidthSelection, True)
        selection.cursor = self.leText.textCursor()
        selection.cursor.clearSelection()
        extra_selections.append(selection)
        self.leText.setExtraSelections(extra_selections)

    @pyqtSlot()
    def on_leText_textChanged(self):
        self.model.text = self.leText.toPlainText()

    @pyqtSlot()
    def on_pbFont_clicked(self):
        self.text_font, ok = QFontDialog.getFont(self.text_font, self)
        if ok:
            self.model.font = self.text_font.toString()
            self.set_text_font_button()
            self.update_plain_widget()

    @pyqtSlot()
    def on_pbTextColor_clicked(self):
        color = QColorDialog.getColor(QColor(self.model.foreground))
        if color.isValid():
            self.model.foreground = color.name()
            self.set_text_color_button()
            self.update_plain_widget()

    @pyqtSlot(bool)
    def on_cbBackground_toggled(self, checked):
        self.pbBackground.setEnabled(checked)
        if not checked:
            self.model.background = 'transparent'
            self.set_text_background_button()
        self.update_plain_widget()

    @pyqtSlot()
    def on_pbBackground_clicked(self):
        color = QColorDialog.getColor(QColor(self.model.background))
        if color.isValid():
            self.model.background = color.name()
            self.set_text_background_button()
        self.update_plain_widget()
