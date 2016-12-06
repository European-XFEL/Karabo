#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 20, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
"""
This module contains a class which represents a model to display projects in
a treeview.
"""
from io import StringIO
import os
import os.path

from PyQt4.QtCore import pyqtSignal, QAbstractItemModel, QFileInfo, Qt
from PyQt4.QtGui import (
    QDialog, QFileDialog, QInputDialog, QItemSelectionModel, QMessageBox,
    QStandardItem, QStandardItemModel)

from karabo.middlelayer import (
    Hash, MacroModel, read_macro, read_scene, SceneModel, write_scene)
from karabo.middlelayer_api.project import Monitor, Project, ProjectAccess
from karabo_gui.configuration import Configuration
import karabo_gui.globals as globals
import karabo_gui.icons as icons
from karabo_gui.dialogs.configurationdialog import (
    SelectMultipleProjectConfigurationDialog)
from karabo_gui.dialogs.devicedialogs import DeviceGroupDialog
from karabo_gui.dialogs.dialogs import MacroDialog
from karabo_gui.dialogs.duplicatedialog import DuplicateDialog
from karabo_gui.dialogs.monitordialog import MonitorDialog
from karabo_gui.dialogs.scenedialog import SceneDialog
from karabo_gui.guiproject import Category, Device, DeviceGroup, GuiProject
from karabo_gui.events import (
    broadcast_event, KaraboBroadcastEvent, KaraboEventSender,
    register_for_broadcasts)
from karabo_gui.messagebox import MessageBox
from karabo_gui.singletons.api import get_manager, get_network
from karabo_gui.topology import getDevice
from karabo_gui.util import getOpenFileName, getSaveFileName


class ProjectModel(QStandardItemModel):
    # To import a plugin a server connection needs to be established
    signalItemChanged = pyqtSignal(str, object)  # type, configuration
    signalSelectionChanged = pyqtSignal(list)
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
        self.setSupportedDragActions(Qt.CopyAction)

        # Register to KaraboBroadcastEvent, Note: unregister_from_broadcasts is
        # not necessary for self due to the fact that the singleton mediator
        # object and `self` are being destroyed when the GUI exists
        register_for_broadcasts(self)

    def eventFilter(self, obj, event):
        if isinstance(event, KaraboBroadcastEvent):
            if event.sender is KaraboEventSender.OpenSceneLink:
                data = event.data
                self.openSceneLink(data.get("target"), data.get('project'))
                return False
        return super(ProjectModel, self).eventFilter(obj, event)

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
            conf = getDevice(object.id)
            config, _ = conf.toHash()  # Ignore returned attributes
        else:
            conf = object
            if conf.descriptor is not None:
                config, _ = conf.toHash()  # Ignore returned attributes
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
        childItem.setToolTip(Project.MACROS_LABEL)
        projectItem.appendRow(childItem)
        
        # Monitors
        childItem = QStandardItem(Project.MONITORS_LABEL)
        childItem.setData(Category(Project.MONITORS_LABEL), ProjectModel.ITEM_OBJECT)
        #childItem.setEditable(False)
        childItem.setIcon(icons.folder)
        childItem.setToolTip(Project.MONITORS_LABEL)
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


    def updateCategoryIcon(self, category, project):
        """
        This function updates the icon of the \category.
        """
        projectItem = self.findItem(project)
        item = self.getCategoryItem(category, projectItem)
        if item is None: return
        
        if category == Project.DEVICES_LABEL:
            pass
        elif category == Project.SCENES_LABEL:
            pass
        elif category == Project.CONFIGURATIONS_LABEL:
            pass
        elif category == Project.MACROS_LABEL:
            pass
        elif category == Project.MONITORS_LABEL:
            item.setIcon(icons.folderRecord if project.isMonitoring else icons.folder)


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

        projectItem = self._getProjectItemForObject(device)
        # Find folder for devices
        parentItem = self.getCategoryItem(Project.DEVICES_LABEL, projectItem)
        parentItem.appendRow(item)
        self.signalExpandIndex.emit(self.indexFromItem(parentItem), True)


    def insertDeviceItem(self, row, device):
        """
        This function inserts the given \device at the given \row of the model.
        """
        item = self.createDeviceItem(device)
        
        projectItem = self._getProjectItemForObject(device)
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
        deviceGroup.signalDeviceGroupStatusChanged.connect(self.updateDeviceItems)

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

        projectItem = self._getProjectItemForObject(deviceGroup)
        # Find folder for devices
        parentItem = self.getCategoryItem(Project.DEVICES_LABEL, projectItem)
        parentItem.appendRow(item)
        self.signalExpandIndex.emit(self.indexFromItem(parentItem), True)


    def insertDeviceGroupItem(self, row, deviceGroup):
        """
        This function inserts the given \deviceGroup at the given \row of the model.
        """
        item = self.createDeviceGroupItem(deviceGroup)
        
        projectItem = self._getProjectItemForObject(deviceGroup)
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
        """ put the running macros from the systemHash into the project model
        """
        macros = {}
        hash = get_manager().systemHash.get("macro", Hash())
        for k, v, a in hash.iterall():
            macros.setdefault((a["project"], a["module"]), []).append(k)

        for row in range(self.rowCount()):
            projectItem = self.item(row)
            project = projectItem.data(ProjectModel.ITEM_OBJECT)
            item = self.getCategoryItem(Project.MACROS_LABEL, projectItem)
            for r in range(item.rowCount()):
                module_item = item.child(r)
                proj_name = project.name
                macro_title = module_item.text()
                macro_instances = macros.get((proj_name, macro_title), [])
                module_item.removeRows(0, module_item.rowCount())
                macro_model = module_item.data(ProjectModel.ITEM_OBJECT)
                # Add macro instances to macro model
                macro_model.instances = macro_instances
                for instance in macro_instances:
                    classId = hash[instance, "classId"]
                    childItem = QStandardItem(classId)
                    device = getDevice(k)
                    childItem.setData(device, ProjectModel.ITEM_OBJECT)
                    childItem.setEditable(False)
                    module_item.appendRow(childItem)
                    data = {'model': macro_model, 'instance': instance}
                    # Create KaraboBroadcastEvent
                    broadcast_event(KaraboBroadcastEvent(
                        KaraboEventSender.ConnectMacroInstance, data))

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
        if deviceGroup.status != "offline" and deviceGroup.error:
            item.setIcon(icons.deviceInstanceError)
        else:
            item.setIcon(dict(error=icons.deviceInstanceError,
                              noserver=icons.deviceOfflineNoServer,
                              noplugin=icons.deviceOfflineNoPlugin,
                              offline=icons.deviceGroupOffline,
                              incompatible=icons.deviceIncompatible,
                              ).get(deviceGroup.status, icons.deviceGroupInstance))


    def updateMonitorItem(self, monitor):
        item = self.findItem(monitor)
        item.setText(monitor.name)


    def createSceneItem(self, sceneModel):
        """
        This function creates a QStandardItem for the given \sceneModel and
        returns it.
        """
        item = QStandardItem(sceneModel.title)
        item.setIcon(icons.image)
        item.setData(sceneModel, ProjectModel.ITEM_OBJECT)
        item.setEditable(False)
        item.setToolTip(sceneModel.title)
        
        return item


    def addSceneItem(self, sceneModel):
        """ This function adds the given `sceneModel` at the right position to
            the model.
        """
        item = self.createSceneItem(sceneModel)

        projectItem = self._getProjectItemForObject(sceneModel)
        # Find folder for scenes
        parentItem = self.getCategoryItem(Project.SCENES_LABEL, projectItem)
        parentItem.appendRow(item)
        self.signalExpandIndex.emit(self.indexFromItem(parentItem), True)


    def insertSceneItem(self, row, sceneModel):
        """ This function inserts the given \sceneModel at the given \row of the
            model.
        """
        item = self.createSceneItem(sceneModel)
        
        projectItem = self._getProjectItemForObject(sceneModel)
        # Find folder for scenes
        parentItem = self.getCategoryItem(Project.SCENES_LABEL, projectItem)
        parentItem.insertRow(row, item)


    def renameScene(self, sceneModel):
        item = self.findItem(sceneModel)
        item.setText(sceneModel.title)
        # XXX: TODO update dirty flag

        data = {'model': sceneModel}
        # Create KaraboBroadcastEvent
        broadcast_event(KaraboBroadcastEvent(
            KaraboEventSender.RenameSceneView, data))

    def createConfigurationItem(self, deviceId, configuration):
        """
        This function creates QStandardItems for the given \deviceId and 
        \configuration as child item and returns them both.
        """
        projectItem = self._getProjectItemForObject(configuration)
        # Find folder for configurations
        labelItem = self.getCategoryItem(Project.CONFIGURATIONS_LABEL, projectItem)
        
        # Check, if item for this deviceId already exists
        deviceItem = self.findItemString(deviceId, labelItem)
        if deviceItem is None:
            # Add item for device it belongs to
            deviceItem = QStandardItem(deviceId)
            deviceItem.setEditable(False)
            deviceItem.setToolTip(deviceId)
            labelItem.appendRow(deviceItem)
        
        # Add item with configuration file
        configItem = QStandardItem(configuration.filename)
        configItem.setIcon(icons.file)
        configItem.setData(configuration, ProjectModel.ITEM_OBJECT)
        configItem.setEditable(False)
        configItem.setToolTip(configuration.filename)
        
        return labelItem, deviceItem, configItem


    def addConfigurationItem(self, deviceId, configuration):
        labelItem, deviceItem, configItem = self.createConfigurationItem(deviceId, configuration)
        deviceItem.appendRow(configItem)
        
        self.signalExpandIndex.emit(self.indexFromItem(labelItem), True)
        self.signalExpandIndex.emit(self.indexFromItem(deviceItem), True)


    def insertConfigurationItem(self, row, deviceId, configuration):
        labelItem, deviceItem, configItem = self.createConfigurationItem(deviceId, configuration)
        deviceItem.insertRow(row, configItem)
        
        self.signalExpandIndex.emit(self.indexFromItem(labelItem), True)
        self.signalExpandIndex.emit(self.indexFromItem(deviceItem), True)

    def createMacroItem(self, macroModel):
        """
        This function creates a QStandardItem for the given \sceneModel and
        returns it.
        """
        item = QStandardItem(macroModel.title)
        item.setIcon(icons.file)
        item.setData(macroModel, ProjectModel.ITEM_OBJECT)
        item.setEditable(False)
        item.setToolTip(macroModel.title)
        
        return item

    def addMacroItem(self, macroModel):
        item = self.createMacroItem(macroModel)

        projectItem = self._getProjectItemForObject(macroModel)
        # Find folder for scenes
        parentItem = self.getCategoryItem(Project.MACROS_LABEL, projectItem)
        parentItem.appendRow(item)
        self.signalExpandIndex.emit(self.indexFromItem(parentItem), True)

    def insertMacroItem(self, row, macroModel):
        """ This function inserts the given \macroModel at the given \row of the
            model.
        """
        item = self.createMacroItem(macroModel)
        
        projectItem = self._getProjectItemForObject(macroModel)
        # Find folder for scenes
        parentItem = self.getCategoryItem(Project.MACROS_LABEL, projectItem)
        parentItem.insertRow(row, item)

    def renameMacro(self, macroModel):
        item = self.findItem(macroModel)
        item.setText(macroModel.title)
        # XXX: TODO update dirty flag

        data = {'model': macroModel}
        # Create KaraboBroadcastEvent
        broadcast_event(KaraboBroadcastEvent(
            KaraboEventSender.RenameMacro, data))

    def createMonitorItem(self, monitor):
        """
        This function creates a QStandardItem for the given \monitor and
        returns it.
        """
        item = QStandardItem(monitor.name)
        item.setIcon(icons.trendline)
        item.setData(monitor, ProjectModel.ITEM_OBJECT)
        item.setEditable(False)

        item.setToolTip("{}".format(monitor.name))
        
        return item


    def addMonitorItem(self, monitor):
        """
        This function adds the given \monitor at the right position to the model.
        """
        item = self.createMonitorItem(monitor)

        projectItem = self._getProjectItemForObject(monitor)
        # Find folder for devices
        parentItem = self.getCategoryItem(Project.MONITORS_LABEL, projectItem)
        parentItem.appendRow(item)
        self.signalExpandIndex.emit(self.indexFromItem(parentItem), True)

    def insertMonitorItem(self, row, monitor):
        """
        This function inserts the given ``monitor`` at the given ``row`` of the
        model.
        """
        item = self.createMonitorItem(monitor)
       
        projectItem = self._getProjectItemForObject(monitor)
        # Find folder for devices
        parentItem = self.getCategoryItem(Project.MONITORS_LABEL, projectItem)
        parentItem.insertRow(row, item)

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
                            hsh, _ = device.toHash()  # Ignore attributes
                            device.initConfig = hsh
                        device.parameterEditor.clear()


    def updateNeeded(self):
        # Update project device view 
        self.updateDeviceItems()
        # Select index again, in case device status changed - show correct page
        self.selectIndex(self.currentIndex())
        # Update deviceDialog data
        if self.deviceDialog is not None:
            self.deviceDialog.updateServerTopology(get_manager().systemHash)
        self.updateMacros()


    def currentDevice(self):
        device = self.currentIndex().data(ProjectModel.ITEM_OBJECT)
        if not isinstance(device, Device) and not isinstance(device, DeviceGroup):
            return None
        
        return device

    def currentScene(self):
        scene = self.currentIndex().data(ProjectModel.ITEM_OBJECT)
        if not isinstance(scene, SceneModel):
            return None
        
        return scene

    def currentMacro(self):
        macro = self.currentIndex().data(ProjectModel.ITEM_OBJECT)
        if isinstance(macro, MacroModel):
            return macro

    def currentMonitor(self):
        monitor = self.currentIndex().data(ProjectModel.ITEM_OBJECT)
        if not isinstance(monitor, Monitor):
            return None
        
        return monitor


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

    def _getProjectItemForObject(self, obj):
        """ Returns the project item of the given `obj`.
        """
        item = self.findItem(obj)
        if item is None:
            return None

        while not isinstance(obj, Project):
            item = item.parent()
            obj = item.data(ProjectModel.ITEM_OBJECT)
        return item

    def getProjectForObject(self, obj):
        """ Returns the project object of the given `obj`.
        """
        index = self.findIndex(obj)
        if index is None:
            return None

        while not isinstance(obj, Project):
            index = index.parent()
            obj = index.data(ProjectModel.ITEM_OBJECT)
        return obj

    def currentProject(self):
        """
        This function returns the current project from which something might
        be selected.
        """
        index = self.currentIndex()
        if not index.isValid():
            return None

        obj = index.data(ProjectModel.ITEM_OBJECT)
        return self.getProjectForObject(obj)

    def modifiedProjects(self):
        """
        A list of modified projects is returned.
        """
        return [p for p in self.projects if p.isModified]

    def _localCloudProjectPath(self, projectName):
        """
        The local file system of the given CLOUD \projectName is returned.
        """
        network = get_network()
        return os.path.join(globals.KARABO_PROJECT_FOLDER, network.username,
                            projectName)

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

    def selectAllProjects(self):
        sm = self.selectionModel
        sm.clearSelection()
        for i in range(self.rowCount()):
            sm.select(self.index(i, 0), sm.Select)

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
                    project.saveProject()
            
            self.projectClose(project)

        return True


    def projectClose(self, project):
        """
        This function closes the project related scenes and removes it from the
        project list.
        """
        for sceneModel in project.scenes:
            data = {'model': sceneModel}
            # Create KaraboBroadcastEvent
            broadcast_event(KaraboBroadcastEvent(
                KaraboEventSender.RemoveSceneView, data))

        for macroModel in project.macros:
            data = {'model': macroModel}
            # Create KaraboBroadcastEvent
            broadcast_event(KaraboBroadcastEvent(
                KaraboEventSender.RemoveMacro, data))
        
        self.removeProject(project)
        
        if project.access == ProjectAccess.CLOUD:
            network = get_network()
            network.onCloseProject(project.basename)
            os.remove(self._localCloudProjectPath(project.basename))


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
        project.signalSceneInserted.connect(self.insertSceneItem)
        project.signalConfigurationAdded.connect(self.addConfigurationItem)
        project.signalConfigurationInserted.connect(self.insertConfigurationItem)
        project.signalMacroAdded.connect(self.addMacroItem)
        project.signalMacroInserted.connect(self.insertMacroItem)
        project.signalMonitorAdded.connect(self.addMonitorItem)
        project.signalMonitorInserted.connect(self.insertMonitorItem)
        
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
        project.saveProject()
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
            msg = "Open project <b>{}</b> failed!<br>Reason: '{}'".format(
                project.name, e)
            MessageBox().showError(msg, "Open failed")
            raise

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
        
        project.saveProject()
        
        return project


    def projectSaveAs(self, filename, access, project=None):
        """
        This function saves the \project into the file \filename.

        If the \project is None, the current project is taken.
        """
        if project is None:
            project = self.currentProject()

        project.access = access
        project.saveProject(filename=filename)
        project.filename = filename
        self.onProjectModified(project)
        
        return project

    def writeCloudProjectData(self, projectName, data):
        # Write cloud project to local file system
        filename = self._localCloudProjectPath(projectName)
        with open(filename, "wb") as out:
            out.write(data)
        
        return filename

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
        manager = get_manager()
        if not self.deviceDialog.updateServerTopology(manager.systemHash, device):
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
                    config, _ = device.toHash()  # Ignore returned attributes
            else:
                config = None
            
            # Remove device of project and get index for later insert to keep the
            # order
            index = self.removeObject(project, device, False)
            
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
            index = self.removeObject(project, device, False)
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
            index = self.removeObject(project, deviceGroup, False)
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

        project = self.getProjectForObject(device)
        for index in range(dialog.startIndex, dialog.endIndex+1):
            deviceId = "{}{}".format(dialog.displayPrefix, index)
            newDevice = self.addDevice(project, device.serverId,
                                       device.classId, deviceId, device.ifexists)
            
            if device.descriptor is not None:
                config, _ = device.toHash()  # Ignore returned attributes
            else:
                config = device.initConfig
            
            newDevice.initConfig = config
        
        self.selectObject(newDevice)


    def editScene(self, sceneModel=None):
        """ This method changes the title of the scene model.
        """
        dialog = SceneDialog(sceneModel)
        if dialog.exec_() == QDialog.Rejected:
            return

        if sceneModel is None:
            self.addScene(self.currentProject(), dialog.sceneName())
        else:
            sceneModel.title = dialog.sceneName()
            self.renameScene(sceneModel)

    def addScene(self, project, title, filename_or_fileobj=None):
        """
        Create new SceneModel object for given \project.
        """
        sceneModel = project.getScene(title)
        if sceneModel is not None:
            reply = QMessageBox.question(None, 'Scene already exists',
                "Another scene with the same name \"<b>{}</b>\" <br> "
                "already exists. Do you want to overwrite it?".format(title),
                QMessageBox.Yes | QMessageBox.No, QMessageBox.No)

            if reply == QMessageBox.No:
                return None

            # Overwrite existing scene
            index = self.removeObject(project, sceneModel, False)
            sceneModel = self.insertScene(index, project, title,
                                          filename_or_fileobj)
        else:
            sceneModel = read_scene(filename_or_fileobj)
            # Set the scene model title
            sceneModel.title = title
            project.addScene(sceneModel)

        self.openScene(sceneModel, project)
        self.selectObject(sceneModel)

        return sceneModel

    def closeScene(self, sceneModel):
        data = {'model': sceneModel}
        # Create KaraboBroadcastEvent
        broadcast_event(KaraboBroadcastEvent(
            KaraboEventSender.RemoveSceneView, data))

    def insertScene(self, index, project, title, filename_or_fileobj=None):
        """
        Insert a scene model for the given \project with the given data.
        """
        sceneModel = read_scene(filename_or_fileobj)
        sceneModel.title = title
        project.insertScene(index, sceneModel)

        return sceneModel

    def duplicateScene(self, oldSceneModel):
        dialog = DuplicateDialog(oldSceneModel.title[:-4])
        if dialog.exec_() == QDialog.Rejected:
            return

        project = self.getProjectForObject(oldSceneModel)
        xml = write_scene(oldSceneModel)
        fileObj = StringIO(xml)
        for index in range(dialog.startIndex, dialog.endIndex+1):
            title = "{}{}".format(dialog.displayPrefix, index)
            newSceneModel = self.addScene(project, title, fileObj)
            fileObj.seek(0)
            # XXX: TODO set dirty flag project modified
        self.selectObject(newSceneModel)

    def openScene(self, sceneModel, project=None):
        """ This method gets a `sceneModel` and triggers a signal to open a
            scene view in the GUI.
        """
        if project is None:
            project = self.getProjectForObject(sceneModel)
        data = {'model': sceneModel, 'project': project}
        # Create KaraboBroadcastEvent
        broadcast_event(KaraboBroadcastEvent(
            KaraboEventSender.OpenSceneView, data))

    def openSceneLink(self, title, project):
        sceneModel = project.getScene(title)
        if sceneModel is not None:
            self.openScene(sceneModel)

    def editMacro(self, macroModel=None):
        """ This method changes the title of the macro model.
        """
        dialog = MacroDialog(macroModel.title if macroModel is not None else "")
        if dialog.exec_() == QDialog.Rejected:
            return

        if macroModel is None:
            self.addMacro(self.currentProject(), dialog.name)
        else:
            macroModel.title = dialog.name
            self.renameMacro(macroModel)

    def addMacro(self, project, title, filename_or_fileobj=None):
        """
        Create new MacroModel object for given \project.
        """
        macroModel = project.getMacro(title)
        if macroModel is not None:
            reply = QMessageBox.question(None, 'Macro already exists',
                "Another macro with the same name \"<b>{}</b>\" <br> "
                "already exists. Do you want to overwrite it?".format(title),
                QMessageBox.Yes | QMessageBox.No, QMessageBox.No)

            if reply == QMessageBox.No:
                return None

            # Overwrite existing macro
            index = self.removeObject(project, macroModel, False)
            macroModel = self.insertMacro(index, project, title,
                                          filename_or_fileobj)
        else:
            macroModel = read_macro(filename_or_fileobj)
            # Set the scene model title
            macroModel.title = title
            project.addMacro(macroModel)

        self.openMacro(macroModel, project)
        self.selectObject(macroModel)

        return macroModel

    def insertMacro(self, index, project, title, filename_or_fileobj=None):
        """
        Insert a macro model for the given \project with the given data.
        """
        macroModel = read_macro(filename_or_fileobj)
        macroModel.title = title
        project.insertMacro(index, macroModel)

        return macroModel

    def openMacro(self, macroModel, project=None):
        """ This method gets a ``macroModel`` and triggers a signal to open a
            macro panel in the GUI.
        """
        if project is None:
            project = self.getProjectForObject(macroModel)
        data = {'model': macroModel, 'project': project}
        # Create KaraboBroadcastEvent
        broadcast_event(KaraboBroadcastEvent(
            KaraboEventSender.OpenMacro, data))

    def editMonitor(self, monitor=None):
        """
        Within a dialog the properties of a monitor can be modified.
        
        Depending on the given parameter \monitor (either None or set) it is
        either created or edited via the dialog.
        """
        # Get project name
        project = self.currentProject()
        
        # Show dialog to select plugin
        monitorDialog = MonitorDialog(project, monitor)
        if monitorDialog.exec_() == QDialog.Rejected:
            return
        
        # Create hash object for monitor
        h = Hash()
        h.set("name", monitorDialog.name)
        h.set("deviceId", monitorDialog.deviceId)
        h.set("deviceProperty", monitorDialog.deviceProperty)
        h.set("metricPrefixSymbol", monitorDialog.metricPrefixSymbol)
        h.set("unitSymbol", monitorDialog.unitSymbol)
        h.set("format", monitorDialog.format)
        
        if monitor is not None:
            monitor.name = monitorDialog.name
            monitor.config = h
            # Update view
            self.updateMonitorItem(monitor)
            project.setModified(True)
        else:
            monitor = self.addMonitor(project, monitorDialog.name, h)

        self.selectObject(monitor)


    def addMonitor(self, project, name, config):
        """
        Add a monitor for the given \project with the given data.
        """
        monitor = project.getMonitor(name)
        if monitor is not None:
            reply = QMessageBox.question(None, 'Monitor already exists',
                "Another monitor with the same name \"<b>{}</b>\" <br> "
                "already exists. Do you want to overwrite it?".format(name),
                QMessageBox.Yes | QMessageBox.No, QMessageBox.No)

            if reply == QMessageBox.No:
                return None

            # Overwrite existing monitor
            index = self.removeObject(project, monitor, False)
            monitor = self.insertMonitor(index, project, name, config)
            return monitor
        
        monitor = Monitor(name, config)
        project.addMonitor(monitor)
        
        return monitor

    def insertMonitor(self, index, project, name, config):
        """
        Insert a monitor for the given \project with the given data.
        """
        monitor = Monitor(name, config)
        project.insertMonitor(index, monitor)
        
        return monitor

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
                    conf = getDevice(object.id)
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
                    project.saveProject()
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
        project = self.currentProject()
        fn = getOpenFileName(caption="Load macro",
                             filter="Python Macros (*.py)")
        if not fn:
            return
        # Create macro model
        title = os.path.splitext(os.path.basename(fn))[0]
        self.addMacro(project, title, fn)

    def onDuplicateMacro(self):
        print("TODO: duplicate macro...")


    def onEditMonitor(self):
        self.editMonitor(self.currentMonitor())


    def onDuplicateMonitor(self):
        print("TODO: duplicate monitor...")


    def onDefineMonitorFilename(self):
        filename = getSaveFileName(
                        caption="Filename for monitors",
                        filter="Comma-separated value files (*.csv)",
                        suffix="csv",
                        selectFile=self.currentProject().monitorFilename)

        if filename:
            self.currentProject().monitorFilename = filename


    def onDefineMonitorInterval(self):
        interval, ok = QInputDialog.getDouble(None, "Monitor interval",
            "Enter the interval (in seconds) the monitor should be saved.\n"
            "Enter 0, if it should be triggered by property changes.",
            self.currentProject().monitorInterval)
        if ok:
            self.currentProject().monitorInterval = interval


    def onStartMonitoring(self):
        project = self.currentProject()
        project.startMonitoring()
        # Update monitors icon
        self.updateCategoryIcon(Project.MONITORS_LABEL, project)
               
                
    def onStopMonitoring(self):
        project = self.currentProject()
        project.stopMonitoring()
        # Update monitors icon
        self.updateCategoryIcon(Project.MONITORS_LABEL, project)


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
        sceneModel = self.currentScene()
        fn = getSaveFileName(caption="Save scene to file",
                             filter="SVG (*.svg)",
                             suffix="svg",
                             selectFile=sceneModel.title)
        if not fn:
            return

        if not fn.endswith(".svg"):
            fn = "{}.svg".format(fn)

        with open(fn, "w") as fout:
            fout.write(write_scene(sceneModel))

    def onOpenScene(self):
        project = self.currentProject()
        fn = getOpenFileName(caption="Open scene",
                             filter="SVG (*.svg)")
        if not fn:
            return
        # Create scene model
        self.addScene(project, os.path.basename(fn), fn)

    def onRemove(self):
        """
        This slot removes the currently selected index from the model.
        """
        selectedIndexes = self.selectionModel.selectedIndexes()
        nbSelected = len(selectedIndexes)
        if nbSelected > 1:
            reply = QMessageBox.question(None, 'Delete items',
                "Do you really want to delete selected items?",
                QMessageBox.Yes | QMessageBox.No, QMessageBox.No)

            if reply == QMessageBox.No:
                return

        # The removal needs to be done in two steps:
        # 1. Fetch data which is about to be removed
        # list with tuples including project and object
        rm_proj_obj = []
        while selectedIndexes:
            index = selectedIndexes.pop()
            obj = index.data(ProjectModel.ITEM_OBJECT)
            # Get project to given object
            project = self.getProjectForObject(obj)
            rm_proj_obj.append((project, obj))

        # 2. Remove data from project and update tree
        while rm_proj_obj:
            project, obj = rm_proj_obj.pop()
            self.removeObject(project, obj, nbSelected == 1)

    def onRemoveConfiguration(self):
        selectedIndexes = self.selectionModel.selectedIndexes()
        nbSelected = len(selectedIndexes)
        if nbSelected > 1:
            reply = QMessageBox.question(None, 'Delete items',
                "Do you really want to delete selected items?",
                QMessageBox.Yes | QMessageBox.No, QMessageBox.No)

            if reply == QMessageBox.No:
                return

        while selectedIndexes:
            index = selectedIndexes.pop()
            parentIndex = index.parent()
            deviceId = parentIndex.data()
            configuration = index.data(ProjectModel.ITEM_OBJECT)
            
            project = self.getProjectForObject(configuration)
            project.removeConfiguration(deviceId, configuration)
            
            if project.configurations.get(deviceId) is None:
                # Remove item for deviceId, if no configuration available anymore
                self.removeRow(parentIndex.row(), parentIndex.parent())

    def onRemoveDevices(self):
        reply = QMessageBox.question(None, 'Delete devices',
            "Do you really want to delete all devices?",
            QMessageBox.Yes | QMessageBox.No, QMessageBox.No)

        if reply == QMessageBox.No:
            return

        project = self.currentProject()
        while project.devices:
            device = project.devices[-1]
            self.removeObject(project, device, False)

    def removeObject(self, project, obj, showConfirm=True):
        """
        The \obj is removed from the \project.
        """
        if showConfirm:
            reply = QMessageBox.question(None, 'Delete object',
                "Do you really want to delete the object?",
                QMessageBox.Yes | QMessageBox.No, QMessageBox.No)

            if reply == QMessageBox.No:
                return

        index = project.remove(obj)

        if isinstance(obj, SceneModel):
            self.closeScene(obj)

        if isinstance(obj, MacroModel):
            data = {'model': obj}
            # Create KaraboBroadcastEvent
            broadcast_event(KaraboBroadcastEvent(
                KaraboEventSender.RemoveMacro, data))

        if isinstance(obj, Configuration):
            for s in project.scenes:
                # XXX: TODO: remove obj from scene model
                # Remove all related workflow devices of project scenes
                # s.removeItemByObject(obj)
                print("TODO: remove workflow related object from scene model")

        if showConfirm:
            self.selectObject(project)

        return index
