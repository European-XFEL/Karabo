#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on March 06, 2015
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


__all__ = ["MonitorDialog"]

import icons

from PyQt4 import uic
#from PyQt4.QtCore import pyqtSlot, QRegExp, Qt, QSize
from PyQt4.QtGui import (QDialog, QDialogButtonBox)

import os.path


class MonitorDialog(QDialog):


    def __init__(self, monitor):
        QDialog.__init__(self)
        uic.loadUi(os.path.join(os.path.dirname(__file__), 'monitordialog.ui'), self)
        
        self.leName.textChanged.connect(self.onChanged)
        self.leDeviceId.textChanged.connect(self.onChanged)
        self.leDeviceProperty.textChanged.connect(self.onChanged)
        
        if monitor is None:
            title = "Add monitor"
            
            self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)
        else:
            title = "Edit monitor"
            
            # Set monitor data to dialog
            self.leName.setText(monitor.name)
            config = monitor.config
            self.leDeviceId.setText(config.get("deviceId"))
            self.leDeviceProperty.setText(config.get("deviceProperty")) 
            self.leMetricPrefixSymbol.setText(config.get("metricPrefixSymbol"))
            self.leUnitSymbol.setText(config.get("unitSymbol"))
            self.leFormat.setText(config.get("format"))
        
        self.setWindowTitle(title)
        
        self.pbSelectDeviceId.setIcon(icons.deviceInstance)
        self.pbSelectDeviceId.clicked.connect(self.onSelectDeviceId)
        
        self.pbSelectProperty.setIcon(icons.editFind)
        self.pbSelectProperty.clicked.connect(self.onSelectDeviceProperty)
        self.pbSelectProperty.setDisabled(not self.leDeviceId.text())


    @property
    def name(self):
        return self.leName.text()


    @property
    def deviceId(self):
        return self.leDeviceId.text()


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
        disable = not self.leName.text() or not self.leDeviceId.text() or \
                 not self.leDeviceProperty.text()
        self.buttonBox.button(QDialogButtonBox.Ok).setDisabled(disable)


    def onSelectDeviceId(self):
        print("onSelectDeviceId")


    def onSelectDeviceProperty(self):
        print("onSelectDeviceProperty")
