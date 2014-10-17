
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
import globals
import icons
from dialogs.duplicatedialog import DuplicateDialog
from dialogs.devicedialogs import DeviceDialog
from dialogs.scenedialog import SceneDialog
from guiproject import Category, Device, DeviceGroup, GuiProject
from scene import Scene
import manager

from karabo.project import Project

from PyQt4.QtCore import pyqtSignal, QFileInfo, Qt
from PyQt4.QtGui import (QDialog, QFileDialog, QItemSelectionModel,
                         QMessageBox, QStandardItem, QStandardItemModel)
import os.path
from zipfile import ZipFile


class ProjectModel(QStandardItemModel):
    # To import a plugin a server connection needs to be established
    signalItemChanged = pyqtSignal(str, object) # type, configuration
    signalSelectionChanged = pyqtSignal(list)
    signalAddScene = pyqtSignal(object) # scene
    signalRemoveScene = pyqtSignal(object) # scene
    signalRenameScene = pyqtSignal(object) # scene

    ITEM_OBJECT = Qt.UserRole


    def __init__(self, parent=None):
        super(ProjectModel, self).__init__(parent)
        
        # List stores projects
        self.projects = []

        # Dialog to add and change a device
        self.deviceDialog = None
        
        self.setHorizontalHeaderLabels(["Projects"])
        self.selectionModel = QItemSelectionModel(self, self)
        self.selectionModel.selectionChanged.connect(self.onSelectionChanged)


    def indexInfo(self, index):
        if not index.isValid():
            return { }

        object = index.data(ProjectModel.ITEM_OBJECT)
        
        if not isinstance(object, Device):
            return { }
        
        if object.isOnline():
            conf = manager.getDevice(object.id)
            config = conf.toHash()
        else:
            conf = object
            if conf.descriptor is not None:
                config = conf.toHash()
            else:
                config = conf.initConfig

        return dict(conf=conf,
                    serverId=object.serverId,
                    classId=object.classId,
                    deviceId=object.id,
                    config=config)


    def updateData(self):
        # Get last selected object
        selectedIndexes = self.selectionModel.selectedIndexes()
        if selectedIndexes:
            lastSelectionObj = selectedIndexes[0].data(ProjectModel.ITEM_OBJECT)
        else:
            lastSelectionObj = None
        
        self.beginResetModel()
        if self.hasChildren():
            self.removeRows(0, self.rowCount())

        rootItem = self.invisibleRootItem()
        
        for project in self.projects:
            # Project names - toplevel items
            item = QStandardItem(project.name)
            
            if project.isModified:
                # Change color to blue
                brush = item.foreground()
                brush.setColor(Qt.blue)
                item.setForeground(brush)
                item.setText("*{}".format(project.name))

            item.setData(project, ProjectModel.ITEM_OBJECT)
            item.setEditable(False)
            font = item.font()
            font.setBold(True)
            item.setFont(font)
            item.setIcon(icons.folder)
            item.setToolTip(project.filename)
            rootItem.appendRow(item)
            
            # Devices
            childItem = QStandardItem(Project.DEVICES_LABEL)
            childItem.setData(Category(Project.DEVICES_LABEL), ProjectModel.ITEM_OBJECT)
            childItem.setEditable(False)
            childItem.setIcon(icons.folder)
            childItem.setToolTip(Project.DEVICES_LABEL)
            item.appendRow(childItem)
            for deviceObj in project.devices:
                if isinstance(deviceObj, Device):
                    leafItem = QStandardItem(deviceObj.id)
                    leafItem.setData(deviceObj, ProjectModel.ITEM_OBJECT)
                    leafItem.setEditable(False)

                    if deviceObj.status != "offline" and deviceObj.error:
                        leafItem.setIcon(icons.deviceInstanceError)
                    else:
                        leafItem.setIcon(dict(error=icons.deviceInstanceError,
                                              noserver=icons.deviceOfflineNoServer,
                                              noplugin=icons.deviceOfflineNoPlugin,
                                              offline=icons.deviceOffline,
                                              incompatible=icons.deviceIncompatible,
                                             ).get(deviceObj.status, icons.deviceInstance))
                    leafItem.setToolTip("{} <{}>".format(deviceObj.id, deviceObj.serverId))
                    childItem.appendRow(leafItem)
                else:
                    # Device groups
                    leafItem = QStandardItem(deviceObj.id)
                    leafItem.setData(deviceObj, ProjectModel.ITEM_OBJECT)
                    leafItem.setEditable(False)
                    leafItem.setIcon(icons.device_group)
                    leafItem.setToolTip("{}".format(deviceObj.id))
                    childItem.appendRow(leafItem)
                    
                    # Iterate through device of group
                    for device in deviceObj.devices:
                        subLeafItem = QStandardItem(device.id)
                        subLeafItem.setData(device, ProjectModel.ITEM_OBJECT)
                        subLeafItem.setEditable(False)

                        if device.status != "offline" and device.error:
                            subLeafItem.setIcon(icons.deviceInstanceError)
                        else:
                            subLeafItem.setIcon(dict(error=icons.deviceInstanceError,
                                                  noserver=icons.deviceOfflineNoServer,
                                                  noplugin=icons.deviceOfflineNoPlugin,
                                                  offline=icons.deviceOffline,
                                                  incompatible=icons.deviceIncompatible,
                                                 ).get(device.status, icons.deviceInstance))
                        subLeafItem.setToolTip("{} <{}>".format(device.id, device.serverId))
                        leafItem.appendRow(subLeafItem)

            # Scenes
            childItem = QStandardItem(Project.SCENES_LABEL)
            childItem.setData(Category(Project.SCENES_LABEL), ProjectModel.ITEM_OBJECT)
            childItem.setEditable(False)
            childItem.setIcon(icons.folder)
            childItem.setToolTip(Project.SCENES_LABEL)
            item.appendRow(childItem)
            for scene in project.scenes:
                leafItem = QStandardItem(scene.filename)
                leafItem.setIcon(icons.image)
                leafItem.setData(scene, ProjectModel.ITEM_OBJECT)
                leafItem.setEditable(False)
                leafItem.setToolTip(scene.filename)
                childItem.appendRow(leafItem)

            # Configurations
            childItem = QStandardItem(Project.CONFIGURATIONS_LABEL)
            childItem.setData(Category(Project.CONFIGURATIONS_LABEL), ProjectModel.ITEM_OBJECT)
            childItem.setEditable(False)
            childItem.setIcon(icons.folder)
            childItem.setToolTip(Project.CONFIGURATIONS_LABEL)
            item.appendRow(childItem)
            
            for deviceId, configList in project.configurations.items():
                # Add item for device it belongs to
                leafItem = QStandardItem(deviceId)
                leafItem.setEditable(False)
                leafItem.setToolTip(deviceId)
                childItem.appendRow(leafItem)

                for config in configList:
                    # Add item with configuration file
                    subLeafItem = QStandardItem(config.filename)
                    subLeafItem.setIcon(icons.file)
                    subLeafItem.setData(config, ProjectModel.ITEM_OBJECT)
                    subLeafItem.setEditable(False)
                    subLeafItem.setToolTip(config.filename)
                    leafItem.appendRow(subLeafItem)

        self.endResetModel()
        
        # Set last selected object
        if lastSelectionObj is not None:
            self.selectObject(lastSelectionObj)


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
        # Update project view and deviceDialog data
        self.updateData()
        if self.deviceDialog is not None:
            self.deviceDialog.updateServerTopology(manager.Manager().systemHash)


    def currentDevice(self):
        device = self.currentIndex().data(ProjectModel.ITEM_OBJECT)
        if not isinstance(device, Device):
            return None
        
        return device


    def currentScene(self):
        scene = self.currentIndex().data(ProjectModel.ITEM_OBJECT)
        if not isinstance(scene, Scene):
            return None
        
        return scene


    def currentIndex(self):
        return self.selectionModel.currentIndex()


    def selectObject(self, object):
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
        for i in range(item.rowCount()):
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
        while not isinstance(object, Project):
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
        
        while self.projects:
            project = self.projects[-1]
            
            if project.isModified:
                reply = QMessageBox.question(None, "Save changes before closing",
                    "Do you want to save your project<br><b>\"{}\"</b><br>before closing?"
                    .format(project.name),
                    QMessageBox.Save | QMessageBox.Discard, QMessageBox.Discard)
                if reply == QMessageBox.Save:
                    project.zip()
            
            self.projectClose(project)
        
        # Clear selection
        self.selectionModel.clear()
        self.updateData()
        return True


    def projectClose(self, project):
        """
        This function closes the project related scenes and removes it from the
        project list.
        """
        for scene in project.scenes:
            self.signalRemoveScene.emit(scene)
        
        self.removeProject(project)


    def appendProject(self, project):
        """
        Bind project to model.
        
        This includes a connection, if project changes and the view update after
        appending project to list.
        """
        # Whenever the project is modified - view must be updated
        project.signalProjectModified.connect(self.updateData)
        project.signalSelectObject.connect(self.selectObject)
        self.projects.append(project)
        self.updateData()
        self.selectObject(project)


    def projectNew(self, filename):
        """ Create and return a new project and add it to the model """
        self.closeExistentProject(filename)
        
        project = GuiProject(filename)
        project.zip()
        self.appendProject(project)
        return project


    def projectOpen(self, filename):
        """
        This function opens a project file, creates a new projects, adds it to
        the project list and updates the view.
        """
        self.closeExistentProject(filename)
        
        project = GuiProject(filename)
        try:
            project.unzip()
        except Exception as e:
            e.message = "While reading the project a <b>critical error</b> " \
                        "occurred."
            raise

        self.appendProject(project)

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


    def editDevice(self, device=None):
        """
        Within a dialog the properties of a device can be modified.
        
        Depending on the given parameter \device (either None or set) it is
        either created or edited via the dialog.
        """
        # Get project name
        project = self.currentProject()
        
        # Show dialog to select plugin
        self.deviceDialog = DeviceDialog()
        if not self.deviceDialog.updateServerTopology(manager.Manager().systemHash, device):
            QMessageBox.warning(None, "No servers available",
            "There are no servers available.<br>Please check, if all servers "
            "are <br>started correctly!")
            return
        
        if self.deviceDialog.exec_() == QDialog.Rejected:
            return
        
        if device is not None:
            # Get configuration of device, if classId is the same
            if device.classId == self.deviceDialog.classId:
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
                                       self.deviceDialog.serverId,
                                       self.deviceDialog.classId,
                                       self.deviceDialog.deviceId,
                                       self.deviceDialog.startupBehaviour)
            
            # Set config, if set
            if config is not None:
                device.initConfig = config

            self.updateData()
        else:
            # Add new device
            device = self.addDevice(project,
                                    self.deviceDialog.serverId,
                                    self.deviceDialog.classId,
                                    self.deviceDialog.deviceId,
                                    self.deviceDialog.startupBehaviour)
        
        self.selectObject(device)
        self.deviceDialog = None


    def addDevice(self, project, serverId, classId, deviceId, ifexists, updateNeeded=True):
        """
        Add a device for the given \project with the given data.
        """
        
        for d in project.devices:
            if isinstance(d, DeviceGroup):
                continue

            if deviceId == d.id:
                reply = QMessageBox.question(None, 'Device already exists',
                    "Another device with the same device ID \"<b>{}</b>\" <br> "
                    "already exists. Do you want to overwrite it?".format(deviceId),
                    QMessageBox.Yes | QMessageBox.No, QMessageBox.No)

                if reply == QMessageBox.No:
                    return None

                # Overwrite existing device
                index = project.remove(d)
                device = self.insertDevice(index, project, serverId,
                                           classId, deviceId, ifexists)
                return device
        
        device = project.newDevice(serverId, classId, deviceId, ifexists, updateNeeded)
        return device


    def insertDevice(self, index, project, serverId, classId, deviceId, ifexists):
        """
        Insert a device for the given \project with the given data.
        """
        device = Device(serverId, classId, deviceId, ifexists)
        project.insertDevice(index, device)
        
        return device


    def duplicateDevice(self, device):
        dialog = DuplicateDialog(device.id)
        if dialog.exec_() == QDialog.Rejected:
            return

        for index in range(dialog.startIndex, dialog.endIndex):
            deviceId = "{}{}{}".format(device.id, dialog.displayPrefix, index)
            newDevice = self.addDevice(self.currentProject(), device.serverId,
                                       device.classId, deviceId, device.ifexists,
                                       False)
            
            if device.descriptor is not None:
                config = device.toHash()
            else:
                config = device.initConfig
            
            newDevice.initConfig = config
        
        self.updateData()
        self.selectObject(newDevice)


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

            # Send signal to view to update the name as well
            self.signalRenameScene.emit(scene)
            scene.project.setModified(True)


    def _createScene(self, project, sceneName):
        scene = Scene(project, sceneName)
        project.addScene(scene)
        
        return scene


    def addScene(self, project, sceneName):
        """
        Create new Scene object for given \project.
        """
        scene = self._createScene(project, sceneName)
        self.openScene(scene)

        #self.updateData()
        self.selectObject(scene)
        
        return scene


    def duplicateScene(self, scene):
        dialog = DuplicateDialog(scene.filename[:-4])
        if dialog.exec_() == QDialog.Rejected:
            return

        xml = scene.toXml()
        for index in range(dialog.startIndex, dialog.endIndex):
            filename = "{}{}{}".format(scene.filename[:-4], dialog.displayPrefix, index)
            newScene = self.addScene(self.currentProject(), filename)
            newScene.fromXml(xml)
        
        self.updateData()
        self.selectObject(newScene)


    def openScene(self, scene):
        self.signalAddScene.emit(scene)
        

### slots ###

    def onSelectionChanged(self, selected, deselected):
        selectedIndexes = self.selectionModel.selectedIndexes()
        # Send signal to projectPanel to update toolbar actions
        self.signalSelectionChanged.emit(selectedIndexes)

        if not selectedIndexes:
            return
        
        if len(selectedIndexes) > 1:
            device = None
        else:
            index = selectedIndexes[0]
            device = index.data(ProjectModel.ITEM_OBJECT)
            if device is not None and isinstance(device, Configuration):
                # Check whether device is already online
                if device.isOnline():
                    if device.type in ("device", "projectClass"):
                        conf = manager.getDevice(device.id)
                    elif device.type == 'deviceGroupClass':
                        instance = device.createInstance()
                        
                        conf = instance
                        
                        # Check descriptor only with first selection
                        conf.checkDeviceSchema()
                else:
                    conf = device
                    # Check descriptor only with first selection
                    conf.checkClassSchema()
                
                type = conf.type
            else:
                conf = None
                type = "other"

            self.signalItemChanged.emit(type, conf)


    def onCloseProject(self):
        """
        This slot closes the currently selected projects and updates the model.
        """
        selectedIndexes = self.selectionModel.selectedIndexes()
        for index in selectedIndexes:
            project = index.data(ProjectModel.ITEM_OBJECT)
            if project.isModified:
                msgBox = QMessageBox()
                msgBox.setWindowTitle("Save changes before closing")
                msgBox.setText("Do you want to save your project \"<b>{}</b>\" "
                               "before closing?".format(project.name))
                msgBox.setStandardButtons(QMessageBox.Save | QMessageBox.Discard | 
                                          QMessageBox.Cancel)
                msgBox.setDefaultButton(QMessageBox.Save)

                reply = msgBox.exec_()
                if reply == QMessageBox.Cancel:
                    continue

                if reply == QMessageBox.Save:
                    project.zip()
            else:
                reply = QMessageBox.question(None, 'Close project',
                    "Do you really want to close the project \"<b>{}</b>\"?"
                    .format(project.name), QMessageBox.Yes | QMessageBox.No, QMessageBox.No)

            if reply == QMessageBox.No:
                continue
        
            self.projectClose(project)
        
        self.updateData()


    def onEditDevice(self):
        self.editDevice(self.currentDevice())


    def onDuplicateDevice(self):
        self.duplicateDevice(self.currentDevice())


    def onInitDevices(self):
        self.currentProject().instantiateAll()


    def onKillDevices(self):
        reply = QMessageBox.question(None, 'Shutdown devices',
            "Do you really want to shutdown all devices?",
            QMessageBox.Yes | QMessageBox.No, QMessageBox.No)

        if reply == QMessageBox.No:
            return
        
        self.currentProject().shutdownAll()


    def onEditScene(self):
        self.editScene(self.currentScene())


    def onDuplicateScene(self):
        self.duplicateScene(self.currentScene())


    def onSaveAsScene(self):
        project = self.currentProject()
        scene = self.currentScene()
        fn = QFileDialog.getSaveFileName(None, "Save scene to file",
                                         globals.HIDDEN_KARABO_FOLDER,
                                         "SVG (*.svg)")
        if not fn:
            return
        with ZipFile(project.filename, "r") as zf:
            s = zf.read("{}/{}".format(project.SCENES_KEY, scene.filename))
        with open(fn, "w") as fout:
            fout.write(s)


    def onOpenScene(self):
        project = self.currentProject()
        fn = QFileDialog.getOpenFileName(None, "Open scene",
                                         globals.HIDDEN_KARABO_FOLDER,
                                         "SVG (*.svg)")
        if not fn:
            return
        scene = Scene(project, os.path.basename(fn))
        with open(fn, "r") as fin:
            scene.fromXml(fin.read())
        project.addScene(scene)
        self.selectObject(scene)


    def onRemove(self):
        """
        This slot removes the currently selected index from the model.
        """
        selectedIndexes = self.selectionModel.selectedIndexes()
        nbSelected = len(selectedIndexes)
        if nbSelected > 1:
            reply = QMessageBox.question(None, 'Remove items',
                "Do you really want to remove selected items?",
                QMessageBox.Yes | QMessageBox.No, QMessageBox.No)

            if reply == QMessageBox.No:
                return
        
        # Get object before QModelIndexes are gone due to update of view
        objects = [index.data(ProjectModel.ITEM_OBJECT) for index in selectedIndexes]
        
        for obj in objects:
            # Remove data from project
            self.removeObject(obj.project, obj, nbSelected == 1)
        
        if nbSelected > 1:
            self.updateData()


    def onRemoveDevices(self):
        reply = QMessageBox.question(None, 'Remove devices',
            "Do you really want to remove all devices?",
            QMessageBox.Yes | QMessageBox.No, QMessageBox.No)

        if reply == QMessageBox.No:
            return
        
        project = self.currentProject()
        while project.devices:
            object = project.devices[-1]
            self.removeObject(project, object, False)


    def removeObject(self, project, object, showConfirm=True):
        """
        The \object is removed from the \project.
        """
        if showConfirm:
            reply = QMessageBox.question(None, 'Remove object',
                "Do you really want to remove the object?",
                QMessageBox.Yes | QMessageBox.No, QMessageBox.No)

            if reply == QMessageBox.No:
                return
        
        project.remove(object)
        
        if isinstance(object, Scene):
            self.signalRemoveScene.emit(object)
        
        if showConfirm:
            self.updateData()
            self.selectObject(project)
        
