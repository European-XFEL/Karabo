#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on December 21, 2011
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#############################################################################
from qtpy import uic
from qtpy.QtCore import Qt, Slot
from qtpy.QtGui import QIntValidator
from qtpy.QtWidgets import QComboBox, QDialog

from .utils import get_dialog_ui


class LoginDialog(QDialog):
    def __init__(self, username="", password="", hostname="", port="",
                 gui_servers=[], parent=None):
        super(LoginDialog, self).__init__(parent)
        filepath = get_dialog_ui("logindialog.ui")
        uic.loadUi(filepath, self)
        self.setWindowFlags(self.windowFlags() | Qt.WindowStaysOnTopHint)

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
        self.ui_port.setValidator(QIntValidator(None))

    @Slot(str)
    def on_hostname_changed(self, value):
        """Split the selected text into hostname and port
        """
        if ":" in value:
            hostname, port = self.ui_hostname.currentText().split(":")
            self.ui_hostname.setEditText(hostname)
            self.ui_port.setText(port)

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
