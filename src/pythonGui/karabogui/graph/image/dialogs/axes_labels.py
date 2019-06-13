import os

from PyQt4 import uic
from PyQt4.QtGui import QDialog

from karabogui.graph.common.const import LABEL, UNITS


class AxesLabelsDialog(QDialog):
    def __init__(self, labels, parent=None):
        super(AxesLabelsDialog, self).__init__(parent)

        # load ui file
        ui_path = os.path.join(os.path.abspath(os.path.dirname(__file__)),
                               'axes_labels.ui')
        uic.loadUi(ui_path, self)

        # populate label fields
        x_label, y_label = labels

        self.ui_xlabel.setText(x_label[LABEL])
        self.ui_xunits.setText(x_label[UNITS])
        self.ui_ylabel.setText(y_label[LABEL])
        self.ui_yunits.setText(y_label[UNITS])

        self.button_box.accepted.connect(self.accept)
        self.button_box.rejected.connect(self.reject)

    @property
    def labels(self):
        config = {
            "x_label": self.ui_xlabel.text(),
            "x_units": self.ui_xunits.text(),
            "y_label": self.ui_ylabel.text(),
            "y_units": self.ui_yunits.text()
        }

        return config

    @staticmethod
    def get(labels, parent=None):
        dialog = AxesLabelsDialog(labels, parent)
        result = dialog.exec_() == QDialog.Accepted
        content = {}
        content.update(dialog.labels)

        return content, result
