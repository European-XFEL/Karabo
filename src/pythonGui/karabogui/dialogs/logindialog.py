#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on December 21, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import os.path as op

from PyQt4 import uic
from PyQt4.QtCore import pyqtSlot, QEvent, QObject, Qt
from PyQt4.QtGui import QComboBox, QDialog, QIntValidator, QKeyEvent


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
            username = 'operator'
        self.leUsername.setText(username)
        self.leUsername.selectAll()

        if not password:
            password = 'karabo'
        self.lePassword.setText(password)

        if provider:
            index = self.cbProvider.findText(provider)
            self.cbProvider.setCurrentIndex(index)

        self.cbHostname.editTextChanged.connect(self.onHostnameTextChanged)

        self.keyPressHandler = KeyPressEventFilter()
        self.cbHostname.installEventFilter(self.keyPressHandler)

        self.cbHostname.setEditable(True)
        self.cbHostname.setEditText(hostname)

        for server in guiservers:
            self.cbHostname.addItem(server)

        if not hostname:
            hostname = 'localhost'

        if not port:
            port = '44444'
        else:
            port = str(port)
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
        return self.leUsername.text().lower()

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
