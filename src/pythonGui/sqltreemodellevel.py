#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 9, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents ...
"""

__all__ = ["SqlTreeModelLevel"]


class SqlTreeModelLevel(object):
    
    def __init__(self, model):
        super(SqlTreeModelLevel, self).__init__()
        
        self.__model = model # QSqlQueryModel
        self.__columnMapping = [] # QList<int>()
        self.__ids = [] # QVector<int>()
        self.__parentIds = [] # QVector<int>()
        self.__nodes = dict() # QHash<int, SqlTreeModelNode>()


    def clear(self):
        self.__ids = []
        self.__nodes = dict()


    def _getModel(self):
        return self.__model
    model = property(fget=_getModel)
    
    
    def setIdAt(self, index, id):
        self.__ids[index] = id
    
    
    def idAt(self, index):
        if index >= 0 and index < len(self.__ids):
            return self.__ids[index]
        return -1


    def idIndexOf(self, id):
        if id in self.__ids:
            return self.__ids.index(id)
        return -1


    def resizeIds(self, size):
        if size < len(self.__ids):
            # Remove elements
            for i in range(len(self.__ids)-size):
                self.__ids.pop()
        elif size > len(self.__ids):
            # Add elements
            for i in range(size-len(self.__ids)):
                key = 0
                self.__ids.append(-1)


    def setParentIdAt(self, index, parentId):
        self.__parentIds[index] = parentId


    def parentIdAt(self, index):
        if index >= 0 and index < len(self.__parentIds):
            return self.__parentIds[index]
        return -1


    def resizeParentIds(self, size):
        if size < len(self.__parentIds):
            # Remove elements
            for i in range(len(self.__parentIds)-size):
                self.__parentIds.pop()
        elif size > len(self.__parentIds):
            # Add elements
            for i in range(size-len(self.__parentIds)):
                self.__parentIds.append(-1)


    def insertNode(self, row, node):
        self.__nodes[row] = node


    def getNodeValueByKey(self, row):
        return self.__nodes.get(row)


    def nbColumnMapping(self):
        return len(self.__columnMapping)


    def appendColumnMapping(self, value):
        self.__columnMapping.append(value)


    def columnMappingAt(self, index):
        if index >= 0 and index < len(self.__columnMapping):
            return self.__columnMapping[index]
        return -1
    
    
    def columnMappingValue(self, index, defaultValue):
        if index >= 0 and index < len(self.__columnMapping):
            return self.__columnMapping[index]
        return defaultValue


    def _getColumnMapping(self):
        return self.__columnMapping
    def _setColumnMapping(self, columnMapping):
        self.__columnMapping = columnMapping
    columnMapping = property(fget=_getColumnMapping, fset=_setColumnMapping)

