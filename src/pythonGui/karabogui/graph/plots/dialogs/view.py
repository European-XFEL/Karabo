import os

from PyQt5 import uic
from PyQt5.QtCore import pyqtSlot, Qt
from PyQt5.QtGui import QColor, QPixmap, QIcon
from PyQt5.QtWidgets import QColorDialog, QDialog


class GraphViewDialog(QDialog):

    def __init__(self, config={}, parent=None):
        super(GraphViewDialog, self).__init__(parent)
        ui_path = os.path.join(os.path.abspath(os.path.dirname(__file__)),
                               'view.ui')
        uic.loadUi(ui_path, self)

        self.graph_title = config['title']
        self.ui_title.setText(self.graph_title)
        self.graph_bg_color = config['background']
        self.set_text_background_button()
        self.ui_cb_background.setChecked(
            self.graph_bg_color != 'transparent')

    def set_text_background_button(self):
        pixmap = QPixmap(24, 16)
        pixmap.fill(QColor(self.graph_bg_color))
        self.ui_pb_background.setIcon(QIcon(pixmap))

    @pyqtSlot(int)
    def on_ui_cb_background_stateChanged(self, state):
        if state != Qt.Checked:
            self.graph_bg_color = 'transparent'

    @pyqtSlot(bool)
    def on_ui_cb_background_toggled(self, checked):
        self.ui_pb_background.setEnabled(checked)
        if not checked:
            self.graph_bg_color = 'transparent'

    @pyqtSlot()
    def on_ui_pb_background_clicked(self):
        color = QColorDialog.getColor(QColor(self.graph_bg_color))
        if color.isValid():
            self.graph_bg_color = color.name()
            self.set_text_background_button()

    @property
    def settings(self):
        config = {
            "background": self.graph_bg_color,
            "title": self.ui_title.text(),
        }

        return config

    @staticmethod
    def get(configuration, parent=None):
        dialog = GraphViewDialog(configuration, parent)
        result = dialog.exec_() == QDialog.Accepted
        content = {}
        content.update(dialog.settings)

        return content, result
