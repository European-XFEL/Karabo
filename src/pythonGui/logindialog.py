#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on December 21, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the login dialog."""

__all__ = ["LoginDialog"]


import sys

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class LoginDialog(QDialog):

    def __init__(self):
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
        self.__leUsername = QLineEdit("operator") #("username")
        formLayout.addRow("Username:", self.__leUsername)
        self.__leUsername.textChanged.connect(self.onUsernameChanged)
        
        self.__lePassword = QLineEdit("karabo") #("default")
        self.__lePassword.setEchoMode(QLineEdit.Password)
        formLayout.addRow("Password:", self.__lePassword)
        
        # I was getting this error: QSpiAccessible::accessibleEvent not handled: "8008" obj: QObject(0x0) " invalid interface!"
        # when making mouse-over different options in the ComboBox.
        # I solve it as documented here: http://code.google.com/p/clementine-player/issues/detail?id=1706
        # Current open bug: https://bugs.launchpad.net/ubuntu/+source/qtcreator/+bug/959722
        # Running "sudo apt-get remove qt-at-spi"
        self.__leProvider = QComboBox()
        self.__leProvider.setEditable(False)
        self.__leProvider.addItems(["LOCAL", "KERBEROS"])
        formLayout.addRow("Provider:", self.__leProvider)
        
        self.__leHostname = QLineEdit("localhost")#("131.169.212.42")
        formLayout.addRow("Hostname:", self.__leHostname)
        
        self.__lePort = QLineEdit("44444")
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
        return str(self.__leUsername.text().lower())


    @property
    def password(self):
        return str(self.__lePassword.text())

    
    @property
    def provider(self):
        return str(self.__leProvider.currentText())


    @property
    def hostname(self):
        return str(self.__leHostname.text())


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
    
    