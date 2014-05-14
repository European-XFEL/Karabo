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


import qrc_icons

from enums import NavigationItemTypes
from manager import Manager

from PyQt4.QtCore import pyqtSignal, Qt
from PyQt4.QtGui import (QAbstractItemView, QAction, QCursor, QIcon, QMenu, QTreeView)


class NavigationTreeView(QTreeView):
    signalItemChanged = pyqtSignal(object)
    
    
    def __init__(self, parent):
        super(NavigationTreeView, self).__init__(parent)
        
        self.setModel(Manager().systemTopology)
        self.setSelectionModel(self.model().selectionModel)
        self.model().modelReset.connect(self.expandAll)
        
        self.setSelectionMode(QAbstractItemView.SingleSelection)
        self.setSelectionBehavior(QAbstractItemView.SelectRows)
        #self.setSortingEnabled(True)
        #self.sortByColumn(0, Qt.AscendingOrder)
        
        self._setupContextMenu()
        self.customContextMenuRequested.connect(self.onCustomContextMenuRequested)
        self.setDragEnabled(True)
        
        self.model().selectionModel.selectionChanged.connect(self.onSelectionChanged)


    def _setupContextMenu(self):
        self.setContextMenuPolicy(Qt.CustomContextMenu)
        # Device server instance menu
        self.__mServerItem = QMenu(self)
        
        text = "Kill instance"
        self.__acKillServer = QAction(QIcon(":delete"), text, self)
        self.__acKillServer.setStatusTip(text)
        self.__acKillServer.setToolTip(text)
        self.__acKillServer.triggered.connect(self.onKillInstance)
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
        self.__acKillDevice.triggered.connect(self.onKillInstance)
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


    def selectItem(self, path):
        index = self.findIndex(path)
        self.model().selectIndex(index)


    def onKillInstance(self):
        itemInfo = self.indexInfo()
        
        type = itemInfo.get('type')
        
        if type is NavigationItemTypes.DEVICE:
            deviceId = itemInfo.get('deviceId')
            Manager().killDevice(deviceId)
        elif type is NavigationItemTypes.SERVER:
            serverId = itemInfo.get('serverId')
            Manager().killServer(serverId)


    def onFileSaveAs(self):
        itemInfo = self.indexInfo()
        
        deviceId = itemInfo.get('deviceId')
        classId = itemInfo.get('classId')
        serverId = itemInfo.get('serverId')
        
        Manager().onSaveAsXml(deviceId, classId, serverId)


    def onFileOpen(self): # TODO
        itemInfo = self.indexInfo()
        
        deviceId = itemInfo.get('deviceId')
        classId = itemInfo.get('classId')
        serverId = itemInfo.get('serverId')
        
        Manager().onFileOpen(deviceId, classId, serverId)


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


    def onSelectionChanged(self, selected, deselected):
        selectedIndexes = selected.indexes()
        if not selectedIndexes:
            return
        
        index = selectedIndexes[0]

        if not index.isValid():
            return

        level = self.model().getHierarchyLevel(index)

        classId = None
        path = ""

        if level == 2:
            parentIndex = index.parent()
            serverId = parentIndex.data()
            classId = index.data()
            conf = Manager().getClass(serverId, classId)
        elif level == 3:
            deviceId = index.data()
            conf = Manager().getDevice(deviceId)
        else:
            conf = None

        self.signalItemChanged.emit(conf)


    def mimeData(self, items):
        mimeData = QMimeData()
        # Source type
        mimeData.setData("sourceType", "NavigationTreeView")
        return mimeData

