#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on April 28, 2020
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
from qtpy import uic
from qtpy.QtCore import QSize, Qt, Slot
from qtpy.QtGui import QColor, QFont, QIcon, QPixmap, QTextFormat
from qtpy.QtWidgets import QApplication, QColorDialog, QDialog, QTextEdit

from karabogui.dialogs.font_dialog import FontDialog
from karabogui.fonts import get_alias_from_font, get_qfont, substitute_font

from .utils import get_dialog_ui

HIGHLIGHT_COLOR = QColor(Qt.yellow).lighter(180)
GREY = "#d3d3d3"
BUTTON_FONT_SIZE = 10


class StickerDialog(QDialog):

    def __init__(self, model=None, parent=None):
        super().__init__(parent)
        uic.loadUi(get_dialog_ui('stickerdialog.ui'), self)

        self.model = model.clone_traits()
        substitute_font(self.model)

        # Set default font if the model font is empty
        if self.model.font:
            self.text_font = get_qfont(self.model.font, adjust_size=False)
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
        self.leText.setFixedSize(QSize(int(model.width), int(model.height)))
        self.leText.setPlainText(model.text)
        sheet = []
        font_string = get_qfont(model.font, adjust_size=True).toString()
        sheet.append(f'qproperty-font: "{font_string}";')
        sheet.append(f'color: "{model.foreground}";')
        sheet.append(f'background-color: "{model.background}";')

        # Add borders
        bg = model.background
        if bg == 'transparent':
            bg = GREY
        color = QColor(bg).darker(120).name()
        sheet.append('border: 1px solid;')
        sheet.append('border-top: 5px solid;')
        sheet.append(f'border-color: {color};')

        self.leText.setStyleSheet("QPlainTextEdit {{ {} }}".format("".join(
            sheet)))

    def set_text_font_button(self):
        """
        Update the  text and font family of the button. The font size
        should not be changed in order to keep the button size fixed.
        """
        font = QFont(self.text_font)
        font.setPointSize(BUTTON_FONT_SIZE)
        self.pbFont.setFont(font)
        family = get_alias_from_font(self.text_font.family())
        button_text = f"{family}, {self.text_font.pointSize()}pt"
        self.pbFont.setText(button_text)

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

    @Slot()
    def on_pbFont_clicked(self):
        dialog = FontDialog(self.text_font, parent=self)
        if dialog.exec() == QDialog.Accepted:
            self.text_font = dialog.qfont
            self.model.font = self.text_font.toString()
            self.set_text_font_button()
            self.update_plain_widget()

    @Slot()
    def on_pbTextColor_clicked(self):
        color = QColorDialog.getColor(QColor(self.model.foreground))
        if color.isValid():
            self.model.foreground = color.name()
            self.set_text_color_button()
            self.update_plain_widget()

    @Slot(bool)
    def on_cbBackground_toggled(self, checked):
        self.pbBackground.setEnabled(checked)
        if not checked:
            self.model.background = 'transparent'
            self.set_text_background_button()
        self.update_plain_widget()

    @Slot()
    def on_pbBackground_clicked(self):
        color = QColorDialog.getColor(QColor(self.model.background))
        if color.isValid():
            self.model.background = color.name()
            self.set_text_background_button()
        self.update_plain_widget()
