#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 20, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""
This module contains a class which represents a model to display projects in
a treeview.
"""

__all__ = ["ProjectModel"]


from copy import copy
from enums import NavigationItemTypes
import manager
from karabo.karathon import (Hash, HashMergePolicy, loadFromFile, saveToFile,
                             VectorHash)
from dialogs.plugindialog import PluginDialog
from dialogs.scenedialog import SceneDialog

from PyQt4.QtCore import pyqtSignal, QDir, Qt
from PyQt4.QtGui import (QDialog, QIcon, QItemSelectionModel, QMessageBox,
                         QStandardItem, QStandardItemModel)


class ProjectModel(QStandardItemModel):
    # To import a plugin a server connection needs to be established
    signalConnectToServer = pyqtSignal()
    signalAddScene = pyqtSignal(str) # scene title
    signalItemChanged = pyqtSignal(dict)
    signalSelectionChanged = pyqtSignal(list)

    ITEM_PATH = Qt.UserRole
    ITEM_CATEGORY = Qt.UserRole + 1
    ITEM_SERVER_ID = Qt.UserRole + 2
    ITEM_CLASS_ID = Qt.UserRole + 3

    PROJECT_KEY = "project"

    DEVICES_KEY = "devices"
    SCENES_KEY = "scenes"
    MACROS_KEY = "macros"
    MONITORS_KEY = "monitors"
    RESOURCES_KEY = "resources"
    CONFIGURATIONS_KEY = "configurations"

    DEVICES_LABEL = "Devices"
    SCENES_LABEL = "Scenes"
    MACROS_LABEL = "Macros"
    MONITORS_LABEL = "Monitors"
    RESOURCES_LABEL = "Resources"
    CONFIGURATIONS_LABEL = "Configurations"


    def __init__(self, parent=None):
        super(ProjectModel, self).__init__(parent)
        
        # Hash stores current system topology
        self.systemTopology = None
        # Hash stores project tree information
        self.projectHash = Hash()
        
        # Dialog to add and change a device
        self.pluginDialog = None
        
        self.setHorizontalHeaderLabels(["Projects"])
        self.selectionModel = QItemSelectionModel(self)
        self.selectionModel.selectionChanged.connect(self.onSelectionChanged)


    def _handleLeafItems(self, childItem, projectPath, categoryKey, config):
        if (config is None) or config.empty():
            return

        if categoryKey.startswith(ProjectModel.SCENES_KEY):
            #fileName = config.get("filename")
            alias = config.get("alias")

            leafItem = QStandardItem(alias)
            leafItem.setEditable(False)
            leafPath = projectPath + "." + categoryKey #+ "." + alias
            leafItem.setData(leafPath, ProjectModel.ITEM_PATH)
            leafItem.setData(ProjectModel.SCENES_KEY, ProjectModel.ITEM_CATEGORY)
            childItem.appendRow(leafItem)
        else:
            for leafKey in config.keys():
                leafItem = QStandardItem(leafKey)
                leafItem.setEditable(False)
                leafPath = projectPath + "." + categoryKey + "." + leafKey
                leafItem.setData(leafPath, ProjectModel.ITEM_PATH)
                childItem.appendRow(leafItem)

                if categoryKey == ProjectModel.DEVICES_KEY:
                    leafItem.setData(ProjectModel.DEVICES_KEY, ProjectModel.ITEM_CATEGORY)
                    
                    # Update icon on availability of device

                    if (self.systemTopology is not None) and \
                       (self.systemTopology.has("device.{}".format(leafKey))):
                        leafItem.setIcon(QIcon(":device-instance"))
                    else:
                        leafItem.setIcon(QIcon(":offline"))

                    classConfig = config.get(leafKey)
                    for classId in classConfig.keys():
                        serverId = classConfig.get(classId + ".serverId")
                        # Set server and class ID
                        leafItem.setData(serverId, ProjectModel.ITEM_SERVER_ID)
                        leafItem.setData(classId, ProjectModel.ITEM_CLASS_ID)


    def updateData(self):
        #print ""
        #print self.projectHash
        #print ""

        self.beginResetModel()
        self.clear()
        self.setHorizontalHeaderLabels(["Projects"])

        rootItem = self.invisibleRootItem()

        # Add child items
        for projectKey in self.projectHash.keys():
            # Project names - toplevel items
            item = QStandardItem(projectKey)
            item.setEditable(False)
            font = item.font()
            font.setBold(True)
            item.setFont(font)
            item.setIcon(QIcon(":folder"))
            rootItem.appendRow(item)

            projectConfig = self.projectHash.get(projectKey)
            for p in projectConfig.keys():
                # Project key

                # Get children
                categoryConfig = projectConfig.get(p)
                for categoryKey in categoryConfig.keys():
                    # Categories - sub items
                    childItem = QStandardItem(categoryConfig.getAttribute(categoryKey, "label"))
                    childItem.setEditable(False)
                    childItem.setIcon(QIcon(":folder"))
                    item.appendRow(childItem)

                    projectPath = projectKey + "." + p
                    subConfig = categoryConfig.get(categoryKey)
                    if isinstance(subConfig, VectorHash):
                        # Vector of Hashes
                        for i, indexConfig in enumerate(subConfig):
                            self._handleLeafItems(childItem, projectPath, "{}[{}]".format(categoryKey, i), indexConfig)
                    else:
                        # Normal Hash
                        self._handleLeafItems(childItem, projectPath, categoryKey, subConfig)

        self.endResetModel()


    def updateNeeded(self):
        # Update project view and pluginDialog data
        self.updateData()
        if self.pluginDialog is not None:
            self.pluginDialog.updateServerTopology(self.systemTopology)


    def checkSystemTopology(self):
        """
        This function checks whether the systemTopology is set correctly.
        If not, a signal to connect to the server is emitted.
        """
        if self.systemTopology is not None:
            return True
        
        reply = QMessageBox.question(None, "No server connection",
                                     "There is no connection to the server.<br>"
                                     "Do you want to establish a server connection?",
                                     QMessageBox.Yes | QMessageBox.No, QMessageBox.Yes)

        if reply == QMessageBox.No:
            return False
        self.signalConnectToServer.emit()
        return False


    def systemTopologyChanged(self, config):
        """
        This function updates the status (on/offline) of the project devices and
        the server/classes which are available over the network.
        """
        # TODO: remove copying hash
        config = copy(config)
        if self.systemTopology is None:
            self.systemTopology = config
        else:
            self.systemTopology.merge(config, HashMergePolicy.MERGE_ATTRIBUTES)

        # Update relevant
        self.updateNeeded()


    def handleInstanceGone(self, instanceId):
        path = None
        if self.systemTopology.has("server." + instanceId):
            path = "server." + instanceId
        elif self.systemTopology.has("device." + instanceId):
            path = "device." + instanceId

        self.systemTopology.erase(path)
        
        # Update relevant
        self.updateNeeded()


    def selectPath(self, path):
        index = self.findIndex(path)
        if index is None:
            return

        self.selectionModel.select(index, QItemSelectionModel.ClearAndSelect)


    def findIndex(self, path):
        return self._rFindIndex(self.invisibleRootItem(), path)


    def _rFindIndex(self, item, path):
        for i in xrange(item.rowCount()):
            childItem = item.child(i)
            resultItem = self._rFindIndex(childItem, path)
            if resultItem:
                return resultItem
        
        indexPath = item.data(ProjectModel.ITEM_PATH)
        if indexPath == path:
            return item.index()
        return None


    def _currentProjectName(self):
        """
        This function returns the current project name, if a category like
        Devices, Scenes etc. is selected.
        
        It returns None, if no index can be found.
        """
        index = self.selectionModel.currentIndex()
        if not index.isValid():
            return None
        
        while (index.parent().data(Qt.DisplayRole) is not None):
            index = index.parent()
        
        return index.data(Qt.DisplayRole)


    def _projectExists(self, projectName):
        """
        This functions checks whether a project with the \projectName already exists.
        """
        return self.projectHash.has(projectName)


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


    def createNewProject(self, projectName, directory):
        """
        This function creates a hash for a new project and saves it to the given
        \directory.
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
        configurationKey = ProjectModel.PROJECT_KEY + "." + ProjectModel.CONFIGURATIONS_KEY
        projectConfig.set(configurationKey, None)
        projectConfig.setAttribute(configurationKey, "label", ProjectModel.CONFIGURATIONS_LABEL)

        self._addNewProject(projectName, directory, projectConfig)


    def _addNewProject(self, projectName, directory, projectConfig):
        """
        This function updates the project hash with the given project
        configuration and updates the view with the new changes.
        """
        # Check whether project already exists
        alreadyExists = self._projectExists(projectName)
        if alreadyExists:
            # Overwrite?
            reply = QMessageBox.question(None, "Project already exists",
                "A project with the same name already exists.<br>"
                "Do you want to overwrite it?",
                QMessageBox.Yes | QMessageBox.No, QMessageBox.Yes)

            if reply == QMessageBox.No:
                return
        
        self.projectHash.set(projectName, projectConfig)
        self.projectHash.setAttribute(projectName, "directory", directory)
        self.updateData()


    def openProject(self, filename):
        projectConfig = loadFromFile(str(filename))
        # Consider projectName to merge correctly into project hash
        projectName = projectConfig.getAttribute("project", "name")
        projectConfig = Hash(projectName, projectConfig)
        # Merge loaded hash into the current project hash
        self.addProjectConfiguration(projectConfig)


    def saveProject(self, projectName, directory, overwrite=False):
        absoluteProjectPath = directory + "/" + projectName
        dir = QDir()
        if not QDir(absoluteProjectPath).exists():
            dir.mkpath(absoluteProjectPath)
        else:
            if not overwrite:
                reply = QMessageBox.question(None, "New project",
                    "A project folder named \"" + projectName + "\" already exists.<br>"
                    "Do you want to replace it?",
                    QMessageBox.Yes | QMessageBox.No, QMessageBox.Yes)

                if reply == QMessageBox.No:
                    return

                self._clearProjectDir(absoluteProjectPath)

        projectConfig = self.projectHash.get(projectName)
        # Create folder structure and save content
        for pKey in projectConfig.keys():
            categoryConfig = projectConfig.get(pKey)
            for cKey in categoryConfig.keys():
                if cKey == ProjectModel.DEVICES_KEY:
                    continue

                # Create folder for label
                label = categoryConfig.getAttribute(cKey, "label")
                dir.mkpath(absoluteProjectPath + "/" + label)

                subConfig = categoryConfig.get(cKey)
                if subConfig is None:
                    continue

                if cKey == ProjectModel.CONFIGURATIONS_KEY:
                    # Save configurations
                    print "configurations"
                elif cKey == ProjectModel.MACROS_KEY:
                    # Save macros
                    print "macros"
                elif cKey == ProjectModel.MONITORS_KEY:
                    # Save monitors
                    print "monitors"
                elif cKey == ProjectModel.RESOURCES_KEY:
                    # Save resources
                    print "resources"
                elif cKey == ProjectModel.SCENES_KEY:
                    # Save scenes
                    for i, sceneConfig in enumerate(subConfig):
                        filename = sceneConfig.get("filename")
                        alias = sceneConfig.get("alias")
                        # Save scene to SVG
                        #print "path", "{}.{}.{}[{}]".format(projectName, pKey, cKey, i)
                        #print "save to svg", filename

        # Save project.xml
        saveToFile(projectConfig, "{}/{}/project.xml".format(directory, projectName))


    def addProjectConfiguration(self, config):
        self.projectHash.merge(config, HashMergePolicy.MERGE_ATTRIBUTES)
        self.updateData()


    def addSceneToProject(self, projScenePath, sceneConfig):
        # Get old config of scenes
        vecConfig = self.projectHash.get(projScenePath)

        if vecConfig is None:
            # Create vector of hashes, if not existent yet
            vecConfig = [sceneConfig]
        else:
            # Append new scene to vector of hashes
            vecConfig.append(sceneConfig)
        
        self.projectHash.set(projScenePath, vecConfig)
        self.updateData()


    def editDevice(self, path=None):
        if not self.checkSystemTopology():
            return

        if (path is not None) and self.projectHash.has(path):
            deviceConfig = self.projectHash.get(path)
        else:
            deviceConfig = None

        # Show dialog to select plugin
        self.pluginDialog = PluginDialog()
        if not self.pluginDialog.updateServerTopology(self.systemTopology, deviceConfig):
            QMessageBox.warning(None, "No servers available",
            "There are no servers available.<br>Please check, if all servers "
            "are <br>started correctly!")
            return
        if self.pluginDialog.exec_() == QDialog.Rejected:
            return

        # Get project name
        projectName = self._currentProjectName()

        # Remove old device
        if path is not None:
            self.onRemove()
        
        # Add new device
        self.addDevice(projectName)
        self.pluginDialog = None


    def addDevice(self, projectName):
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
        self.addProjectConfiguration(config)

        # Select added device
        self.selectPath(devicePath)


    def editScene(self, path=None):
        if (path is not None) and self.projectHash.has(path):
            sceneConfig = self.projectHash.get(path)
        else:
            sceneConfig = None
        
        dialog = SceneDialog(sceneConfig)
        if dialog.exec_() == QDialog.Rejected:
            return
        
        # Get project name
        projectName = self._currentProjectName()
        self.addScene(projectName, dialog.sceneName, dialog.sceneName, path is not None)


    def addScene(self, projectName, fileName, alias, overwrite=False):
        projScenePath = "{}.{}.{}".format(projectName, ProjectModel.PROJECT_KEY, 
                                          ProjectModel.SCENES_KEY)

        # Put info in Hash
        config = Hash("filename", fileName, "alias", alias)

        # Remove old scene
        if overwrite:
            self.onRemove()
            #self.signalRemoveScene(alias) # TODO: remove scene from mainwindow
        
        # Add new scene to project hash
        self.addSceneToProject(projScenePath, config)

        # Select added scene
        self.selectPath(projScenePath)
        
        # Send signal to mainWindow to add scene
        self.signalAddScene.emit(alias)
        

### slots ###
    def onSelectionChanged(self, selected, deselected):
        selectedIndexes = selected.indexes()
        # Send signal to projectPanel to update toolbar actions
        self.signalSelectionChanged.emit(selectedIndexes)
        
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

        if not self.checkSystemTopology():
            return

        # Check whether deviceId is already online
        if self.systemTopology.has("device.{}".format(deviceId)):
            # Get schema
            schema = manager.Manager().getDeviceSchema(deviceId)
            itemInfo = dict(key=deviceId, classId=classId, \
                            type=NavigationItemTypes.DEVICE, schema=schema)
        else:
            # Get schema
            schema = manager.Manager().getClassSchema(serverId, classId)
            # Set path which is used to get class schema
            naviPath = "{}.{}".format(serverId, classId)
            itemInfo = dict(key=path, projNaviPathTuple=(naviPath, path),
                            classId=classId, type=NavigationItemTypes.CLASS, \
                            schema=schema)
        
        manager.Manager().onSchemaAvailable(itemInfo)
        # Notify configurator of changes
        self.signalItemChanged.emit(itemInfo)


    def onServerConnectionChanged(self, isConnected):
        """
        If the server connection is changed, the model needs an update.
        """
        if not isConnected:
            self.systemTopology = None


    def onEditDevice(self):
        self.editDevice()


    def onEditScene(self):
        self.editScene()


    def onRemove(self):
        """
        This slot removes the currently selected index from the model.
        """
        index = self.selectionModel.currentIndex()
        if not index.isValid():
            return
        
        # Remove data from project hash
        self.projectHash.erase(index.data(ProjectModel.ITEM_PATH))
        # Remove data from model
        self.removeRow(index.row(), index.parent())

