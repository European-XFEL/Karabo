#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 1, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a model to display a hierarchical
   navigation in a treeview."""

__all__ = ["NavigationHierarchyModel"]


from collections import OrderedDict
import globals
from karabo.karathon import HashMergePolicy
from navigationhierarchynode import NavigationHierarchyNode
import manager

from PyQt4.QtCore import (QAbstractItemModel, QByteArray, QMimeData,
                          QModelIndex, Qt, pyqtSignal)
from PyQt4.QtGui import QItemSelectionModel, QIcon

from enums import NavigationItemTypes

class NavigationHierarchyModel(QAbstractItemModel):
    signalItemChanged = pyqtSignal(dict)


    def __init__(self, parent=None):
        super(NavigationHierarchyModel, self).__init__(parent)
        
        # Datastructure to store hierarchy of model
        self.hierarchyTree = OrderedDict()
        
        self.setSupportedDragActions(Qt.CopyAction)
        self.selectionModel = QItemSelectionModel(self)
        self.selectionModel.selectionChanged.connect(self.onSelectionChanged)


    def updateData(self, config):
        # Get last selection path
        #selectedIndexes = self.selectionModel.selectedIndexes()
        #if selectedIndexes:
        #    lastSelectionPath = selectedIndexes[0].internalPointer().path
        #else:
        #    lastSelectionPath = None
        
        self.beginResetModel()
        
        # Define some often used keys
        hostAttrKey = "host"
        versionAttrKey = "version"
        visibilityAttrKey = "visibility"
        
        # Get server data
        serverKey = "server"
        if config.has(serverKey):
            serverConfig = config.get(serverKey)
            serverIds = serverConfig.getKeys()
            
            for serverId in serverIds:
                # Get attributes
                if serverConfig.hasAttribute(serverId, hostAttrKey):
                    host = serverConfig.getAttribute(serverId, hostAttrKey)
                else:
                    host = "UNKNOWN"

                if serverConfig.hasAttribute(serverId, versionAttrKey):
                    version = serverConfig.getAttribute(serverId, versionAttrKey)
                else:
                    version = None

                if serverConfig.hasAttribute(serverId, visibilityAttrKey):
                    visibility = serverConfig.getAttribute(serverId, visibilityAttrKey)
                else:
                    visibility = AccessLevel.OBSERVER
                                
                # TODO: later in view update
                #if visibility > globals.GLOBAL_ACCESS_LEVEL:
                #    continue
                
                # Create entry for host
                hostNode = self.hierarchyTree.setdefault(host, OrderedDict())
                hostNode.path = host
                hostNode.displayName = host
                
                # Create entry for server
                serverNode = self.hierarchyTree[host].setdefault(serverId, OrderedDict())
                serverNode.path = serverId
                serverNode.displayName = serverId
                serverNode.visibility = visibility
                
                # Create entries for classes
                devClaAttrKey = "deviceClasses"
                if serverConfig.hasAttribute(serverId, devClaAttrKey):
                    classes = serverConfig.getAttribute(serverId, devClaAttrKey)
                    
                    visibilitiesAttrKey = "visibilities"
                    if serverConfig.hasAttribute(serverId, visibilitiesAttrKey):
                        visibilities = serverConfig.getAttribute(serverId, visibilitiesAttrKey)
                    else:
                        visibilities = []
                    
                    i = 0
                    for classId in classes:
                        if visibilities[i] <= globals.GLOBAL_ACCESS_LEVEL:
                            classNode = self.hierarchyTree[host][serverId].setdefault(classId, OrderedDict())
                            classNode.path = "{}.{}".format(serverId, classId)
                            classNode.displayName = classId
                            classNode.visibility = visibilities[i]
                        i = i + 1
        
        # Get device data
        deviceKey = "device"
        if config.has(deviceKey):
            deviceConfig = config.get(deviceKey)
            deviceIds = deviceConfig.getKeys()
            for deviceId in deviceIds:
                # Get attributes
                if deviceConfig.hasAttribute(deviceId, visibilityAttrKey):
                    visibility = deviceConfig.getAttribute(deviceId, visibilityAttrKey)
                else:
                    visibility = AccessLevel.OBSERVER
                
                # TODO: later in view update
                #if visibility > globals.GLOBAL_ACCESS_LEVEL:
                #    continue
                
                if deviceConfig.hasAttribute(deviceId, hostAttrKey):
                    host = deviceConfig.getAttribute(deviceId, hostAttrKey)
                else:
                    host = "UNKNOWN"
                
                serverIdAttrKey = "serverId"
                if deviceConfig.hasAttribute(deviceId, serverIdAttrKey):
                    serverId = deviceConfig.getAttribute(deviceId, serverIdAttrKey)
                else:
                    serverId = "unknown-server"
                
                classIdAttrKey = "classId"
                if deviceConfig.hasAttribute(deviceId, classIdAttrKey):
                    classId = deviceConfig.getAttribute(deviceId, classIdAttrKey)
                else:
                    classId = "unknown-class"
                
                if deviceConfig.hasAttribute(deviceId, versionAttrKey):
                    version = deviceConfig.getAttribute(deviceId, versionAttrKey)
                else:
                    version = None
                
                statusAttrKey = "status"
                if deviceConfig.hasAttribute(deviceId, statusAttrKey):
                    status = deviceConfig.getAttribute(deviceId, statusAttrKey)
                else:
                    status = "ok"

                # Host node
                hostNode = self.hierarchyTree.setdefault(host, OrderedDict())
                hostNode.path = host
                hostNode.displayName = host
                
                # Server node
                serverNode = self.hierarchyTree[host].setdefault(serverId, OrderedDict())
                serverNode.path = serverId
                serverNode.displayName = serverId

                # Class node
                classNode = self.hierarchyTree[host][serverId].setdefault(classId, OrderedDict())
                classNode.path = "{}.{}".format(serverId, classId)
                classNode.displayName = classId

                # Device node
                deviceNode = self.hierarchyTree[host][serverId][classId].setdefault(deviceId, OrderedDict())
                deviceNode.path = deviceId
                deviceNode.displayName = deviceId
                deviceNode.status = status
        
        print "===="
        print self.hierarchyTree
        print "===="
        self.endResetModel()
        
        # Set last selection path
        #if lastSelectionPath is not None:
        #    self.selectPath(lastSelectionPath)


    def updateData2(self, config):
        #print "+++ NavigationHierarchyModel.updateData"
        #print config
        #print ""
        
        # TODO: do not use internal hash - just model instead
        if self.currentConfig != config:
            self.currentConfig = config

        selectedIndexes = self.selection_model.selectedIndexes()
        if selectedIndexes:
            lastSelectionPath = selectedIndexes[0].internalPointer().path
        else:
            lastSelectionPath = None
        
        self.beginResetModel()
        self.__rootItem.clearChildItems()
        
        # Get server data
        serverKey = "server"
        if config.has(serverKey):
            serverConfig = config.get(serverKey)
            serverIds = list()
            serverConfig.getKeys(serverIds)
            
            #for serverId in serverIds:
            #    server = root.setdefault(serverId, OrderedDict())
            #    server.visibility =
            #    for classes in clase;
            for serverId in serverIds:
                # Get attributes
                #serverAttributes = serverConfig.getAttributes(serverId)
                #version = serverConfig.getAttribute(serverId, "version")
                
                host = str("UNKNOWN")
                if serverConfig.hasAttribute(serverId, "host"):
                    host = serverConfig.getAttribute(serverId, "host")

                visibility = serverConfig.getAttribute(serverId, "visibility")
                if visibility > globals.GLOBAL_ACCESS_LEVEL:
                    continue
                
                # Host item already exists?
                hostItem = self.__rootItem.getItem(host)
                if not hostItem:
                    hostItem = NavigationHierarchyNode(host, host, self.__rootItem)
                    self.__rootItem.appendChildItem(hostItem)
                
                path = serverId
                serverItem = NavigationHierarchyNode(serverId, path, hostItem)
                hostItem.appendChildItem(serverItem)
            
                if serverConfig.hasAttribute(serverId, "deviceClasses"):
                    classes = serverConfig.getAttribute(serverId, "deviceClasses")
                    visibilities = serverConfig.getAttribute(serverId, "visibilities")
                    i = 0
                    for deviceClass in classes:
                        if visibilities[i] <= globals.GLOBAL_ACCESS_LEVEL:
                            path = "{}.{}".format(serverId, deviceClass)
                            classItem = NavigationHierarchyNode(deviceClass, path, serverItem)
                            serverItem.appendChildItem(classItem)
                        i = i + 1
        
        # Get device data
        deviceKey = "device"
        if config.has(deviceKey):
            deviceConfig = config.get(deviceKey)
            deviceIds = list()
            deviceConfig.getKeys(deviceIds)
            for deviceId in deviceIds:
                # Get attributes
                visibility = deviceConfig.getAttribute(deviceId, "visibility")
                if visibility > globals.GLOBAL_ACCESS_LEVEL:
                    continue
                
                host = deviceConfig.getAttribute(deviceId, "host")
                classId = deviceConfig.getAttribute(deviceId, "classId")
                serverId = deviceConfig.getAttribute(deviceId, "serverId")
                #version = deviceConfig.getAttribute(deviceId, "version")
                status = "ok"
                if deviceConfig.hasAttribute(deviceId, "status"):
                    status = deviceConfig.getAttribute(deviceId, "status")

                # Host item already exists?
                hostItem = self.__rootItem.getItem(host)
                if not hostItem:
                    hostItem = NavigationHierarchyNode(host, host, self.__rootItem)
                    self.__rootItem.appendChildItem(hostItem)
                
                # Server item already exists?
                serverItem = hostItem.getItem(serverId)
                if not serverItem:
                    if serverId == "__none__":
                        path = serverId
                        serverItem = NavigationHierarchyNode(serverId, path, hostItem)
                        hostItem.appendChildItem(serverItem)
                    else:
                        continue

                # Class item already exists?
                classItem = serverItem.getItem(classId)
                if not classItem:
                    if serverId == "__none__":
                        path = "{}.{}".format(serverId, classId)
                        classItem = NavigationHierarchyNode(classId, path, serverItem)
                        serverItem.appendChildItem(classItem)
                    else:
                        continue

                path = deviceId
                deviceItem = NavigationHierarchyNode(deviceId, path, classItem)
                deviceItem.status = status
                classItem.appendChildItem(deviceItem)

        self.endResetModel()
        if lastSelectionPath is not None:
            self.selectPath(lastSelectionPath)


    def has(self, path):
        if self.findIndex(path) is None:
            return False
        return True


    def erase(self, host, serverId, classId, deviceId):
        del self.hierarchyTree[host][serverId][classId][deviceId]


    def merge(self, config):
        # TODO: do not use internal hash - just model instead
        self.currentConfig.merge(config, HashMergePolicy.MERGE_ATTRIBUTES)
        self.updateData(self.currentConfig)


    def instanceNew(self, config):
        print "instanceNew"
        print config
        print ""
        # TODO: do not use internal hash - just model instead
        self.merge(config)


    def instanceUpdated(self, config):
        print "instanceUpdated"
        print config
        print ""
        # TODO: do not use internal hash - just model instead
        self.merge(config)


    def instanceGone(self, path):
        print "instanceGone", path
        # TODO: do not use internal hash - just model instead
        if self.currentConfig.has(path):
            self.currentConfig.erase(path)
            self.updateData(self.currentConfig)


    def onSelectionChanged(self, selected, deselected):
        selectedIndexes = selected.indexes()
        if len(selectedIndexes) < 1:
            return
        
        index = selectedIndexes[0]

        if not index.isValid():
            return

        level = self.getHierarchyLevel(index)

        classId = None
        path = ""

        if level == 0:
            type = NavigationItemTypes.HOST
        elif level == 1:
            type = NavigationItemTypes.SERVER
            path = index.data()
        elif level == 2:
            type = NavigationItemTypes.CLASS
            parentIndex = index.parent()
            serverId = parentIndex.data()
            classId = index.data()

            schema = manager.Manager().getClassSchema(serverId, classId)
            path = "{}.{}".format(serverId, classId)
            manager.Manager().onSchemaAvailable(dict(key=path, classId=classId,
                                                     type=type, schema=schema))
        elif level == 3:
            type = NavigationItemTypes.DEVICE
            deviceId = index.data()
            classIndex = index.parent()
            classId = classIndex.data()
            #serverIndex = classIndex.parent()
            #serverId = serverIndex.data()

            schema = manager.Manager().getDeviceSchema(deviceId)
            path = deviceId
            manager.Manager().onSchemaAvailable(dict(key=path, classId=classId,
                                                     type=type, schema=schema))

        itemInfo = dict(key=path, classId=classId, type=type)
        self.signalItemChanged.emit(itemInfo)


    def getHierarchyLevel(self, index):
        # Find out the hierarchy level of the selected item
        hierarchyLevel = 0
        seekRoot = index
        
        while seekRoot.parent() != QModelIndex():
            seekRoot = seekRoot.parent()
            hierarchyLevel += 1
        return hierarchyLevel


    def selectIndex(self, index):
        if not index:
            return

        self.selectionModel.setCurrentIndex(index,
                                            QItemSelectionModel.ClearAndSelect)


    def findIndex(self, path):
        # Recursive search
        return self._rFindIndex(self.__rootItem, path)


    def _rFindIndex(self, item, path):
        for i in xrange(item.childCount()):
            childItem = item.childItem(i)
            resultItem = self._rFindIndex(childItem, path)
            if resultItem:
                return resultItem

        if (item.path != "") and path.startswith(item.path):
            return self.createIndex(item.row(), 0, item)
        return None


    def selectPath(self, path):
        index = self.findIndex(path)
        if index is not None:
            self.selectionModel.select(index,
                                       QItemSelectionModel.ClearAndSelect)


    def index(self, row, column, parent=QModelIndex()):
        """
        Reimplemented function of QAbstractItemModel.
        """
        if not self.hasIndex(row, column, parent):
            return QModelIndex()

        if not parent.isValid():
            parentNode = self.hierarchyTree
        else:
            parentNode = parent.internalPointer()

        print "parentNode", parentNode
        #childNode = parentNode.internalPointer().childItem(row)
        #if childNode:
        #    return self.createIndex(row, column, childNode)
        #else:
        return QModelIndex()


    def parent(self, index):
        """
        Reimplemented function of QAbstractItemModel.
        """
        if not index.isValid():
            return QModelIndex()
        
        childItem = index.internalPointer()
        if not childItem:
            return QModelIndex()
        
        parentItem = childItem.parentItem
        if not parentItem:
            return QModelIndex()
        
        if parentItem == self.__rootItem:
            return QModelIndex()

        return self.createIndex(parentItem.row(), 0, parentItem)


    def rowCount(self, parent=QModelIndex()):
        """
        Reimplemented function of QAbstractItemModel.
        """
        if parent.column() > 0:
            return None

        if not parent.isValid():
            parentNode = self.hierarchyTree
        else:
            parentNode = parent.internalPointer()

        return len(parentNode.items())


    def columnCount(self, parentIndex=QModelIndex()):
        """
        Reimplemented function of QAbstractItemModel.
        """
        return 1


    def data(self, index, role=Qt.DisplayRole):
        """
        Reimplemented function of QAbstractItemModel.
        """
        row = index.row()
        column = index.column()
        print "data", row, column

        if role == Qt.DisplayRole:
            node = index.internalPointer()
            print "node", node, node.path, node.displayName
            return node.data(column)
        elif (role == Qt.DecorationRole) and (column == 0):
            # Find out the hierarchy level of the selected item
            hierarchyLevel = self.getHierarchyLevel(index)
            if hierarchyLevel == 0:
                return QIcon(":host")
            elif hierarchyLevel == 1:
                #status = self.rawData(level, row, 3)
                #if status == "offline":
                #    return QIcon(":no")
                #elif status == "starting" or status == "online":
                return QIcon(":yes")
            elif hierarchyLevel == 2:
                return QIcon(":device-class")
            elif hierarchyLevel == 3:
                item = index.internalPointer()
                if item.status == "error":
                    return QIcon(":device-instance-error")
                else:
                    return QIcon(":device-instance")


    def flags(self, index):
        """
        Reimplemented function of QAbstractItemModel.
        """
        if not index.isValid():
            return None
        
        ret = Qt.ItemIsEnabled | Qt.ItemIsSelectable
        if self.getHierarchyLevel(index) > 0:
            ret |= Qt.ItemIsDragEnabled
        return ret



    def headerData(self, section, orientation, role):
        """
        Reimplemented function of QAbstractItemModel.
        """
        if role == Qt.DisplayRole:
            if (orientation == Qt.Horizontal) and (section == 0):
                    return "Hierarchical view"


    def indexInfo(self, index):
        if not index.isValid():
            return { }

        level = self.getHierarchyLevel(index)

        if level == 0:
            type = NavigationItemTypes.HOST
            return { }
        elif level == 1:
            type = NavigationItemTypes.SERVER
            serverId = index.data()
            return dict(type=type, serverId=serverId)
        elif level == 2:
            type = NavigationItemTypes.CLASS
            parentIndex = index.parent()
            serverId = parentIndex.data()
            classId = index.data()
            return dict(type=type, serverId=serverId, classId=classId)
        elif level == 3:
            type = NavigationItemTypes.DEVICE
            parentIndex = index.parent()
            classId = parentIndex.data()
            deviceId = index.data()
            return dict(type=type, classId=classId, deviceId=deviceId)


    def mimeData(self, items):
        itemInfo = self.indexInfo(items[0])

        serverId  = itemInfo.get("serverId")
        navigationItemType = itemInfo.get("type")
        displayName = ""

        if navigationItemType == NavigationItemTypes.CLASS:
            displayName = itemInfo.get("classId")

        mimeData = QMimeData()

        mimeData.setData("sourceType", "NavigationTreeView")
        if navigationItemType:
            mimeData.setData("navigationItemType",
                             QByteArray.number(navigationItemType))
        if serverId:
            mimeData.setData("serverId", serverId)
        if displayName:
            mimeData.setData("displayName", displayName)

        return mimeData
