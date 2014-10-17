#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 1, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""
This module contains a class which represents a node of a hierarchical model.
"""

__all__ = ["TreeNode"]


from enums import AccessLevel


class TreeNode(object):


    def __init__(self, displayName="", path="", parentNode=None):
        super(TreeNode, self).__init__()
        
        self.parentNode = parentNode
        self.childNodes = list()
        
        self.displayName = displayName
        self.path = path
        
        self.visibility = AccessLevel.OBSERVER
        self.status = "ok"
        
        # A dict which stores all node associated attributes, e.g. version
        self.attributes = dict()


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
            if childNode.displayName == displayName:
                return childNode
        return None


    def hasNode(self, displayName):
        for childNode in self.childNodes:
            if childNode.displayName == displayName:
                return True
        return False


    def row(self):
        if not self.parentNode:
            return None
        
        return self.parentNode.indexOfChildNode(self)


    def printTree(self, indent=-2):
        indent = indent + 2;
        for childNode in self.childNodes:
            print(" " * indent, childNode.displayName)
            childNode.printTree(indent)
        
