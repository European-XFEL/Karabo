#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on March 06, 2015
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


__all__ = ["MonitorDialog", "DevicePropertyDialog"]

import icons
import manager
from schema import Schema

from PyQt4 import uic
from PyQt4.QtCore import Qt
from PyQt4.QtGui import (QDialog, QDialogButtonBox, QLineEdit, QTreeWidget,
                         QTreeWidgetItem, QVBoxLayout)

import os.path


class MonitorDialog(QDialog):


    def __init__(self, project, monitor):
        """
        Create a dialog to edit a \monitor object which belongs to a certain
        \project.
        """
        QDialog.__init__(self)
        uic.loadUi(os.path.join(os.path.dirname(__file__), 'monitordialog.ui'), self)
        
        # Lineedit to add a user-defined not yet available device of the project
        self.leDeviceId = QLineEdit()
        self.cbDeviceId.setLineEdit(self.leDeviceId)
        
        # Add all available devices of project
        for device in project.devices:
            if device.type in ("deviceGroup", "deviceGroupClass"):
                for d in device.devices:
                    # Descriptor needed
                    d.checkClassSchema()
                    self.cbDeviceId.addItem(d.id, d)
            else:
                # Descriptor needed
                device.checkClassSchema()
                self.cbDeviceId.addItem(device.id, device)
        
        self.leName.textChanged.connect(self.onChanged)
        self.cbDeviceId.currentIndexChanged.connect(self.onSelectDeviceId)
        self.leDeviceId.textChanged.connect(self.onSelectDeviceId)
        self.leDeviceProperty.textChanged.connect(self.onChanged)
        
        if monitor is None:
            title = "Add monitor"
        else:
            title = "Edit monitor"
            
            # Set monitor data to dialog
            self.leName.setText(monitor.name)
            config = monitor.config
            
            deviceId = config.get("deviceId")
            index = self.cbDeviceId.findText(deviceId)
            if index > 0:
                self.cbDeviceId.setCurrentIndex(index)
            else:
                self.leDeviceId.setText(deviceId)
            self.leDeviceProperty.setText(config.get("deviceProperty")) 
            self.leMetricPrefixSymbol.setText(config.get("metricPrefixSymbol"))
            self.leUnitSymbol.setText(config.get("unitSymbol"))
            self.leFormat.setText(config.get("format"))
        
        self.setWindowTitle(title)
        
        self.tbDeviceProperty.setIcon(icons.editFind)
        self.tbDeviceProperty.clicked.connect(self.onSelectDeviceProperty)
        self.onSelectDeviceId()


    @property
    def name(self):
        return self.leName.text()


    @property
    def deviceId(self):
        return self.cbDeviceId.currentText()


    @property
    def deviceProperty(self):
        return self.leDeviceProperty.text()


    @property
    def metricPrefixSymbol(self):
        return self.leMetricPrefixSymbol.text()


    @property
    def unitSymbol(self):
        return self.leUnitSymbol.text()


    @property
    def format(self):
        return self.leFormat.text()


    def onChanged(self, text):
        disable = not self.name or not self.deviceId or not self.deviceProperty
        self.buttonBox.button(QDialogButtonBox.Ok).setDisabled(disable)


    def onSelectDeviceId(self):
        self.tbDeviceProperty.setDisabled(not self.leDeviceId.text() or \
                                          self.cbDeviceId.currentIndex() < 0)
        self.onChanged(self.deviceId)


    def onSelectDeviceProperty(self):
        device = self.cbDeviceId.itemData(self.cbDeviceId.currentIndex())
        # Show dialog to select device property
        propertyDialog = DevicePropertyDialog(device)
        if propertyDialog.exec_() == QDialog.Rejected:
            return

        self.leDeviceProperty.setFocus(Qt.OtherFocusReason)
        self.leDeviceProperty.setText(propertyDialog.deviceProperty)


class DevicePropertyDialog(QDialog):


    def __init__(self, device):
        """
        Create a dialog to select a device property.
        """
        QDialog.__init__(self)
        self.setWindowTitle("Select device property")
        
        self.twDeviceProperties = QTreeWidget()
        self.twDeviceProperties.headerItem().setHidden(True)
        descr = device.descriptor
        if descr is not None:
            self._fillWidget(descr, device.boxvalue, self.twDeviceProperties.invisibleRootItem())
        self.twDeviceProperties.itemClicked.connect(self.onDevicePropertySelected)
        self.twDeviceProperties.itemDoubleClicked.connect(self.accept)
        
        vLayout = QVBoxLayout(self)
        vLayout.setContentsMargins(5,5,5,5)
        vLayout.addWidget(self.twDeviceProperties)
        
        self.buttonBox = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel)
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)
        self.buttonBox.accepted.connect(self.accept)
        self.buttonBox.rejected.connect(self.reject)
        vLayout.addWidget(self.buttonBox)


    @property
    def deviceProperty(self):
        return self.twDeviceProperties.currentItem().data(0, Qt.UserRole)


    def _fillWidget(self, descriptor, box, item):
        """
        Fill tree widget recursively with all device properties.
        """
        for k, v in descriptor.dict.items():
            childBox = getattr(box, k, None)
            if childBox is None:
                continue
            
            if childBox.isAccessible():
                childItem = QTreeWidgetItem()
                childItem.setData(0, Qt.UserRole, k)
                displayText = childBox.descriptor.displayedName
                childItem.setText(0, displayText)
                if hasattr(v, "icon"):
                    childItem.setIcon(0, v.icon)
                item.addChild(childItem)

                if isinstance(v, Schema):
                    self._fillWidget(v, childBox.boxvalue, childItem)


    def onDevicePropertySelected(self, item, column):
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(True)

