import os

from qtpy import uic
from qtpy.QtCore import Qt, Slot
from qtpy.QtWidgets import QDialog

from karabogui.graph.common.utils import float_to_string


class LevelsDialog(QDialog):

    def __init__(self, levels, image_range, auto_levels, parent=None):
        super(LevelsDialog, self).__init__(parent)
        ui_path = os.path.join(os.path.abspath(os.path.dirname(__file__)),
                               'levels_dialog.ui')
        uic.loadUi(ui_path, self)
        self.setWindowFlags(self.windowFlags() | Qt.WindowStaysOnTopHint)

        # Check if autolevel: image levels and range are almost equal.
        # This is with a tolerance of 1%.
        self.automatic_checkbox.setChecked(auto_levels)
        self.automatic_checkbox.stateChanged.connect(self.set_automatic_levels)

        self.values_widget.setEnabled(not auto_levels)

        self.button_box.accepted.connect(self.accept)
        self.button_box.rejected.connect(self.reject)

        # Set widget default values
        default = image_range if auto_levels else levels
        self._image_range = image_range
        self._set_default_values(default)

    def _set_default_values(self, levels):
        min_level, max_level = levels

        self.min_label.setText(float_to_string(min_level))
        self.max_label.setText(float_to_string(max_level))

        self.min_spinbox.setValue(min_level)
        self.max_spinbox.setValue(max_level)

    @Slot(int)
    def set_automatic_levels(self, state):
        self.values_widget.setEnabled(state == Qt.Unchecked)
        self.min_spinbox.setValue(self._image_range[0])
        self.max_spinbox.setValue(self._image_range[1])

    @property
    def levels(self):
        levels = None if self.automatic_checkbox.isChecked() else \
            [self.min_spinbox.value(), self.max_spinbox.value()]
        return levels
