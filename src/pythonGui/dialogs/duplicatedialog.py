#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 1, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""
This module contains a class which represents the dialog to duplicate devices.
"""

__all__ = ["DuplicateDialog"]


import globals
from util import SignalBlocker

from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import (QDialog, QDialogButtonBox, QFormLayout, QLineEdit, 
                         QSpinBox, QVBoxLayout)


class DuplicateDialog(QDialog):

    def __init__(self, name):
        super(DuplicateDialog, self).__init__()

        self.setWindowTitle("Duplicate")
        
        vLayout = QVBoxLayout(self)
        
        fLayout = QFormLayout()
        fLayout.setContentsMargins(5,5,5,5)
        
        self.leDisplayPrefix = QLineEdit(name)
        self.leDisplayPrefix.textChanged.connect(self.onChanged)
        fLayout.addRow("Prefix:", self.leDisplayPrefix)
        
        self.sbStartIndex = QSpinBox()
        self.sbStartIndex.setRange(0, globals.MAX_INT32)
        self.sbStartIndex.valueChanged.connect(self.onChanged)
        fLayout.addRow("Start:", self.sbStartIndex)
        
        self.sbCount = QSpinBox()
        self.sbCount.setRange(0, globals.MAX_INT32)
        self.sbCount.valueChanged.connect(self.onChanged)
        fLayout.addRow("Count:", self.sbCount)
        vLayout.addLayout(fLayout)
        
        self.buttonBox = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel)
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)
        self.buttonBox.accepted.connect(self.accept)
        self.buttonBox.rejected.connect(self.reject)
        vLayout.addWidget(self.buttonBox)


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
        enabled = len(self.leDisplayPrefix.text()) > 0 \
                  and self.sbStartIndex.value() >=0 and self.sbCount.value() > 0
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(enabled)

