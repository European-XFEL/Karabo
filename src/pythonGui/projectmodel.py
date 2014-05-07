from __future__ import unicode_literals
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


from configuration import Configuration
from copy import copy
from karabo.hash import Hash, HashMergePolicy, XMLParser
from dialogs.plugindialog import PluginDialog
from dialogs.scenedialog import SceneDialog
import manager
from project import Project, Scene, Category

from PyQt4.QtCore import pyqtSignal, QDir, Qt
from PyQt4.QtGui import (QDialog, QIcon, QItemSelectionModel, QMessageBox,
                         QStandardItem, QStandardItemModel)
import os.path


class ProjectModel(QStandardItemModel):
    # To import a plugin a server connection needs to be established
    signalServerConnection = pyqtSignal(bool) # connect?
    signalAddScene = pyqtSignal(object) # scene
    signalOpenScene = pyqtSignal(object, str) # scene, filename
    signalSaveScene = pyqtSignal(object, str) # scene, filename
    
    signalShowProjectConfiguration = pyqtSignal(object) # configuration

    ITEM_OBJECT = Qt.UserRole


    def __init__(self, parent=None):
        super(ProjectModel, self).__init__(parent)
        
        # Hash stores current system topology
        self.systemTopology = None
        # List stores projects
        self.projects = []
        
        # Dict for later descriptor update
        self.classConfigDescriptorMap = dict()
        
        # Dialog to add and change a device
        self.pluginDialog = None
        
        # States whether the server connection is already requested to prevent
        # that selectionChanged signal/slot will ask twice for systemTopology
        self.serverConnectionRequested = False
        
        self.setHorizontalHeaderLabels(["Projects"])
        self.selectionModel = QItemSelectionModel(self)


    def updateData(self):
        self.beginResetModel()
        self.clear()
        self.setHorizontalHeaderLabels(["Projects"])

        rootItem = self.invisibleRootItem()
        
        for project in self.projects:
            # Project names - toplevel items
            item = QStandardItem(project.name)
            item.setData(project, ProjectModel.ITEM_OBJECT)
            item.setEditable(False)
            font = item.font()
            font.setBold(True)
            item.setFont(font)
            item.setIcon(QIcon(":folder"))
            rootItem.appendRow(item)
            
            # Devices
            childItem = QStandardItem(Project.DEVICES_LABEL)
            childItem.setData(Category(Project.DEVICES_LABEL), ProjectModel.ITEM_OBJECT)
            childItem.setEditable(False)
            childItem.setIcon(QIcon(":folder"))
            item.appendRow(childItem)
            for device in project.devices:
                leafItem = QStandardItem(device.path)
                leafItem.setData(device, ProjectModel.ITEM_OBJECT)
                leafItem.setEditable(False)

                # Update icon on availability of device
                if self.isDeviceOnline(device.path):
                    leafItem.setIcon(QIcon(":device-instance"))
                else:
                    leafItem.setIcon(QIcon(":offline"))
                childItem.appendRow(leafItem)

            # Scenes
            childItem = QStandardItem(Project.SCENES_LABEL)
            childItem.setData(Category(Project.SCENES_LABEL), ProjectModel.ITEM_OBJECT)
            childItem.setEditable(False)
            childItem.setIcon(QIcon(":folder"))
            item.appendRow(childItem)
            for scene in project.scenes:
                leafItem = QStandardItem(scene.name)
                leafItem.setData(scene, ProjectModel.ITEM_OBJECT)
                leafItem.setEditable(False)
                childItem.appendRow(leafItem)

            # Macros
            childItem = QStandardItem(Project.MACROS_LABEL)
            childItem.setData(Category(Project.MACROS_LABEL), ProjectModel.ITEM_OBJECT)
            childItem.setEditable(False)
            childItem.setIcon(QIcon(":folder"))
            item.appendRow(childItem)
            for macro in project.macros:
                leafItem = QStandardItem(macro)
                leafItem.setData(macro, ProjectModel.ITEM_OBJECT)
                leafItem.setEditable(False)
                childItem.appendRow(leafItem)

            # Monitors
            childItem = QStandardItem(Project.MONITORS_LABEL)
            childItem.setData(Category(Project.MONITORS_LABEL), ProjectModel.ITEM_OBJECT)
            childItem.setEditable(False)
            childItem.setIcon(QIcon(":folder"))
            item.appendRow(childItem)
            for monitor in project.monitors:
                leafItem = QStandardItem(monitor)
                leafItem.setData(monitor, ProjectModel.ITEM_OBJECT)
                leafItem.setEditable(False)
                childItem.appendRow(leafItem)

            # Resources
            childItem = QStandardItem(Project.RESOURCES_LABEL)
            childItem.setData(Category(Project.RESOURCES_LABEL), ProjectModel.ITEM_OBJECT)
            childItem.setEditable(False)
            childItem.setIcon(QIcon(":folder"))
            item.appendRow(childItem)
            for resource in project.resources:
                leafItem = QStandardItem(resource)
                leafItem.setData(resource, ProjectModel.ITEM_OBJECT)
                leafItem.setEditable(False)
                childItem.appendRow(leafItem)

            # Configurations
            childItem = QStandardItem(Project.CONFIGURATIONS_LABEL)
            childItem.setData(Category(Project.CONFIGURATIONS_LABEL), ProjectModel.ITEM_OBJECT)
            childItem.setEditable(False)
            childItem.setIcon(QIcon(":folder"))
            item.appendRow(childItem)
            for config in project.configurations:
                leafItem = QStandardItem(config)
                leafItem.setData(config, ProjectModel.ITEM_OBJECT)
                leafItem.setEditable(False)
                childItem.appendRow(leafItem)
        
        self.endResetModel()


    def updateNeeded(self):
        # Update project view and pluginDialog data
        self.updateData()
        if self.pluginDialog is not None:
            self.pluginDialog.updateServerTopology(self.systemTopology)


    def isDeviceOnline(self, deviceId):
        """
        Returns, if the \deviceId is online or not.
        """
        return self.systemTopology is not None and \
               self.systemTopology.has("device.{}".format(deviceId))


    def checkSystemTopology(self):
        """
        This function checks whether the systemTopology is set correctly.
        If not, a signal to connect to the server is emitted.
        """
        if self.systemTopology is not None:
            return True
        
        if self.serverConnectionRequested:
            return False
        
        reply = QMessageBox.question(None, "No server connection",
                                     "There is no connection to the server.<br>"
                                     "Do you want to establish a server connection?",
                                     QMessageBox.Yes | QMessageBox.No, QMessageBox.Yes)

        if reply == QMessageBox.No:
            return False
        # Send signal to establish server connection to projectpanel
        self.signalServerConnection.emit(True)
        self.serverConnectionRequested = True
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


    def selectItem(self, object):
        """
        This function gets an object which can be of type Configuration or Scene
        and selects the corresponding item.
        """
        index = self.findIndex(object)
        if index is not None:
            self.selectionModel.setCurrentIndex(index, QItemSelectionModel.ClearAndSelect)


    def findIndex(self, object):
        return self._rFindIndex(self.invisibleRootItem(), object)


    def _rFindIndex(self, item, object):
        for i in xrange(item.rowCount()):
            childItem = item.child(i)
            resultItem = self._rFindIndex(childItem, object)
            if resultItem:
                return resultItem
        
        indexObject = item.data(ProjectModel.ITEM_OBJECT)
        if indexObject == object:
            return item.index()
        
        return None


    def currentProject(self):
        """
        This function returns the current project from which something might
        be selected.
        """
        index = self.selectionModel.currentIndex()
        if not index.isValid():
            return None

        while (index.parent().data(ProjectModel.ITEM_OBJECT) is not None):
            index = index.parent()

        return index.data(ProjectModel.ITEM_OBJECT)


    def projectExists(self, directory, projectName):
        """
        This functions checks whether a project with the \projectName already exists.
        """
        absoluteProjectPath = os.path.join(directory, projectName)
        return QDir(absoluteProjectPath).exists()


    def createNewProject(self, projectName, directory):
        """
        This function updates the project list updates the view.
        """
        # Project name to lower case
        projectName = projectName.lower()
        
        project = Project(projectName, directory)
        self.projects.append(project)
        self.updateData()
        
        return project


    def createNewProjectFromHash(self, hash):
        """
        This function creates a new project via the given \hash and returns it.
        """
        project = Project()
        project.name = hash.getAttribute(Project.PROJECT_KEY, "name")
        project.directory = hash.getAttribute(Project.PROJECT_KEY, "directory")
        
        projConfig = hash.get(Project.PROJECT_KEY)
        
        for category in projConfig.keys():
            if category == Project.DEVICES_KEY:
                devices = projConfig.get(category)
                # Vector of hashes
                for d in devices:
                    self.addDevice(project, d)
            elif category == Project.SCENES_KEY:
                scenes = projConfig.get(category)
                # Vector of hashes
                for s in scenes:
                    self.openScene(project, s.get("name"), s.get("filename"))
            elif category == Project.MACROS_KEY:
                pass
            elif category == Project.MONITORS_KEY:
                pass
            elif category == Project.RESOURCES_KEY:
                pass
            elif category == Project.CONFIGURATIONS_KEY:
                pass
        
        return project


    def openProject(self, filename):
        """
        This function opens a project file, creates a new projects, adds it to
        the project list and updates the view.
        """
        p = XMLParser()
        with open(filename, 'r') as file:
            projectConfig = p.read(file.read())
        
        # Create empty project and fill
        project = self.createNewProjectFromHash(projectConfig)
        self.projects.append(project)
        self.updateData()


    def editDevice(self, device):
        if not self.checkSystemTopology():
            return

        deviceConfig = device.toHash()

        # Show dialog to select plugin
        self.pluginDialog = PluginDialog()
        if not self.pluginDialog.updateServerTopology(self.systemTopology, deviceConfig):
            QMessageBox.warning(None, "No servers available",
            "There are no servers available.<br>Please check, if all servers "
            "are <br>started correctly!")
            return
        if self.pluginDialog.exec_() == QDialog.Rejected:
            return
        
        # Add new device
        config = Hash("deviceId", self.pluginDialog.deviceId,
                      "serverId", self.pluginDialog.serverId,
                      "classId", self.pluginDialog.classId)
        device.path = self.pluginDialog.deviceId
        device.merge(config)
        self.updateData()
        self.selectItem(device)
        self.pluginDialog = None


    def addDevice(self, project, config):
        """
        Add a device configuration for the given \project with the given
        parameters of the \config.
        """
        deviceId = config.get("deviceId")
        serverId = config.get("serverId")
        classId = config.get("classId")
        
        if self.isDeviceOnline(deviceId):
            # Get device configuration
            device = manager.Manager().getDevice(deviceId)
        else:
            # Get class configuration
            conf = manager.Manager().getClass(serverId, classId)
        
            descriptor = conf.getDescriptor()
            if descriptor is None:
                conf.signalConfigurationNewDescriptor.connect(self.onConfigurationNewDescriptor)

            device = Configuration(deviceId, "projectClass", descriptor)
            # Save configuration for later descriptor update
            if conf in self.classConfigDescriptorMap:
                self.classConfigDescriptorMap[conf].append(device)
            else:
                self.classConfigDescriptorMap[conf] = [device]
        
        device.futureHash = config
        
        if device.getDescriptor() is not None:
            device.merge(device.futureHash)
            # Set default values for configuration
            device.setDefault()
        
        project.addDevice(device)
        self.updateData()
        
        return device


    def editScene(self, scene):
        dialog = SceneDialog(scene)
        if dialog.exec_() == QDialog.Rejected:
            return
        
        scene.name = dialog.sceneName
        self.updateData()
        # TODO: send signal to view to update the name as well


    def _createScene(self, project, sceneName):
        scene = Scene(sceneName)
        scene.initView()
        project.signalSaveScene.connect(self.signalSaveScene)
        
        project.addScene(scene)
        
        return scene


    def addScene(self, project, sceneName): #, overwrite=False):
        """
        Create new Scene object for given \project.
        """
        scene = self._createScene(project, sceneName)
        self.updateData()
        self.signalAddScene.emit(scene)
        
        self.selectItem(scene)
        
        return scene


    def openScene(self, project, sceneName, filename):
        scene = self._createScene(project, sceneName)
        filename = os.path.join(project.directory, project.name, Project.SCENES_LABEL, filename)
        self.updateData()
        self.signalOpenScene.emit(scene, filename)


### slots ###
    def onServerConnectionChanged(self, isConnected):
        """
        If the server connection is changed, the model needs an update.
        """
        if not isConnected:
            self.systemTopology = None
            self.serverConnectionRequested = False


    def onEditDevice(self):
        index = self.selectionModel.currentIndex()
        self.editDevice(index.data(ProjectModel.ITEM_OBJECT))


    def onEditScene(self):
        index = self.selectionModel.currentIndex()
        self.editScene(index.data(ProjectModel.ITEM_OBJECT))


    def onRemove(self):
        """
        This slot removes the currently selected index from the model.
        """
        index = self.selectionModel.currentIndex()
        if not index.isValid():
            return
        
        # Remove data from project
        self.currentProject().remove(index.data(ProjectModel.ITEM_OBJECT))
        # Remove data from model
        self.removeRow(index.row(), index.parent())


    def onConfigurationNewDescriptor(self, conf):
        """
        This slot is called from the Configuration, whenever a new descriptor is
        available \conf is given.
        """
        # Update all associated project configurations with new descriptor
        configurations = self.classConfigDescriptorMap[conf]
        for c in configurations:
            c.setDescriptor(conf.getDescriptor())
            
            # Merge hash configuration into configuration
            if c.futureHash is not None:
                c.merge(c.futureHash)
            
            # Set default values for configuration
            c.setDefault()
            self.signalShowProjectConfiguration.emit(c)

