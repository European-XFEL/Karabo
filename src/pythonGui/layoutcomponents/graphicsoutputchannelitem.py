#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 1, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a class which is an output channel
   connected to a GraphicsCustomItem"""

__all__ = ["GraphicsOutputChannelItem"]

import layoutcomponents.graphicsinputchannelitem
from channelconnection import ChannelConnection

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class GraphicsOutputChannelItem(QGraphicsObject):
    # signals
    #signalValueChanged = pyqtSignal(str, object) # key, value


    def __init__(self, parentItem, connectionType, isEditable=False):
        super(GraphicsOutputChannelItem, self).__init__(parentItem)
        
        self.__internalKey = parentItem.internalKey() + ".output"
        
        self.__connectionType = connectionType
        self.__connection = None
        
        # List of connection line items
        self.__channelConnections = set()


### public ###
    def _getOutputChannelConnection(self):
        return self.parentItem().value + "@output"
    outputChannelConnection = property(fget=_getOutputChannelConnection)


    def _getPredefinedDevInstId(self):
        return str(self.parentItem().value)
    predefinedDevInstId = property(fget=_getPredefinedDevInstId)


    def _outputPos(self):
        return QPointF(40.0, 0.0)
    outputPos = property(fget=_outputPos)


    def addChannelConnection(self, channelConnection):
        self.__channelConnections.add(channelConnection)


    def removeChannelConnection(self, channelConnection):
        self.__channelConnections.remove(channelConnection)


    def trackChannelConnection(self):
        for channelConnection in self.__channelConnections:
            channelConnection.trackItems()


### protected ###
    def mousePressEvent(self, event):
        #print "GraphicsOutputChannelItem.mousePressEvent"
        
        pos = self.mapToScene(event.pos())
        self.__connection = QGraphicsLineItem()
        self.scene().addItem(self.__connection)
        self.__connection.setPos(pos.x(), pos.y())
        
        #QGraphicsItem.mousePressEvent(self, event)


    def mouseMoveEvent(self, event):
        #print "GraphicsOutputChannelItem.mouseMoveEvent"
        
        pos = self.mapToScene(event.pos())
        if self.__connection:
            linePos = self.__connection.pos()
            pos = QPointF(pos.x()-linePos.x(), pos.y()-linePos.y())
            newLine = QLineF(QPointF(), QPointF(pos))
            self.__connection.setLine(newLine)
        
        #QGraphicsItem.mouseMoveEvent(self, event)


    def mouseReleaseEvent(self, event):
        #print "GraphicsOutputChannelItem.mouseReleaseEvent"
        
        inputItem = self.scene().itemAt(self.mapToScene(event.pos()), QTransform())
        if inputItem and (type(inputItem) == layoutcomponents.graphicsinputchannelitem.GraphicsInputChannelItem):
            inputItem.addConnectedOutputChannel(self.outputChannelConnection)

        # Remove connection line again (gets drawn in GraphicsInputChannelItem...
        self.scene().removeItem(self.__connection)
        self.__connection = None
        
        #QGraphicsItem.mouseReleaseEvent(self, event)


    def boundingRect(self):
        margin = 1
        return QRectF(0, 0, 40+margin, 5+margin)


    def paint(self, painter, option, widget=None):
        painter.setBrush(QBrush(Qt.white))
        painter.drawLine(QPointF(0, 0), self.outputPos)
        if self.__connectionType == "NetworkOutput-Hash":
            painter.drawEllipse(self.outputPos, 5, 5)

