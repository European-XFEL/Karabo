#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 4, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""
This module contains a class which represents the treewidget of the project and
configuration panel containing the parameters of a device.
"""

__all__ = ["ProjectTreeView"]


from copy import copy
from enums import NavigationItemTypes
from manager import Manager
from karabo.karathon import Hash, HashMergePolicy, loadFromFile, saveToFile
from dialogs.plugindialog import PluginDialog
from projectmodel import ProjectModel
from dialogs.scenedialog import SceneDialog

from PyQt4.QtCore import (pyqtSignal, QDir, QFile, QFileInfo, QIODevice, Qt)
from PyQt4.QtGui import (QAction, QCursor, QDialog, QFileDialog, QIcon,
                         QInputDialog, QLineEdit, QMenu,
                         QMessageBox, QTreeView, QTreeWidgetItem)


class ProjectTreeView(QTreeView):

    # To import a plugin a server connection needs to be established
    signalConnectToServer = pyqtSignal()
    signalAddScene = pyqtSignal(str) # scene title
    signalItemChanged = pyqtSignal(dict)


    def __init__(self, parent=None):
        super(ProjectTreeView, self).__init__(parent)

        # Hash contains server topology of the live system
        self.serverTopology = None

        # Dialog to add and change a device
        self.pluginDialog = None

        # Set same mode for each project view
        self.setModel(Manager().projectTopology)
        self.expandAll()
        self.model().modelReset.connect(self.expandAll)
        self.setSelectionModel(Manager().projectTopology.selectionModel)
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

        deviceKey = ProjectModel.PROJECT_KEY + "." + ProjectModel.DEVICES_KEY
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
        dir.mkpath(absoluteProjectPath + "/" + ProjectModel.DEVICES_LABEL)
        dir.mkpath(absoluteProjectPath + "/" + ProjectModel.SCENES_LABEL)
        dir.mkpath(absoluteProjectPath + "/" + ProjectModel.MACROS_LABEL)
        dir.mkpath(absoluteProjectPath + "/" + ProjectModel.MONITORS_LABEL)
        dir.mkpath(absoluteProjectPath + "/" + ProjectModel.RESOURCES_LABEL)

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


    def _addDevice(self):
        projectName = self._currentProjectName()
        if projectName is None: return
        
        # Path for device in project hash
        devicePath = projectName + "." + ProjectModel.PROJECT_KEY + "." + \
                     ProjectModel.DEVICES_KEY + "." + self.pluginDialog.deviceId
        # Path for device configuration
        configPath = devicePath + "." + self.pluginDialog.plugin

        # Put info in Hash
        config = Hash()
        config.set(configPath + ".deviceId", self.pluginDialog.deviceId)
        config.set(configPath + ".serverId", self.pluginDialog.server)
        # Add device to project hash
        Manager().addConfigToProject(config)

        # Select added device
        self._selectPath(devicePath)


    def _addScene(self, projectName, fileName, alias):
        projScenePath = projectName + ".project.scenes"

        # Update project hash
        scenePath = projScenePath + "." + alias

        # Put info in Hash
        config = Hash("filename", fileName, "alias", alias)
        # Add device to project hash
        Manager().addSceneToProject(projScenePath, config)

        # Select added device
        self._selectPath(scenePath)
        
        # Send signal to mainWindow to add scene
        self.signalAddScene.emit(alias)


    def _selectPath(self, path):
        """
        The item with the given \path is selected.
        """
        self.model().selectPath(path)
        

    def _currentProjectName(self):
        """
        This function returns the current project name, if a category like
        Devices, Scenes etc. is selected.
        
        It returns None, if no index can be found.
        """
        index = self.selectionModel().currentIndex()
        if not index.isValid():
            return None
        
        projectIndex = index.parent()
        if projectIndex is None:
            return None
        
        return projectIndex.data(Qt.DisplayRole)
        


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
        filename = QFileDialog.getOpenFileName(None, "Open saved project", \
                                               QDir.tempPath(), "XML (*.xml)")
        if len(filename) < 1:
            return
        
        file = QFile(filename)
        if file.open(QIODevice.ReadOnly | QIODevice.Text) == False:
            return
        
        projectConfig = loadFromFile(str(filename))
        # TODO: this function merges the loaded hash into the current project hash
        # consider projectName to overwrite path
        Manager().addConfigToProject(projectConfig)


    def saveProject(self):
        filename = QFileDialog.getSaveFileName(None, "Save project as", QDir.tempPath(), "XML (*.xml)")
        if len(filename) < 1:
            return
        
        fi = QFileInfo(filename)
        if len(fi.suffix()) < 1:
            filename += ".xml"
        
        # TODO: save selected project
        name = self._currentProjectName()
        if name is None:
            return
        
        projectConfig = Manager().projectHash.get(name)
        saveToFile(projectConfig, filename)


    def setupDefaultProject(self):
        """
        This function sets up a default project.
        So basically a new project is created and saved in the users /tmp/ folder.
        Previous data is overwritten.
        """
        projectName = "default_project"
        sceneName = "default_scene"
        
        self._createNewProject(projectName, "/tmp/", True)
        self._addScene(projectName, sceneName, sceneName)


    def serverConnectionChanged(self, isConnected):
        if not isConnected:
            self.serverTopology = None


    def mouseDoubleClickEvent(self, event):
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
        if self.serverTopology is None:
            reply = QMessageBox.question(self, "No server connection",
                                         "There is no connection to the server.<br>"
                                         "Do you want to establish a server connection?",
                                         QMessageBox.Yes | QMessageBox.No, QMessageBox.Yes)

            if reply == QMessageBox.No:
                return
            self.signalConnectToServer.emit()
            return

        # Show dialog to select plugin
        self.pluginDialog = PluginDialog()
        if not self.pluginDialog.updateServerTopology(self.serverTopology):
            QMessageBox.warning(self, "No servers available",
            "There are no servers available.<br>Please check, if all servers "
            "are <br>started correctly!")
            return
        if self.pluginDialog.exec_() == QDialog.Rejected:
            return

        self._addDevice()
        self.pluginDialog = None


    def onAddScene(self):
        dialog = SceneDialog()
        if dialog.exec_() == QDialog.Rejected:
            return
        
        # Get project name
        projectName = self._currentProjectName()
        self._addScene(projectName, dialog.fileName, dialog.alias)


    def onSelectionChanged(self, selected, deselected):
        selectedIndexes = selected.indexes()
        if len(selectedIndexes) < 1:
            # TODO: update load/save buttons in projectPanel toolbar
            return

        index = selectedIndexes[0]

        path = index.data(ProjectModel.ITEM_PATH)
        if path is None: return
        
        serverId = index.data(ProjectModel.ITEM_SERVER_ID)
        classId = index.data(ProjectModel.ITEM_CLASS_ID)
        deviceId = index.data(Qt.DisplayRole)

        if (serverId is None) or (classId is None) or (deviceId is None):
            return
        
        # Check whether deviceId is already online
        if Manager().systemTopology.has(deviceId):
            # Get schema
            schema = Manager().getDeviceSchema(deviceId)
            itemInfo = dict(key=deviceId, classId=classId, \
                            type=NavigationItemTypes.DEVICE, schema=schema)
        else:
            # Get schema
            schema = Manager().getClassSchema(serverId, classId)
            # Set path which is used to get class schema
            naviPath = "{}.{}".format(serverId, classId)
            itemInfo = dict(key=path, projNaviPathTuple=(naviPath, path),
                            classId=classId, type=NavigationItemTypes.CLASS, \
                            schema=schema)
        
        Manager().onSchemaAvailable(itemInfo)
        # Notify configurator of changes
        self.signalItemChanged.emit(itemInfo)


    def onSystemTopologyChanged(self, config):
        serverKey = "server"
        if not config.has(serverKey):
            return

        # Create copy of nested hash - TODO: remove when hash in native python
        self.serverTopology = copy(config.get(serverKey))
        
        if self.pluginDialog is not None:
            self.pluginDialog.updateServerTopology(self.serverTopology)
        
        # TODO: Update on/offline status devices in tree

