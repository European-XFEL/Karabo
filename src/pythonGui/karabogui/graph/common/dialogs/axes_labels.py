import os

from qtpy import uic
from qtpy.QtWidgets import QDialog


class AxesLabelsDialog(QDialog):

    def __init__(self, config, parent=None):
        super(AxesLabelsDialog, self).__init__(parent)
        self.setModal(False)
        # load ui file
        ui_path = os.path.join(os.path.abspath(os.path.dirname(__file__)),
                               'axes_labels.ui')
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
        dialog = AxesLabelsDialog(configuration, parent)
        result = dialog.exec_() == QDialog.Accepted
        content = {}
        content.update(dialog.labels)

        return content, result
