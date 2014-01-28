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
        self.__serverPluginConfig = None

        self.__twProject = QTreeWidget(self)
        self.__twProject.setHeaderLabels(["Projects"])
        self.__twProject.setContextMenuPolicy(Qt.CustomContextMenu)
        self.__twProject.customContextMenuRequested.connect(self.onCustomContextMenuRequested)
        #self.__twProject.itemSelectionChanged.connect(self.projectItemSelectionChanged)
        
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


    #def projectItemSelectionChanged(self):
    #    item = self.__twProject.currentItem()
    #    if not item: return
    #    Manager().notifier.signalProjectItemChanged.emit(dict(type=NavigationItemTypes.CLASS, key=item.data(0, const.INTERNAL_KEY)))


    def onUpdate(self, projectHash):
        self.__twProject.clear()

        # Add child items
        for k in projectHash.keys():
            # toplevel keys - project name
            item = QTreeWidgetItem(self.__twProject)
            item.setText(0, k)
            font = item.font(0)
            font.setBold(True)
            item.setFont(0, font)
            item.setIcon(0, QIcon(":folder"))
            item.setExpanded(True)

            config = projectHash.get(k)
            for c in config.keys():
                # child keys -  categories
                childItem = QTreeWidgetItem(item, [c])
                childItem.setIcon(0, QIcon(":folder"))


    def onSystemTopologyChanged(self, config):
        serverKey = "server"
        if not config.has(serverKey):
            return

        self.__serverPluginConfig = config.get(serverKey)


    def onServerConnectionChanged(self, isConnected):
        if not isConnected:
            self.__serverPluginConfig = None


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
            # Show standard context menu
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
        if not self.__serverPluginConfig or self.__serverPluginConfig.empty():
            reply = QMessageBox.question(self, "No server connection",
                                         "Do you want to establish a server connection?",
                                         QMessageBox.Yes | QMessageBox.No, QMessageBox.Yes)

            if reply == QMessageBox.No:
                return
            self.signalConnectToServer.emit()
            return

        # Show dialog to select plugin
        dialog = PluginDialog(self.__serverPluginConfig)
        if dialog.exec_() == QDialog.Rejected:
            return

        # Check all values are set

        print "dialog accepted"
        print "deviceId", dialog.deviceId
        print "plugin", dialog.plugin
        print "server", dialog.server
        #currentItem = self.__twProject.currentItem()


    # virtual function
    def onUndock(self):
        pass


    # virtual function
    def onDock(self):
        pass


