import os

from qtpy import uic
from qtpy.QtCore import Qt, Slot
from qtpy.QtWidgets import QDialog

from karabogui.graph.common.utils import float_to_string
from karabogui.widgets.range_slider import RangeSlider


class LevelsDialog(QDialog):

    def __init__(self, levels, image_range, auto_levels, parent=None):
        super(LevelsDialog, self).__init__(parent)
        ui_path = os.path.join(os.path.abspath(os.path.dirname(__file__)),
                               'levels_dialog.ui')
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

        # Connect signals

        self.slider.sliderMoved.connect(self.sliderMoved)
        self.min_spinbox.valueChanged.connect(self.levelChanged)
        self.max_spinbox.valueChanged.connect(self.levelChanged)

        # Set slider and editor widget default values
        min_level, max_level = levels
        min_range, max_range = image_range

        slider_min = min(min_level, min_range)
        slider_max = max(max_level, max_range)
        self.slider.initialize(slider_min, slider_max)

        default = image_range if auto_levels else levels
        self._set_editor_default(default)

    def _set_editor_default(self, levels):
        """Set the default values of the editor widgets and the slider"""
        min_level, max_level = levels

        self.min_label.setText(float_to_string(min_level))
        self.max_label.setText(float_to_string(max_level))

        self.min_spinbox.setValue(min_level)
        self.max_spinbox.setValue(max_level)
        self.slider.setValue(min_level, max_level)

    @Slot(float)
    def levelChanged(self, value):
        min_position = self.min_spinbox.value()
        max_position = self.max_spinbox.value()
        self.slider.setValue(min_position, max_position)

    @Slot(int)
    def set_automatic_levels(self, state):
        self.values_widget.setEnabled(state == Qt.Unchecked)
        low, high = self._image_range
        self.min_spinbox.setValue(low)
        self.max_spinbox.setValue(high)
        self.slider.setValue(low, high)

    @Slot(int, int)
    def sliderMoved(self, low, high):
        self.min_spinbox.setValue(low)
        self.max_spinbox.setValue(high)

    @property
    def levels(self):
        if self.automatic_checkbox.isChecked():
            return None

        min_level = min([self.min_spinbox.value(), self.max_spinbox.value()])
        max_level = max([self.min_spinbox.value(), self.max_spinbox.value()])
        levels = [min_level, max_level]
        return levels
