#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 11, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the project panel on the
   middle left of the MainWindow which is un/dockable.
"""

__all__ = ["ProjectPanel"]

import const

from enums import NavigationItemTypes
from karabo.karathon import Hash
from manager import Manager
from plugindialog import PluginDialog

from PyQt4.QtCore import pyqtSignal, Qt
from PyQt4.QtGui import (QAction, QCursor, QDialog, QIcon, QInputDialog, QLineEdit,
                         QMenu, QMessageBox, QTreeWidget, QTreeWidgetItem,
                         QVBoxLayout, QWidget)


class ProjectPanel(QWidget):
    ##########################################
    # Dockable widget class used in DivWidget
    # Requires following interface:
    # 
    #def setupActions(self):
    #    pass
    #def setupToolBars(self, standardToolBar, parent):
    #    pass
    #def onUndock(self):
    #    pass
    #def onDock(self):
    #    pass
    ##########################################

    # To import a plugin a server connection needs to be established
    signalConnectToServer = pyqtSignal()
    
    def __init__(self):
        super(ProjectPanel, self).__init__()
        
        title = "Projects"
        self.setWindowTitle(title)

        # Hash contains server/plugin topology
        self.__serverTopology = None

        self.__pluginDialog = None

        self.__twProject = QTreeWidget(self)
        self.__twProject.setHeaderLabels(["Projects"])
        self.__twProject.setContextMenuPolicy(Qt.CustomContextMenu)
        self.__twProject.customContextMenuRequested.connect(self.onCustomContextMenuRequested)
        self.__twProject.itemSelectionChanged.connect(self.onItemSelectionChanged)
        
        mainLayout = QVBoxLayout(self)
        mainLayout.setContentsMargins(5,5,5,5)
        mainLayout.addWidget(self.__twProject)

        self.setupActions()
        Manager().notifier.signalProjectHashChanged.connect(self.onUpdate)
        Manager().notifier.signalSystemTopologyChanged.connect(self.onSystemTopologyChanged)


    def setupActions(self):
        text = "New project"
        self.__acProjectNew = QAction(QIcon(":new"), "&New project", self)
        self.__acProjectNew.setStatusTip(text)
        self.__acProjectNew.setToolTip(text)
        self.__acProjectNew.triggered.connect(self.onProjectNew)

        text = "Open project"
        self.__acProjectOpen = QAction(QIcon(":open"), "&Open project", self)
        self.__acProjectOpen.setStatusTip(text)
        self.__acProjectOpen.setToolTip(text)
        self.__acProjectOpen.triggered.connect(self.onProjectOpen)

        text = "Save project"
        self.__acProjectSave = QAction(QIcon(":save"), "&Save project", self)
        self.__acProjectSave.setStatusTip(text)
        self.__acProjectSave.setToolTip(text)
        self.__acProjectSave.setEnabled(False)
        self.__acProjectSave.triggered.connect(self.onProjectSave)


    def setupToolBars(self, standardToolBar, parent):
        standardToolBar.addAction(self.__acProjectNew)
        standardToolBar.addAction(self.__acProjectOpen)
        standardToolBar.addAction(self.__acProjectSave)


    def onItemSelectionChanged(self):
        print "onItemSelectionChanged"
        item = self.__twProject.currentItem()
        if not item: return

        # Get schema
        #Manager().notifier.signalProjectItemChanged.emit(dict(type=NavigationItemTypes.CLASS, key=item.data(0, const.INTERNAL_KEY)))


    def onUpdate(self, projectHash):
        print projectHash
        self.__twProject.clear()

        # Add child items
        for k in projectHash.keys():
            # toplevel keys - project names
            item = QTreeWidgetItem(self.__twProject)
            item.setText(0, k)
            font = item.font(0)
            font.setBold(True)
            item.setFont(0, font)
            item.setIcon(0, QIcon(":folder"))
            item.setExpanded(True)

            config = projectHash.get(k)
            for c in config.keys():
                # sub keys -  categories
                childItem = QTreeWidgetItem(item, [c])
                childItem.setIcon(0, QIcon(":folder"))
                childItem.setExpanded(True)
                
                leafKey = k + "." + c
                subConfig = projectHash.get(leafKey)
                for l in subConfig.keys():
                    leafItem = QTreeWidgetItem(childItem, [l])
                    leafItem.setIcon(0, QIcon(":device-instance"))


    def onSystemTopologyChanged(self, config):
        serverKey = "server"
        if not config.has(serverKey):
            return

        self.__serverTopology = config.get(serverKey)
        if self.__pluginDialog:
            self.__pluginDialog.updateServerTopology(self.__serverTopology)


    def onServerConnectionChanged(self, isConnected):
        if not isConnected:
            self.__serverTopology = None


    def _createNewProject(self, projectName):
        """
        This function creates a new project in the panel.
        """
        # Project name to lower case
        projectName = str(projectName).lower()

        projectConfig = Hash()
        projectConfig.set("Devices", Hash())
        projectConfig.set("Scenes", Hash())
        projectConfig.set("Macros", Hash())
        projectConfig.set("Monitors", Hash())
        projectConfig.set("Resources", Hash())

        # Send changes to manager
        Manager().addNewProject(projectName, projectConfig)


### Slots ###
    def onProjectNew(self):
        projectName = QInputDialog.getText(self, "New project", \
                                           "Enter project name:", QLineEdit.Normal, "")

        if not projectName[1]:
            return

        if len(projectName[0]) < 1:
            reply = QMessageBox.question(self, "Project name", "Please enter a name!",
                QMessageBox.Ok | QMessageBox.Cancel, QMessageBox.Ok)

            if reply == QMessageBox.Cancel:
                return

            # Call function again
            self.onProjectNew()
            return

        self._createNewProject(projectName[0])


    def onProjectOpen(self):
        print "onProjectOpen"


    def onProjectSave(self):
        print "onProjectSave"


    def onCustomContextMenuRequested(self, pos):
        item = self.__twProject.itemAt(pos)
        if item is None:
            return

        if item.text(0) == "Devices":
            # Show devices menu
            menu = QMenu()
            text = "Import plugin"
            acImportPlugin = QAction(QIcon(":device-class"), text, None)
            acImportPlugin.setStatusTip(text)
            acImportPlugin.setToolTip(text)
            acImportPlugin.triggered.connect(self.onImportPlugin)

            menu.addAction(acImportPlugin)
            menu.exec_(QCursor.pos())


    def onImportPlugin(self):
        if not self.__serverTopology or self.__serverTopology.empty():
            reply = QMessageBox.question(self, "No server connection",
                                         "Do you want to establish a server connection?",
                                         QMessageBox.Yes | QMessageBox.No, QMessageBox.Yes)

            if reply == QMessageBox.No:
                return
            self.signalConnectToServer.emit()

        # Show dialog to select plugin
        self.__pluginDialog = PluginDialog(self.__serverTopology)
        if self.__pluginDialog.exec_() == QDialog.Rejected:
            return

        currentItem = self.__twProject.currentItem()
        if currentItem:
            projectItem = currentItem.parent()
            if projectItem:
                # Add device to project hash
                Manager().addDeviceToProject(projectItem.text(0),
                                             self.__pluginDialog.deviceId,
                                             self.__pluginDialog.plugin,
                                             self.__pluginDialog.server)

        self.__pluginDialog = None


    # virtual function
    def onUndock(self):
        pass


    # virtual function
    def onDock(self):
        pass


