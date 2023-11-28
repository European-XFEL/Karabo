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
from pathlib import Path

import numpy as np
from qtpy import uic
from qtpy.QtCore import Qt, Signal, Slot
from qtpy.QtWidgets import QDialog

from karabogui.graph.common.utils import float_to_string
from karabogui.util import SignalBlocker
from karabogui.widgets.range_slider import RangeSlider


def get_decimals(array, offset=2, default=1):
    """Calculates the number of decimals to display with the input number
    e.g., 0.01 -> 0.0100
          10.0 -> 10.0
    """
    array = np.abs(array)
    if not np.all((array > 0) & (array < 1)):
        return default

    decimals = max(-1 * np.floor(np.log10(array)))
    return int(decimals + offset)


class LevelsDialog(QDialog):
    levelsPreview = Signal(object)

    def __init__(self, levels, image_range, auto_levels, limits=None,
                 parent=None):
        super().__init__(parent)
        ui_path = Path(__file__).parent.joinpath("levels_dialog.ui")
        uic.loadUi(ui_path, self)
        self.setWindowFlags(self.windowFlags() | Qt.WindowStaysOnTopHint)
        self.values_widget.setEnabled(not auto_levels)

        self.button_box.accepted.connect(self.accept)
        self.button_box.rejected.connect(self.reject)

        # Check if autolevel: image levels and range are almost equal.
        # This is with a tolerance of 1%.
        self.automatic_checkbox.setChecked(auto_levels)
        self.automatic_checkbox.stateChanged.connect(self.set_automatic_levels)

        self._image_range = image_range
        self.slider = RangeSlider(Qt.Horizontal, parent=self)
        self.slider_layout.addWidget(self.slider)

        # Set slider and editor widget default values
        min_level, max_level = levels
        min_range, max_range = image_range

        slider_min = min(min_level, min_range)
        slider_max = max(max_level, max_range)
        self.slider.initialize(slider_min, slider_max)

        # Handle floats
        is_float = (isinstance(slider_min, np.floating)
                    or isinstance(slider_max, np.floating))
        decimals = get_decimals([slider_min, slider_max]) if is_float else 0

        # Set decimals
        self.min_spinbox.setDecimals(decimals)
        self.max_spinbox.setDecimals(decimals)

        # Set single steps
        step = 10 ** -(decimals - 1) if decimals else 1
        self.min_spinbox.setSingleStep(step)
        self.max_spinbox.setSingleStep(step)

        if limits is not None:
            min_value, max_value = limits
            self.min_spinbox.setMinimum(min_value)
            self.max_spinbox.setMinimum(min_value)
            self.min_spinbox.setMaximum(max_value)
            self.max_spinbox.setMaximum(max_value)

        default = image_range if auto_levels else levels
        self._set_editor_default(default)

        # Connect signals
        self.slider.sliderMoved.connect(self.sliderMoved)
        self.min_spinbox.valueChanged.connect(self.levelChanged)
        self.max_spinbox.valueChanged.connect(self.levelChanged)

        # Finally select the spinbox
        if not auto_levels:
            self.min_spinbox.setFocus()
            self.min_spinbox.selectAll()

    def _set_editor_default(self, levels):
        """Set the default values of the editor widgets and the slider"""
        min_level, max_level = levels
        decimals = get_decimals(levels)

        self.min_label.setText(float_to_string(min_level, decimals))
        self.max_label.setText(float_to_string(max_level, decimals))
        self.min_spinbox.setValue(min_level)
        self.max_spinbox.setValue(max_level)
        self.slider.setValue(min_level, max_level)

    @Slot()
    def levelChanged(self):
        values = [self.min_spinbox.value(), self.max_spinbox.value()]
        min_level = min(values)
        max_level = max(values)
        with SignalBlocker(self.slider):
            self.slider.setValue(min_level, max_level)
        levels = [min_level, max_level]
        self.levelsPreview.emit(levels)

    @Slot(int, int)
    def sliderMoved(self, low, high):
        self.min_spinbox.setValue(low)
        self.max_spinbox.setValue(high)

    @Slot(int)
    def set_automatic_levels(self, state):
        auto_levels = state == Qt.Checked
        self.values_widget.setEnabled(not auto_levels)
        low, high = self._image_range
        with SignalBlocker(self.min_spinbox, self.max_spinbox, self.slider):
            self.min_spinbox.setValue(low)
            self.max_spinbox.setValue(high)
            self.slider.setValue(low, high)
            if not auto_levels:
                self.min_spinbox.setFocus()
                self.min_spinbox.selectAll()

        levels = None if auto_levels else [low, high]
        self.levelsPreview.emit(levels)

    @property
    def levels(self):
        if self.automatic_checkbox.isChecked():
            return None

        min_level = min([self.min_spinbox.value(), self.max_spinbox.value()])
        max_level = max([self.min_spinbox.value(), self.max_spinbox.value()])
        levels = [min_level, max_level]
        return levels
