#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 21, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains dialog classes which allow the loading and saving of
configurations."""

__all__ = ["SelectProjectDialog"]


from PyQt4.QtCore import Qt
from PyQt4.QtGui import (QDialog, QDialogButtonBox, QFormLayout, QLineEdit,
                         QListWidget, QListWidgetItem, QVBoxLayout)


class SelectProjectDialog(QDialog):

    def __init__(self, name, projects):
        """
        The constructor expects a list of projects.
        """
        super(SelectProjectDialog, self).__init__()

        self.setWindowTitle("Select name and project")
        
        vLayout = QVBoxLayout(self)
        
        formLayout = QFormLayout()
        self.leName = QLineEdit()
        self.leName.setToolTip("Enter configuration name")
        if name is not None:
            self.leName.setText(name)
        self.leName.textChanged.connect(self.onNameChanged)
        
        formLayout.addRow("Configurationname: ", self.leName)
        vLayout.addLayout(formLayout)
        
        self.projectWidget = QListWidget(self)
        for p in projects:
            item = QListWidgetItem(p.name)
            item.setData(Qt.UserRole, p)
            self.projectWidget.addItem(item)
        self.projectWidget.itemClicked.connect(self.onProjectSelectionChanged)
        
        vLayout.addWidget(self.projectWidget)
        
        self.buttonBox = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel)
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)
        self.buttonBox.accepted.connect(self.accept)
        self.buttonBox.rejected.connect(self.reject)
        vLayout.addWidget(self.buttonBox)


    def selectedProject(self):
        """
        Returns the selected project.
        """
        item = self.projectWidget.currentItem()
        if item is None:
            return None
        return item.data(Qt.UserRole)


    def configurationName(self):
        return self.leName.text()


    def enableOkButton(self):
        self.buttonBox.button(QDialogButtonBox.Ok) \
                      .setEnabled(len(self.leName.text()) > 0 and \
                                  len(self.projectWidget.selectedItems()) > 0)


    def onNameChanged(self, name):
        self.enableOkButton()


    def onProjectSelectionChanged(self, item):
        self.enableOkButton()

