#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 1, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""
This module contains a class which represents the dialog to duplicate devices.
"""

__all__ = ["DuplicateDialog", "DuplicateWidget"]


import karabo_gui.globals as globals

from PyQt4.QtCore import pyqtSignal
from PyQt4.QtGui import (QDialog, QDialogButtonBox, QFormLayout, QGroupBox,
                         QHBoxLayout, QLabel, QLineEdit, QSpinBox, QVBoxLayout,
                         QWidget)


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
    def endIndex(self):
        return self.duplicateWidget.endIndex


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
        
        self.leDisplayPrefix = QLineEdit(name)
        self.leDisplayPrefix.textChanged.connect(self.onChanged)
        prefixLayout = QHBoxLayout()
        prefixLayout.addWidget(self.leDisplayPrefix)
        fLayout.addRow("Prefix:", prefixLayout)
        vLayout.addWidget(self.gbSelectPrefix)
        
        self.gbSelectStartIndex = QGroupBox("Select start", self)
        fLayout = QFormLayout(self.gbSelectStartIndex)
        fLayout.setContentsMargins(5,5,5,5)
        self.sbStartIndex = QSpinBox()
        self.sbStartIndex.setRange(0, globals.MAX_INT32)
        self.sbStartIndex.valueChanged.connect(self.onChanged)
        fLayout.addRow("Start index:", self.sbStartIndex)
        vLayout.addWidget(self.gbSelectStartIndex)
        
        self.gbSelectEndIndex = QGroupBox("Select end", self)
        fLayout = QFormLayout(self.gbSelectEndIndex)
        fLayout.setContentsMargins(5,5,5,5)
        self.sbEndIndex = QSpinBox()
        self.sbEndIndex.setRange(0, globals.MAX_INT32)
        self.sbEndIndex.valueChanged.connect(self.onChanged)
        fLayout.addRow("End index:  ", self.sbEndIndex)
        vLayout.addWidget(self.gbSelectEndIndex)
        
        self.laCountText = QLabel()
        self.updateCountText()
        vLayout.addWidget(self.laCountText)


    @property
    def displayPrefix(self):
        return self.leDisplayPrefix.text()


    @displayPrefix.setter
    def displayPrefix(self, text):
        self.leDisplayPrefix.setText(text)


    @property
    def startIndex(self):
        return self.sbStartIndex.value()


    @startIndex.setter
    def startIndex(self, index):
        self.sbStartIndex.setValue(index)


    @property
    def endIndex(self):
        return self.sbEndIndex.value()


    @endIndex.setter
    def endIndex(self, index):
        self.sbEndIndex.setValue(index)


    def nbDevices(self):
        if self.endIndex > self.startIndex:
            return self.endIndex - self.startIndex + 1
        else:
            return 0


    def updateCountText(self):
        self.laCountText.setText("You are about to create <b>{}</b> devices".format(self.nbDevices()))


    def onChanged(self):
        """
        Called whenever something changes in the dialog to update the ok-button.
        """
        isValid = self.endIndex > self.startIndex
        self.signalValidInput.emit(isValid)
        
        if not isValid:
            self.endIndex = self.startIndex

        self.updateCountText()


