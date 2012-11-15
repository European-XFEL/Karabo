#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 15, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a component for the middle
   panel which connects input and output channel items.
"""

__all__ = ["ChannelConnection"]

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class ChannelConnection(QGraphicsLineItem):

    def __init__(self, outputChannel, inputChannel):
        super(ChannelConnection, self).__init__()
        
        self.__outputChannel = outputChannel
        self.__inputChannel= inputChannel
        
        self.__outputChannel.addChannelConnection(self)
        self.__inputChannel.addChannelConnection(self)
        
        self.setZValue(-1)
        self.trackItems()
        
        self.setFlags(QGraphicsItem.ItemIsSelectable)


    def __del__(self):
        print "ChannelConnection.__del__"

        self.__outputChannel.removeChannelConnection(self)
        self.__inputChannel.removeChannelConnection(self)


    def trackItems(self):
        self.setLine(QLineF(self.__outputChannel.mapToScene(self.__outputChannel.outputPos), self.__inputChannel.mapToScene(self.__inputChannel.inputPos)))

