#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 9, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a group of QTreeView items which
   have the same parent.
"""

__all__ = ["SqlTreeModelNode"]


from PyQt4.QtCore import *


class SqlTreeModelNode(object):
    
    def __init__(self, level=0, index=0):
        super(SqlTreeModelNode, self).__init__()
        
        self.__level = level # integer
        self.__index = index # integer
        self.__rows = [] # QList<int>()


    def _getLevel(self):
        return self.__level
    level = property(fget=_getLevel)


    def _getIndex(self):
        return self.__index
    index = property(fget=_getIndex)


    def _getRows(self):
        return self.__rows
    rows = property(fget=_getRows)


    def appendRow(self, row):
        self.__rows.append(row)


    def nbRows(self):
        return len(self.__rows)


    def rowAt(self, index):
        return self.__rows[index]


    def rowIndexOf(self, value):
        if value in self.__rows:
            return self.__rows.index(value)
        return -1
    
    
    def rowValue(self, index, defaultValue):
        if index >= 0 and index < len(self.__rows):
            return self.__rows[index]
        return defaultValue
    
    
    def clearRows(self):
        self.__rows = []

