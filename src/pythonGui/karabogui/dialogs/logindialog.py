#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on December 21, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import os.path as op

from PyQt4 import uic
from PyQt4.QtCore import pyqtSlot, QEvent, QObject, Qt
from PyQt4.QtGui import QComboBox, QDialog, QIntValidator, QKeyEvent

import karabogui.globals as krb_globals


class KeyPressEventFilter(QObject):
    """Purge a ComboBox on Shift+Delete events
    """
    def eventFilter(self, obj, event):
        if event.type() == QEvent.KeyPress:
            if isinstance(event, QKeyEvent):
                if (event.key() == Qt.Key_Delete and
                        event.modifiers() == Qt.ShiftModifier):
                    if isinstance(obj, QComboBox) and obj.count():
                        index = obj.currentIndex()
                        obj.removeItem(index)
                        obj.setCurrentIndex(index if index < obj.count()
                                            else index - 1)
                        return True
        return False


class LoginDialog(QDialog):
    def __init__(self, username='', password='', provider='', hostname='',
                 port='', guiservers=[], parent=None):
        super(LoginDialog, self).__init__(parent)
        filepath = op.join(op.abspath(op.dirname(__file__)), 'logindialog.ui')
        uic.loadUi(filepath, self)

        if not username:
            username = krb_globals.GLOBAL_USER
        index = self.leUsername.findText(username)
        self.leUsername.setCurrentIndex(index)

        # password
        if not password:
            password = 'karabo'
        self.lePassword.setText(password)

        if provider:
            index = self.cbProvider.findText(provider)
            self.cbProvider.setCurrentIndex(index)

        hostname = 'localhost' if not hostname else hostname
        self.cbHostname.editTextChanged.connect(self.onHostnameTextChanged)
        self.cbHostname.setInsertPolicy(QComboBox.NoInsert)

        self.keyPressHandler = KeyPressEventFilter()
        self.cbHostname.installEventFilter(self.keyPressHandler)

        self.cbHostname.setEditable(True)
        for server in guiservers:
            self.cbHostname.addItem(server)
        self.cbHostname.setEditText(hostname)

        port = str(port) if port else'44444'
        self.lePort.setText(port)
        self.lePort.setValidator(QIntValidator(None))

    @pyqtSlot(str)
    def onHostnameTextChanged(self, value):
        """Split the selected text into hostname and port
        """
        if ':' in value:
            hostname, port = self.cbHostname.currentText().split(':')
            self.cbHostname.setEditText(hostname)
            self.lePort.setText(port)

    @property
    def username(self):
        return self.leUsername.currentText()

    @property
    def password(self):
        return self.lePassword.text()

    @property
    def provider(self):
        return self.cbProvider.currentText()

    @property
    def hostname(self):
        return self.cbHostname.currentText()

    @property
    def port(self):
        return int(self.lePort.text())

    @property
    def guiservers(self):
        return [self.cbHostname.itemText(i)
                for i in range(self.cbHostname.count())]
