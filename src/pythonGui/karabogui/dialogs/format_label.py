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
from qtpy.QtCore import Slot
from qtpy.QtWidgets import QDialog

from karabo.common.scenemodel.const import SCENE_FONT_SIZES, SCENE_FONT_WEIGHTS

from .utils import get_dialog_ui


class FormatLabelDialog(QDialog):

    def __init__(self, font_size=10, font_weight="normal", parent=None):
        super().__init__(parent=parent)
        uic.loadUi(get_dialog_ui("format_label.ui"), self)

        # Populate font size combobox and set current index
        sizes_string = [str(size) for size in SCENE_FONT_SIZES]
        self.font_size_combobox.addItems(sizes_string)
        index = SCENE_FONT_SIZES.index(font_size)
        self.font_size_combobox.setCurrentIndex(index)

        # Populate font weight combobox and set current index
        self.font_weight_combobox.addItems(SCENE_FONT_WEIGHTS)
        index = SCENE_FONT_WEIGHTS.index(font_weight)
        self.font_weight_combobox.setCurrentIndex(index)

        # Add triggers to change preview
        self.font_size_combobox.currentIndexChanged.connect(self._preview)
        self.font_weight_combobox.currentIndexChanged.connect(self._preview)

        # Update preview to reflect current format
        self._preview()

    @Slot()
    def _preview(self):
        font = self.preview_label.font()
        font.setPointSize(self.font_size)
        font.setBold(self.font_weight == "bold")
        self.preview_label.setFont(font)

    @property
    def font_size(self):
        return int(self.font_size_combobox.currentText())

    @property
    def font_weight(self):
        return self.font_weight_combobox.currentText()
