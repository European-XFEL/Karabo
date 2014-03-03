#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on March 3, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a dialog to add a scene.
"""

__all__ = ["SceneDialog"]


from PyQt4.QtGui import (QDialog)


class SceneDialog(QDialog):

    def __init__(self):
        super(SceneDialog, self).__init__()

        self.setWindowTitle("Add scene")

        vLayout = QVBoxLayout(self)
        vLayout.setContentsMargins(5,5,5,5)

        self.gbSelectFileName = QGroupBox("Select file name", self)
        fLayout = QFormLayout(self.gbSelectSceneFileName)
        fLayout.setContentsMargins(5,5,5,5)
        self.leFileName = QLineEdit("")
        self.leFileName.textChanged.connect(self.onTextChanged)
        fLayout.addRow("File name:", self.leDeviceId)
        vLayout.addWidget(self.gbSelectFileName)

        self.gbSelectAlias = QGroupBox("Select alias", self)
        fLayout = QFormLayout(self.gbSelectAlias)
        fLayout.setContentsMargins(5,5,5,5)
        self.leAlias = QLineEdit("")
        self.leAlias.textChanged.connect(self.onTextChanged)
        fLayout.addRow("Alias:", self.leAlias)
        vLayout.addWidget(self.gbSelectAlias)

        self.buttonBox = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel)
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)
        self.buttonBox.accepted.connect(self.accept)
        self.buttonBox.rejected.connect(self.reject)
        vLayout.addWidget(self.buttonBox)


    @property
    def fileName(self):
        return self.leFileName.text()


    @property
    def alias(self):
        return self.leAlias.text()


### Slots ###
    def onTextChanged(self, text):
        enabled = (len(self.leFileName.text()) > 0) and (len(self.leAlias.text()) > 0)
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(enabled)

