#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on January 27, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""
This module contains classes of dialogs and widgets for devices.
"""

__all__ = ["DeviceDialog", "DeviceGroupDialog", "DeviceDefinitionWidget"]

import globals

from duplicatedialog import DuplicateWidget

from PyQt4.QtCore import pyqtSignal
from PyQt4.QtGui import (QCheckBox, QComboBox, QDialog, QDialogButtonBox,
                         QFormLayout, QFrame, QGroupBox, QLineEdit, QVBoxLayout,
                         QWidget)


class DeviceDialog(QDialog):
    """
    A dialog to add and edit a device.
    """
    
    def __init__(self):
        super(DeviceDialog, self).__init__()

        self.setWindowTitle("Add device")

        self.deviceWidget = DeviceDefinitionWidget()
        self.deviceWidget.signalValidDeviceId.connect(self.onValidDeviceId)
        
        vLayout = QVBoxLayout(self)
        vLayout.setContentsMargins(5,5,5,5)
        vLayout.addWidget(self.deviceWidget)
        
        self.buttonBox = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel)
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)
        self.buttonBox.accepted.connect(self.accept)
        self.buttonBox.rejected.connect(self.reject)
        vLayout.addWidget(self.buttonBox)


    def updateServerTopology(self, systemTopology, device=None):
        """
        This function broadcasts the parameters to the device widget.
        """
        return self.deviceWidget.updateServerTopology(systemTopology, device)


    @property
    def deviceId(self):
        return self.deviceWidget.deviceId


    @property
    def classId(self):
        return self.deviceWidget.classId


    @property
    def serverId(self):
        return self.deviceWidget.serverId


    @property
    def startupBehaviour(self):
        return self.deviceWidget.startupBehaviour


    def onValidDeviceId(self, deviceId):
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(len(deviceId) > 0)




class DeviceGroupDialog(QDialog):
    """
    A dialog to setup groups of devices.
    """

    def __init__(self, systemHash, serverId=None, classId=None):
        super(DeviceGroupDialog, self).__init__()

        self.setWindowTitle("Add device")

        self.deviceWidget = DeviceDefinitionWidget()
        self.deviceWidget.signalValidDeviceId.connect(self.onValidDeviceId)
        
        vLayout = QVBoxLayout(self)
        vLayout.setContentsMargins(5,5,5,5)
        vLayout.addWidget(self.deviceWidget)
        
        line = QFrame()
        line.setFrameShape(QFrame.HLine)
        vLayout.addWidget(line)
        
        self.cbDeviceGroup = QCheckBox("Add device group", self)
        self.cbDeviceGroup.toggled.connect(self.onShowDuplicateWidget)
        vLayout.addWidget(self.cbDeviceGroup)
        
        self.duplicateWidget = DuplicateWidget()
        self.duplicateWidget.signalValidInput.connect(self.onValidDuplicateWidgetInput)
        vLayout.addWidget(self.duplicateWidget)
        self.duplicateWidget.setVisible(False)
        
        self.buttonBox = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel)
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)
        self.buttonBox.accepted.connect(self.accept)
        self.buttonBox.rejected.connect(self.reject)
        vLayout.addWidget(self.buttonBox)
        
        self.updateServerTopology(systemHash)
        # Select server and plugin
        self.deviceWidget.serverId = serverId
        self.deviceWidget.classId = classId
        
        self.w = 0
        self.h = 0


    def updateServerTopology(self, systemTopology, device=None):
        """
        This function broadcasts the parameters to the device widget.
        """
        return self.deviceWidget.updateServerTopology(systemTopology, device)


    def onValidDeviceId(self, deviceId):
        if self.cbDeviceGroup.isChecked():
            newIsValid = self.duplicateWidget.count > 0 and len(deviceId) > 0
        else:
            newIsValid = len(deviceId) > 0
        
        self.duplicateWidget.deviceId = deviceId
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(newIsValid)


    def onValidDuplicateWidgetInput(self, isValid):
        newIsValid = len(self.deviceWidget.deviceId) > 0 and isValid
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(newIsValid)


    def onShowDuplicateWidget(self, on):
        if on:
            self.w = self.width()
            self.h = self.height()
        
        self.duplicateWidget.setVisible(on)
        
        if not on:
            self.setMaximumSize(self.w, self.h)
        
        # Update ok-button
        self.onValidDeviceId(self.deviceWidget.deviceId)


class DeviceDefinitionWidget(QWidget):
    """
    A widget which includes all parameters to define a device.
    """
    
    signalValidDeviceId = pyqtSignal(str)
    
    def __init__(self):
        super(DeviceDefinitionWidget, self).__init__()

        vLayout = QVBoxLayout(self)
        vLayout.setContentsMargins(0,0,0,0)

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
        # Lineedit to add a user-defined not yet available server
        self.leServer = QLineEdit()
        self.cbServer.setLineEdit(self.leServer)
        fLayout.addRow("Server:     ", self.cbServer)
        vLayout.addWidget(self.gbSelectServer)

        self.__gbSelectPlugin = QGroupBox("Select device type", self)
        fLayout = QFormLayout(self.__gbSelectPlugin)
        fLayout.setContentsMargins(5,5,5,5)
        self.cbPlugin = QComboBox()
        self.cbPlugin.setSizeAdjustPolicy(QComboBox.AdjustToContents)
        # Lineedit to add a user-defined not yet available server
        self.lePlugin = QLineEdit()
        self.cbPlugin.setLineEdit(self.lePlugin)
        fLayout.addRow("Device:    ", self.cbPlugin)
        vLayout.addWidget(self.__gbSelectPlugin)

        self.gbStartUp = QGroupBox("Select startup behaviour", self)
        fLayout = QFormLayout(self.gbStartUp)
        fLayout.setContentsMargins(5,5,5,5)
        self.cbStartUp = QComboBox()
        self.cbStartUp.addItems(["ignore", "restart"])
        self.cbStartUp.setSizeAdjustPolicy(QComboBox.AdjustToContents)
        fLayout.addRow("If already online:", self.cbStartUp)
        vLayout.addWidget(self.gbStartUp)


    def updateServerTopology(self, systemTopology, device=None):
        """
        This function updates the widget with all available servers and plugins
        of the current \systemTopology.
        
        If a \device is given the configuration of it is shown in the widget.
        
        \returns False, if no servers are available.
        """
        serverKey = "server"
        if systemTopology is None or not systemTopology.has(serverKey):
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

            self.cbServer.addItem(serverId, visibleClasses)

        # No servers and therefore no plugins available?
        if self.cbServer.count() < 1:
            return False

        self.cbPlugin.adjustSize()
        self.cbServer.adjustSize()

        if device is not None:
            self.leDeviceId.setText(device.id)
            self.serverId = device.serverId
            self.classId = device.classId
            index = self.cbStartUp.findText(device.ifexists)
            self.cbStartUp.setCurrentIndex(index)
        
        return True


    @property
    def deviceId(self):
        return self.leDeviceId.text()


    @property
    def classId(self):
        return self.cbPlugin.currentText()


    @classId.setter
    def classId(self, value):
        index = self.cbPlugin.findText(value)
        if index < 0:
            self.lePlugin.setText(value)
        else:
            self.cbPlugin.setCurrentIndex(index)


    @property
    def serverId(self):
        return self.cbServer.currentText()


    @serverId.setter
    def serverId(self, value):
        index = self.cbServer.findText(value)
        if index < 0:
            self.leServer.setText(value)
        else:
            self.cbServer.setCurrentIndex(index)


    @property
    def startupBehaviour(self):
        return self.cbStartUp.currentText()


### Slots ###
    def onDeviceIdChanged(self, text):
        self.signalValidDeviceId.emit(text)


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

