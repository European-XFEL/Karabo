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
import icons
from karabo.hash import Hash, HashMergePolicy
from dialogs.plugindialog import PluginDialog
from dialogs.scenedialog import SceneDialog
import manager
from messagebox import MessageBox
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
    signalAddScene = pyqtSignal(object) # scene
    signalRemoveScene = pyqtSignal(object) # scene
    
    signalShowProjectConfiguration = pyqtSignal(object) # configuration

    ITEM_OBJECT = Qt.UserRole


    def __init__(self, parent=None):
        super(ProjectModel, self).__init__(parent)
        
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
                leafItem = QStandardItem(device.key)
                leafItem.setData(device, ProjectModel.ITEM_OBJECT)
                leafItem.setEditable(False)

                if device.error:
                    leafItem.setIcon(icons.deviceInstanceError)
                else:
                    leafItem.setIcon(dict(error=icons.deviceInstanceError,
                                          noserver=icons.deviceOfflineNoServer,
                                          noplugin=icons.deviceOfflineNoPlugin,
                                          offline=icons.deviceOffline,
                                          incompatible=icons.deviceOffline
                                         ).get(
                                device.status, icons.deviceInstance))
                childItem.appendRow(leafItem)

            # Scenes
            childItem = QStandardItem(Project.SCENES_LABEL)
            childItem.setData(Category(Project.SCENES_LABEL), ProjectModel.ITEM_OBJECT)
            childItem.setEditable(False)
            childItem.setIcon(QIcon(":folder"))
            item.appendRow(childItem)
            for scene in project.scenes:
                leafItem = QStandardItem(scene.filename)
                leafItem.setIcon(icons.image)
                leafItem.setData(scene, ProjectModel.ITEM_OBJECT)
                leafItem.setEditable(False)
                childItem.appendRow(leafItem)

            # Configurations
            childItem = QStandardItem(Project.CONFIGURATIONS_LABEL)
            childItem.setData(Category(Project.CONFIGURATIONS_LABEL), ProjectModel.ITEM_OBJECT)
            childItem.setEditable(False)
            childItem.setIcon(QIcon(":folder"))
            item.appendRow(childItem)
            
            for deviceId, configList in project.configurations.iteritems():
                # Add item for device it belongs to
                leafItem = QStandardItem(deviceId)
                leafItem.setEditable(False)
                childItem.appendRow(leafItem)

                for config in configList:
                    # Add item with configuration file
                    subLeafItem = QStandardItem(config.filename)
                    subLeafItem.setIcon(icons.file)
                    subLeafItem.setData(config, ProjectModel.ITEM_OBJECT)
                    subLeafItem.setEditable(False)
                    leafItem.appendRow(subLeafItem)

            # Macros
            #childItem = QStandardItem(Project.MACROS_LABEL)
            #childItem.setData(Category(Project.MACROS_LABEL), ProjectModel.ITEM_OBJECT)
            #childItem.setEditable(False)
            #childItem.setIcon(QIcon(":folder"))
            #item.appendRow(childItem)
            #for macro in project.macros:
            #    leafItem = QStandardItem(macro)
            #    leafItem.setData(macro, ProjectModel.ITEM_OBJECT)
            #    leafItem.setEditable(False)
            #    childItem.appendRow(leafItem)

            # Monitors
            #childItem = QStandardItem(Project.MONITORS_LABEL)
            #childItem.setData(Category(Project.MONITORS_LABEL), ProjectModel.ITEM_OBJECT)
            #childItem.setEditable(False)
            #childItem.setIcon(QIcon(":folder"))
            #item.appendRow(childItem)
            #for monitor in project.monitors:
            #    leafItem = QStandardItem(monitor)
            #    leafItem.setData(monitor, ProjectModel.ITEM_OBJECT)
            #    leafItem.setEditable(False)
            #    childItem.appendRow(leafItem)

            # Resources
            #childItem = QStandardItem(Project.RESOURCES_LABEL)
            #childItem.setData(Category(Project.RESOURCES_LABEL), ProjectModel.ITEM_OBJECT)
            #childItem.setEditable(False)
            #childItem.setIcon(QIcon(":folder"))
            #item.appendRow(childItem)
            #for resource in project.resources:
            #    leafItem = QStandardItem(resource)
            #    leafItem.setData(resource, ProjectModel.ITEM_OBJECT)
            #    leafItem.setEditable(False)
            #    childItem.appendRow(leafItem)
        
        self.endResetModel()
        
        # Set last selected object
        if lastSelectionObj is not None:
            self.selectItem(lastSelectionObj)


    def updateNeeded(self):
        # Update project view and pluginDialog data
        self.updateData()
        if self.pluginDialog is not None:
            self.pluginDialog.updateServerTopology(
                manager.Manager().systemHash())


    def currentDevice(self):
        device = self.currentIndex().data(ProjectModel.ITEM_OBJECT)
        if not isinstance(device, Device):
            return None
        
        return device


    def currentIndex(self):
        return self.selectionModel.currentIndex()


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


    def closeAllProjects(self):
        """
        This function removes all projects and closes its open scenes.
        """
        for project in self.projects:
            for scene in project.scenes:
                self.signalRemoveScene.emit(scene)
        self.projects = []
        self.updateData()


    def projectExists(self, directory, projectFile):
        """
        This functions checks whether a project with the \projectFile already exists.
        """
        absoluteProjectPath = os.path.join(directory, projectFile)
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
        #    MessageBox.showError(message + str(e))
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
        if not manager.Manager().checkSystemHash():
            return
        
        # Get project name
        project = self.currentProject()
        
        # Show dialog to select plugin
        self.pluginDialog = PluginDialog()
        if not self.pluginDialog.updateServerTopology(
                manager.Manager().systemHash, device):
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
        
        conf.signalNewDescriptor.connect(device.onNewDescriptor)
        if conf.descriptor is not None:
            device.onNewDescriptor(conf)

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
            scene.filename = dialog.sceneName
            fi = QFileInfo(scene.filename)
            if len(fi.suffix()) < 1:
                scene.filename = "{}.svg".format(scene.filename)
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

        if not manager.Manager().checkSystemHash():
            return

        # Check whether device is already online
        if device.isOnline():
            conf = manager.Manager().getDevice(device.key)
        else:
            conf = device

        self.signalItemChanged.emit(conf)


    def onEditDevice(self):
        index = self.selectionModel.currentIndex()
        object = index.data(ProjectModel.ITEM_OBJECT)
        if isinstance(object, Category):
            object = None
        self.editDevice(object)


    def onInitDevices(self):
        if not manager.Manager().checkSystemHash():
            return
        
        project = self.currentProject()
        for device in project.devices:
            # TODO: check for startup behavior
            serverId = device.futureConfig.get("serverId")
            manager.Manager().initDevice(serverId, device.classId, device.toHash())


    def onKillDevices(self):
        if not manager.Manager().checkSystemHash():
            return
        
        project = self.currentProject()
        for device in project.devices:
            if device.isOnline():
                manager.Manager().killDevice(device.key)


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
            device.descriptor = conf.descriptor
            
            # Merge hash configuration into configuration
            device.mergeFutureConfig()
            self.signalShowProjectConfiguration.emit(device)

