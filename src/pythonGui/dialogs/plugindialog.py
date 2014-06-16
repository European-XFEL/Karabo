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
                         QGroupBox, QLineEdit, QMessageBox, QVBoxLayout)


class PluginDialog(QDialog):

    def __init__(self):
        super(PluginDialog, self).__init__()

        self.setWindowTitle("Add device")

        vLayout = QVBoxLayout(self)
        vLayout.setContentsMargins(5,5,5,5)

        self.gbSelectDeviceId = QGroupBox("Select device ID", self)
        fLayout = QFormLayout(self.gbSelectDeviceId)
        fLayout.setContentsMargins(5,5,5,5)
        self.leDeviceId = QLineEdit("")
        self.leDeviceId.textChanged.connect(self.onDeviceIdChanged)
        fLayout.addRow("Device ID:", self.leDeviceId)
        vLayout.addWidget(self.gbSelectDeviceId)

        self.gbSelectServer = QGroupBox("Select server", self)
        fLayout = QFormLayout(self.gbSelectServer)
        fLayout.setContentsMargins(5,5,5,5)
        self.cbServer = QComboBox()
        self.cbServer.setSizeAdjustPolicy(QComboBox.AdjustToContents)
        self.cbServer.currentIndexChanged[int].connect(self.onServerChanged)
        fLayout.addRow("Server:     ", self.cbServer)
        vLayout.addWidget(self.gbSelectServer)

        self.__gbSelectPlugin = QGroupBox("Select device type", self)
        fLayout = QFormLayout(self.__gbSelectPlugin)
        fLayout.setContentsMargins(5,5,5,5)
        self.cbPlugin = QComboBox()
        self.cbPlugin.setSizeAdjustPolicy(QComboBox.AdjustToContents)
        self.cbPlugin.currentIndexChanged[int].connect(self.onPluginChanged)
        fLayout.addRow("Device:    ", self.cbPlugin)
        vLayout.addWidget(self.__gbSelectPlugin)

        self.gbStartUp = QGroupBox("Select startup behaviour", self)
        fLayout = QFormLayout(self.gbStartUp)
        fLayout.setContentsMargins(5,5,5,5)
        self.cbStartUp = QComboBox()
        self.cbStartUp.addItems(["restart", "ignore"])
        self.cbStartUp.setSizeAdjustPolicy(QComboBox.AdjustToContents)
        fLayout.addRow("Startup behaviour:", self.cbStartUp)
        vLayout.addWidget(self.gbStartUp)

        self.buttonBox = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel)
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)
        self.buttonBox.accepted.connect(self.accept)
        self.buttonBox.rejected.connect(self.reject)
        vLayout.addWidget(self.buttonBox)


    def updateServerTopology(self, systemTopology, device=None):
        """
        This function updates the comboboxes for the servers and the plugins on
        the servers which are given by the \systemTopology.
        
        If the \config is set this configuration is set in the dialog.
        """
        serverKey = "server"
        if not systemTopology.has(serverKey):
            return
        
        serverTopology = systemTopology.get(serverKey)
        
        self.cbPlugin.clear()
        self.cbServer.clear()

        for serverId in serverTopology.keys():

            visibility = serverTopology.getAttribute(serverId, "visibility")
            if visibility > globals.GLOBAL_ACCESS_LEVEL:
                continue

            deviceClasses = None
            if serverTopology.hasAttribute(serverId, "deviceClasses"):
                deviceClasses = serverTopology.getAttribute(serverId, "deviceClasses")

            if not deviceClasses:
                continue

            visibleClasses = []
            visibilities = serverTopology.getAttribute(serverId, "visibilities")
            i = -1
            for classId in deviceClasses:
                i = i + 1
                if visibilities[i] <= globals.GLOBAL_ACCESS_LEVEL:
                    visibleClasses.append(classId)


            #self.cbServer.blockSignals(True)
            self.cbServer.addItem(serverId, visibleClasses)
            #self.cbServer.blockSignals(False)

            #for plugin in deviceClasses:
            #    index = self.cbPlugin.findText(plugin)
            #    if index > -1:
            #        data = self.cbPlugin.itemData(index)
            #        data.append(serverId)
            #        self.cbPlugin.setItemData(index, data)
            #        continue

            #    self.cbPlugin.blockSignals(True)
            #    self.cbPlugin.addItem(plugin, [serverId])
            #    self.cbPlugin.blockSignals(False)

        # No servers and therefore no plugins available?
        if self.cbServer.count() < 1:
            return False

        self.cbPlugin.adjustSize()
        self.cbServer.adjustSize()

        if device is not None:
            config = device.futureConfig
            deviceId = config.get("deviceId")
            serverId = config.get("serverId")

            self.leDeviceId.setText(deviceId)
            index = self.cbServer.findText(serverId)
            self.cbServer.setCurrentIndex(index)
            index = self.cbPlugin.findText(device.classId)
            self.cbPlugin.setCurrentIndex(index)
            index = self.cbStartUp.findText(device.ifexists)
            self.cbStartUp.setCurrentIndex(index)
        
        return True


    @property
    def deviceId(self):
        return self.leDeviceId.text()


    @property
    def classId(self):
        return self.cbPlugin.currentText()


    @property
    def serverId(self):
        return self.cbServer.currentText()


    @property
    def startupBehaviour(self):
        return self.cbStartUp.currentText()


### Slots ###
    def onDeviceIdChanged(self, text):
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(len(text) > 0)


    def onServerChanged(self, index):
        if index < 0:
            return
        # Get plugins which exist on server
        data = self.cbServer.itemData(index)
        self.cbPlugin.clear()
        for d in data:
            self.cbPlugin.blockSignals(True)
            self.cbPlugin.addItem(d)
            self.cbPlugin.blockSignals(False)


    def onPluginChanged(self, index):
        # Get servers where plugin exists
        pass
        #data = self.cbPlugin.itemData(index)
        #print "onPluginChanged", data

