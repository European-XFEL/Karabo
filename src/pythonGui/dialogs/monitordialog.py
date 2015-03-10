#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on March 06, 2015
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


__all__ = ["MonitorDialog"]

import icons

from PyQt4 import uic
from PyQt4.QtGui import (QDialog, QDialogButtonBox, QLineEdit)

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
        for d in project.devices:
            self.cbDeviceId.addItem(d.id, d)
        
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
        print("onSelectDeviceProperty")
