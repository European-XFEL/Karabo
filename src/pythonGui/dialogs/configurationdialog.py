#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 21, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains dialog classes which allow the loading and saving of
configurations."""

__all__ = ["SelectProjectDialog", "SelectProjectConfigurationDialog",
           "SelectMultipleProjectConfigurationDialog"]


from karabo.project import ProjectConfiguration

from PyQt4.QtCore import pyqtSignal, Qt
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
        self.projectWidget.setCurrentRow(0)
        
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
            item = QTreeWidgetItem(self.projConfWidget, [p.name])
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


class SelectMultipleProjectConfigurationDialog(QDialog):
    """
    Select configurations of a project and apply them to the specific devices.
    """
    
    def __init__(self, project):
        super(SelectMultipleProjectConfigurationDialog, self).__init__()

        self.setWindowTitle("Select configurations")
        
        self.projConfWidget = CheckableTreeWidget(self)
        self.projConfWidget.headerItem().setHidden(True)
        self._populate(project)
        self.projConfWidget.itemChecked.connect(self.onHandleItemChecked)
        
        vLayout = QVBoxLayout(self)
        vLayout.addWidget(self.projConfWidget)
        
        self.buttonBox = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel)
        self.buttonBox.accepted.connect(self.accept)
        self.buttonBox.rejected.connect(self.reject)
        vLayout.addWidget(self.buttonBox)


    def _populate(self, project):
        for deviceId, configList in project.configurations.items():
            deviceItem = QTreeWidgetItem(self.projConfWidget, [deviceId])
            self.projConfWidget.addTopLevelItem(deviceItem)
            deviceItem.setExpanded(True)

            for index, c in enumerate(configList):
                confItem = CheckableTreeWidgetItem([c.filename])
                confItem.setData(0, Qt.UserRole, c)
                deviceItem.addChild(confItem)
                confItem.setExpanded(True)
                
                if index == 0:
                    # First element selected
                    confItem.setCheckState(0, Qt.Checked)
                else:
                    confItem.setCheckState(0, Qt.Unchecked)


    def projectConfigurations(self):
        """
        This function returns all selected project configurations associated to
        a deviceId.
        """
        configurations = {}
        for i in range(self.projConfWidget.topLevelItemCount()):
            item = self.projConfWidget.topLevelItem(i)
            
            for j in range(item.childCount()):
                childItem = item.child(j)
                if childItem.checkState(0) == Qt.Checked:
                    configurations[item.text(0)] = childItem.data(0, Qt.UserRole)

        return configurations


    def onHandleItemChecked(self, item, column):
        siblings = item.siblings()
        if item.checkState(column) == Qt.Checked:
            # Uncheck all siblings
            for s in siblings:
                self.projConfWidget.blockSignals(True)
                s.setCheckState(0, Qt.Unchecked)
                self.projConfWidget.blockSignals(False)
        else:
            if siblings: item.setCheckState(0, Qt.Checked)


class CheckableTreeWidget(QTreeWidget):
    itemChecked = pyqtSignal(object, int)

    def __init__(self, parent):
        QTreeWidget.__init__(self, parent)


class CheckableTreeWidgetItem(QTreeWidgetItem):
    
    def __init__(self, strings):
        QTreeWidgetItem.__init__(self, strings)
        self.setFlags(self.flags() | Qt.ItemIsUserCheckable)


    def setData(self, column, role, value):
        state = self.checkState(column)
        QTreeWidgetItem.setData(self, column, role, value)
        
        if role == Qt.CheckStateRole and state != self.checkState(column):
            treewidget = self.treeWidget()
            if treewidget is not None:
                treewidget.itemChecked.emit(self, column)


    def siblings(self):
        """
        This function returns a list of all siblings of this item.
        """
        siblings = []
        parent = self.parent()
        if parent is not None:
            for i in range(parent.childCount()):
                childItem = parent.child(i)
                if childItem != self:
                    siblings.append(childItem)
        else:
            parent = self.treeWidget()
            for i in range(parent.topLevelItemCount()):
                childItem = parent.topLevelItem(i)
                if childItem != self:
                    siblings.append(childItem)
        
        return siblings        
