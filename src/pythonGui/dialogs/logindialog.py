#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on December 21, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the login dialog."""

__all__ = ["LoginDialog"]


from PyQt4.QtGui import (QComboBox, QDialog, QFormLayout, QFrame, QHBoxLayout,
                         QIntValidator, QLabel, QLineEdit, QPushButton, QVBoxLayout)


class LoginDialog(QDialog):

    def __init__(self, username=None, password=None, provider=None, hostname=None,
                 port=None):
        super(LoginDialog, self).__init__()

        self.setWindowTitle("Login")
        
        hLayout = QHBoxLayout()
        self.__leSelectConnection = QLabel("Select connection type:", self)
        self.__leSelectConnection.setVisible(False)
        self.__cbSelectConnection = QComboBox(self)
        self.__cbSelectConnection.setVisible(False)
        self.__cbSelectConnection.addItem("GuiServer connection")
        self.__cbSelectConnection.addItem("Direct broker connection")
        hLayout.addWidget(self.__leSelectConnection)
        hLayout.addWidget(self.__cbSelectConnection)
        
        self.__hLine = QFrame(self)
        #self.__hLine.setGeometry(QRect(320, 150, 118, 3))
        self.__hLine.setVisible(False)
        self.__hLine.setFrameShape(QFrame.HLine)
        self.__hLine.setFrameShadow(QFrame.Sunken)
        
        formLayout = QFormLayout()
        if not username:
            username = "operator"
        self.__leUsername = QLineEdit(username)
        formLayout.addRow("Username:", self.__leUsername)
        self.__leUsername.textChanged.connect(self.onUsernameChanged)
        
        if not password:
            password = "karabo"
        self.__lePassword = QLineEdit(password)
        self.__lePassword.setEchoMode(QLineEdit.Password)
        formLayout.addRow("Password:", self.__lePassword)
        
        # I was getting this error: QSpiAccessible::accessibleEvent not handled: "8008" obj: QObject(0x0) " invalid interface!"
        # when making mouse-over different options in the ComboBox.
        # I solve it as documented here: http://code.google.com/p/clementine-player/issues/detail?id=1706
        # Current open bug: https://bugs.launchpad.net/ubuntu/+source/qtcreator/+bug/959722
        # Running "sudo apt-get remove qt-at-spi"
        self.__cbProvider = QComboBox()
        self.__cbProvider.setEditable(False)
        self.__cbProvider.addItems(["LOCAL", "KERBEROS"])
        if provider:
            index  = self.__cbProvider.findText(provider)
            self.__cbProvider.setCurrentIndex(index)
        formLayout.addRow("Provider:", self.__cbProvider)
        
        if not hostname:
            hostname = "localhost"
        self.__leHostname = QLineEdit(hostname)
        formLayout.addRow("Hostname:", self.__leHostname)
        
        if port is None:
            port = "44444"
        else:
            port = str(port)
        self.__lePort = QLineEdit(port)
        self.__lePort.setValidator(QIntValidator(None))
        formLayout.addRow("Port:", self.__lePort)
        
        self.__pbOk = QPushButton("Connect")
        self.__pbOk.clicked.connect(self.accept)
        self.__pbCancel = QPushButton("Cancel")
        self.__pbCancel.clicked.connect(self.reject)
        btnLayout = QHBoxLayout()
        btnLayout.addStretch(0)
        btnLayout.addWidget(self.__pbOk)
        btnLayout.addWidget(self.__pbCancel)
        
        vLayout = QVBoxLayout(self)
        vLayout.addLayout(hLayout)
        vLayout.addWidget(self.__hLine)
        vLayout.addLayout(formLayout)
        vLayout.addLayout(btnLayout)

    @property
    def username(self):
        return self.__leUsername.text().lower()


    @property
    def password(self):
        return self.__lePassword.text()

    
    @property
    def provider(self):
        return self.__cbProvider.currentText()


    @property
    def hostname(self):
        return self.__leHostname.text()


    @property
    def port(self):
        return int(self.__lePort.text())


    def _showConnectionSelection(self, show):
        self.__leSelectConnection.setVisible(show)
        self.__cbSelectConnection.setVisible(show)
        self.__hLine.setVisible(show)


### slots ###
    def onUsernameChanged(self, text):
        # Here comes the easter egg...
        text = text.lower()
        if text == "admin":
            self._showConnectionSelection(True)
        else:
            self._showConnectionSelection(False)
    
    