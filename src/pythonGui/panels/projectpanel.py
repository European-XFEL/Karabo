#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 11, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the project panel on the
   middle left of the MainWindow which is un/dockable.
"""

__all__ = ["ProjectPanel"]

from enums import NavigationItemTypes
from karabo.karathon import Hash
from manager import Manager
from plugindialog import PluginDialog

from PyQt4.QtCore import pyqtSignal, QDir, Qt
from PyQt4.QtGui import (QAction, QCursor, QDialog, QFileDialog, QIcon,
                         QInputDialog, QLineEdit, QMenu, QMessageBox, QTreeWidget,
                         QTreeWidgetItem, QVBoxLayout, QWidget)


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

    ITEM_KEY = Qt.UserRole
    ITEM_SERVER_ID = Qt.UserRole + 1
    ITEM_CLASS_ID = Qt.UserRole + 2

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


    def _createNewProject(self, projectName, directory):
        """
        This function creates a new project in the panel.
        """
        # Project name to lower case
        projectName = str(projectName).lower()

        projectConfig = Hash("project")
        projectConfig.setAttribute("project", "name", projectName)

        deviceLabel = "Devices"
        projectConfig.set("project.devices", Hash())
        projectConfig.setAttribute("project.devices", "label", deviceLabel)
        sceneLabel = "Scenes"
        projectConfig.set("project.scenes", Hash())
        projectConfig.setAttribute("project.scenes", "label", sceneLabel)
        macroLabel = "Macros"
        projectConfig.set("project.macros", Hash())
        projectConfig.setAttribute("project.macros", "label", macroLabel)
        monitorLabel = "Monitors"
        projectConfig.set("project.monitors", Hash())
        projectConfig.setAttribute("project.monitors", "label", monitorLabel)
        resourceLabel = "Resources"
        projectConfig.set("project.resources", Hash())
        projectConfig.setAttribute("project.resources", "label", resourceLabel)

        absoluteProjectPath = directory + "/" + projectName
        dir = QDir()
        if not QDir(absoluteProjectPath).exists():
            dir.mkpath(absoluteProjectPath)
        else:
            self._clearProjectDir(absoluteProjectPath)

        # Add subfolders
        dir.mkpath(absoluteProjectPath + "/" + deviceLabel)
        dir.mkpath(absoluteProjectPath + "/" + sceneLabel)
        dir.mkpath(absoluteProjectPath + "/" + macroLabel)
        dir.mkpath(absoluteProjectPath + "/" + monitorLabel)
        dir.mkpath(absoluteProjectPath + "/" + resourceLabel)

        # Send changes to manager
        Manager().addNewProject(projectName, directory, projectConfig)


    def _clearProjectDir(self, absolutePath):
        if len(absolutePath) < 1:
            return

        dirToDelete = QDir(absolutePath)
        # Remove all files from directory
        fileEntries = dirToDelete.entryList(QDir.Files | QDir.CaseSensitive)
        while len(fileEntries) > 0:
            dirToDelete.remove(fileEntries.pop())

        # Remove all sub directories
        dirEntries = dirToDelete.entryList(QDir.AllDirs | QDir.NoDotAndDotDot | QDir.CaseSensitive)
        while len(dirEntries) > 0:
            subDirPath = absolutePath + "/" + dirEntries.pop()
            subDirToDelete = QDir(subDirPath)
            if len(subDirToDelete.entryList()) > 0:
                self._clearProjectDir(subDirPath)
            subDirToDelete.rmpath(subDirPath)


### slots ###
    def onItemSelectionChanged(self):
        item = self.__twProject.currentItem()
        if not item: return

        print "onItemSelectionChanged", item.text(0)
        
        path = item.data(0, ProjectPanel.ITEM_KEY)
        if not path:
            return

        serverId = item.data(0, ProjectPanel.ITEM_SERVER_ID)
        classId = item.data(0, ProjectPanel.ITEM_CLASS_ID)
        print "serverId, classId", serverId, classId
        # Get schema
        schema = Manager().getClassSchema(serverId, classId)
        print "path", path
        print ""
        Manager().onSchemaAvailable(dict(key=path, classId=classId, type=NavigationItemTypes.CLASS, schema=schema))


    def onUpdate(self, projectHash):
        print "###################"
        print projectHash
        print ""
        self.__twProject.clear()

        # Project hash structure
        #projectName directory="directory" +
        #  project name="projectName" +
        #    devices label="Devices" +
        #    scenes label="Scenes" +
        #    macros label="Macros" +
        #    monitors label="Monitors" +
        #    resources label="Resources" +

        # Add child items
        for k in projectHash.keys():
            # Project names - toplevel items
            item = QTreeWidgetItem(self.__twProject)
            item.setText(0, k)
            font = item.font(0)
            font.setBold(True)
            item.setFont(0, font)
            item.setIcon(0, QIcon(":folder"))
            item.setExpanded(True)

            projectConfig = projectHash.get(k)
            for l in projectConfig.keys():
                # Project tag

                # Get children
                categoryConfig = projectConfig.get(l)
                for m in categoryConfig.keys():
                    # Categories - sub items
                    childItem = QTreeWidgetItem(item, [categoryConfig.getAttribute(m, "label")])
                    childItem.setIcon(0, QIcon(":folder"))
                    childItem.setExpanded(True)

                    subConfig = categoryConfig.get(m)
                    if subConfig.empty():
                        continue

                    for n in subConfig.keys():
                        leafItem = QTreeWidgetItem(childItem, [n])
                        # TODO: update icon on availability of device
                        leafItem.setIcon(0, QIcon(":device-instance"))

                        deviceId = k + "." + l + "." + m + "." + n
                        leafItem.setData(0, ProjectPanel.ITEM_KEY, deviceId)

                        classConfig = subConfig.get(n)
                        for classId in classConfig.keys():
                            serverId = classConfig.get(classId + ".serverId")
                            # Set server and class ID
                            leafItem.setData(0, ProjectPanel.ITEM_SERVER_ID, serverId)
                            leafItem.setData(0, ProjectPanel.ITEM_CLASS_ID, classId)


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

        directory = QFileDialog.getExistingDirectory(self, "Saving location of project", \
                        "/tmp/", QFileDialog.ShowDirsOnly | QFileDialog.DontResolveSymlinks)
        if not directory:
            return

        self._createNewProject(projectName[0], directory)


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
            text = "Add device"
            acImportPlugin = QAction(QIcon(":device-class"), text, None)
            acImportPlugin.setStatusTip(text)
            acImportPlugin.setToolTip(text)
            acImportPlugin.triggered.connect(self.onAddDevice)

            menu.addAction(acImportPlugin)
            menu.exec_(QCursor.pos())


    def onAddDevice(self):
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


