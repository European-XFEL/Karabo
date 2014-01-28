#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on January 27, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a dialog to select a plugin.
"""

__all__ = ["PluginDialog"]


from PyQt4.QtGui import (QComboBox, QDialog, QDialogButtonBox, QFormLayout,
                         QGroupBox, QLineEdit, QVBoxLayout)


class PluginDialog(QDialog):


    def __init__(self, serverPluginConfig):
        super(PluginDialog, self).__init__()

        self.setWindowTitle("Import plugin")

        vLayout = QVBoxLayout(self)

        self.__gbSelectDeviceId = QGroupBox("Select device ID", self)
        fLayout = QFormLayout(self.__gbSelectDeviceId)
        self.__leDeviceId = QLineEdit("")
        fLayout.addRow("Device ID:", self.__leDeviceId)
        vLayout.addWidget(self.__gbSelectDeviceId)

        self.__gbSelectPlugin = QGroupBox("Select plugin", self)
        fLayout = QFormLayout(self.__gbSelectPlugin)
        self.__cbPlugin = QComboBox()
        fLayout.addRow("Plugin:", self.__cbPlugin)
        vLayout.addWidget(self.__gbSelectPlugin)

        self.__gbSelectServer = QGroupBox("Select server", self)
        fLayout = QFormLayout(self.__gbSelectServer)
        self.__cbServer = QComboBox()
        fLayout.addRow("Server:", self.__cbServer)
        vLayout.addWidget(self.__gbSelectServer)

        self.__buttonBox = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel)
        self.__buttonBox.accepted.connect(self.accept)
        self.__buttonBox.rejected.connect(self.reject)
        vLayout.addWidget(self.__buttonBox)


    @property
    def deviceId(self):
        return self.__leDeviceId.text()


    @property
    def plugin(self):
        return self.__cbPlugin.currentText()


    @property
    def server(self):
        return self.__cbServer.currentText()

