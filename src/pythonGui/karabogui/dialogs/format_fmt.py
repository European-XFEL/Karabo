# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from qtpy import uic
from qtpy.QtCore import Slot
from qtpy.QtWidgets import QDialog

from .utils import get_dialog_ui

DEFAULT_FORMAT = "f"
DEFAULT_FRACTIONS = "3"


class FormatFmtDialog(QDialog):

    def __init__(self, value=None, fmt=DEFAULT_FORMAT,
                 decimals=DEFAULT_FRACTIONS, parent=None):
        super().__init__(parent=parent)
        uic.loadUi(get_dialog_ui("format_fmt.ui"), self)
        self.value = value
        index = self.combo_fmt.findText(fmt)
        self.combo_fmt.setCurrentIndex(index)

        index = self.combo_decimals.findText(decimals)
        self.combo_decimals.setCurrentIndex(index)

        # Add triggers to change preview
        self.combo_fmt.currentIndexChanged.connect(self._preview)
        self.combo_decimals.currentIndexChanged.connect(self._preview)
        # Update preview to reflect current format
        self._preview()

    @Slot()
    def _preview(self):
        """Preview the value value formatting"""
        combo_fmt = self.combo_fmt.currentText()
        combo_decimals = self.combo_decimals.currentText()

        preview = "{{:.{}{}}}".format(combo_decimals, combo_fmt)
        if self.value is not None:
            try:
                preview = preview.format(self.value)
            except Exception:
                preview = "Not a valid formatting .."
        self.preview_label.setText(preview)

    @property
    def fmt(self):
        return self.combo_fmt.currentText()

    @property
    def decimals(self):
        return self.combo_decimals.currentText()
