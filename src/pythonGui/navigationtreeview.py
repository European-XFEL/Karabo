#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 29, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the treeview of the navigation
   panel containing the items for the host, device server instance and device
   class/instance.
"""

__all__ = ["NavigationTreeView"]


import const
import qrc_icons

from enums import ConfigChangeTypes
from enums import NavigationItemTypes
from manager import Manager
#from navigationhierarchymodel import NavigationHierarchyModel

from PyQt4.QtCore import *
from PyQt4.QtGui import *

try:
    from PyQt4.QtSql import QSqlQuery
except:
    print "*ERROR* The PyQt4 sql module is not installed"

class NavigationTreeView(QTreeView):


    def __init__(self, parent, model):
        super(NavigationTreeView, self).__init__(parent)
        
        self.__model = model
        self.setModel(self.__model)
        #self.__model.layoutChanged.connect(self.updateView)
        
        self.setSelectionMode(QAbstractItemView.SingleSelection)
        self.setSortingEnabled(True)
        self.sortByColumn(0, Qt.AscendingOrder)
        
        self._setupContextMenu()
        self.customContextMenuRequested.connect(self.onCustomContextMenuRequested)


### protected ###
    def mousePressEvent(self, event):
        QTreeView.mousePressEvent(self, event)
        
        if event.buttons() != Qt.LeftButton:
            return
        
        self._performDrag()
        

### private ###
    def _performDrag(self):
        itemInfo = self.currentIndexInfo()
        if len(itemInfo) == 0:
            return
        
        devSerInsId  = itemInfo.get(QString('devSerInsId'))
        if devSerInsId is None:
            devSerInsId = itemInfo.get('devSerInsId')
        
        navigationItemType = itemInfo.get(QString('type'))
        if navigationItemType is None:
            navigationItemType = itemInfo.get('type')
        
        internalKey = itemInfo.get(QString('internalKey'))
        if internalKey is None:
            internalKey = itemInfo.get('internalKey')
        
        displayName = internalKey
        schema = None
        if navigationItemType is NavigationItemTypes.DEVICE_CLASS:
            displayName = itemInfo.get(QString('devClaId'))
            if displayName is None:
                displayName = itemInfo.get('devClaId')
        
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
        if devSerInsId:
            # Device server instance id
            mimeData.setData("devSerInsId", QString(devSerInsId).toAscii())
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
        self.__mDevSrvInsItem = QMenu(self)
        
        text = "Kill instance"
        self.__acKillDevSrvIns = QAction(QIcon(":delete"), text, self)
        self.__acKillDevSrvIns.setStatusTip(text)
        self.__acKillDevSrvIns.setToolTip(text)
        self.__acKillDevSrvIns.triggered.connect(self.onKillDeviceServerInstance)
        self.__mDevSrvInsItem.addAction(self.__acKillDevSrvIns)
        
        # Device class/instance menu
        self.__mDevClaInsItem = QMenu(self)
        
        text = "Open configuration (*.xml)"
        self.__acFileOpen = QAction(QIcon(":open"), "Open configuration", self)
        self.__acFileOpen.setStatusTip(text)
        self.__acFileOpen.setToolTip(text)
        self.__acFileOpen.triggered.connect(self.onFileOpen)
        self.__mDevClaInsItem.addAction(self.__acFileOpen)
        
        text = "Save configuration as (*.xml)"
        self.__acFileSaveAs = QAction(QIcon(":save-as"), "Save configuration as", self)
        self.__acFileSaveAs.setStatusTip(text)
        self.__acFileSaveAs.setToolTip(text)
        self.__acFileSaveAs.triggered.connect(self.onFileSaveAs)
        self.__mDevClaInsItem.addAction(self.__acFileSaveAs)

        text = "Kill instance"
        self.__acKillDevIns = QAction(QIcon(":delete"), text, self)
        self.__acKillDevIns.setStatusTip(text)
        self.__acKillDevIns.setToolTip(text)
        self.__acKillDevIns.triggered.connect(self.onKillDeviceInstance)
        self.__mDevClaInsItem.addAction(self.__acKillDevIns)

        self.__mDevClaInsItem.addSeparator()


### public ###
    def createNewItem(self, itemInfo, insertIntoDb=False):
        if insertIntoDb:
            # Only insert data once
            self.__model.insertInto(itemInfo)
        # Update view with model...
        self.__model.updateQueries()
        # TODO: can be very expensive when model is huge...
        self.expandAll()


    def selectItem(self, itemInfo):
        #print "selectItem", itemInfo
        id = itemInfo.get(QString('id'))
        if id is None:
            id = itemInfo.get('id')
        
        level = itemInfo.get(QString('level'))
        if level is None:
            level = itemInfo.get('level')
        if level is None:
            # Was just not set, select DEVICE_INSTANCE
            level = 3
        
        index = self.__model.findIndex(level=level, id=id, column=1)
        self.setCurrentIndex(index)


    def itemClicked(self):
        index = self.currentIndex()
        if not index.isValid():
            return NavigationItemTypes.UNDEFINED
        
        level = self.__model.levelOf(index)
        row = self.__model.mappedRow(index)
        rowId = self.__model.rowId(index)
        key = index.data().toString()
        
        devClaId = None
        
        if level == 0:
            type = NavigationItemTypes.NODE
        elif level == 1:
            type = NavigationItemTypes.DEVICE_SERVER_INSTANCE
        elif level == 2:
            type = NavigationItemTypes.DEVICE_CLASS
            parentIndex = index.parent()
            key = parentIndex.data().toString() + "+" + index.data().toString()
            devClaId = index.data().toString()
            # Get schema from model
            schema = self.__model.getSchema(level, row)
            Manager().onSchemaAvailable(dict(key=key, type=type, schema=schema))
        elif level == 3:
            type = NavigationItemTypes.DEVICE_INSTANCE
            parentIndex = index.parent()
            devClaId = parentIndex.data().toString()
            # Get schema from model
            schema = self.__model.getSchema(level, row)
            Manager().onSchemaAvailable(dict(devClaId=parentIndex.data().toString(), key=key, type=type, schema=schema))
        
        itemInfo = dict(key=key, type=type, devClaId=devClaId, level=level, rowId=rowId, column=1)
        Manager().onNavigationItemChanged(itemInfo)
        
        return type # Needed in ConfigurationPanel


    def itemChanged(self, itemInfo):

        #key = itemInfo.get(QString('key'))
        #if key is None:
        #    key = itemInfo.get('key')
            
        #name = itemInfo.get(QString('name'))
        #if name is None:
        #    name = itemInfo.get('name')

        #type = itemInfo.get(QString('type'))
        #if type is None:
        #    type = itemInfo.get('type')

        level = itemInfo.get('level')
        rowId = itemInfo.get('rowId')
        column = itemInfo.get('column')
        index = self.__model.findIndex(level, rowId, column)
        
        if index.isValid():
            self.setCurrentIndex(index)
        else:
            self.clearSelection()


    def updateDeviceServerInstance(self, itemInfo):
        id = itemInfo.get(QString('id'))
        if id is None:
            id = itemInfo.get('id')
        # Select DEVICE_SERVER_INSTANCE
        self.selectItem(dict(id=id, level=1))


    def updateDeviceInstance(self, itemInfo):
        id = itemInfo.get(QString('id'))
        if id is None:
            id = itemInfo.get('id')
        
        index = self.__model.findIndex(level=3, id=id, column=1)
        parentId = self.__model.rowParentId(index)
        # Select DEVICE_CLASS parent index
        self.selectItem(dict(id=parentId, level=2))


    def currentIndex(self):
        """Returns the current index of the treeview"""
        selection = self.selectedIndexes()
        if len(selection) == 0:
            return QModelIndex()

        return selection[0]


    def currentIndexType(self):
        """Returns the type of the current index (NODE, DEVICE_SERVER_INSTANCE,
           DEVICE_CLASS, DEVICE_INSTANCE"""

        index = self.currentIndex()
        if not index.isValid():
            return NavigationItemTypes.UNDEFINED
        
        level = self.__model.levelOf(index)
        if level == 0:
            return NavigationItemTypes.NODE
        elif level == 1:
            return NavigationItemTypes.DEVICE_SERVER_INSTANCE
        elif level == 2:
            return NavigationItemTypes.DEVICE_CLASS
        elif level == 3:
            return NavigationItemTypes.DEVICE_INSTANCE
        
        return NavigationItemTypes.UNDEFINED


    def currentIndexInfo(self):
        index = self.currentIndex()
        if not index.isValid():
            return dict()
        
        level = self.__model.levelOf(index)
        if level == 0:
            # NODE
            internalKey = index.data().toString()
            return dict(internalKey=internalKey, type=NavigationItemTypes.NODE)
        elif level == 1:
            # DEVICE_SERVER_INSTANCE
            devSerInsId = index.data().toString()
            internalKey = devSerInsId
            return dict(devSerInsId=devSerInsId, internalKey=internalKey, type=NavigationItemTypes.DEVICE_SERVER_INSTANCE)
        elif level == 2:
            # DEVICE_CLASS
            parentIndex = index.parent()
            devSerInsId = parentIndex.data().toString()
            devClaId = index.data().toString()
            internalKey = devSerInsId+"+"+devClaId
            # Get schema
            level = self.__model.levelOf(index)
            row = self.__model.mappedRow(index)
            schema = self.__model.getSchema(level, row)
            return dict(devSerInsId=devSerInsId, devClaId=devClaId, internalKey=internalKey, schema=schema, type=NavigationItemTypes.DEVICE_CLASS)
        elif level == 3:
            # DEVICE_INSTANCE
            parentIndex = index.parent()
            devClaId = parentIndex.data().toString()
            devSerInsId = parentIndex.parent().data().toString()
            internalKey = index.data().toString()
            # Get schema
            level = self.__model.levelOf(index)
            row = self.__model.mappedRow(index)
            schema = self.__model.getSchema(level, row)
            return dict(devSerInsId=devSerInsId, devClaId=devClaId, internalKey=internalKey, schema=schema, type=NavigationItemTypes.DEVICE_INSTANCE)


    def currentInternalDeviceKey(self):
        type = self.currentIndexType()
        if type is NavigationItemTypes.UNDEFINED:
            return str()
        
        index = self.currentIndex()
        if not index.isValid():
            return str()
        
        if type is NavigationItemTypes.DEVICE_CLASS:
            parentIndex = index.parent()
            return parentIndex.data().toString() + "+" + index.data().toString()
        
        return index.data().toString()


    def setErrorState(self, instanceId, hasError):
        self.__model.updateErrorState(instanceId, hasError)


    def updateDeviceInstanceSchema(self, instanceId, schema):
        self.__model.updateDeviceInstanceSchema(instanceId, schema)
        Manager().onSchemaAvailable(dict(key=instanceId, type=NavigationItemTypes.DEVICE_INSTANCE, schema=schema))


### slots ###
    def onKillDeviceInstance(self):
        itemInfo = self.currentIndexInfo()
        devSerInsId = itemInfo.get(QString('devSerInsId'))
        if devSerInsId is None:
            devSerInsId = itemInfo.get('devSerInsId')
        
        internalKey = itemInfo.get(QString('internalKey'))
        if internalKey is None:
            internalKey = itemInfo.get('internalKey')
        Manager().killDeviceInstance(devSerInsId, internalKey)


    def onKillDeviceServerInstance(self):
        Manager().killDeviceServerInstance(self.currentInternalDeviceKey())


    def onFileSaveAs(self):
        itemInfo = self.currentIndexInfo()
        devClaId = itemInfo.get(QString('devClaId'))
        if devClaId is None:
            devClaId = itemInfo.get('devClaId')
        
        internalKey = itemInfo.get(QString('internalKey'))
        if internalKey is None:
            internalKey = itemInfo.get('internalKey')
        
        Manager().onSaveAsXml(str(devClaId), str(internalKey))


    def onFileOpen(self):
        type = self.currentIndexType()
        index = self.currentIndex()
        
        configChangeType = None
        devClaId = str()
        instanceId = str()
        if type is NavigationItemTypes.DEVICE_CLASS:
            configChangeType = ConfigChangeTypes.DEVICE_CLASS_CONFIG_CHANGED
            devClaId = index.data().toString()
            parentIndex = index.parent()
            instanceId = parentIndex.data().toString()+"+"+devClaId
        elif type is NavigationItemTypes.DEVICE_INSTANCE:
            configChangeType = ConfigChangeTypes.DEVICE_INSTANCE_CONFIG_CHANGED
            parentIndex = index.parent()
            devClaId = parentIndex.data().toString()
            instanceId = index.data().toString()
        
        # TODO: Remove dirty hack for scientific computing again!!!
        croppedDevClaId = devClaId.split("-")
        devClaId = croppedDevClaId[0]
        
        Manager().onFileOpen(configChangeType, str(instanceId), str(devClaId))


    def onCustomContextMenuRequested(self, pos):
        type = self.currentIndexType()
        # Show context menu for DEVICE_CLASS and DEVICE_INSTANCE
        if type is NavigationItemTypes.DEVICE_SERVER_INSTANCE:
            self.__mDevSrvInsItem.exec_(QCursor.pos())
        elif type is NavigationItemTypes.DEVICE_CLASS:
            self.__acKillDevIns.setVisible(False)
            self.__mDevClaInsItem.exec_(QCursor.pos())
        elif type is NavigationItemTypes.DEVICE_INSTANCE:
            self.__acKillDevIns.setVisible(True)
            self.__mDevClaInsItem.exec_(QCursor.pos())
        

