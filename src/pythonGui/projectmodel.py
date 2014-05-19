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
import icons
from karabo.hash import Hash, HashMergePolicy
from dialogs.plugindialog import PluginDialog
from dialogs.scenedialog import SceneDialog
import manager
from project import Category, Device, Project, Scene

from PyQt4.QtCore import pyqtSignal, QDir, Qt
from PyQt4.QtGui import (QDialog, QIcon, QItemSelectionModel, QMessageBox,
                         QStandardItem, QStandardItemModel)
import os.path
from zipfile import is_zipfile


class ProjectModel(QStandardItemModel):
    # To import a plugin a server connection needs to be established
    signalItemChanged = pyqtSignal(object)
    signalSelectionChanged = pyqtSignal(list)
    signalServerConnection = pyqtSignal(bool) # connect?
    signalAddScene = pyqtSignal(object) # scene
    
    signalShowProjectConfiguration = pyqtSignal(object) # configuration

    ITEM_OBJECT = Qt.UserRole


    def __init__(self, parent=None):
        super(ProjectModel, self).__init__(parent)
        
        # Hash stores current system topology
        self.systemTopology = None
        # List stores projects
        self.projects = []
        
        # Dict for later descriptor update
        self.classConfigProjDeviceMap = dict() # {Configuration, [Device]}
        
        # Dialog to add and change a device
        self.pluginDialog = None
        
        self.setHorizontalHeaderLabels(["Projects"])
        self.selectionModel = QItemSelectionModel(self)
        self.selectionModel.selectionChanged.connect(self.onSelectionChanged)


    def indexInfo(self, index):
        if not index.isValid():
            return { }

        object = index.data(ProjectModel.ITEM_OBJECT)
        
        if isinstance(object, Device):
            return dict(serverId=object.futureConfig.get("serverId"),
                        classId=object.classId,
                        deviceId=object.futureConfig.get("deviceId"),
                        config=object.toHash())
        
        return { }


    def updateData(self):
        # Get last selected object
        selectedIndexes = self.selectionModel.selectedIndexes()
        if selectedIndexes:
            lastSelectionObj = selectedIndexes[0].data(ProjectModel.ITEM_OBJECT)
        else:
            lastSelectionObj = None
        
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
                if self.isDeviceOnline(device):
                    status = self.systemTopology.getAttribute("device.{}".format(device.path), "status")
                    if status == "error":
                        leafItem.setIcon(icons.deviceInstanceError)
                    else:
                        leafItem.setIcon(icons.deviceInstance)
                else:
                    # There are three flavours of 'offline'
                    # (1) connected to server, but a) server of project device
                    # is not available or b) class plugin is not available on
                    # server
                    # (2) not connected to server so really offline
                    iconSet = False
                    if self.systemTopology is not None:
                        if not self.isDeviceServerAvailable(device):
                            leafItem.setIcon(icons.deviceOfflineNoServer)
                            iconSet = True
                        else:
                            if not self.isDevicePluginAvailable(device):
                                leafItem.setIcon(icons.deviceOfflineNoPlugin)
                                iconSet = True
                    
                    # Set icon, if not done yet
                    if not iconSet:
                        leafItem.setIcon(icons.deviceOffline)
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
        
        # Set last selected object
        if lastSelectionObj is not None:
            self.selectItem(lastSelectionObj)


    def updateNeeded(self):
        # Update project view and pluginDialog data
        self.updateData()
        if self.pluginDialog is not None:
            self.pluginDialog.updateServerTopology(self.systemTopology)


    def isDeviceOnline(self, device):
        """
        Returns, if the \device is online or not, classId is considered as well.
        """
        path = "device.{}".format(device.path)
        return (self.systemTopology is not None and 
                self.systemTopology.has(path) and 
                self.systemTopology.getAttribute(path, "serverId") == device.futureConfig.get("serverId") and
                self.systemTopology.getAttribute(path, "classId") == device.classId)


    def isDeviceServerAvailable(self, device):
        """
        Returns, if the server on which the \device should live,
        is available or not.
        """
        path = "server.{}".format(device.futureConfig.get("serverId"))
        return (self.systemTopology is not None and
                self.systemTopology.has(path))


    def isDevicePluginAvailable(self, device):
        """
        Returns, if the server and plugin combination on which the \device is
        based, is available or not.
        """
        path = "server.{}".format(device.futureConfig.get("serverId"))
        return (self.systemTopology is not None and
                self.systemTopology.has(path) and
                self.systemTopology.hasAttribute(path, "deviceClasses") and
                device.classId in self.systemTopology.getAttribute(path, "deviceClasses"))


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
        if self.systemTopology is None:
            self.systemTopology = config
        else:
            self.systemTopology.merge(config, HashMergePolicy.MERGE_ATTRIBUTES)

        # Update relevant
        self.updateNeeded()


    def handleInstanceGone(self, instanceId):
        if self.systemTopology is None: return
        
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
        return is_zipfile(absoluteProjectPath)


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


    def projectOpen(self, filename):
        """
        This function opens a project file, creates a new projects, adds it to
        the project list and updates the view.
        """
        project = Project()
        #try:
        project.unzip(filename)
        #except Exception, e:
        #    message = "While reading the project a <b>critical error</b> occurred:<br><br>"
        #    QMessageBox.critical(None, "Error", message + str(e))
        #    return
        
        self.projects.append(project)
        self.updateData()
        
        for device in project.devices:
            self.checkDescriptor(device)
        
        # Open new loaded project scenes
        for scene in project.scenes:
            self.signalAddScene.emit(scene)


    def projectSave(self, project=None):
        """
        This function saves the \project.
        
        If the \project is None, the current project is taken.
        """
        if project is None:
            project = self.currentProject()
        
        project.zip()


    def projectSaveAs(self, directory, project=None):
        """
        This function saves the \project into the \directory.
        
        If the \project is None, the current project is taken.
        """
        if project is None:
            project = self.currentProject()
        
        project.directory = directory
        project.zip()


    def editDevice(self, device=None):
        """
        Within a dialog the properties of a device can be modified.
        
        Depending on the given parameter \device (either None or set) it is
        either created or edited via the dialog.
        """
        if not self.checkSystemTopology():
            return
        
        # Get project name
        project = self.currentProject()
        
        # Show dialog to select plugin
        self.pluginDialog = PluginDialog()
        if not self.pluginDialog.updateServerTopology(self.systemTopology, device):
            QMessageBox.warning(None, "No servers available",
            "There are no servers available.<br>Please check, if all servers "
            "are <br>started correctly!")
            return
        
        if self.pluginDialog.exec_() == QDialog.Rejected:
            return
        
        config = Hash("deviceId", self.pluginDialog.deviceId,
                      "serverId", self.pluginDialog.serverId)
        
        if device is not None:
            # Remove old device configuration
            project.remove(device)
        
        # Add new device
        device = self.addDevice(project, self.pluginDialog.deviceId, self.pluginDialog.classId, config)
        
        self.updateData()
        self.selectItem(device)
        self.pluginDialog = None


    def addDevice(self, project, deviceId, classId, config):
        """
        Add a device configuration for the given \project with the given \classId
        and the \config.
        """
        device = Device(deviceId, classId, config)
        self.checkDescriptor(device)
        project.addDevice(device)
        self.updateData()
        
        return device


    def checkDescriptor(self, device):
        serverId = device.futureConfig.get("serverId")
        classId = device.classId
        
        # Get class configuration
        conf = manager.Manager().getClass(serverId, classId)
        
        # Get descriptor and connect, if None
        descriptor = conf.getDescriptor()
        if descriptor is None:
            conf.signalConfigurationNewDescriptor.connect(self.onConfigurationNewDescriptor)

        # Save configuration for later descriptor update
        if conf in self.classConfigProjDeviceMap:
            self.classConfigProjDeviceMap[conf].append(device)
        else:
            self.classConfigProjDeviceMap[conf] = [device]


    def editScene(self, scene=None):
        dialog = SceneDialog(scene)
        if dialog.exec_() == QDialog.Rejected:
            return
        
        if scene is None:
            self.addScene(self.currentProject(), dialog.sceneName)
        else:
            scene.name = dialog.sceneName
            self.updateData()
        # TODO: send signal to view to update the name as well


    def _createScene(self, project, sceneName):
        scene = Scene(project, sceneName)
        project.addScene(scene)
        
        return scene


    def addScene(self, project, sceneName):
        """
        Create new Scene object for given \project.
        """
        scene = self._createScene(project, sceneName)
        self.updateData()
        self.signalAddScene.emit(scene)
        
        self.selectItem(scene)
        
        return scene


    def openScene(self, project, sceneName):
        scene = self._createScene(project, sceneName)
        self.updateData()
        self.showScene(scene)


    def showScene(self, scene):
        scene.load()
        

### slots ###

    def onSelectionChanged(self, selected, deselected):
        selectedIndexes = selected.indexes()
        # Send signal to projectPanel to update toolbar actions
        self.signalSelectionChanged.emit(selectedIndexes)
        
        if not selectedIndexes:
            return

        index = selectedIndexes[0]

        device = index.data(ProjectModel.ITEM_OBJECT)
        if device is None: return
        if not isinstance(device, Configuration):
            return

        if not self.checkSystemTopology():
            return

        # Check whether device is already online
        if self.isDeviceOnline(device):
            conf = manager.Manager().getDevice(device.path)
        else:
            conf = device

        self.signalItemChanged.emit(conf)


    def onServerConnectionChanged(self, isConnected):
        """
        If the server connection is changed, the model needs an update.
        """
        if isConnected: return
        
        self.systemTopology = None


    def onEditDevice(self):
        index = self.selectionModel.currentIndex()
        object = index.data(ProjectModel.ITEM_OBJECT)
        if isinstance(object, Category):
            object = None
        self.editDevice(object)


    def onInitDevices(self):
        if not self.checkSystemTopology():
            return
        
        project = self.currentProject()
        for device in project.devices:
            # TODO: check for startup behavior
            serverId = device.futureConfig.get("serverId")
            manager.Manager().initDevice(serverId, device.classId, device.toHash())


    def onKillDevices(self):
        if not self.checkSystemTopology():
            return
        
        project = self.currentProject()
        for device in project.devices:
            if self.isDeviceOnline(device):
                manager.Manager().killDevice(device.path)


    def onEditScene(self):
        index = self.selectionModel.currentIndex()
        object = index.data(ProjectModel.ITEM_OBJECT)
        if isinstance(object, Category):
            object = None
        self.editScene(object)


    def onRemove(self):
        """
        This slot removes the currently selected index from the model.
        """
        index = self.selectionModel.currentIndex()
        if not index.isValid():
            return
        
        # Remove data from project
        self.currentProject().remove(index.data(ProjectModel.ITEM_OBJECT))
        self.updateData()


    def onConfigurationNewDescriptor(self, conf):
        """
        This slot is called from the Configuration, whenever a new descriptor is
        available \conf is given.
        """
        # Update all associated project devices with new descriptor
        devices = self.classConfigProjDeviceMap[conf]
        for device in devices:
            device.setDescriptor(conf.getDescriptor())
            
            # Merge hash configuration into configuration
            device.mergeFutureConfig()
            self.signalShowProjectConfiguration.emit(device)

