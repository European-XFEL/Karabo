#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 1, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a model to display a hierarchical
   navigation in a treeview."""

__all__ = ["NavigationTreeModel"]


from enums import NavigationItemTypes
import globals
from karabo.hash import Hash
import manager
from treenode import TreeNode
import icons

from PyQt4.QtCore import (QAbstractItemModel, QByteArray, QMimeData,
                          QModelIndex, Qt, pyqtSignal)
from PyQt4.QtGui import QItemSelectionModel


class NavigationTreeModel(QAbstractItemModel):
    signalItemChanged = pyqtSignal(object)
    signalInstanceNewReset = pyqtSignal(str) # path


    def __init__(self, parent=None):
        super(NavigationTreeModel, self).__init__(parent)
        
        # Root node of hierarchy tree
        self.rootNode = TreeNode()
        
        self.setSupportedDragActions(Qt.CopyAction)
        self.selectionModel = QItemSelectionModel(self)
        self.selectionModel.selectionChanged.connect(self.onSelectionChanged)


    def _handleServerData(self, config):
        """
        This private function checks whether the incoming configuration has server
        data. If this is the case, this data is put into the tree structure.
        """
        # Define some often used keys
        hostAttrKey = "host"
        versionAttrKey = "version"
        visibilityAttrKey = "visibility"
        
        # Get server data
        serverKey = "server"
        if not config.has(serverKey):
            return
        
        serverConfig = config.get(serverKey)
        for serverId in serverConfig.getKeys():
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

            # Create node for host
            hostNode = self.rootNode.getNode(host)
            if hostNode is None:
                hostNode = TreeNode(host, host, self.rootNode)
                self.rootNode.appendChildNode(hostNode)

            # Create node for server
            serverNode = hostNode.getNode(serverId)
            if serverNode is None:
                serverNode = TreeNode(serverId, serverId, hostNode)
                hostNode.appendChildNode(serverNode)
            serverNode.visibility = visibility

            # Create nodes for classes
            devClaAttrKey = "deviceClasses"
            if serverConfig.hasAttribute(serverId, devClaAttrKey):
                classes = serverConfig.getAttribute(serverId, devClaAttrKey)

                visibilitiesAttrKey = "visibilities"
                if serverConfig.hasAttribute(serverId, visibilitiesAttrKey):
                    visibilities = serverConfig.getAttribute(serverId, visibilitiesAttrKey)
                else:
                    visibilities = []

                for visibility, classId in zip(visibilities, classes):
                    path = "{}.{}".format(serverId, classId)
                    classNode = serverNode.getNode(classId)
                    if classNode is None:
                        classNode = TreeNode(classId, path, serverNode)
                        serverNode.appendChildNode(classNode)
                    classNode.visibility = visibility


    def _handleDeviceData(self, config):
        """
        This private function checks whether the incoming configuration has device
        data. If this is the case, this data is put into the tree structure.
        """
        hostAttrKey = "host"
        versionAttrKey = "version"
        visibilityAttrKey = "visibility"
        
        # Get device data
        deviceKey = "device"
        if not config.has(deviceKey):
            return
        
        deviceConfig = config.get(deviceKey)
        deviceIds = deviceConfig.getKeys()
        for deviceId in deviceIds:
            # Get attributes
            if deviceConfig.hasAttribute(deviceId, visibilityAttrKey):
                visibility = deviceConfig.getAttribute(deviceId, visibilityAttrKey)
            else:
                visibility = AccessLevel.OBSERVER

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
            hostNode = self.rootNode.getNode(host)
            if hostNode is None:
                hostNode = TreeNode(host, host, self.rootNode)
                self.rootNode.appendChildNode(hostNode)

            # Server node
            serverNode = hostNode.getNode(serverId)
            if serverNode is None:
                if serverId == "__none__":
                    serverNode = TreeNode(serverId, serverId, hostNode)
                    hostNode.appendChildNode(serverNode)
                else:
                    continue

            # Class node
            classNode = serverNode.getNode(classId)
            if classNode is None:
                if serverId == "__none__":
                    path = "{}.{}".format(serverId, classId)
                    classNode = TreeNode(classId, path, serverNode)
                    serverNode.appendChildNode(classNode)
                else:
                    continue

            # Device node
            deviceNode = classNode.getNode(deviceId)
            if deviceNode is None:
                deviceNode = TreeNode(deviceId, deviceId, classNode)
                classNode.appendChildNode(deviceNode)
            deviceNode.status = status
            deviceNode.visibility = visibility


    def updateData(self, config):
        """
        This function is called whenever the system topology has changed and the
        view needs an update.
        
        The incoming \config represents the system topology.
        """
        # Get last selection path
        selectedIndexes = self.selectionModel.selectedIndexes()
        if selectedIndexes:
            lastSelectionPath = selectedIndexes[0].internalPointer().path
        else:
            lastSelectionPath = None
        
        self.beginResetModel()
        self._handleServerData(config)
        self._handleDeviceData(config)
        self.endResetModel()
        
        # Set last selection path
        if lastSelectionPath is not None:
            self.selectPath(lastSelectionPath)


    def has(self, path):
        if self.findIndex(path) is None:
            return False
        return True


    def erase(self, instanceId):
        index = self.findIndex(instanceId)
        if (index is None) or (not index.isValid()):
            return None
        
        self.beginResetModel()
        childNode = index.internalPointer()
        parentNode = childNode.parentNode
        parentNode.removeChildNode(childNode)
        self.endResetModel()
        
        return parentNode.path


    def removeExistingInstances(self, config):
        """
        This function checks whether instances already exist in the tree.
        
        if \True, these instance is erased from the tree
        if \False, nothing happens
        
        A list with removed instances is returned.
        """
        
        removedInstanceIds = []
        serverKey = "server"
        # Check servers
        if config.has(serverKey):
            serverConfig = config.get(serverKey)
            serverIds = serverConfig.keys()
            for serverId in serverIds:
                # Check, if serverId is already in central hash
                index = self.findIndex(serverId)
                if index is None:
                    continue
                
                serverNode = index.internalPointer()
                classNodes = serverNode.childNodes
                for classNode in classNodes:
                    # Check for running device instances on server
                    deviceNodes = classNode.childNodes
                    for deviceNode in deviceNodes:
                        self.erase(deviceNode.path)
                        removedInstanceIds.append(deviceNode.path)
                    # Remove configuration page for associated class
                    self.signalInstanceNewReset.emit(classNode.path)
                    
                # Remove server from tree
                self.erase(serverId)
                removedInstanceIds.append(serverId)
        
        return removedInstanceIds

                
    def globalAccessLevelChanged(self):
        self.modelReset.emit()


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

        if level == 2:
            parentIndex = index.parent()
            serverId = parentIndex.data()
            classId = index.data()
            conf = manager.Manager().getClass(serverId, classId)
        elif level == 3:
            deviceId = index.data()
            conf = manager.Manager().getDevice(deviceId)
        else:
            conf = None

        self.signalItemChanged.emit(conf)


    def onServerConnectionChanged(self, isConnected):
        """
        If the server connection is changed, the model needs a reset.
        """
        if not isConnected:
            self.beginResetModel()
            self.rootNode.parentNode = None
            self.rootNode.childNodes = []
            self.endResetModel()


    def getAsHash(self):
        """
        This function creates a hash object, fills it with the current visible
        topology and returns that hash.
        """
        h = Hash()
        self._rGetAsHash(self.rootNode, h)
        return h
        

    def _rGetAsHash(self, node, hash, path=""):
        """
        This function goes recursively through the tree and stores its data in
        a hash object, depending on the visibility.
        """
        if path == "":
            path = node.displayName
        else:
            path = path + "." + node.displayName
            hash.set(path, None)
        
        for childNode in node.childNodes:
            if childNode.visibility > globals.GLOBAL_ACCESS_LEVEL:
                continue
            self._rGetAsHash(childNode, hash, path)


    def getHierarchyLevel(self, index):
        # Find out the hierarchy level of the selected node
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
        return self._rFindIndex(self.rootNode, path)


    def _rFindIndex(self, node, path):
        for i in xrange(node.childCount()):
            childNode = node.childNode(i)
            resultNode = self._rFindIndex(childNode, path)
            if resultNode:
                return resultNode

        if (node.path != "") and (path == node.path):
            return self.createIndex(node.row(), 0, node)
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
            parentNode = self.rootNode
        else:
            parentNode = parent.internalPointer()

        childNode = parentNode.childNode(row)
        if childNode is not None:
            # Consider visibility
            if childNode.visibility > globals.GLOBAL_ACCESS_LEVEL:
                return QModelIndex()
            
            return self.createIndex(row, column, childNode)
        else:
            return QModelIndex()


    def parent(self, index):
        """
        Reimplemented function of QAbstractItemModel.
        """
        if not index.isValid():
            return QModelIndex()
        
        childNode = index.internalPointer()
        if not childNode:
            return QModelIndex()
        
        parentNode = childNode.parentNode
        if not parentNode:
            return QModelIndex()
        
        if parentNode == self.rootNode:
            return QModelIndex()

        # Consider visibility
        if parentNode.visibility > globals.GLOBAL_ACCESS_LEVEL:
            return QModelIndex()

        return self.createIndex(parentNode.row(), 0, parentNode)


    def rowCount(self, parent=QModelIndex()):
        """
        Reimplemented function of QAbstractItemModel.
        """
        if parent.column() > 0:
            #print "rowCount 1"
            return None

        if not parent.isValid():
            parentNode = self.rootNode
        else:
            parentNode = parent.internalPointer()

        return parentNode.childCount()


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

        if role == Qt.DisplayRole:
            node = index.internalPointer()
            return node.displayName
        elif (role == Qt.DecorationRole) and (column == 0):
            # Find out the hierarchy level of the selected node
            hierarchyLevel = self.getHierarchyLevel(index)
            if hierarchyLevel == 0:
                return icons.host
            elif hierarchyLevel == 1:
                return icons.yes
            elif hierarchyLevel == 2:
                return icons.deviceClass
            elif hierarchyLevel == 3:
                node = index.internalPointer()
                if node.status == "error":
                    return icons.deviceInstance
                else:
                    return icons.deviceInstanceError


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


    def mimeData(self, nodes):
        itemInfo = self.indexInfo(nodes[0])

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
