import os

from PyQt5 import uic
from PyQt5.QtCore import pyqtSlot
from PyQt5.QtWidgets import QDialog


class RangeXDialog(QDialog):
    def __init__(self, config=None, actual=None, parent=None):
        super(RangeXDialog, self).__init__(parent)
        self.setModal(False)
        # load ui file
        ui_path = os.path.join(os.path.abspath(os.path.dirname(__file__)),
                               'range_config.ui')
        uic.loadUi(ui_path, self)

        # Show the actual view!
        self.ui_view_min.setText("{:.1f}".format(actual['x_min']))
        self.ui_view_max.setText("{:.1f}".format(actual['x_max']))

        # Load the model settings!
        self.ui_min.setValue(config['x_min'])
        self.ui_max.setValue(config['x_max'])
        state = config['x_autorange']
        self.ui_autorange.setChecked(state)
        self.ui_autorange.toggled.connect(self._check_box_triggered)
        self.ui_min.setEnabled(not state)
        self.ui_max.setEnabled(not state)
        self.button_box.accepted.connect(self.accept)
        self.button_box.rejected.connect(self.reject)

    @pyqtSlot(bool)
    def _check_box_triggered(self, state):
        self.ui_min.setEnabled(not state)
        self.ui_max.setEnabled(not state)

    @property
    def limits(self):
        config = {
            "x_min": self.ui_min.value(),
            "x_max": self.ui_max.value(),
            "x_autorange": self.ui_autorange.isChecked()}

        return config

    @staticmethod
    def get(model, actual, parent=None):
        dialog = RangeXDialog(model, actual, parent)
        result = dialog.exec_() == QDialog.Accepted
        content = {}
        content.update(dialog.limits)

        return content, result


class RangeYDialog(QDialog):
    def __init__(self, config=None, actual=None, parent=None):
        super(RangeYDialog, self).__init__(parent)
        self.setModal(False)
        # load ui file
        ui_path = os.path.join(os.path.abspath(os.path.dirname(__file__)),
                               'range_config.ui')
        uic.loadUi(ui_path, self)

        # Show the actual view!
        self.ui_view_min.setText("{:.1f}".format(actual['y_min']))
        self.ui_view_max.setText("{:.1f}".format(actual['y_max']))

        # Load the model settings!
        self.ui_min.setValue(config['y_min'])
        self.ui_max.setValue(config['y_max'])
        state = config['y_autorange']
        self.ui_autorange.setChecked(state)
        self.ui_autorange.toggled.connect(self._check_box_triggered)
        self.ui_min.setEnabled(not state)
        self.ui_max.setEnabled(not state)
        self.button_box.accepted.connect(self.accept)
        self.button_box.rejected.connect(self.reject)

    @pyqtSlot(bool)
    def _check_box_triggered(self, state):
        self.ui_min.setEnabled(not state)
        self.ui_max.setEnabled(not state)

    @property
    def limits(self):
        config = {
            "y_min": self.ui_min.value(),
            "y_max": self.ui_max.value(),
            "y_autorange": self.ui_autorange.isChecked()}

        return config

    @staticmethod
    def get(model, actual, parent=None):
        dialog = RangeYDialog(model, actual, parent)
        result = dialog.exec_() == QDialog.Accepted
        content = {}
        content.update(dialog.limits)

        return content, result
