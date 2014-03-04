#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 4, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""
This module contains a class which represents the treewidget of the project and
configuration panel containing the parameters of a device.
"""

__all__ = ["ProjectTree"]


from enums import NavigationItemTypes
from manager import Manager
from karabo.karathon import Hash
from dialogs.plugindialog import PluginDialog
from projectmodel import ProjectModel
from dialogs.scenedialog import SceneDialog

from PyQt4.QtCore import (pyqtSignal, QDir, Qt)
from PyQt4.QtGui import (QAction, QCursor, QDialog, QFileDialog, QIcon,
                         QInputDialog, QLineEdit, QMenu,
                         QMessageBox, QTreeView, QTreeWidgetItem)


class ProjectTree(QTreeView):

    # To import a plugin a server connection needs to be established
    signalConnectToServer = pyqtSignal()
    signalAddScene = pyqtSignal(str) # scene title
    signalItemChanged = pyqtSignal(dict)


    def __init__(self, parent=None):
        super(ProjectTree, self).__init__(parent)

        # Hash contains server/plugin topology
        self.__serverTopology = None

        # Dialog to add and change a device
        self.__pluginDialog = None

        # Set same mode for each project view
        self.setModel(Manager().projModel)
        self.expandAll()
        self.model().modelReset.connect(self.expandAll)
        self.setSelectionModel(Manager().projModel.selectionModel)
        self.selectionModel().selectionChanged.connect(self.onSelectionChanged)

        self.setContextMenuPolicy(Qt.CustomContextMenu)
        self.customContextMenuRequested.connect(self.onCustomContextMenuRequested)
        
        Manager().signalSystemTopologyChanged.connect(self.onSystemTopologyChanged)


    def _createNewProject(self, projectName, directory, overwrite=False):
        """
        This function creates a new project in the panel.
        """
        # Project name to lower case
        projectName = str(projectName).lower()

        projectConfig = Hash(ProjectModel.PROJECT_KEY)
        projectConfig.setAttribute(ProjectModel.PROJECT_KEY, "name", projectName)

        deviceKey = ProjectModel.PROJECT_KEY + "." + ProjectModel.DEVICE_KEY
        projectConfig.set(deviceKey, None)
        projectConfig.setAttribute(deviceKey, "label", ProjectModel.DEVICES_LABEL)
        sceneKey = ProjectModel.PROJECT_KEY + "." + ProjectModel.SCENES_KEY
        projectConfig.set(sceneKey, None)
        projectConfig.setAttribute(sceneKey, "label", ProjectModel.SCENES_LABEL)
        macroKey = ProjectModel.PROJECT_KEY + "." + ProjectModel.MACROS_KEY
        projectConfig.set(macroKey, None)
        projectConfig.setAttribute(macroKey, "label", ProjectModel.MACROS_LABEL)
        monitorKey = ProjectModel.PROJECT_KEY + "." + ProjectModel.MONITORS_KEY
        projectConfig.set(monitorKey, None)
        projectConfig.setAttribute(monitorKey, "label", ProjectModel.MONITORS_LABEL)
        resourceKey = ProjectModel.PROJECT_KEY + "." + ProjectModel.RESOURCES_KEY
        projectConfig.set(resourceKey, None)
        projectConfig.setAttribute(resourceKey, "label", ProjectModel.RESOURCES_LABEL)

        absoluteProjectPath = directory + "/" + projectName
        dir = QDir()
        if not QDir(absoluteProjectPath).exists():
            dir.mkpath(absoluteProjectPath)
        else:
            if not overwrite:
                reply = QMessageBox.question(self, "New project",
                    "A project folder named \"" + projectName + "\" already exists.<br>"
                    "Do you want to replace it?",
                    QMessageBox.Yes | QMessageBox.No, QMessageBox.Yes)

                if reply == QMessageBox.No:
                    return

                self._clearProjectDir(absoluteProjectPath)

        # Add subfolders
        dir.mkpath(absoluteProjectPath + "/" + deviceKey)
        dir.mkpath(absoluteProjectPath + "/" + sceneKey)
        dir.mkpath(absoluteProjectPath + "/" + macroKey)
        dir.mkpath(absoluteProjectPath + "/" + monitorKey)
        dir.mkpath(absoluteProjectPath + "/" + resourceKey)

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


    def _addScene(self, fileName, alias):
        # TODO: use correct path!!!
        projScenePath = "default_project.project.scenes"

        # Update project hash
        scenePath = projScenePath + "." + alias

        # Put info in Hash
        config = Hash("filename", fileName, "alias", alias)
        # Add device to project hash
        Manager().addSceneToProject(projScenePath, config)

        # Select added device
        self.model().selectPath(scenePath)
        
        # Send signal to mainWindow to add scene
        self.signalAddScene.emit(alias)


    def newProject(self):
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

        projectName = projectName[0]

        directory = QFileDialog.getExistingDirectory(self, "Saving location of project", \
                        "/tmp/", QFileDialog.ShowDirsOnly | QFileDialog.DontResolveSymlinks)
        if directory is None:
            return

        self._createNewProject(projectName, directory)


    def openProject(self):
        print "openProject"


    def saveProject(self):
        print "saveProject"


    def setupDefaultProject(self):
        """
        This function sets up a default project.
        So basically a new project is created and saved in the users /tmp/ folder.
        Previous data is overwritten.
        """
        self._createNewProject("default_project", "/tmp/", True)
        self._addScene("default_scene", "default_scene")


    def serverConnectionChanged(self, isConnected):
        if not isConnected:
            self.__serverTopology = None


    def mouseDoubleClickEvent(self, event):
        print "mouseDoubleClickEvent"
        index = self.selectionModel().currentIndex()
        if index is None: return
        if not index.isValid(): return

        print "path", index.data(ProjectModel.ITEM_PATH)



### slots ###
    def onCustomContextMenuRequested(self, pos):
        index = self.selectionModel().currentIndex()

        if not index.isValid():
            return

        if index.data(Qt.DisplayRole) == "Devices":
            # Show devices menu
            menu = QMenu()
            text = "Add device"
            acImportPlugin = QAction(text, self)
            acImportPlugin.setStatusTip(text)
            acImportPlugin.setToolTip(text)
            acImportPlugin.triggered.connect(self.onAddDevice)

            menu.addAction(acImportPlugin)
            menu.exec_(QCursor.pos())
        elif index.data(Qt.DisplayRole) == "Scenes":
            # Show devices menu
            menu = QMenu()
            text = "Add scene"
            acAddScene = QAction(text, self)
            acAddScene.setStatusTip(text)
            acAddScene.setToolTip(text)
            acAddScene.triggered.connect(self.onAddScene)

            menu.addAction(acAddScene)
            menu.exec_(QCursor.pos())


    def onAddDevice(self):
        if not self.__serverTopology or self.__serverTopology.empty():
            reply = QMessageBox.question(self, "No server connection",
                                         "There is no connection to the server.<br>"
                                         "Do you want to establish a server connection?",
                                         QMessageBox.Yes | QMessageBox.No, QMessageBox.Yes)

            if reply == QMessageBox.No:
                return
            self.signalConnectToServer.emit()
            return

        # Show dialog to select plugin
        self.__pluginDialog = PluginDialog()
        if not self.__pluginDialog.updateServerTopology(self.__serverTopology):
            QMessageBox.warning(self, "No servers available",
            "There are no servers available.<br>Please check, if all servers "
            "are <br>started correctly!")
            return
        if self.__pluginDialog.exec_() == QDialog.Rejected:
            return

        index = self.selectionModel().currentIndex()
        if index.isValid():
            projectIndex = index.parent()
            if projectIndex:
                # Path for device in project hash
                devicePath = projectIndex.data(Qt.DisplayRole) + ".project.device." + \
                             self.__pluginDialog.deviceId
                # Path for device configuration
                configPath = devicePath + "." + self.__pluginDialog.plugin

                # Put info in Hash
                config = Hash()
                config.set(configPath + ".deviceId", self.__pluginDialog.deviceId)
                config.set(configPath + ".serverId", self.__pluginDialog.server)
                # Add device to project hash
                Manager().addConfigToProject(config)

                # Select added device
                self.model().selectPath(devicePath)

        self.__pluginDialog = None


    def onAddScene(self):
        dialog = SceneDialog()
        if dialog.exec_() == QDialog.Rejected:
            return
        
        self._addScene(dialog.fileName, dialog.alias)


    def onSelectionChanged(self, selected, deselected):
        selectedIndexes = selected.indexes()
        if len(selectedIndexes) < 1:
            return

        index = selectedIndexes[0]

        path = index.data(ProjectModel.ITEM_PATH)
        if path is None: return
        
        serverId = index.data(ProjectModel.ITEM_SERVER_ID)
        classId = index.data(ProjectModel.ITEM_CLASS_ID)
        deviceId = index.data(Qt.DisplayRole)

        if (serverId is None) or (classId is None) or (deviceId is None):
            return

        # Get schema
        schema = Manager().getClassSchema(serverId, classId)
        
        # Check whether deviceId is already online
        if Manager().hash.has(ProjectModel.DEVICE_KEY + deviceId):
            itemInfo = dict(key=path, classId=classId, type=NavigationItemTypes.DEVICE, schema=schema)
        else:
            itemInfo = dict(key=path, classId=classId, type=NavigationItemTypes.CLASS, schema=schema)
        
        Manager().onSchemaAvailable(itemInfo)
        # Notify configurator of changes
        self.signalItemChanged.emit(itemInfo)


    def onSystemTopologyChanged(self, config):
        serverKey = "server"
        if not config.has(serverKey):
            return

        self.__serverTopology = config.get(serverKey)
        if self.__pluginDialog:
            self.__pluginDialog.updateServerTopology(self.__serverTopology)

