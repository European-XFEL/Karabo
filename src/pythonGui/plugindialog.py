#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on January 27, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a dialog to select a plugin.
"""

__all__ = ["PluginDialog"]

import globals

from PyQt4.QtCore import Qt
from PyQt4.QtGui import (QComboBox, QDialog, QDialogButtonBox, QFormLayout,
                         QGroupBox, QLineEdit, QVBoxLayout)


class PluginDialog(QDialog):

    def __init__(self, serverTopology):
        super(PluginDialog, self).__init__()

        self.setWindowTitle("Import plugin")

        vLayout = QVBoxLayout(self)
        vLayout.setContentsMargins(5,5,5,5)

        self.__gbSelectDeviceId = QGroupBox("Select device ID", self)
        fLayout = QFormLayout(self.__gbSelectDeviceId)
        fLayout.setContentsMargins(5,5,5,5)
        self.__leDeviceId = QLineEdit("")
        self.__leDeviceId.textChanged.connect(self.onDeviceIdChanged)
        fLayout.addRow("Device ID:", self.__leDeviceId)
        vLayout.addWidget(self.__gbSelectDeviceId)

        self.__gbSelectPlugin = QGroupBox("Select plugin", self)
        fLayout = QFormLayout(self.__gbSelectPlugin)
        fLayout.setContentsMargins(5,5,5,5)
        self.__cbPlugin = QComboBox()
        self.__cbPlugin.currentIndexChanged[int].connect(self.onPluginChanged)
        fLayout.addRow("Plugin:      ", self.__cbPlugin)
        vLayout.addWidget(self.__gbSelectPlugin)

        self.__gbSelectServer = QGroupBox("Select server", self)
        fLayout = QFormLayout(self.__gbSelectServer)
        fLayout.setContentsMargins(5,5,5,5)
        self.__cbServer = QComboBox()
        self.__cbServer.currentIndexChanged[int].connect(self.onServerChanged)
        fLayout.addRow("Server:     ", self.__cbServer)
        vLayout.addWidget(self.__gbSelectServer)

        self.__buttonBox = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel)
        self.__buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)
        self.__buttonBox.accepted.connect(self.accept)
        self.__buttonBox.rejected.connect(self.reject)
        vLayout.addWidget(self.__buttonBox)
        
        self.updateServerTopology(serverTopology)


    def updateServerTopology(self, serverTopology):
        self.__serverTopology = serverTopology
        if not self.__serverTopology:
            return

        self.__cbPlugin.clear()
        self.__cbServer.clear()

        for serverId in self.__serverTopology.keys():

            visibility = self.__serverTopology.getAttribute(serverId, "visibility")
            if visibility > globals.GLOBAL_ACCESS_LEVEL:
                continue

            deviceClasses = None
            if self.__serverTopology.hasAttribute(serverId, "deviceClasses"):
                deviceClasses = self.__serverTopology.getAttribute(serverId, "deviceClasses")

            if not deviceClasses:
                return

            #self.__cbServer.blockSignals(True)
            self.__cbServer.addItem(serverId, deviceClasses)
            #self.__cbServer.blockSignals(False)

            #for plugin in deviceClasses:
            #    index = self.__cbPlugin.findText(plugin)
            #    if index > -1:
            #        data = self.__cbPlugin.itemData(index)
            #        data.append(serverId)
            #        self.__cbPlugin.setItemData(index, data)
            #        continue

            #    self.__cbPlugin.blockSignals(True)
            #    self.__cbPlugin.addItem(plugin, [serverId])
            #    self.__cbPlugin.blockSignals(False)

        self.__cbPlugin.adjustSize()
        self.__cbServer.adjustSize()


    @property
    def deviceId(self):
        return self.__leDeviceId.text()


    @property
    def plugin(self):
        return self.__cbPlugin.currentText()


    @property
    def server(self):
        return self.__cbServer.currentText()


### Slots ###
    def onDeviceIdChanged(self, text):
        self.__buttonBox.button(QDialogButtonBox.Ok).setEnabled(len(text) > 0)


    def onPluginChanged(self, index):
        # Get servers where plugin exists
        pass
        #data = self.__cbPlugin.itemData(index)
        #print "onPluginChanged", data


    def onServerChanged(self, index):
        # Get plugins which exist on server
        data = self.__cbServer.itemData(index)
        self.__cbPlugin.clear()
        for d in data:
            self.__cbPlugin.blockSignals(True)
            self.__cbPlugin.addItem(d)
            self.__cbPlugin.blockSignals(False)

