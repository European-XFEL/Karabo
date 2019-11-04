import os

from PyQt5 import uic
from PyQt5.QtCore import pyqtSlot
from PyQt5.QtWidgets import QDialog


class RangeDialog(QDialog):
    def __init__(self, config, actual=None, parent=None):
        super(RangeDialog, self).__init__(parent)
        self.setModal(False)
        # load ui file
        ui_path = os.path.join(os.path.abspath(os.path.dirname(__file__)),
                               'range_config.ui')
        uic.loadUi(ui_path, self)

        # Show the actual view!
        self.ui_view_min_x.setText("{:.1f}".format(actual['x_min']))
        self.ui_view_min_y.setText("{:.1f}".format(actual['y_min']))
        self.ui_view_max_x.setText("{:.1f}".format(actual['x_max']))
        self.ui_view_max_y.setText("{:.1f}".format(actual['y_max']))

        # Load the model settings!
        self.ui_x_min.setValue(config['x_min'])
        self.ui_y_min.setValue(config['y_min'])
        self.ui_x_max.setValue(config['x_max'])
        self.ui_y_max.setValue(config['y_max'])
        state = config['autorange']
        self.ui_autorange.setChecked(state)
        self.ui_autorange.stateChanged.connect(self._check_box_triggered)
        self.ui_x_min.setEnabled(not state)
        self.ui_y_min.setEnabled(not state)
        self.ui_x_max.setEnabled(not state)
        self.ui_y_max.setEnabled(not state)

        self.button_box.accepted.connect(self.accept)
        self.button_box.rejected.connect(self.reject)

    @pyqtSlot(bool)
    def _check_box_triggered(self, state):
        self.ui_x_min.setEnabled(not state)
        self.ui_y_min.setEnabled(not state)
        self.ui_x_max.setEnabled(not state)
        self.ui_y_max.setEnabled(not state)

    @property
    def limits(self):
        config = {
            "x_min": self.ui_x_min.value(),
            "y_min": self.ui_y_min.value(),
            "x_max": self.ui_x_max.value(),
            "y_max": self.ui_y_max.value(),
            "autorange": self.ui_autorange.isChecked()}

        return config

    @staticmethod
    def get(model, actual, parent=None):
        dialog = RangeDialog(model, actual, parent)
        result = dialog.exec_() == QDialog.Accepted
        content = {}
        content.update(dialog.limits)

        return content, result
