import os

from qtpy import uic
from qtpy.QtCore import Slot
from qtpy.QtWidgets import QDialog

from karabogui.validators import NumberValidator

DECIMALS = 6


class RangeDialog(QDialog):
    def __init__(self, config=None, actual=None, axis=0, parent=None):
        super(RangeDialog, self).__init__(parent)
        self.setModal(False)
        ui_path = os.path.join(os.path.abspath(os.path.dirname(__file__)),
                               'range_config.ui')
        uic.loadUi(ui_path, self)
        self.button_box.accepted.connect(self.accept)
        self.button_box.rejected.connect(self.reject)

        self.get_axis_notation(axis=axis)
        self.ui_min.setValidator(NumberValidator(decimals=DECIMALS))
        self.ui_max.setValidator(NumberValidator(decimals=DECIMALS))

        # Show the actual view!
        self.ui_view_min.setText('{:.2e}'.format(actual[self.axis_min]))
        self.ui_view_max.setText('{:.2e}'.format(actual[self.axis_max]))

        # Load the model settings!
        self.ui_min.setText('{}'.format(config[self.axis_min]))
        self.ui_max.setText('{}'.format(config[self.axis_max]))

        state = config[self.axis_auto]
        self.ui_autorange.setChecked(state)
        self.ui_autorange.toggled.connect(self._check_box_triggered)
        self.ui_min.setEnabled(not state)
        self.ui_max.setEnabled(not state)

    def get_axis_notation(self, axis):
        axis = 'x' if axis == 0 else 'y'
        self.axis_min = '{}_min'.format(axis)
        self.axis_max = '{}_max'.format(axis)
        self.axis_auto = '{}_autorange'.format(axis)
        self.ui_axis.setText('Axis View: {}-axis'.format(axis))

    @Slot(bool)
    def _check_box_triggered(self, state):
        self.ui_min.setEnabled(not state)
        self.ui_max.setEnabled(not state)

    @property
    def limits(self):
        config = {
            self.axis_min: float(self.ui_min.text()),
            self.axis_max: float(self.ui_max.text()),
            self.axis_auto: self.ui_autorange.isChecked()}

        return config

    @staticmethod
    def get(model, actual, axis=0, parent=None):
        dialog = RangeDialog(model, actual, axis, parent)
        result = dialog.exec_() == QDialog.Accepted
        content = {}
        content.update(dialog.limits)

        return content, result
