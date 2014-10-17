#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 21, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains dialog classes which allow the loading and saving of
configurations."""

__all__ = ["SelectProjectDialog", "SelectProjectConfigurationDialog"]


from karabo.project import ProjectConfiguration

from PyQt4.QtCore import Qt
from PyQt4.QtGui import (QDialog, QDialogButtonBox, QFormLayout, QLineEdit,
                         QListWidget, QListWidgetItem, QTreeWidget,
                         QTreeWidgetItem, QVBoxLayout)


class SelectProjectDialog(QDialog):
    """
    Select a name for the configuration and a project.
    """
    def __init__(self, projects):
        """
        The constructor expects a default name for the configuration and a list
        of projects.
        """
        super(SelectProjectDialog, self).__init__()

        self.setWindowTitle("Select name and project")
        
        formLayout = QFormLayout()
        self.leName = QLineEdit()
        self.leName.setToolTip("Enter configuration name")
        self.leName.textChanged.connect(self.onNameChanged)
        
        formLayout.addRow("Configuration name: ", self.leName)
        
        vLayout = QVBoxLayout(self)
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



class SelectProjectConfigurationDialog(QDialog):
    """
    Select a project configuration.
    """
    def __init__(self, projects):
        """
        The constructor expects a list of projects.
        """
        super(SelectProjectConfigurationDialog, self).__init__()

        self.setWindowTitle("Select project configuration")
        
        self.projConfWidget = QTreeWidget(self)
        self.projConfWidget.headerItem().setHidden(True)
        self._populate(projects)
        self.projConfWidget.itemClicked.connect(self.onProjConfSelectionChanged)
        
        vLayout = QVBoxLayout(self)
        vLayout.addWidget(self.projConfWidget)
        
        self.buttonBox = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel)
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)
        self.buttonBox.accepted.connect(self.accept)
        self.buttonBox.rejected.connect(self.reject)
        vLayout.addWidget(self.buttonBox)


    def _populate(self, projects):
        for p in projects:
            item = QTreeWidgetItem([p.name])
            item.setData(0, Qt.UserRole, p)
            self.projConfWidget.addTopLevelItem(item)
            item.setExpanded(True)
            
            for deviceId, configList in p.configurations.items():
                deviceItem = QTreeWidgetItem([deviceId])
                item.addChild(deviceItem)
                deviceItem.setExpanded(True)
                
                for c in configList:
                    confItem = QTreeWidgetItem([c.filename])
                    confItem.setData(0, Qt.UserRole, c)
                    deviceItem.addChild(confItem)
                    confItem.setExpanded(True)


    def projectConfiguration(self):
        return self.projConfWidget.currentItem().data(0, Qt.UserRole)


    def onProjConfSelectionChanged(self, item):
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled((item is not None) \
                and isinstance(item.data(0, Qt.UserRole), ProjectConfiguration))

