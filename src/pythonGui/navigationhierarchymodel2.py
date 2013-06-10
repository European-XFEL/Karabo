#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 1, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a model to display a hierarchical
   navigation in a treeview."""

__all__ = ["NavigationHierarchyModel"]

from navigationhierarchynode import *
from libkarathon import *

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class NavigationHierarchyModel(QAbstractItemModel):


    def __init__(self, parent=None):
        super(NavigationHierarchyModel, self).__init__(parent)
        
        self.__rootItem = NavigationHierarchyNode("Hierarchical view")


    def updateData(self, config):
        #print "+++ NavigationHierarchyModel.updateData"
        #print config
        #print ""
        
        self.layoutAboutToBeChanged.emit()
        self.__rootItem.clearChildItems()
        
        # Get server data
        serverKey = "server"
        if config.has(serverKey):
            serverConfig = config.get(serverKey)
            serverIds = list()
            serverConfig.getKeys(serverIds)
            for serverId in serverIds:
                # Get attributes
                #serverAttributes = serverConfig.getAttributes(serverId)
                if serverConfig.hasAttribute(serverId, "host"):
                    host = serverConfig.getAttribute(serverId, "host")
                #version = serverConfig.getAttribute(serverId, "version")

                # Host item already exists?
                hostItem = self.__rootItem.getItem(host)
                if not hostItem:
                    hostItem = NavigationHierarchyNode(host, host, self.__rootItem)
                    self.__rootItem.appendChildItem(hostItem)

                path = "server." + serverId
                serverItem = NavigationHierarchyNode(serverId, path, hostItem)
                hostItem.appendChildItem(serverItem)
                
                if serverConfig.hasAttribute(serverId, "deviceClasses"):
                    classes = serverConfig.getAttribute(serverId, "deviceClasses")
                    for deviceClass in classes:
                        path = "server." + serverId + ".classes." + deviceClass
                        classItem = NavigationHierarchyNode(deviceClass, path, serverItem)
                        serverItem.appendChildItem(classItem)

                # Get classes data
                #classesConfig = serverConfig.get(serverId + ".classes")
                #classes = list()
                #classesConfig.getKeys(classes)
                #for classId in classes:
                #    classItem = NavigationHierarchyNode(classId, serverItem)
                #    serverItem.appendChildItem(classItem)
            
        # Get device data
        deviceKey = "device"
        if config.has(deviceKey):
            deviceConfig = config.get(deviceKey)
            deviceIds = list()
            deviceConfig.getKeys(deviceIds)
            for deviceId in deviceIds:
                # Get attributes
                #deviceAttributes = deviceConfig.getAttributes(deviceId)
                host = deviceConfig.getAttribute(deviceId, "host")
                classId = deviceConfig.getAttribute(deviceId, "classId")
                serverId = deviceConfig.getAttribute(deviceId, "serverId")
                #status = deviceConfig.getAttribute(deviceId, "status")

                # Host item already exists?
                hostItem = self.__rootItem.getItem(host)
                if not hostItem:
                    hostItem = NavigationHierarchyNode(host, host, self.__rootItem)
                    self.__rootItem.appendChildItem(hostItem)

                # Server item already exists?
                serverItem = hostItem.getItem(serverId)
                if not serverItem:
                    path = "server." + serverId
                    serverItem = NavigationHierarchyNode(serverId, path, hostItem)
                    hostItem.appendChildItem(serverItem)

                # Class item already exists?
                classItem = serverItem.getItem(classId)
                if not classItem:
                    path = "server." + serverId + ".classes." + classId
                    classItem = NavigationHierarchyNode(classId, path, serverItem)
                    serverItem.appendChildItem(classItem)

                path = "device." + deviceId
                deviceItem = NavigationHierarchyNode(deviceId, path, classItem)
                classItem.appendChildItem(deviceItem)

        self.layoutChanged.emit()


    def getHierarchyLevel(self, index):
        # Find out the hierarchy level of the selected item
        hierarchyLevel = 0
        seekRoot = index
        while seekRoot.parent() != QModelIndex():
            seekRoot = seekRoot.parent()
            hierarchyLevel += 1
        return hierarchyLevel


    def findIndex(self, path):
        # Recursive search
        return self._rFindIndex(self.__rootItem, path)


    def _rFindIndex(self, item, path):
        for i in xrange(item.childCount()):
            childItem = item.childItem(i)
            resultItem = self._rFindIndex(childItem, path)
            if resultItem:
                return resultItem

        if path == item.path:
            return self.createIndex(item.row(), 0, item)
        return None


    def rowCount(self, parent=QModelIndex()):
        
        if parent.column() > 0:
            #print "rowCount 1"
            return None

        if not parent.isValid():
            #print "root"
            parentItem = self.__rootItem
        else:
            #print "parent"
            parentItem = parent.internalPointer()

        #print "rowCount", parentItem.childCount()
        return parentItem.childCount()


    def columnCount(self, parentIndex=QModelIndex()):
        return 1


    def data(self, index, role=Qt.DisplayRole):
        
        #row = index.row()
        column = index.column()

        if role == Qt.DisplayRole:
            item = index.internalPointer()
            return item.data(column)
        elif (role == Qt.DecorationRole) and (column == 0):
            # Find out the hierarchy level of the selected item
            hierarchyLevel = self.getHierarchyLevel(index)
            if hierarchyLevel == 0:
                return QIcon(":host")
            elif hierarchyLevel == 1:
                #status = self.rawData(level, row, 3).toString()
                #if status == "offline":
                #    return QIcon(":no")
                #elif status == "starting" or status == "online":
                return QIcon(":yes")
            elif hierarchyLevel == 2:
                return QIcon(":device-class")
            elif hierarchyLevel == 3:
                #status = self.rawData(level, row, 4).toString()
                #if status == "error":
                #    return QIcon(":device-instance-error")
                #else:
                return QIcon(":device-instance")

        return QVariant()


    #def flags(self, index):
    #    if not index.isValid():
    #        return None

    #    return Qt.ItemIsEnabled | Qt.ItemIsSelectable


    def headerData(self, section, orientation, role):
        
        if role == Qt.DisplayRole:
            if (orientation == Qt.Horizontal) and (section == 0):
                    return QString("Hierarchical view")
        return QVariant()


    def index(self, row, column, parent=QModelIndex()):
        #print "index"
        
        if not self.hasIndex(row, column, parent):
            return QModelIndex()

        #TreeItem *parentItem;

        if not parent.isValid():
            parentItem = self.__rootItem
        else:
            parentItem = parent.internalPointer()

        childItem = parentItem.childItem(row)
        if childItem:
            return self.createIndex(row, column, childItem)
        else:
            return QModelIndex()


    def parent(self, index):
        
        if not index.isValid():
            return QModelIndex();

        childItem = index.internalPointer()
        if not childItem:
            return QModelIndex()
        
        parentItem = childItem.parentItem

        if parentItem == self.__rootItem:
            return QModelIndex()

        return self.createIndex(parentItem.row(), 0, parentItem)

