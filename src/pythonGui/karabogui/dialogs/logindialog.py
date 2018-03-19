#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on December 21, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import os.path as op

from PyQt4 import uic
from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import QComboBox, QDialog, QIntValidator


class LoginDialog(QDialog):
    def __init__(self, username='', password='', provider='',
                 hostname='', port='', guiservers=[], parent=None):
        super(LoginDialog, self).__init__(parent)
        filepath = op.join(op.abspath(op.dirname(__file__)), 'logindialog.ui')
        uic.loadUi(filepath, self)

        index = self.ui_username.findText(username)
        self.ui_username.setCurrentIndex(index)

        # password
        if not password:
            password = 'karabo'
        self.ui_password.setText(password)

        if provider:
            index = self.ui_provider.findText(provider)
            self.ui_provider.setCurrentIndex(index)

        hostname = hostname if hostname else 'localhost'
        self.ui_hostname.editTextChanged.connect(self.on_hostname_changed)
        self.ui_hostname.setInsertPolicy(QComboBox.NoInsert)

        self.ui_hostname.setEditable(True)
        for server in guiservers:
            self.ui_hostname.addItem(server)
        self.ui_hostname.setEditText(hostname)

        port = str(port) if port else '44444'
        self.ui_port.setText(port)
        self.ui_port.setValidator(QIntValidator(None))

    @pyqtSlot(str)
    def on_hostname_changed(self, value):
        """Split the selected text into hostname and port
        """
        if ':' in value:
            hostname, port = self.ui_hostname.currentText().split(':')
            self.ui_hostname.setEditText(hostname)
            self.ui_port.setText(port)

    @property
    def username(self):
        return self.ui_username.currentText()

    @property
    def password(self):
        return self.ui_password.text()

    @property
    def provider(self):
        return self.ui_provider.currentText()

    @property
    def hostname(self):
        return self.ui_hostname.currentText()

    @property
    def port(self):
        return int(self.ui_port.text())

    @property
    def guiservers(self):
        return [self.ui_hostname.itemText(i)
                for i in range(self.ui_hostname.count())]
