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
from qtpy import uic
from qtpy.QtCore import Qt, Slot
from qtpy.QtGui import QColor, QFont, QIcon, QPixmap
from qtpy.QtWidgets import QColorDialog, QDialog

from karabo.common.scenemodel.api import LabelModel
from karabogui.dialogs.font_dialog import FontDialog
from karabogui.fonts import get_alias_from_font, get_qfont
from karabogui.util import SignalBlocker

from .utils import get_dialog_ui

BUTTON_SIZE = 10


class TextDialog(QDialog):

    def __init__(self, label_model=None, alignment=False, parent=None):
        super().__init__(parent)
        uic.loadUi(get_dialog_ui('textdialog.ui'), self)
        self.has_alignment = alignment
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
        self.set_alignment_combo()
        if self.label_model.frame_width > 0:
            self.cbFrameWidth.setChecked(True)
            self.sbFrameWidth.setValue(self.label_model.frame_width)

        self.cbBackground.setChecked(
            self.label_model.background != 'transparent')

    def set_alignment_combo(self):
        """Set the combobox according to the alignment"""
        if self.has_alignment:
            alignment = Qt.AlignmentFlag(self.label_model.alignh)
            if alignment == Qt.AlignLeft:
                self.cbAlignment.setCurrentIndex(0)
            elif alignment == Qt.AlignRight:
                self.cbAlignment.setCurrentIndex(1)
            elif alignment == Qt.AlignHCenter:
                self.cbAlignment.setCurrentIndex(2)
        else:
            # There is no alignment, we are centered
            with SignalBlocker(self.cbAlignment):
                self.cbAlignment.setCurrentIndex(2)
                self.cbAlignment.setEnabled(False)

    def set_text_font_button(self):
        """
        Update the  text and font family of the button. The font size
        should not be changed in order to keep the button size fixed.
        """
        qfont = QFont(self.text_font)
        qfont.setPointSize(BUTTON_SIZE)
        self.pbFont.setFont(qfont)
        family = get_alias_from_font(self.text_font.family())
        button_text = f"{family}, {self.text_font.pointSize()}pt"
        self.pbFont.setText(button_text)

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

    @Slot(int)
    def on_cbAlignment_currentIndexChanged(self, index):
        if index == 0:
            alignment = Qt.AlignLeft
        elif index == 1:
            alignment = Qt.AlignRight
        elif index == 2:
            alignment = Qt.AlignHCenter

        # Store the int cast of the enum flag
        self.label_model.alignh = int(alignment)

    @Slot(str)
    def on_leText_textChanged(self, text):
        self.label_model.text = text

    @Slot()
    def on_pbFont_clicked(self):
        dialog = FontDialog(self.text_font, parent=self)
        if dialog.exec() == QDialog.Accepted:
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
