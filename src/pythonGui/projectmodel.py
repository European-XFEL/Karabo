
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
from dialogs.configurationdialog import SelectMultipleProjectConfigurationDialog
from dialogs.duplicatedialog import DuplicateDialog
from dialogs.devicedialogs import DeviceGroupDialog
from dialogs.scenedialog import SceneDialog
from dialogs.dialogs import MacroDialog
from guiproject import Category, Device, DeviceGroup, GuiProject, Macro
from scene import Scene
import manager
import network

from karabo.hash import Hash
from karabo.project import Project, ProjectAccess
import karabo

from PyQt4.QtCore import pyqtSignal, QAbstractItemModel, QFileInfo, Qt
from PyQt4.QtGui import (QDialog, QFileDialog,
                         QItemSelectionModel, QMessageBox, QStandardItem,
                         QStandardItemModel)
import os.path
from zipfile import ZipFile


class ProjectModel(QStandardItemModel):
    # To import a plugin a server connection needs to be established
    signalItemChanged = pyqtSignal(str, object) # type, configuration
    signalSelectionChanged = pyqtSignal(list)
    signalAddScene = pyqtSignal(object) # scene
    signalRemoveScene = pyqtSignal(object) # scene
    signalRenameScene = pyqtSignal(object) # scene
    signalAddMacro = pyqtSignal(object)
    signalRemoveMacro = pyqtSignal(object) # macro

    signalExpandIndex = pyqtSignal(object, bool)

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


    def flags(self, index):
        flags = QStandardItemModel.flags(self, index)

        if index.isValid():
            return flags | Qt.ItemIsDragEnabled | Qt.ItemIsDropEnabled
        else:
            return flags | Qt.ItemIsDropEnabled


    def mimeData(self, indexes):
        mimeData = QAbstractItemModel.mimeData(self, indexes)
        mimeData.setData('sourceType', 'internal')
        
        data = ""
        for index in indexes:
            text = "{};{};{}\n".format(index.data(), index.row(), index.column())
            print("mimeData", text)
            data += text
        
        mimeData.setData("data", data)
        return mimeData


    def dropMimeData(self, mimeData, action, row, column, parentIndex):
        print()
        print("dropMimeData", row, column, parentIndex, parentIndex.data())
        
        if mimeData.hasFormat("data"):
            data = mimeData.data("data");
            rowData = data.split("\n")
            print()
            for r in rowData:
                columnData = r.split(";")
                if len(columnData) < 3:
                    continue
                
                itemName = columnData[0].data().decode("utf-8")
                itemRow, _ = columnData[1].toInt()
                itemColumn, _ = columnData[2].toInt()
                
                print("=== dropMimeData", itemName, itemRow, itemColumn)
                
                droppedItem = self.item(itemRow, itemColumn)
                print("droppedItem", droppedItem)
                if droppedItem is not None:
                    item = QStandardItem(itemName)
                    
                    # TODO: recursively take also child items of droppedItem
                    print()
                    print("+++ droppedItem", droppedItem.data(Qt.DisplayRole))
                    if droppedItem.hasChildren():
                        self.copySubTreeItems(droppedItem, item)
                    
                    # Insert item
                    if parentIndex.isValid():
                        parentItem = self.itemFromIndex(parentIndex)
                        parentItem.appendRow(item)
                    else:
                        parentItem = self.invisibleRootItem()
                        self.insertRow(row, item)
                    
                    # Remove item
                    droppedIndex = self.indexFromItem(droppedItem)
                    ok = self.removeRow(itemRow, droppedIndex.parent())
                    
                    self.signalExpandIndex.emit(self.indexFromItem(parentItem), True)

        return True


    def copySubTreeItems(self, oldItem, newItem):
        for row in range(oldItem.rowCount()):
            childItem = oldItem.child(row, oldItem.column())
            print("childItem", childItem.data(Qt.DisplayRole))
            
            # Copy childItem and append to newItem
            newChildItem = QStandardItem(childItem.data(Qt.DisplayRole))
            newItem.appendRow(newChildItem)
            
            if childItem.hasChildren():
                self.copySubTreeItems(childItem, newChildItem)


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


    def addProjectItem(self, project):
        """
        This function adds an item representing the project to the model including
        its subfolder items.
        """
        # Add project item to model
        projectItem = QStandardItem(project.name)
        projectItem.setData(project, ProjectModel.ITEM_OBJECT)
        #projectItem.setEditable(False)
        font = projectItem.font()
        font.setBold(True)
        projectItem.setFont(font)
        projectItem.setIcon({ProjectAccess.LOCAL: icons.folder, \
                             ProjectAccess.CLOUD: icons.folderCloud, \
                             ProjectAccess.CLOUD_READONLY: icons.lock} \
                             .get(project.access, icons.folder))
        projectItem.setToolTip(project.filename)
        self.invisibleRootItem().appendRow(projectItem)
        
        # Devices
        childItem = QStandardItem(Project.DEVICES_LABEL)
        childItem.setData(Category(Project.DEVICES_LABEL), ProjectModel.ITEM_OBJECT)
        #childItem.setEditable(False)
        childItem.setIcon(icons.folder)
        childItem.setToolTip(Project.DEVICES_LABEL)
        projectItem.appendRow(childItem)

        # Scenes
        childItem = QStandardItem(Project.SCENES_LABEL)
        childItem.setData(Category(Project.SCENES_LABEL), ProjectModel.ITEM_OBJECT)
        #childItem.setEditable(False)
        childItem.setIcon(icons.folder)
        childItem.setToolTip(Project.SCENES_LABEL)
        projectItem.appendRow(childItem)
        
        # Configurations
        childItem = QStandardItem(Project.CONFIGURATIONS_LABEL)
        childItem.setData(Category(Project.CONFIGURATIONS_LABEL), ProjectModel.ITEM_OBJECT)
        #childItem.setEditable(False)
        childItem.setIcon(icons.folder)
        childItem.setToolTip(Project.CONFIGURATIONS_LABEL)
        projectItem.appendRow(childItem)

        # Macros
        childItem = QStandardItem(Project.MACROS_LABEL)
        childItem.setData(Category(Project.MACROS_LABEL), ProjectModel.ITEM_OBJECT)
        #childItem.setEditable(False)
        childItem.setIcon(icons.folder)
        projectItem.appendRow(childItem)
        
        # Show projectItem expanded
        self.signalExpandIndex.emit(self.indexFromItem(projectItem), True)


    def onProjectModified(self, project):
        """
        This function updates the Qt.DisplayRole of the item of the given
        \project depending on its isModified parameter.
        """
        item = self.findItem(project)
        if project.isModified:
            # Change color to blue
            brush = item.foreground()
            brush.setColor(Qt.blue)
            item.setForeground(brush)
            item.setText("*{}".format(project.name))
        else:
            # Change color to blue
            brush = item.foreground()
            brush.setColor(Qt.black)
            item.setForeground(brush)
            item.setText(project.name)
        
        font = item.font()
        font.setBold(True)
        item.setFont(font)
        item.setIcon({ProjectAccess.LOCAL: icons.folder, \
                      ProjectAccess.CLOUD: icons.folderCloud, \
                      ProjectAccess.CLOUD_READONLY: icons.lock} \
                        .get(project.access, icons.folder))


    def removeProjectItem(self, project):
        """
        This function removes the item representing the \project to the model
        including its subfolder items.
        """
        item = self.findItem(project)
        self.removeRow(item.row())


    def getCategoryItem(self, category, projectItem):
        """
        This function returns the model index of the given \category belonging
        to the given \projectItem.
        """
        return self.findItemString(category, projectItem)


    def createDeviceItem(self, device):
        """
        This function creates a QStandardItem for the given \device and
        returns it.
        """
        item = QStandardItem(device.id)
        item.setData(device, ProjectModel.ITEM_OBJECT)
        item.setEditable(False)

        self.updateDeviceIcon(item, device)
        item.setToolTip("{} <{}>".format(device.id, device.serverId))
        
        return item


    def addDeviceItem(self, device):
        """
        This function adds the given \device at the right position to the model.
        """
        item = self.createDeviceItem(device)

        projectItem = self.findItem(device.project)
        # Find folder for devices
        parentItem = self.getCategoryItem(Project.DEVICES_LABEL, projectItem)
        parentItem.appendRow(item)
        self.signalExpandIndex.emit(self.indexFromItem(parentItem), True)


    def insertDeviceItem(self, row, device):
        """
        This function inserts the given \device at the given \row of the model.
        """
        item = self.createDeviceItem(device)
        
        projectItem = self.findItem(device.project)
        # Find folder for devices
        parentItem = self.getCategoryItem(Project.DEVICES_LABEL, projectItem)
        parentItem.insertRow(row, item)


    def createDeviceGroupItem(self, deviceGroup):
        """
        This function creates an item representing the given \deviceGroup and
        returns it to add it to the model later.
        """
        item = QStandardItem(deviceGroup.id)
        item.setData(deviceGroup, ProjectModel.ITEM_OBJECT)
        item.setEditable(False)
        self.updateDeviceGroupIcon(item, deviceGroup)
        item.setToolTip("{}".format(deviceGroup.id))

        # Iterate through device of group
        for device in deviceGroup.devices:
            childItem = QStandardItem(device.id)
            childItem.setData(device, ProjectModel.ITEM_OBJECT)
            childItem.setEditable(False)

            self.updateDeviceIcon(childItem, device)
            childItem.setToolTip("{} <{}>".format(device.id, device.serverId))
            item.appendRow(childItem)
        
        return item
        

    def addDeviceGroupItem(self, deviceGroup):
        """
        This function creates an item representing the given \deviceGroup and
        adds it to the model.
        """
        item = self.createDeviceGroupItem(deviceGroup)

        projectItem = self.findItem(deviceGroup.project)
        # Find folder for devices
        parentItem = self.getCategoryItem(Project.DEVICES_LABEL, projectItem)
        parentItem.appendRow(item)
        self.signalExpandIndex.emit(self.indexFromItem(parentItem), True)


    def insertDeviceGroupItem(self, row, deviceGroup):
        """
        This function inserts the given \deviceGroup at the given \row of the model.
        """
        item = self.createDeviceGroupItem(deviceGroup)
        
        projectItem = self.findItem(deviceGroup.project)
        # Find folder for devices
        parentItem = self.getCategoryItem(Project.DEVICES_LABEL, projectItem)
        parentItem.insertRow(row, item)


    def updateDeviceItems(self):
        """
        This function updates all device icons of every open project in this model.
        """
        for row in range(self.rowCount()):
            projectItem = self.item(row)
             # Find folder for devices
            devicesItem = self.getCategoryItem(Project.DEVICES_LABEL, projectItem)
            for childRow in range(devicesItem.rowCount()):
                dItem = devicesItem.child(childRow)
                object = dItem.data(ProjectModel.ITEM_OBJECT)
                if isinstance(object, DeviceGroup):
                    self.updateDeviceGroupIcon(dItem, object)
                    for deviceRow in range(dItem.rowCount()):
                        dChildItem = dItem.child(deviceRow)
                        obj = dChildItem.data(ProjectModel.ITEM_OBJECT)
                        self.updateDeviceIcon(dChildItem, obj)
                else:
                    self.updateDeviceIcon(dItem, object)


    def updateMacros(self):
        macros = {}
        hash = manager.Manager().systemHash.get("macro", Hash())
        for k, v, a in hash.iterall():
            macros.setdefault((a["project"], a["module"]), []).append(k)

        for row in range(self.rowCount()):
            projectItem = self.item(row)
            project = projectItem.data(ProjectModel.ITEM_OBJECT)
            item = self.getCategoryItem(Project.MACROS_LABEL, projectItem)
            for r in range(item.rowCount()):
                module = item.child(r)
                ml = macros.get((project.name, module.text()), [])
                module.removeRows(0, module.rowCount())
                module.data(ProjectModel.ITEM_OBJECT).instances = ml
                for k in ml:
                    childItem = QStandardItem(hash[k, "classId"])
                    childItem.setData(manager.getDevice(k),
                                      ProjectModel.ITEM_OBJECT)
                    childItem.setEditable(False)
                    module.appendRow(childItem)
                    editor = module.data(ProjectModel.ITEM_OBJECT).editor
                    if editor is not None:
                        editor.connect(k)

    def updateDeviceIcon(self, item, device):
        """
        This function updates the icon of the given \item depending on the
        \device status.
        """
        if device.status != "offline" and device.error:
            item.setIcon(icons.deviceInstanceError)
        else:
            item.setIcon(dict(error=icons.deviceInstanceError,
                              noserver=icons.deviceOfflineNoServer,
                              noplugin=icons.deviceOfflineNoPlugin,
                              offline=icons.deviceOffline,
                              incompatible=icons.deviceIncompatible,
                              ).get(device.status, icons.deviceInstance))


    def updateDeviceGroupIcon(self, item, deviceGroup):
        """
        This function updates the icon of the given \item depending on the
        \deviceGroup status.
        """
        if deviceGroup.isOnline():
            item.setIcon(icons.deviceGroupInstance)
        else:
            item.setIcon(icons.deviceGroupOffline)


    def addSceneItem(self, scene):
        """
        This function adds the given \scene at the right position to the model.
        """
        item = QStandardItem(scene.filename)
        item.setIcon(icons.image)
        item.setData(scene, ProjectModel.ITEM_OBJECT)
        item.setEditable(False)
        item.setToolTip(scene.filename)
        
        project = scene.project
        projectItem = self.findItem(project)
        # Find folder for scenes
        parentItem = self.getCategoryItem(Project.SCENES_LABEL, projectItem)
        parentItem.appendRow(item)
        self.signalExpandIndex.emit(self.indexFromItem(parentItem), True)


    def renameScene(self, scene):
        item = self.findItem(scene)
        item.setText(scene.filename)
        scene.project.setModified(True)
        
        # Send signal to mainwindow to update the tab text as well
        self.signalRenameScene.emit(scene)


    def addConfigurationItem(self, deviceId, configuration):
        project = configuration.project
        projectItem = self.findItem(project)
        # Find folder for configurations
        parentItem = self.getCategoryItem(Project.CONFIGURATIONS_LABEL, projectItem)
        
        # Check, if item for this deviceId already exists
        item = self.findItemString(deviceId, parentItem)
        if item is None:
            # Add item for device it belongs to
            item = QStandardItem(deviceId)
            item.setEditable(False)
            item.setToolTip(deviceId)
            parentItem.appendRow(item)

        # Add item with configuration file
        configItem = QStandardItem(configuration.filename)
        configItem.setIcon(icons.file)
        configItem.setData(configuration, ProjectModel.ITEM_OBJECT)
        configItem.setEditable(False)
        configItem.setToolTip(configuration.filename)
        item.appendRow(configItem)
        
        self.signalExpandIndex.emit(self.indexFromItem(parentItem), True)
        self.signalExpandIndex.emit(self.indexFromItem(item), True)


    def addMacroItem(self, macro):
        item = QStandardItem(macro.name)
        item.setIcon(icons.file)
        item.setData(macro, ProjectModel.ITEM_OBJECT)
        item.setEditable(False)
        item.setToolTip(macro.name)
        
        project = macro.project
        projectItem = self.findItem(project)
        # Find folder for scenes
        parentItem = self.getCategoryItem(Project.MACROS_LABEL, projectItem)
        parentItem.appendRow(item)
        self.signalExpandIndex.emit(self.indexFromItem(parentItem), True)
        
        self.addMacroSubItems(macro)


    def addMacroSubItems(self, macro):
        if not macro.macros:
            return
        
        item = self.findItem(macro)
        if item is None:
            return
 
        # Remove possible old rows
        while item.hasChildren():
            row = item.rowCount()-1
            item.removeRow(row)
        
        for k, v in macro.macros.items():
            childItem = QStandardItem(k)
            childItem.setData(v, ProjectModel.ITEM_OBJECT)
            childItem.setEditable(False)
            item.appendRow(childItem)
        
        self.signalExpandIndex.emit(self.indexFromItem(item), True)


    def removeObjectItem(self, object):
        """
        This function removes the item representing the \object to the model..
        """
        index = self.findIndex(object)
        self.removeRow(index.row(), index.parent())


    def clearParameterPages(self, serverClassIds=[]):
        for project in self.projects:
            for device in project.devices:
                if (device.serverId, device.classId) in serverClassIds:
                    if device.parameterEditor is not None:
                        # Put current configuration to future config so that it
                        # does not get lost
                        if device.descriptor is not None:
                            device.initConfig = device.toHash()
                        device.parameterEditor.clear()


    def updateNeeded(self):
        # Update project device view 
        self.updateDeviceItems()
        # Select index again, in case device status changed - show correct page
        self.selectIndex(self.currentIndex())
        # Update deviceDialog data
        if self.deviceDialog is not None:
            self.deviceDialog.updateServerTopology(manager.Manager().systemHash)
        self.updateMacros()

    def currentDevice(self):
        device = self.currentIndex().data(ProjectModel.ITEM_OBJECT)
        if not isinstance(device, Device) and not isinstance(device, DeviceGroup):
            return None
        
        return device


    def currentScene(self):
        scene = self.currentIndex().data(ProjectModel.ITEM_OBJECT)
        if not isinstance(scene, Scene):
            return None
        
        return scene


    def currentMacro(self):
        macro = self.currentIndex().data(ProjectModel.ITEM_OBJECT)
        if not isinstance(macro, Macro):
            return None
        
        return macro


    def currentIndex(self):
        return self.selectionModel.currentIndex()


    def selectIndex(self, index):
        if index.isValid():
            self.selectionModel.select(index, QItemSelectionModel.Clear)
            self.selectionModel.select(index, QItemSelectionModel.Select)
            self.selectionModel.setCurrentIndex(index, QItemSelectionModel.ClearAndSelect)


    def selectObject(self, object):
        """
        This function gets an object which can be of type Configuration or Scene
        and selects the corresponding item.
        """
        index = self.findIndex(object)
        if index is not None and object is not None:
            self.selectIndex(index)
        else:
            self.selectionModel.clear()


    def findIndex(self, object):
        """
        This function looks recursively through the model and returns the
        QModelIndex of the given \object.
        """
        item = self.findItem(object)
        if item is not None:
            return item.index()
        
        return None


    def findItem(self, object, startItem=None):
        """
        This function looks recursively through the model and returns the
        QStandardItem of the given \object.
        
        If no \startItem is given the recursion starts from the root item.
        """
        if startItem is None:
            startItem = self.invisibleRootItem()
        
        return self._rFindItem(startItem, object)


    def _rFindItem(self, item, object):
        for i in range(item.rowCount()):
            childItem = item.child(i)
            if childItem.data(ProjectModel.ITEM_OBJECT) == object:
                return childItem
            
            resultItem = self._rFindItem(childItem, object)
            if resultItem is not None:
                return resultItem
        
        indexObject = item.data(ProjectModel.ITEM_OBJECT)
        if indexObject == object:
            return item
        
        return None


    def findItemString(self, text, startItem=None):
        """
        This function looks recursively through the model and returns the
        QStandardItem of the given item DisplayRole given as \text.
        
        If no \startItem is given the recursion starts from the root item.
        """
        if startItem is None:
            startItem = self.invisibleRootItem()
        
        return self._rFindItemString(startItem, text)


    def _rFindItemString(self, item, text):
        for i in range(item.rowCount()):
            childItem = item.child(i)
            if childItem.data(Qt.DisplayRole) == text:
                return childItem
            
            resultItem = self._rFindItemString(childItem, text)
            if resultItem is not None:
                return resultItem
        
        indexText = item.data(Qt.DisplayRole)
        if indexText == text:
            return item
        
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

        return True


    def projectClose(self, project):
        """
        This function closes the project related scenes and removes it from the
        project list.
        """
        for scene in project.scenes:
            self.signalRemoveScene.emit(scene)
        
        for m in project.macros.values():
            self.signalRemoveMacro.emit(m)
        
        self.removeProject(project)
        
        if project.access == ProjectAccess.CLOUD:
            network.Network().onCloseProject(project.basename)


    def appendProject(self, project):
        """
        Bind project to model.
        
        This includes a connection, if project changes and the view update after
        appending project to list.
        """
        # Whenever the project is modified - view must be updated
        project.signalProjectModified.connect(self.onProjectModified)
        project.signalDeviceAdded.connect(self.addDeviceItem)
        project.signalDeviceInserted.connect(self.insertDeviceItem)
        project.signalDeviceGroupAdded.connect(self.addDeviceGroupItem)
        project.signalDeviceGroupInserted.connect(self.insertDeviceGroupItem)
        project.signalSceneAdded.connect(self.addSceneItem)
        project.signalConfigurationAdded.connect(self.addConfigurationItem)
        project.signalMacroAdded.connect(self.addMacroItem)
        project.signalMacroChanged.connect(self.addMacroSubItems)
        
        project.signalRemoveObject.connect(self.removeObjectItem)
        
        project.signalSelectObject.connect(self.selectObject)
        self.signalItemChanged.connect(project.signalDeviceSelected)
        self.projects.append(project)
        
        # Add item to model
        self.addProjectItem(project)


    def removeProject(self, project):
        # Remove item from model
        self.removeProjectItem(project)
        # Remove project from list
        index = self.projects.index(project)
        del self.projects[index]


    def projectNew(self, filename, access):
        """ Create and return a new project and add it to the model """
        self.closeExistentProject(filename)
        
        project = GuiProject(filename)
        project.access = access
        project.zip()
        self.appendProject(project)
        self.selectObject(project)
        return project


    def projectOpen(self, filename, access):
        """
        This function opens a project file, creates a new project, adds it to
        the project list and updates the view.
        """
        self.closeExistentProject(filename)
        
        project = GuiProject(filename)
        project.access = access
        try:
            # Already append project to get setup signals
            self.appendProject(project)
            project.unzip()
        except Exception as e:
            # Remove project again, if failure
            self.removeProject(project)
            e.message = "While reading the project a <b>critical error</b> " \
                        "occurred."
            raise

        # Open new loaded project scenes
        for scene in project.scenes:
            self.signalAddScene.emit(scene)
        
        self.selectObject(project)
        self.updateMacros()
        return project


    def projectSave(self, project=None):
        """
        This function saves the \project.
        
        If the \project is None, the current project is taken.
        """
        if project is None:
            project = self.currentProject()
        
        project.zip()
        
        return project


    def projectSaveAs(self, filename, access, project=None):
        """
        This function saves the \project into the file \filename.

        If the \project is None, the current project is taken.
        """
        if project is None:
            project = self.currentProject()

        project.filename = filename
        project.access = access
        project.zip()
        self.onProjectModified(project)
        
        return project


    def editDevice(self, device=None):
        """
        Within a dialog the properties of a device can be modified.
        
        Depending on the given parameter \device (either None or set) it is
        either created or edited via the dialog.
        """
        # Get project name
        project = self.currentProject()
        
        # Show dialog to select plugin
        self.deviceDialog = DeviceGroupDialog()
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
                    config = device.initConfig
                else:
                    config = device.toHash()
            else:
                config = None
            
            # Remove device of project and get index for later insert to keep the
            # order
            index = project.remove(device)
            
            if isinstance(device, Device):
                device = self.insertDevice(index, project,
                                           self.deviceDialog.serverId,
                                           self.deviceDialog.classId,
                                           self.deviceDialog.deviceId,
                                           self.deviceDialog.startupBehaviour)
            
            elif isinstance(device, DeviceGroup):
                device = self.insertDeviceGroup(index, project,
                                                self.deviceDialog.deviceGroupName,
                                                self.deviceDialog.serverId,
                                                self.deviceDialog.classId,
                                                self.deviceDialog.startupBehaviour,
                                                self.deviceDialog.displayPrefix,
                                                self.deviceDialog.startIndex,
                                                self.deviceDialog.endIndex)
            
            # Set config, if set
            if config is not None:
                device.initConfig = config
        else:
            if not self.deviceDialog.deviceGroup:
                # Add new device
                device = self.addDevice(project,
                                        self.deviceDialog.serverId,
                                        self.deviceDialog.classId,
                                        self.deviceDialog.deviceId,
                                        self.deviceDialog.startupBehaviour)
            else:
                device = self.addDeviceGroup(project,
                                             self.deviceDialog.deviceGroupName,
                                             self.deviceDialog.serverId,
                                             self.deviceDialog.classId,
                                             self.deviceDialog.startupBehaviour,
                                             self.deviceDialog.displayPrefix,
                                             self.deviceDialog.startIndex,
                                             self.deviceDialog.endIndex)
        
        self.selectObject(device)
        self.deviceDialog = None


    def addDevice(self, project, serverId, classId, deviceId, ifexists):
        """
        Add a device for the given \project with the given data.
        """
        
        device = project.getDevice(deviceId)
        if device is not None:
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
        
        device = project.newDevice(serverId, classId, deviceId, ifexists)
        return device


    def addDeviceGroup(self, project, name, serverId, classId,
                       startupBehaviour, displayPrefix, startIndex,
                       endIndex):
        """
        Add a device group for the given \project with the given data.
        """
        
        deviceGroup = project.getDevice(name)
        if deviceGroup is not None:
            reply = QMessageBox.question(None, 'Device group already exists',
                "Another device group with the same ID \"<b>{}</b>\" <br> "
                "already exists. Do you want to overwrite it?".format(name),
                QMessageBox.Yes | QMessageBox.No, QMessageBox.No)

            if reply == QMessageBox.No:
                return None

            # Overwrite existing device group
            index = project.remove(deviceGroup)
            deviceGroup = self.insertDeviceGroup(index, project, name,
                                                 serverId, classId, startupBehaviour,
                                                 displayPrefix, startIndex, endIndex)
            return deviceGroup
        
        deviceGroup = project.newDeviceGroup(name, serverId, classId, startupBehaviour,
                                             displayPrefix, startIndex, endIndex)
        return deviceGroup


    def insertDevice(self, index, project, serverId, classId, deviceId, ifexists):
        """
        Insert a device for the given \project with the given data.
        """
        device = Device(serverId, classId, deviceId, ifexists)
        project.insertDevice(index, device)
        
        return device


    def insertDeviceGroup(self, index, project, name, serverId, classId,
                          ifexists, displayPrefix, startIndex,
                          endIndex):
        """
        Insert a device group for the given \project with the given data.
        """
        deviceGroup = project.createDeviceGroup(name, serverId, classId,
                                                ifexists, displayPrefix,
                                                startIndex, endIndex)
        project.insertDeviceGroup(index, deviceGroup)
        
        return deviceGroup


    def duplicateDevice(self, device):
        dialog = DuplicateDialog(device.id)
        if dialog.exec_() == QDialog.Rejected:
            return

        for index in range(dialog.startIndex, dialog.endIndex+1):
            deviceId = "{}{}".format(dialog.displayPrefix, index)
            newDevice = self.addDevice(self.currentProject(), device.serverId,
                                       device.classId, deviceId, device.ifexists)
            
            if device.descriptor is not None:
                config = device.toHash()
            else:
                config = device.initConfig
            
            newDevice.initConfig = config
        
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

            self.renameScene(scene)


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

        self.selectObject(scene)
        
        return scene


    def duplicateScene(self, scene):
        dialog = DuplicateDialog(scene.filename[:-4])
        if dialog.exec_() == QDialog.Rejected:
            return

        xml = scene.toXml()
        for index in range(dialog.startIndex, dialog.endIndex+1):
            filename = "{}{}".format(dialog.displayPrefix, index)
            newScene = self.addScene(self.currentProject(), filename)
            newScene.fromXml(xml)
        
        self.selectObject(newScene)


    def openScene(self, scene):
        self.signalAddScene.emit(scene)


    def editMacro(self, macro):
        if macro is None:
            dialog = MacroDialog()
            if dialog.exec_() == QDialog.Rejected:
                return
            
            project = self.currentProject()
            macro = Macro(project, dialog.name)
            project.addMacro(macro)

            self.selectObject(macro)
        
        self.signalAddMacro.emit(macro)


### slots ###

    def onSelectionChanged(self, selected, deselected):
        selectedIndexes = self.selectionModel.selectedIndexes()
        # Send signal to projectPanel to update toolbar actions
        self.signalSelectionChanged.emit(selectedIndexes)

        if not selectedIndexes:
            return
        
        if len(selectedIndexes) > 1:
            object = None
        else:
            index = selectedIndexes[0]
            object = index.data(ProjectModel.ITEM_OBJECT)

        if isinstance(object, Configuration):
            # Check whether device is already online
            if object.isOnline():
                if object.type in ("device", "projectClass"):
                    conf = manager.getDevice(object.id)
                elif object.type == 'deviceGroupClass':
                    instance = object.createInstance()

                    conf = instance

                    # Check descriptor only with first selection
                    conf.checkDeviceSchema()
            else:
                conf = object
                # Check descriptor only with first selection
                conf.checkClassSchema()
            ctype = conf.type
        else:
            conf = None
            ctype = "other"
        
        self.signalItemChanged.emit(ctype, conf)


    def onCloseProject(self):
        """
        This slot closes the currently selected projects and updates the model.
        """
        selectedIndexes = self.selectionModel.selectedIndexes()
        projects = []
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
            
            projects.append(project)
        
        for project in projects:
            self.projectClose(project)


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


    def onRunMacro(self):
        self.currentMacro().run()


    def onEditMacro(self):
        self.editMacro(self.currentMacro())


    def onLoadMacro(self):
        fn = QFileDialog.getOpenFileName(None, "Load macro", 
                                         globals.HIDDEN_KARABO_FOLDER,
                                         "Python Macros (*.py)")
        if not fn:
            return
        
        project = self.currentProject()
        name = os.path.basename(fn).split(".")[0]
        
        macro = Macro(project, name)
        project.addMacro(macro)
        
        self.signalAddMacro.emit(macro)
        with open(fn, "r") as file:
            macro.editor.edit.setPlainText(file.read())

        self.selectObject(macro)


    def onDuplicateMacro(self):
        print("TODO: duplicate macro...")


    def onApplyConfigurations(self):
        """
        """
        project = self.currentProject()
        
        dialog = SelectMultipleProjectConfigurationDialog(project)
        if dialog.exec_() == QDialog.Rejected:
            return
        
        # Get configuration which should be applied
        configs = dialog.projectConfigurations()
        
        # Go through project devices and apply configurations
        for device in project.devices:
            if device.id in configs:
                device.dispatchUserChanges(configs[device.id].hash[device.classId])


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


    def onRemoveConfiguration(self):
        selectedIndexes = self.selectionModel.selectedIndexes()
        nbSelected = len(selectedIndexes)
        if nbSelected > 1:
            reply = QMessageBox.question(None, 'Remove items',
                "Do you really want to remove selected items?",
                QMessageBox.Yes | QMessageBox.No, QMessageBox.No)

            if reply == QMessageBox.No:
                return

        while selectedIndexes:
            index = selectedIndexes.pop()
            parentIndex = index.parent()
            deviceId = parentIndex.data()
            configuration = index.data(ProjectModel.ITEM_OBJECT)
            # Remove data from project
            project = configuration.project
            project.removeConfiguration(deviceId, configuration)
            project.setModified(True)
            
            # Update model
            if self.itemFromIndex(parentIndex).rowCount() == 1:
                # Also remove deviceId item, if only one configuration is child
                self.removeRow(parentIndex.row(), parentIndex.parent())
            else:
                # Remove index from model
                self.removeRow(index.row(), parentIndex)


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
        
        if isinstance(object, Macro):
            self.signalRemoveMacro.emit(object)
        
        if isinstance(object, Configuration):
            for s in project.scenes:
                # Remove all related workflow devices of project scenes
                s.removeItemByObject(object)
        
        if showConfirm:
            self.selectObject(project)
        
