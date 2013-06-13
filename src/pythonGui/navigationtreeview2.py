#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 31, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the treeview of the navigation
   panel containing the items for the host, device server instance and device
   class/instance.
"""

__all__ = ["NavigationTreeView"]


import const
import qrc_icons

from enums import *
from manager import Manager

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class NavigationTreeView(QTreeView):


    def __init__(self, parent, model):
        super(NavigationTreeView, self).__init__(parent)
        
        self.__prevModelIndex = None
        self.setModel(model)
        
        self.setSelectionMode(QAbstractItemView.SingleSelection)
        #self.setSortingEnabled(True)
        #self.sortByColumn(0, Qt.AscendingOrder)
        
        #self._setupContextMenu()
        #self.customContextMenuRequested.connect(self.onCustomContextMenuRequested)


    def currentIndex(self):
        return self.selectionModel().currentIndex()
    

    def currentIndexInfo(self):
        index = self.currentIndex()
        if not index.isValid():
            return dict()
        
        level = self.model().getHierarchyLevel(index)
        
        if level == 0:
            type = NavigationItemTypes.HOST
            
            return dict()
        elif level == 1:
            type = NavigationItemTypes.SERVER
            path = "server." + index.data().toString()
            
            return dict(key=path, type=type)
        elif level == 2:
            type = NavigationItemTypes.CLASS
            parentIndex = index.parent()
            serverId = parentIndex.data().toString()
            classId = index.data().toString()
            path = str("server." + serverId + ".classes." + classId + ".configuration")
            
            return dict(key=path, type=type, serverId=serverId, classId=classId)
        elif level == 3:
            type = NavigationItemTypes.DEVICE
            deviceId = index.data().toString()
            path = str("device." + deviceId)
            
            return dict(key=path, type=type)


    def updateView(self, config):
        self.model().updateData(config)
        self.expandAll()


    def itemClicked(self):
        index = self.currentIndex()
        
        if not index.isValid():
            return NavigationItemTypes.UNDEFINED
        
        if self.__prevModelIndex == index:
            return NavigationItemTypes.UNDEFINED
        self.__prevModelIndex = index
        
        level = self.model().getHierarchyLevel(index)
        
        row = index.row()

        classId = None
        path = ""
        
        if level == 0:
            type = NavigationItemTypes.HOST
        elif level == 1:
            type = NavigationItemTypes.SERVER
            path = "server." + index.data().toString()
        elif level == 2:
            type = NavigationItemTypes.CLASS
            parentIndex = index.parent()
            serverId = parentIndex.data().toString()
            classId = index.data().toString()
            
            schema = Manager().getClassSchema(serverId, classId)
            path = str("server." + serverId + ".classes." + classId)
            Manager().onSchemaAvailable(dict(key=path, type=type, schema=schema))
        elif level == 3:
            type = NavigationItemTypes.DEVICE
            deviceId = index.data().toString()
            #classIndex = index.parent()
            #classId = classIndex.data().toString()
            #serverIndex = classIndex.parent()
            #serverId = serverIndex.data().toString()
            
            path = str("device." + deviceId)
            deviceHash = Manager().getDeviceHash(deviceId)
            schema = None
            if deviceHash:
                descriptionPath = path + ".description"
                if deviceHash.has(descriptionPath):
                    schema = deviceHash.get(descriptionPath)
            Manager().onSchemaAvailable(dict(key=path, type=type, schema=schema))
        
        itemInfo = dict(key=path, type=type, level=level, row=row)
        Manager().onNavigationItemChanged(itemInfo)
        
        return type # Needed in ConfigurationPanel


    def itemChanged(self, itemInfo):
        key = itemInfo.get(QString('key'))
        if key is None:
            key = itemInfo.get('key')
        
        index = self.model().findIndex(key)
        
        if self.__prevModelIndex == index:
            return
        self.__prevModelIndex = index
        
        if index.isValid():
            self.setCurrentIndex(index)
        else:
            self.clearSelection()

