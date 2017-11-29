#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on December 21, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import os.path as op

from PyQt4 import uic
from PyQt4.QtGui import QDialog, QIntValidator


class LoginDialog(QDialog):
    def __init__(self, username='', password='', provider='', hostname='',
                 port='', parent=None):
        super(LoginDialog, self).__init__(parent)
        filepath = op.join(op.abspath(op.dirname(__file__)), 'logindialog.ui')
        uic.loadUi(filepath, self)

        if not username:
            username = 'operator'
        self.leUsername.setText(username)
        self.leUsername.selectAll()

        if not password:
            password = 'karabo'
        self.lePassword.setText(password)

        if provider:
            index = self.cbProvider.findText(provider)
            self.cbProvider.setCurrentIndex(index)

        if not hostname:
            hostname = 'localhost'
        self.leHostname.setText(hostname)

        if not port:
            port = '44444'
        else:
            port = str(port)
        self.lePort.setText(port)
        self.lePort.setValidator(QIntValidator(None))

    @property
    def username(self):
        return self.leUsername.text().lower()

    @property
    def password(self):
        return self.lePassword.text()

    @property
    def provider(self):
        return self.cbProvider.currentText()

    @property
    def hostname(self):
        return self.leHostname.text()

    @property
    def port(self):
        return int(self.lePort.text())
