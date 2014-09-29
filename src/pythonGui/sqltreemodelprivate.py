#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 9, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents...
"""

__all__ = ["SqlTreeModelPrivate"]


from sqltreemodelnode import SqlTreeModelNode

from PyQt4.QtCore import *


class SqlTreeModelPrivate(object):
    
    def __init__(self, model):
        super(SqlTreeModelPrivate, self).__init__()
        
        self.__sqlTreeModel = model # SqlTreeModel
        self.__columns = -1
        self.__sortColumn = -1
        self.__sortOrder = Qt.AscendingOrder
        
        self.__levelData = [] #QList<SqlTreeModelLevel>()

        self.__headers = []
        self.__root = SqlTreeModelNode()


### public ###
    def appendLevelData(self, sqlTreeModelLevel):
        self.__levelData.append(sqlTreeModelLevel)


    def nbLevelData(self):
        return len(self.__levelData)


    def levelDataAt(self, index):
        if index >= 0 and index < len(self.__levelData):
            return self.__levelData[index]
        return -1


    def _getLevelData(self):
        return self.__levelData
    levelData = property(fget=_getLevelData)


    def _getColumns(self):
        return self.__columns
    def _setColumns(self, columns):
        self.__columns = columns
    columns = property(fget=_getColumns, fset=_setColumns)


    def _getRoot(self):
        return self.__root
    root = property(fget=_getRoot)


    def _getSortColumn(self):
        return self.__sortColumn
    def _setSortColumn(self, sortColumn):
        self.__sortColumn = sortColumn
    sortColumn = property(fget=_getSortColumn, fset=_setSortColumn)


    def _getSortOrder(self):
        return self.__sortOrder
    def _setSortOrder(self, sortOrder):
        self.__sortOrder = sortOrder
    sortOrder = property(fget=_getSortOrder, fset=_setSortOrder)


    def nbHeaders(self):
        return len(self.__headers)


    def resizeHeaders(self, size):
        if size < len(self.__headers):
            # Remove elements
            for i in range(len(self.__headers)-size):
                self.__headers.pop()
        elif size > len(self.__headers):
            # Add elements
            for i in range(size-len(self.__headers)):
                self.__headers.append(dict())


    def getHeaderValueAt(self, index, role):
        if index >= 0 and index < len(self.__headers):
            d = self.__headers[index]
            value = d.get(role)
            if value is not None:
                return value


    def _getHeaders(self):
        return self.__headers
    headers = property(fget=_getHeaders)


    def setHeadersAt(self, index, role, value):
        self.__headers[index][role] = value


    def findNode(self, parent):
        if not parent.isValid():
            return self.__root

        level = self.__sqlTreeModel.levelOf(parent)
        if level < 0:
            return None

        # Get SqlTreeModelLevel
        levelData = self.__levelData[level]

        row = self.__sqlTreeModel.mappedRow(parent)
        if row < 0:
            return None

        return levelData.getNodeValueByKey(row)

