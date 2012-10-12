#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 26, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a base class for all node items
   of the scene.
   
   Inherited by: Text, Rectangle
"""

__all__ = ["NodeBase"]


#from PyQt4.QtCore import *
#from PyQt4.QtGui import *


class NodeBase(object):

    def __init__(self):
        super(NodeBase, self).__init__()
        
        self.__links = set()


    def __del__(self):
        print "NodeBase.__del__", self.__links
        for link in self.__links:
            print "del link"
            del link


    def links(self):
        return self.__links


    def addLink(self, link):
        print "addLink"
        self.__links.add(link)


    def removeLink(self, link):
        print "removeLink"
        self.__links.remove(link)


    def trackItems(self):
        for link in self.__links:
            link.trackItems()

