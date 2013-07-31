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
        
        self._setupContextMenu()
        self.customContextMenuRequested.connect(self.onCustomContextMenuRequested)


### protected ###
    def mouseMoveEvent(self, event):
        QTreeView.mouseMoveEvent(self, event)
        
        if event.buttons() != Qt.LeftButton:
            return
        
        # Disabled for now
        #self._performDrag()
        

### private ###
    def _performDrag(self):
        itemInfo = self.currentIndexInfo()
        if len(itemInfo) == 0:
            return
        
        serverId  = itemInfo.get(QString('serverId'))
        if serverId is None:
            serverId = itemInfo.get('serverId')
        
        navigationItemType = itemInfo.get(QString('type'))
        if navigationItemType is None:
            navigationItemType = itemInfo.get('type')
        
        internalKey = itemInfo.get(QString('internalKey'))
        if internalKey is None:
            internalKey = itemInfo.get('internalKey')
        
        displayName = internalKey
        schema = None
        if navigationItemType is NavigationItemTypes.CLASS:
            displayName = itemInfo.get(QString('classId'))
            if displayName is None:
                displayName = itemInfo.get('classId')
        
        schema = itemInfo.get(QString('schema'))
        if schema is None:
            schema = itemInfo.get('schema')

        mimeData = QMimeData()

        # Put necessary data in MimeData:
        # Source type
        mimeData.setData("sourceType", "NavigationTreeView")
        if navigationItemType:
            # Item type
            mimeData.setData("navigationItemType", QByteArray.number(navigationItemType))
        if serverId:
            # Device server instance id
            mimeData.setData("serverId", QString(serverId).toAscii())
        if internalKey:
            # Internal key
            mimeData.setData("internalKey", QString(internalKey).toAscii())
        if displayName:
            # Display name
            mimeData.setData("displayName", QString(displayName).toAscii())
        if schema:
            # Display name
            mimeData.setData("schema", QString(schema).toAscii())

        drag = QDrag(self)
        drag.setMimeData(mimeData)
        if drag.exec_(Qt.MoveAction) == Qt.MoveAction:
            pass


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
            serverId = index.data().toString()
            path = "server." + serverId
            
            return dict(key=path, type=type, serverId=serverId)
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
            
            return dict(key=path, type=type, deviceId=deviceId)


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
            Manager().onSchemaAvailable(dict(key=path, classId=classId, type=type, schema=schema))
        elif level == 3:
            type = NavigationItemTypes.DEVICE
            deviceId = index.data().toString()
            classIndex = index.parent()
            classId = classIndex.data().toString()
            #serverIndex = classIndex.parent()
            #serverId = serverIndex.data().toString()
            
            path = str("device." + deviceId)
            schema = Manager().getDeviceSchema(deviceId)
            Manager().onSchemaAvailable(dict(key=path, classId=classId, type=type, schema=schema))
        
        itemInfo = dict(key=path, classId=classId, type=type, level=level, row=row)
        Manager().onNavigationItemChanged(itemInfo)
        
        return type # Needed in ConfigurationPanel


    def itemChanged(self, itemInfo):
        key = itemInfo.get(QString('key'))
        if key is None:
            key = itemInfo.get('key')
        
        if len(key) == 0:
            return
        
        index = self.model().findIndex(key)
        
        if self.__prevModelIndex == index:
            return
        self.__prevModelIndex = index
        
        if index and index.isValid():
            self.setCurrentIndex(index)
        else:
            self.clearSelection()


    def selectItem(self, path):
        index = self.model().findIndex(path)
        
        if not index:
            return
        
        if index.isValid():
            self.setCurrentIndex(index)
        else:
            self.clearSelection()


    def onKillServer(self):
        itemInfo = self.currentIndexInfo()

        serverId = itemInfo.get(QString('serverId'))
        if serverId is None:
            serverId = itemInfo.get('serverId')
        
        Manager().killServer(serverId)


    def onKillDevice(self):
        itemInfo = self.currentIndexInfo()

        deviceId = itemInfo.get(QString('deviceId'))
        if deviceId is None:
            deviceId = itemInfo.get('deviceId')
        
        Manager().killDevice(deviceId)


    def onFileSaveAs(self):
        itemInfo = self.currentIndexInfo()
        
        classId = itemInfo.get(QString('classId'))
        if classId is None:
            classId = itemInfo.get('classId')
        
        path = itemInfo.get(QString('key'))
        if path is None:
            path = itemInfo.get('key')
        
        Manager().onSaveAsXml(str(classId), str(path))


    def onFileOpen(self): # TODO
        type = self.currentIndexType()
        index = self.currentIndex()
        
        configChangeType = None
        classId = str()
        instanceId = str()
        if type is NavigationItemTypes.CLASS:
            configChangeType = ConfigChangeTypes.DEVICE_CLASS_CONFIG_CHANGED
            classId = index.data().toString()
            parentIndex = index.parent()
            instanceId = parentIndex.data().toString()+"+"+classId
        elif type is NavigationItemTypes.DEVICE:
            configChangeType = ConfigChangeTypes.DEVICE_INSTANCE_CONFIG_CHANGED
            parentIndex = index.parent()
            classId = parentIndex.data().toString()
            instanceId = index.data().toString()
        
        # TODO: Remove dirty hack for scientific computing again!!!
        croppedClassId = classId.split("-")
        classId = croppedClassId[0]
        
        Manager().onFileOpen(configChangeType, str(instanceId), str(classId))


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

