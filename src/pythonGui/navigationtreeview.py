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
        
        # Stores the last selected path of an tree row
        self.__lastSelectionPath = str()
        self.setModel(model)
        
        self.setSelectionMode(QAbstractItemView.SingleSelection)
        self.setSelectionBehavior(QAbstractItemView.SelectRows)
        #self.setSortingEnabled(True)
        #self.sortByColumn(0, Qt.AscendingOrder)
        
        self._setupContextMenu()
        self.customContextMenuRequested.connect(self.onCustomContextMenuRequested)
        self.setDragEnabled(True)


    def _lastSelectionPath(self):
        return self.__lastSelectionPath
    def _setLastSelectionPath(self, lastSelectionPath):
        self.__lastSelectionPath = lastSelectionPath
    lastSelectionPath = property(fget=_lastSelectionPath, fset=_setLastSelectionPath)


    def _setupContextMenu(self):
        self.setContextMenuPolicy(Qt.CustomContextMenu)
        # Device server instance menu
        self.__mServerItem = QMenu(self)
        
        text = "Kill instance"
        self.__acKillServer = QAction(QIcon(":delete"), text, self)
        self.__acKillServer.setStatusTip(text)
        self.__acKillServer.setToolTip(text)
        self.__acKillServer.triggered.connect(self.onKillServer)
        self.__mServerItem.addAction(self.__acKillServer)
        
        # Device class/instance menu
        self.__mClassItem = QMenu(self)
        
        text = "Open configuration (*.xml)"
        self.__acFileOpen = QAction(QIcon(":open"), "Open configuration", self)
        self.__acFileOpen.setStatusTip(text)
        self.__acFileOpen.setToolTip(text)
        self.__acFileOpen.triggered.connect(self.onFileOpen)
        self.__mClassItem.addAction(self.__acFileOpen)
        
        text = "Save configuration as (*.xml)"
        self.__acFileSaveAs = QAction(QIcon(":save-as"), "Save configuration as", self)
        self.__acFileSaveAs.setStatusTip(text)
        self.__acFileSaveAs.setToolTip(text)
        self.__acFileSaveAs.triggered.connect(self.onFileSaveAs)
        self.__mClassItem.addAction(self.__acFileSaveAs)

        text = "Kill instance"
        self.__acKillDevice = QAction(QIcon(":delete"), text, self)
        self.__acKillDevice.setStatusTip(text)
        self.__acKillDevice.setToolTip(text)
        self.__acKillDevice.triggered.connect(self.onKillDevice)
        self.__mClassItem.addAction(self.__acKillDevice)

        self.__mClassItem.addSeparator()


    def currentIndex(self):
        return self.selectionModel().currentIndex()


    def currentIndexType(self):
        """Returns the type of the current index (NODE, DEVICE_SERVER_INSTANCE,
           DEVICE_CLASS, DEVICE_INSTANCE"""

        index = self.currentIndex()
        if not index.isValid():
            return NavigationItemTypes.UNDEFINED
        
        level = self.model().getHierarchyLevel(index)
        if level == 0:
            return NavigationItemTypes.HOST
        elif level == 1:
            return NavigationItemTypes.SERVER
        elif level == 2:
            return NavigationItemTypes.CLASS
        elif level == 3:
            return NavigationItemTypes.DEVICE
        
        return NavigationItemTypes.UNDEFINED


    def indexInfo(self, index=None):
        """ return the info about the index.

        Defaults to the current index if index is None."""
        if index is None:
            index = self.currentIndex()
        return self.model().indexInfo(index)


    def findIndex(self, path):
        # Find modelIndex via path
        return self.model().findIndex(path)


    def selectIndex(self, index):
        if not index:
            return
        
        path = index.internalPointer().path
        if self.lastSelectionPath == path:
            return
        self.lastSelectionPath = path
        
        if index.isValid():
            self.setCurrentIndex(index)
        else:
            self.clearSelection()


    def updateTreeModel(self, config):
        self.model().updateData(config)


    def itemClicked(self):
        index = self.currentIndex()
        
        if not index.isValid():
            return NavigationItemTypes.UNDEFINED
               
        level = self.model().getHierarchyLevel(index)
        row = index.row()

        classId = None
        path = ""
        
        if level == 0:
            type = NavigationItemTypes.HOST
        elif level == 1:
            type = NavigationItemTypes.SERVER
            path = "server." + index.data()
        elif level == 2:
            type = NavigationItemTypes.CLASS
            parentIndex = index.parent()
            serverId = parentIndex.data()
            classId = index.data()
            
            schema = Manager().getClassSchema(serverId, classId)
            path = str("server." + serverId + ".classes." + classId)
            Manager().onSchemaAvailable(dict(key=path, classId=classId, type=type, schema=schema))
        elif level == 3:
            type = NavigationItemTypes.DEVICE
            deviceId = index.data()
            classIndex = index.parent()
            classId = classIndex.data()
            #serverIndex = classIndex.parent()
            #serverId = serverIndex.data()
            
            path = str("device." + deviceId)
            schema = Manager().getDeviceSchema(deviceId)
            Manager().onSchemaAvailable(dict(key=path, classId=classId, type=type, schema=schema))
        
        itemInfo = dict(key=path, classId=classId, type=type, level=level, row=row)
        Manager().onNavigationItemChanged(itemInfo)
        
        return type # Needed in ConfigurationPanel


    def itemChanged(self, itemInfo):
        path = itemInfo.get('key')
        
        if len(path) == 0:
            return
        
        index = self.findIndex(path)
        self.selectIndex(index)


    def selectItem(self, path):
        index = self.findIndex(path)
        self.selectIndex(index)


    def onKillServer(self):
        itemInfo = self.indexInfo()

        serverId = itemInfo.get('serverId')
        
        Manager().killServer(serverId)


    def onKillDevice(self):
        itemInfo = self.indexInfo()

        deviceId = itemInfo.get('deviceId')
        
        Manager().killDevice(deviceId)


    def onFileSaveAs(self):
        itemInfo = self.indexInfo()
        
        classId = itemInfo.get('classId')
        path = itemInfo.get('key')
        
        Manager().onSaveAsXml(str(classId), str(path))


    def onFileOpen(self): # TODO
        type = self.currentIndexType()
        index = self.currentIndex()
        
        configChangeType = None
        classId = str()
        path = str()
        if type is NavigationItemTypes.CLASS:
            configChangeType = ConfigChangeTypes.DEVICE_CLASS_CONFIG_CHANGED
            classId = index.data()
            parentIndex = index.parent()
            path = "server." + parentIndex.data() + ".classes." + classId + ".configuration"
        elif type is NavigationItemTypes.DEVICE:
            configChangeType = ConfigChangeTypes.DEVICE_INSTANCE_CONFIG_CHANGED
            parentIndex = index.parent()
            classId = parentIndex.data()
            path = "devices." + index.data()
        
        # TODO: Remove dirty hack for scientific computing again!!!
        croppedClassId = classId.split("-")
        classId = croppedClassId[0]
        
        Manager().onFileOpen(configChangeType, str(path), str(classId))


    def onCustomContextMenuRequested(self, pos):
        type = self.currentIndexType()
        # Show context menu for DEVICE_CLASS and DEVICE_INSTANCE
        if type is NavigationItemTypes.SERVER:
            self.__mServerItem.exec_(QCursor.pos())
        elif type is NavigationItemTypes.CLASS:
            self.__acKillDevice.setVisible(False)
            self.__mClassItem.exec_(QCursor.pos())
        elif type is NavigationItemTypes.DEVICE:
            self.__acKillDevice.setVisible(True)
            self.__mClassItem.exec_(QCursor.pos())

