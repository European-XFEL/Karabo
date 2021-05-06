from qtpy import uic
from qtpy.QtCore import Slot
from qtpy.QtGui import QFont, QFontDatabase
from qtpy.QtWidgets import QDialog

from karabo.common.scenemodel.const import SCENE_FONT_SIZES
from karabogui.fonts import (
    FONT_ALIAS, get_alias_from_font, get_font_from_alias,
    get_font_size_from_dpi)

from .utils import get_dialog_ui

DEFAULT_STYLE = "Regular"


class FontDialog(QDialog):

    def __init__(self, qfont, parent=None):
        super(FontDialog, self).__init__(parent=parent)
        uic.loadUi(get_dialog_ui("font_dialog.ui"), self)

        # Instantiate variables
        self.qfont = QFont(qfont)
        self._font_database = QFontDatabase()

        # Populate font family
        self.font_combobox.addItems(FONT_ALIAS)
        self.font_combobox.setCurrentText(get_alias_from_font(qfont.family()))

        # Populate font size combobox and set current index/text
        sizes_string = [str(size) for size in SCENE_FONT_SIZES]
        font_size_cb = self.font_size_combobox
        font_size_cb.addItems(sizes_string)
        font_size = qfont.pointSize()
        font_size_cb.setCurrentText(str(font_size))

        # Update effects
        self.bold_checkbox.setChecked(qfont.bold())
        self.italic_checkbox.setChecked(qfont.italic())
        self.strikeout_checkbox.setChecked(qfont.strikeOut())
        self.underline_checkbox.setChecked(qfont.underline())

        # Add triggers to change preview
        self.font_combobox.currentTextChanged.connect(self._update_font)
        font_size_cb.currentTextChanged.connect(self._update_font_size)
        self.bold_checkbox.toggled.connect(self._update_bold)
        self.italic_checkbox.toggled.connect(self._update_italic)
        self.strikeout_checkbox.toggled.connect(self._update_strikeout)
        self.underline_checkbox.toggled.connect(self._update_underline)

        # Update preview to reflect current format
        self._preview()

    @Slot()
    def _preview(self):
        qfont = QFont(self.qfont)
        qfont.setPointSize(get_font_size_from_dpi(qfont.pointSize()))
        self.preview_label.setFont(qfont)

    @Slot(str)
    def _update_font(self, family):
        self.qfont.setFamily(get_font_from_alias(family))
        self._preview()

    @Slot(str)
    def _update_font_size(self, size):
        self.qfont.setPointSize(int(size))
        self._preview()

    @Slot(str)
    def _update_font_style(self, style):
        self.qfont.setStyleName(style)
        self._preview()

    @Slot(bool)
    def _update_bold(self, enabled):
        self.qfont.setBold(enabled)
        self._preview()

    @Slot(bool)
    def _update_italic(self, enabled):
        self.qfont.setItalic(enabled)
        self._preview()

    @Slot(bool)
    def _update_strikeout(self, enabled):
        self.qfont.setStrikeOut(enabled)
        self._preview()

    @Slot(bool)
    def _update_underline(self, enabled):
        self.qfont.setUnderline(enabled)
        self._preview()
