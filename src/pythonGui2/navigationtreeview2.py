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
        
        self.setModel(model)
        
        #self.setSelectionMode(QAbstractItemView.SingleSelection)
        #self.setSortingEnabled(True)
        #self.sortByColumn(0, Qt.AscendingOrder)
        
        #self._setupContextMenu()
        #self.customContextMenuRequested.connect(self.onCustomContextMenuRequested)

    def currentIndex(self):
        return self.selectionModel().currentIndex()


    def updateView(self, config):
        self.model().updateData(config)
        self.expandAll()


    def itemClicked(self):
        index = self.currentIndex()
        if not index.isValid():
            return NavigationItemTypes.UNDEFINED
        
        level = self.model().getHierarchyLevel(index)
        row = index.row()
        key = index.data().toString()
        
        print "level", level
        print "row", row
        print "key", key
        
        classId = None
        
        if level == 0:
            type = NavigationItemTypes.NODE
        elif level == 1:
            type = NavigationItemTypes.DEVICE_SERVER_INSTANCE
        elif level == 2:
            type = NavigationItemTypes.DEVICE_CLASS
            parentIndex = index.parent()
            key = parentIndex.data().toString() + "+" + index.data().toString()
            classId = index.data().toString()
            # Get schema from model
            #schema = self.__model.getSchema(level, row)
            #Manager().onSchemaAvailable(dict(key=key, type=type, schema=schema))
        elif level == 3:
            type = NavigationItemTypes.DEVICE_INSTANCE
            parentIndex = index.parent()
            classId = parentIndex.data().toString()
            # Get schema from model
            #schema = self.__model.getSchema(level, row)
            #Manager().onSchemaAvailable(dict(classId=parentIndex.data().toString(), key=key, type=type, schema=schema))
        
        itemInfo = dict(key=key, type=type, classId=classId, level=level, row=row, column=0)
        Manager().onNavigationItemChanged(itemInfo)
        
        return type # Needed in ConfigurationPanel


    def itemChanged(self, itemInfo):

        key = itemInfo.get(QString('key'))
        if key is None:
            key = itemInfo.get('key')
            
        #name = itemInfo.get(QString('name'))
        #if name is None:
        #    name = itemInfo.get('name')

        #type = itemInfo.get(QString('type'))
        #if type is None:
        #    type = itemInfo.get('type')

        level = itemInfo.get('level')
        row = itemInfo.get('row')
        column = itemInfo.get('column')
        index = self.model().findIndex(level, row, column)
        
        #if index.isValid():
        #    self.setCurrentIndex(index)
        #else:
        #    self.clearSelection()

