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


from collections import OrderedDict
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
    signalSaveScene = pyqtSignal(object, str) # scene, filename
    
    signalShowProjectConfiguration = pyqtSignal(object) # configuration

    ITEM_OBJECT = Qt.UserRole


    def __init__(self, parent=None):
        super(ProjectModel, self).__init__(parent)
        
        # Hash stores current system topology
        self.systemTopology = None
        # List stores projects
        self.projects = OrderedDict()
        
        # Dict for later descriptor update
        self.classConfigDescriptorMap = dict()
        
        # Dialog to add and change a device
        self.pluginDialog = None
        
        self.setHorizontalHeaderLabels(["Projects"])
        self.selectionModel = QItemSelectionModel(self)


    def updateData(self):
        self.beginResetModel()
        self.clear()
        self.setHorizontalHeaderLabels(["Projects"])

        rootItem = self.invisibleRootItem()
        
        for project in self.projects.itervalues():
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
                if (self.systemTopology is not None) and \
                   (self.systemTopology.has("device.{}".format(device.path))):
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
        # Send signal to establish server connection to projectpanel
        self.signalServerConnection.emit(True)
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


    def _projectExists(self, projectName):
        """
        This functions checks whether a project with the \projectName already exists.
        """
        return projectName in self.projects


    def createNewProject(self, projectName, directory):
        """
        This function updates the project list updates the view.
        """
        # Project name to lower case
        projectName = projectName.lower()
        
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
        
        project = Project(projectName, directory)
        self.projects[projectName] = project
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
                    self.addScene(project, s.get("filename"))
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
        self.projects[project.name] = project
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
        project = self.currentProject()

        # Remove old device
        if path is not None:
            self.onRemove()
        
        # Add new device
        config = Hash("deviceId", self.pluginDialog.deviceId,
                      "serverId", self.pluginDialog.serverId,
                      "classId", self.pluginDialog.classId)
        self.addDevice(project, config)
        self.pluginDialog = None


    def addDevice(self, project, config):
        """
        Add a device configuration for the given \project with the given
        parameters of the \config.
        """
        deviceId = config.get("deviceId")
        serverId = config.get("serverId")
        classId = config.get("classId")
        
        if (self.systemTopology is not None) and \
           (self.systemTopology.has("device.{}".format(deviceId))):
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
        
        project.addDevice(device)
        self.updateData()
        
        self.selectItem(device)


    def editScene(self):
        # TODO: edit already existing scene?
        sceneData = None
        
        dialog = SceneDialog(sceneData)
        if dialog.exec_() == QDialog.Rejected:
            return
        
        self.addScene(self.currentProject(), dialog.sceneName)


    def addScene(self, project, sceneName): #, overwrite=False):
        """
        Create new Scene object for given \project.
        """
        scene = Scene(sceneName)
        scene.initView()
        project.signalSaveScene.connect(self.signalSaveScene)
        
        project.addScene(scene)
        self.signalAddScene.emit(scene)
        self.updateData()
        
        self.selectItem(scene)
        

### slots ###
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

