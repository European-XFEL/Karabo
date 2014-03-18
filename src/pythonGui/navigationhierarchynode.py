#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 1, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""
This module contains a class which represents an item of a hierarchical
navigation model.
"""

__all__ = ["NavigationHierarchyNode"]


from enums import AccessLevel


class NavigationHierarchyNode(object):


    def __init__(self, displayName="", path="", parentNode=None):
        super(NavigationHierarchyNode, self).__init__()
        
        self.parentNode = parentNode
        self.childNodes = list()
        
        self.displayName = displayName
        self.path = path
        
        self.visibility = AccessLevel.OBSERVER
        self.status = "ok"


    def appendChildNode(self, childNode):
        self.childNodes.append(childNode)


    def removeChildNode(self, childNode):
        self.childNodes.remove(childNode)


    def indexOfChildNode(self, childNode):
        return self.childNodes.index(childNode)


    def childNode(self, row):
        if row >= 0 and row < len(self.childNodes):
            return self.childNodes[row]
        return -1


    def childCount(self):
        return len(self.childNodes)


    def columnCount(self):
        return None


    def getNode(self, displayName):
        for childNode in self.childNodes:
            if childNode.data(0) == displayName:
                return childNode
        return None


    def hasNode(self, displayName):
        for childNode in self.childNodes:
            if childNode.data(0) == displayName:
                return True
        return False


    def data(self, column):
        return self.displayName


    def row(self):
        if not self.parentNode:
            return None
        
        return self.parentNode.indexOfChildNode(self)


    def printTree(self, indent=-2):
        indent = indent + 2;
        for childNode in self.childNodes:
            print " " * indent, childNode.data(0)
            childNode.printTree(indent)

