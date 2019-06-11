import os

from PyQt4 import uic
from PyQt4.QtGui import QDialog


class AxesDialog(QDialog):

    def __init__(self, config, parent=None):
        super(AxesDialog, self).__init__(parent)
        self.setModal(False)
        # load ui file
        ui_path = os.path.join(os.path.abspath(os.path.dirname(__file__)),
                               'axes_config.ui')
        uic.loadUi(ui_path, self)

        self.ui_xlabel.setText(config['x_label'])
        self.ui_ylabel.setText(config['y_label'])
        self.ui_xunits.setText(config['x_units'])
        self.ui_yunits.setText(config['y_units'])

        self.button_box.accepted.connect(self.accept)
        self.button_box.rejected.connect(self.reject)

    @property
    def labels(self):
        config = {
            "x_label": self.ui_xlabel.text(),
            "x_units": self.ui_xunits.text(),
            "y_label": self.ui_ylabel.text(),
            "y_units": self.ui_yunits.text()}

        return config

    @staticmethod
    def get(configuration, parent=None):
        dialog = AxesDialog(configuration, parent)
        result = dialog.exec_() == QDialog.Accepted
        content = {}
        content.update(dialog.labels)

        return content, result
