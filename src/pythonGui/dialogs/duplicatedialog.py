#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 1, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""
This module contains a class which represents the dialog to duplicate devices.
"""

__all__ = ["DuplicateDialog", "DuplicateWidget"]


import globals

from PyQt4.QtCore import pyqtSignal
from PyQt4.QtGui import (QDialog, QDialogButtonBox, QFormLayout, QGroupBox,
                         QLineEdit, QSpinBox, QVBoxLayout, QWidget)


class DuplicateDialog(QDialog):

    def __init__(self, name):
        super(DuplicateDialog, self).__init__()

        self.setWindowTitle("Duplicate")
        
        self.duplicateWidget = DuplicateWidget(name)
        self.duplicateWidget.signalValidInput.connect(self.onValidInput)
        
        vLayout = QVBoxLayout(self)
        vLayout.setContentsMargins(5,5,5,5)
        vLayout.addWidget(self.duplicateWidget)
        
        self.buttonBox = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel)
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)
        self.buttonBox.accepted.connect(self.accept)
        self.buttonBox.rejected.connect(self.reject)
        vLayout.addWidget(self.buttonBox)


    @property
    def displayPrefix(self):
        return self.duplicateWidget.displayPrefix


    @property
    def startIndex(self):
        return self.duplicateWidget.startIndex


    @property
    def count(self):
        return self.duplicateWidget.count


    def onValidInput(self, isValid):
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(isValid)




class DuplicateWidget(QWidget):
    """
    A widget which includes all parameters to duplicate an object.
    """

    signalValidInput = pyqtSignal(bool)
    
    def __init__(self, name=""):
        super(DuplicateWidget, self).__init__()
        
        vLayout = QVBoxLayout(self)
        vLayout.setContentsMargins(0,0,0,0)
        
        self.gbSelectPrefix = QGroupBox("Select prefix", self)
        fLayout = QFormLayout(self.gbSelectPrefix)
        fLayout.setContentsMargins(5,5,5,5)
        self.leDisplayPrefix = QLineEdit()
        self.leDisplayPrefix.textChanged.connect(self.onChanged)
        fLayout.addRow("Prefix:&nbsp;&nbsp;&nbsp;<i>{}</i>".format(name), self.leDisplayPrefix)
        vLayout.addWidget(self.gbSelectPrefix)
        
        self.gbSelectStartIndex = QGroupBox("Select start", self)
        fLayout = QFormLayout(self.gbSelectStartIndex)
        fLayout.setContentsMargins(5,5,5,5)
        self.sbStartIndex = QSpinBox()
        self.sbStartIndex.setRange(0, globals.MAX_INT32)
        self.sbStartIndex.valueChanged.connect(self.onChanged)
        fLayout.addRow("Start index:", self.sbStartIndex)
        vLayout.addWidget(self.gbSelectStartIndex)
        
        self.gbSelectCount = QGroupBox("Select count", self)
        fLayout = QFormLayout(self.gbSelectCount)
        fLayout.setContentsMargins(5,5,5,5)
        self.sbCount = QSpinBox()
        self.sbCount.setRange(0, globals.MAX_INT32)
        self.sbCount.valueChanged.connect(self.onChanged)
        fLayout.addRow("Count:        ", self.sbCount)
        vLayout.addWidget(self.gbSelectCount)


    @property
    def displayPrefix(self):
        return self.leDisplayPrefix.text()


    @property
    def startIndex(self):
        return self.sbStartIndex.value()


    @property
    def count(self):
        return self.sbCount.value()


    def onChanged(self):
        """
        Called whenever something changes in the dialog to update the ok-button.
        """
        isValid = self.sbStartIndex.value() >=0 and self.sbCount.value() > 0
        self.signalValidInput.emit(isValid)



