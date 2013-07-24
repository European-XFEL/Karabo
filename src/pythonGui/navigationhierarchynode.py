#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 1, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents an item a hierarchical
   navigation treeview."""

__all__ = ["NavigationHierarchyNode"]


from karabo.karathon import *

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class NavigationHierarchyNode(object):


    def __init__(self, displayData, path=str(), parentItem=None):
        super(NavigationHierarchyNode, self).__init__()
        
        self.__parentItem = parentItem
        self.__childItems = list()
        self.__displayData = displayData
        self.__path = path


    def _path(self):
        return self.__path
    path = property(fget=_path)


    def _parentItem(self):
        return self.__parentItem
    parentItem = property(fget=_parentItem)


    def _childItems(self):
        return self.__childItems
    childItems = property(fget=_childItems)


    def appendChildItem(self, childItem):
        self.__childItems.append(childItem)


    def removeChildItem(self, childItem):
        self.__childItems.remove(childItem)


    def indexOfChildItem(self, childItem):
        return self.__childItems.index(childItem)


    def childItem(self, row):
        if row >= 0 and row < len(self.__childItems):
            return self.__childItems[row]
        return -1


    def childCount(self):
        return len(self.__childItems)


    def columnCount(self):
        return None


    def getItem(self, displayData):
        for childItem in self.__childItems:
            if childItem.data(0) == displayData:
                return childItem
        return None


    def hasItem(self, displayData):
        for childItem in self.__childItems:
            if childItem.data(0) == displayData:
                return True
        return False


    def data(self, column):
        return self.__displayData


    def row(self):
        if not self.__parentItem:
            return None
        
        return self.__parentItem.indexOfChildItem(self)


    def clearChildItems(self):
        while len(self.__childItems) > 0:
            childItem = self.__childItems.pop()
            childItem.clearChildItems()


    def printTree(self, indent=-2):
        indent = indent + 2;
        for childItem in self.__childItems:
            print " " * indent, childItem.data(0)
            childItem.printTree(indent)

