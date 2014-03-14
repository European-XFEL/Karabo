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
from karabo.karathon import Hash, HashMergePolicy, VectorHash
from dialogs.plugindialog import PluginDialog
from dialogs.scenedialog import SceneDialog

from PyQt4.QtCore import pyqtSignal, Qt
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

    DEVICES_LABEL = "Devices"
    SCENES_LABEL = "Scenes"
    MACROS_LABEL = "Macros"
    MONITORS_LABEL = "Monitors"
    RESOURCES_LABEL = "Resources"


    def __init__(self, parent=None):
        super(ProjectModel, self).__init__(parent)
        
        self.systemTopology = None
        self.projectHash = Hash()
        
        # Dialog to add and change a device
        self.pluginDialog = None
        
        self.setHorizontalHeaderLabels(["Projects"])
        self.selectionModel = QItemSelectionModel(self)
        self.selectionModel.selectionChanged.connect(self.onSelectionChanged)


    def _handleLeafItems(self, childItem, projectPath, categoryKey, config):
        if (config is None) or config.empty():
            return

        if categoryKey == ProjectModel.SCENES_KEY:
            fileName = config.get("filename")
            alias = config.get("alias")

            leafItem = QStandardItem(alias)
            leafItem.setEditable(False)
            leafPath = projectPath + "." + categoryKey + "." + alias
            leafItem.setData(leafPath, ProjectModel.ITEM_PATH)
            childItem.appendRow(leafItem)
        else:
            for leafKey in config.keys():
                leafItem = QStandardItem(leafKey)
                leafItem.setEditable(False)
                leafPath = projectPath + "." + categoryKey + "." + leafKey
                leafItem.setData(leafPath, ProjectModel.ITEM_PATH)
                childItem.appendRow(leafItem)

                if categoryKey == ProjectModel.DEVICES_KEY:
                    # TODO: Look for other possibilities than managers systemTopology
                    # to check whether deviceId is on/offline
                    # Update icon on availability of device
                    if manager.Manager().systemTopology.has(leafKey):
                        leafItem.setIcon(QIcon(":device-instance"))
                    else:
                        leafItem.setIcon(QIcon(":offline"))

                    classConfig = config.get(leafKey)
                    for classId in classConfig.keys():
                        serverId = classConfig.get(classId + ".serverId")
                        # Set server and class ID
                        leafItem.setData(serverId, ProjectModel.ITEM_SERVER_ID)
                        leafItem.setData(classId, ProjectModel.ITEM_CLASS_ID)


    def updateData(self, projectHash):
        #print ""
        #print projectHash
        #print ""

        self.beginResetModel()
        self.clear()
        self.setHorizontalHeaderLabels(["Projects"])

        rootItem = self.invisibleRootItem()

        # Add child items
        for projectKey in projectHash.keys():
            # Project names - toplevel items
            item = QStandardItem(projectKey)
            item.setEditable(False)
            font = item.font()
            font.setBold(True)
            item.setFont(font)
            item.setIcon(QIcon(":folder"))
            rootItem.appendRow(item)

            projectConfig = projectHash.get(projectKey)
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
                        for indexConfig in subConfig:
                            self._handleLeafItems(childItem, projectPath, categoryKey, indexConfig)
                    else:
                        # Normal Hash
                        self._handleLeafItems(childItem, projectPath, categoryKey, subConfig)

        self.endResetModel()


    def systemTopologyChanged(self, config):
        """
        This function updates the status (on/offline) of the project devices and
        the server/classes which are available over the network.
        """
        print "ProjectModel.updateSystemTopology"
        if self.systemTopology is None:
            self.systemTopology = config
        else:
            self.systemTopology.merge(config, HashMergePolicy.MERGE_ATTRIBUTES)
        
        #serverKey = "server"
        #if not config.has(serverKey):
        #    return

        # Create copy of nested hash - TODO: remove when hash in native python
        #self.systemTopology = copy(config.get(serverKey))
        
        if self.pluginDialog is not None:
            self.pluginDialog.updateServerTopology(self.systemTopology)


    def handleInstanceGone(self, instanceId):
        print "ProjectModel.handleInstanceGone", instanceId


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
        
        projectIndex = index.parent()
        if projectIndex is None:
            return None
        
        return projectIndex.data(Qt.DisplayRole)


    def _projectExists(self, projectName):
        """
        This functions checks whether a project with the \projectName already exists.
        """
        return self.projectHash.has(projectName)


    def addNewProject(self, projectName, directory, projectConfig):
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
        self.updateData(self.projectHash)


    def addConfigToProject(self, config):
        self.projectHash.merge(config, HashMergePolicy.MERGE_ATTRIBUTES)
        self.updateData(self.projectHash)


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
        self.updateData(self.projectHash)


    def addDevice(self):
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
        self.addConfigToProject(config)

        # Select added device
        self.selectPath(devicePath)


    def addScene(self, projectName, fileName, alias):
        projScenePath = projectName + ".project.scenes"

        # Update project hash
        scenePath = projScenePath + "." + alias

        # Put info in Hash
        config = Hash("filename", fileName, "alias", alias)
        # Add device to project hash
        self.addSceneToProject(projScenePath, config)

        # Select added device
        self.selectPath(scenePath)
        
        # Send signal to mainWindow to add scene
        self.signalAddScene.emit(alias)


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
        
        # Check whether deviceId is already online
        if manager.Manager().systemTopology.has(deviceId):
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
        print "onServerConnectionChanged", isConnected
        if not isConnected:
            self.systemTopology = None


    def onAddDevice(self):
        if self.systemTopology is None:
            reply = QMessageBox.question(None, "No server connection",
                                         "There is no connection to the server.<br>"
                                         "Do you want to establish a server connection?",
                                         QMessageBox.Yes | QMessageBox.No, QMessageBox.Yes)

            if reply == QMessageBox.No:
                return
            self.signalConnectToServer.emit()
            return

        # Show dialog to select plugin
        self.pluginDialog = PluginDialog()
        if not self.pluginDialog.updateServerTopology(self.systemTopology):
            QMessageBox.warning(self, "No servers available",
            "There are no servers available.<br>Please check, if all servers "
            "are <br>started correctly!")
            return
        if self.pluginDialog.exec_() == QDialog.Rejected:
            return

        self.addDevice()
        self.pluginDialog = None


    def onAddScene(self):
        dialog = SceneDialog()
        if dialog.exec_() == QDialog.Rejected:
            return
        
        # Get project name
        projectName = self._currentProjectName()
        self.addScene(projectName, dialog.fileName, dialog.alias)

