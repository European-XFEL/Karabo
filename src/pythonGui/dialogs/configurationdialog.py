#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 21, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains dialog classes which allow the loading and saving of
configurations."""

__all__ = ["SelectProjectDialog"]


from PyQt4.QtCore import Qt
from PyQt4.QtGui import (QDialog, QDialogButtonBox, QListWidget, QListWidgetItem,
                         QVBoxLayout)


class SelectProjectDialog(QDialog):

    def __init__(self, projects):
        """
        The constructor expects a list of projects.
        """
        super(SelectProjectDialog, self).__init__()

        self.setWindowTitle("Select project")
        
        self.projectWidget = QListWidget(self)
        for p in projects:
            item = QListWidgetItem(p.name)
            item.setData(Qt.UserRole, p)
            self.projectWidget.addItem(item)
        self.projectWidget.itemClicked.connect(self.onProjectSelectionChanged)
            
        vLayout = QVBoxLayout(self)
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


    def onProjectSelectionChanged(self, item):
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(item is not None)

