#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on March 3, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a dialog to add a scene.
"""

__all__ = ["SceneDialog"]


from PyQt4.QtCore import QFileInfo
from PyQt4.QtGui import (QDialog, QDialogButtonBox, QFormLayout, QLineEdit,
                         QVBoxLayout)

class SceneDialog(QDialog):

    def __init__(self, scene):
        super(SceneDialog, self).__init__()

        self.setWindowTitle("Add scene")

        vLayout = QVBoxLayout(self)
        vLayout.setContentsMargins(5,5,5,5)

        fLayout = QFormLayout()
        fLayout.setContentsMargins(5,5,5,5)
        self.leSceneName = QLineEdit("")
        self.leSceneName.textChanged.connect(self.onTextChanged)
        fLayout.addRow("Enter scene name:", self.leSceneName)
        vLayout.addLayout(fLayout)

        self.buttonBox = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel)
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)
        self.buttonBox.accepted.connect(self.accept)
        self.buttonBox.rejected.connect(self.reject)
        vLayout.addWidget(self.buttonBox)

        if scene is not None:
            self.leSceneName.setText(scene.filename)

    def sceneName(self):
        """
        The name of the scene is returned including the suffix .svg
        """
        text = self.leSceneName.text()
        fi = QFileInfo(text)
        if len(fi.suffix()) < 1:
            text = "{}.svg".format(text)
        return text

### Slots ###
    def onTextChanged(self, text):
        enabled = len(self.leSceneName.text()) > 0
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(enabled)

