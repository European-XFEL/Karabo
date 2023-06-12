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
import os

from qtpy import uic
from qtpy.QtCore import Qt, Slot
from qtpy.QtGui import QColor, QIcon, QPixmap
from qtpy.QtWidgets import QColorDialog, QDialog


class GraphViewDialog(QDialog):

    def __init__(self, config={}, parent=None):
        super().__init__(parent)
        ui_path = os.path.join(os.path.abspath(os.path.dirname(__file__)),
                               "view.ui")
        uic.loadUi(ui_path, self)

        self.graph_title = config.get("title", "")
        self.ui_title.setText(self.graph_title)
        self.graph_bg_color = config.get("background", "transparent")
        self.set_text_background_button()
        self.ui_cb_background.setChecked(
            self.graph_bg_color != "transparent")

    def set_text_background_button(self):
        pixmap = QPixmap(24, 16)
        pixmap.fill(QColor(self.graph_bg_color))
        self.ui_pb_background.setIcon(QIcon(pixmap))

    @Slot(int)
    def on_ui_cb_background_stateChanged(self, state):
        if state != Qt.Checked:
            self.graph_bg_color = "transparent"

    @Slot(bool)
    def on_ui_cb_background_toggled(self, checked):
        self.ui_pb_background.setEnabled(checked)
        if not checked:
            self.graph_bg_color = "transparent"

    @Slot()
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
        result = dialog.exec() == QDialog.Accepted
        content = {}
        content.update(dialog.settings)

        return content, result
