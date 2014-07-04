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
from karabo.hash import Hash
from dialogs.duplicatedialog import DuplicateDialog
from dialogs.plugindialog import PluginDialog
from dialogs.scenedialog import SceneDialog
from scene import Scene
import manager
from messagebox import MessageBox
from project import Category, Device, Project

from PyQt4.QtCore import pyqtSignal, QDir, QModelIndex, Qt
from PyQt4.QtGui import (QDialog, QIcon, QItemSelectionModel, QMessageBox,
                         QStandardItem, QStandardItemModel)
import os.path
from zipfile import is_zipfile


class ProjectModel(QStandardItemModel):
    # To import a plugin a server connection needs to be established
    signalItemChanged = pyqtSignal(str, object) # type, configuration
    signalSelectionChanged = pyqtSignal(list)
    signalAddScene = pyqtSignal(object) # scene
    signalRemoveScene = pyqtSignal(object) # scene

    ITEM_OBJECT = Qt.UserRole


    def __init__(self, parent=None):
        super(ProjectModel, self).__init__(parent)
        
        # List stores projects
        self.projects = []

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
            return dict(serverId=object.serverId,
                        classId=object.classId,
                        deviceId=object.id,
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
                leafItem = QStandardItem(device.id)
                leafItem.setData(device, ProjectModel.ITEM_OBJECT)
                leafItem.setEditable(False)

                if device.error:
                    leafItem.setIcon(icons.deviceInstanceError)
                else:
                    leafItem.setIcon(dict(error=icons.deviceInstanceError,
                                          noserver=icons.deviceOfflineNoServer,
                                          noplugin=icons.deviceOfflineNoPlugin,
                                          offline=icons.deviceOffline,
                                          incompatible=icons.deviceIncompatible,
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


    def clearParameterPages(self, serverClassIds=[]):
        for project in self.projects:
            for device in project.devices:
                if (device.serverId, device.classId) in serverClassIds:
                    if device.parameterEditor is not None:
                        # Put current configuration to future config so that it
                        # does not get lost
                        device.futureConfig = device.toHash()
                        device.parameterEditor.clear()
        


    def updateNeeded(self):
        # Update project view and pluginDialog data
        self.updateData()
        if self.pluginDialog is not None:
            self.pluginDialog.updateServerTopology(manager.Manager().systemHash)


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
        if index is not None and object is not None:
            self.selectionModel.setCurrentIndex(index, QItemSelectionModel.ClearAndSelect)
        else:
            self.selectionModel.clear()
            # TODO: for some reason the selectionChanged signal is not emitted
            # that's why it is done manually here
            self.signalSelectionChanged.emit([])


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
        index = self.currentIndex()
        if not index.isValid():
            return None

        object = index.data(ProjectModel.ITEM_OBJECT)
        while (object is not None) and not isinstance(object, Project):
            index = index.parent()
            object = index.data(ProjectModel.ITEM_OBJECT)

        return object


    def closeExistentProject(self, filename):
        """
        If a project with the \filename already exists, it is removed from the
        project list.
        """
        # Check for projects with the same filename and remove
        for p in self.projects:
            if p.filename == filename:
                self.projectClose(p)
                #self.updateData()
                break


    def removeProject(self, project):
        index = self.projects.index(project)
        del self.projects[index]


    def closeAllProjects(self):
        """
        This function removes all projects and updates the model.
        
        \False, if no project was there to be closed, otherwise \True
        """
        if not self.projects:
            # no need to go on
            return False
        
        for project in self.projects:
            self.projectClose(project)
        self.updateData()
        return True


    def projectClose(self, project):
        """
        This function closes the project related scenes and removes it from the
        project list.
        """
        self.removeProject(project)
        
        for scene in project.scenes:
            self.signalRemoveScene.emit(scene)


    def projectNew(self, filename):
        """ Create and return a new project and add it to the model """
        self.closeExistentProject(filename)
        
        project = Project(filename)
        project.zip()
        self.projects.append(project)
        self.updateData()
        self.selectItem(project)
        return project


    def projectOpen(self, filename):
        """
        This function opens a project file, creates a new projects, adds it to
        the project list and updates the view.
        """
        self.closeExistentProject(filename)
        
        project = Project(filename)
        try:
            project.unzip()
        except Exception as e:
            e.message = "While reading the project a <b>critical error</b> " \
                        "occurred."
            raise

        self.projects.append(project)
        self.updateData()
        self.selectItem(project)
        
        for device in project.devices:
            self.checkDescriptor(device)

        # Open new loaded project scenes
        for scene in project.scenes:
            self.signalAddScene.emit(scene)
        
        return project


    def projectSave(self, project=None):
        """
        This function saves the \project.
        
        If the \project is None, the current project is taken.
        """
        if project is None:
            project = self.currentProject()
        
        project.zip()


    def projectSaveAs(self, filename, project=None):
        """
        This function saves the \project into the file \filename.

        If the \project is None, the current project is taken.
        """
        if project is None:
            project = self.currentProject()

        project.zip(filename)
        project.filename = filename
        self.updateData()


    def editDevice(self, device=None):
        """
        Within a dialog the properties of a device can be modified.
        
        Depending on the given parameter \device (either None or set) it is
        either created or edited via the dialog.
        """
        # Get project name
        project = self.currentProject()
        
        # Show dialog to select plugin
        self.pluginDialog = PluginDialog()
        if not self.pluginDialog.updateServerTopology(manager.Manager().systemHash, device):
            QMessageBox.warning(None, "No servers available",
            "There are no servers available.<br>Please check, if all servers "
            "are <br>started correctly!")
            return
        
        if self.pluginDialog.exec_() == QDialog.Rejected:
            return
        
        if device is not None:
            # Get configuration of device, if classId is the same
            if device.classId == self.pluginDialog.classId:
                if device.descriptor is None:
                    config = device.futureConfig
                else:
                    config = device.toHash()
            else:
                config = None
            
            # Remove device of project and get index for later insert to keep the
            # order
            index = project.remove(device)
            device = self.insertDevice(index, project,
                                       self.pluginDialog.serverId,
                                       self.pluginDialog.classId,
                                       self.pluginDialog.deviceId,
                                       self.pluginDialog.startupBehaviour)
            
            # Set config, if set
            if config is not None:
                device.futureConfig = config
        else:
            # Add new device
            device = self.addDevice(project,
                                    self.pluginDialog.serverId,
                                    self.pluginDialog.classId,
                                    self.pluginDialog.deviceId,
                                    self.pluginDialog.startupBehaviour)
        
        self.updateData()
        self.selectItem(device)
        self.pluginDialog = None


    def addDevice(self, project, serverId, classId, deviceId, ifexists):
        """
        Add a device for the given \project with the given data.
        """
        device = Device(serverId, classId, deviceId, ifexists)
        self.checkDescriptor(device)
        project.addDevice(device)
        self.updateData()
        
        return device


    def insertDevice(self, index, project, serverId, classId, deviceId, ifexists):
        """
        Insert a device for the given \project with the given data.
        """
        device = Device(serverId, classId, deviceId, ifexists)
        self.checkDescriptor(device)
        project.insertDevice(index, device)
        self.updateData()
        
        return device


    def duplicateDevice(self, device):
        dialog = DuplicateDialog(device)
        if dialog.exec_() == QDialog.Rejected:
            return

        for i in xrange(dialog.startIndex, dialog.count):
            deviceId = "{}{}".format(dialog.deviceIdPrefix, i)
            newDevice = self.addDevice(self.currentProject(), device.serverId,
                                       device.classId, deviceId, device.ifexists)
            newDevice.futureConfig = device.toHash()


    def checkDescriptor(self, device):
        # Get class configuration
        conf = manager.getClass(device.serverId, device.classId)
        
        conf.signalNewDescriptor.connect(device.onNewDescriptor)
        if conf.descriptor is not None:
            device.onNewDescriptor(conf)


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
        self.openScene(scene)

        self.selectItem(scene)
        
        return scene


    def openScene(self, scene):
        self.signalAddScene.emit(scene)
        

### slots ###

    def onSelectionChanged(self, selected, deselected):
        selectedIndexes = selected.indexes()
        # Send signal to projectPanel to update toolbar actions
        self.signalSelectionChanged.emit(selectedIndexes)
        
        if not selectedIndexes:
            index = QModelIndex()
        else:
            index = selectedIndexes[0]

        device = index.data(ProjectModel.ITEM_OBJECT)
        if device is not None and isinstance(device, Configuration):
            # Check whether device is already online
            if device.isOnline():
                conf = manager.getDevice(device.id)
            else:
                conf = device
            type = conf.type
        else:
            conf = None
            type = "other"

        self.signalItemChanged.emit(type, conf)


    def onCloseProject(self):
        """
        This slot closes the currently selected projects and updates the model.
        """
        index = self.selectionModel.currentIndex()
        
        reply = QMessageBox.question(None, 'Close project',
            "Do you really want to close the project \"<b>{}</b>\"?"
            .format(index.data()),
            QMessageBox.Yes | QMessageBox.No, QMessageBox.No)

        if reply == QMessageBox.No:
            return
        
        object = index.data(ProjectModel.ITEM_OBJECT)
        self.projectClose(object)
        self.updateData()


    def onEditDevice(self):
        index = self.selectionModel.currentIndex()
        object = index.data(ProjectModel.ITEM_OBJECT)
        if isinstance(object, Category):
            object = None
        self.editDevice(object)


    def onDuplicateDevice(self):
        self.duplicateDevice(self.currentDevice())


    def onInitDevices(self):
        project = self.currentProject()
        for device in project.devices:
            self.initDevice(device)


    def initDevice(self, device):
        if device.descriptor is None:
            return
        
        if device.isOnline():
            if device.ifexists == "ignore":
                return
            elif device.ifexists == "restart":
                self.killDevice(device)
        
        manager.Manager().initDevice(device.serverId, device.classId, device.id,
                                     device.toHash())


    def onKillDevices(self):
        reply = QMessageBox.question(None, 'Kill devices',
            "Do you really want to kill all devices?",
            QMessageBox.Yes | QMessageBox.No, QMessageBox.No)

        if reply == QMessageBox.No:
            return
        
        project = self.currentProject()
        for device in project.devices:
            self.killDevice(device, False)


    def killDevice(self, device, showConfirm=True):
        if device.isOnline():
            manager.Manager().killDevice(device.id, showConfirm)


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
        self.removeObject(self.currentProject(), index.data(ProjectModel.ITEM_OBJECT))


    def onRemoveDevices(self):
        reply = QMessageBox.question(None, 'Remove devices',
            "Do you really want to remove all devices?",
            QMessageBox.Yes | QMessageBox.No, QMessageBox.No)

        if reply == QMessageBox.No:
            return
        
        project = self.currentProject()
        while len(project.devices) > 0:
            object = project.devices[-1]
            self.removeObject(project, object)


    def removeObject(self, project, object):
        """
        The \object is removed from the \project.
        """
        project.remove(object)
        self.updateData()
        self.selectItem(project)
        
