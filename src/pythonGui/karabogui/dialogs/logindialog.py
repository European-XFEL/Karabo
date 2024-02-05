#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on December 21, 2011
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
#############################################################################
from qtpy import uic
from qtpy.QtCore import Qt, Slot
from qtpy.QtWidgets import QComboBox, QDialog

from karabogui.util import generateObjectName
from karabogui.validators import IntValidator

from .utils import get_dialog_ui

FINE_COLOR = "black"
ERROR_COLOR = "red"


class LoginDialog(QDialog):
    def __init__(self, username="", password="", hostname="", port="",
                 gui_servers=[], parent=None):
        super().__init__(parent)
        filepath = get_dialog_ui("logindialog.ui")
        uic.loadUi(filepath, self)
        self.setWindowFlags(self.windowFlags() | Qt.WindowStaysOnTopHint)

        objectName = generateObjectName(self.ui_port)
        self._port_sheet = (f"QWidget#{objectName}" +
                            " {{ color: {}; }}")
        self.ui_port.setObjectName(objectName)
        sheet = self._port_sheet.format(FINE_COLOR)

        index = self.ui_username.findText(username)
        self.ui_username.setCurrentIndex(index)

        # password
        if not password:
            password = "karabo"
        self.ui_password.setText(password)

        hostname = hostname if hostname else "localhost"
        self.ui_hostname.editTextChanged.connect(self.on_hostname_changed)
        self.ui_hostname.setInsertPolicy(QComboBox.NoInsert)

        self.ui_hostname.setEditable(True)
        for server in gui_servers:
            self.ui_hostname.addItem(server)
        self.ui_hostname.setEditText(hostname)

        port = str(port) if port else "44444"
        self.ui_port.setText(port)
        self.ui_port.setValidator(IntValidator(maxInc=65535, parent=self))
        self.ui_port.setStyleSheet(sheet)
        self.ui_port.textChanged.connect(self.on_port_changed)

    @Slot(str)
    def on_hostname_changed(self, value):
        """Split the selected text into hostname and port
        """
        if ":" in value:
            hostname, port = self.ui_hostname.currentText().split(":")
            self.ui_hostname.setEditText(hostname)
            self.ui_port.setText(port)
        self._update_dialog()

    @Slot()
    def on_port_changed(self):
        self._update_dialog()

    @property
    def username(self):
        return self.ui_username.currentText()

    @property
    def password(self):
        return self.ui_password.text()

    @property
    def hostname(self):
        return self.ui_hostname.currentText()

    @property
    def port(self):
        return int(self.ui_port.text())

    @property
    def gui_servers(self):
        return [self.ui_hostname.itemText(i)
                for i in range(self.ui_hostname.count())]

    def _update_dialog(self):
        acceptable_port = self.ui_port.hasAcceptableInput()
        color = FINE_COLOR if acceptable_port else ERROR_COLOR
        sheet = self._port_sheet.format(color)
        self.ui_port.setStyleSheet(sheet)

        acceptable_host = self.ui_hostname.currentText() != ""
        enabled = acceptable_host and acceptable_port
        self.ui_connect.setEnabled(enabled)
