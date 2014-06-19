#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 1, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a class which is an input channel
   connected to a GraphicsCustomItem"""

__all__ = ["GraphicsInputChannelItem"]

import layoutcomponents.graphicsoutputchannelitem
from channelconnection import ChannelConnection

from PyQt4.QtCore import *
from PyQt4.QtGui import *

#from manager import Manager # TODO: avoid ring import


class GraphicsInputChannelItem(QGraphicsObject):
    # signals
    signalValueChanged = pyqtSignal(str, object) # key, value


    def __init__(self, parentItem, keyName, connectionType, isEditable=False):
        super(GraphicsInputChannelItem, self).__init__(parentItem)

        self.__keyName = keyName

        self.__internalKey = parentItem.internalKey() + "." + self.__keyName
        self.__connectionType = connectionType
        self.__connection = None
        
        # List of strings with connected output channel entries
        self.__connectedOutputChannels = []
        
        # List of connection line items
        self.__channelConnectionItems = []
        
        # Connect customItem signal to Manager, DEVICE_CLASS
        self.signalValueChanged.connect(Manager().onDeviceClassValueChanged)
        # Register for value changes of connectedOutputChannels
        Manager().registerEditableComponent(self.connectedOutputChannelsKey, self)
        # Register for value changes of dataDistribution
        Manager().registerEditableComponent(self.dataDistributionKey, self)


### public ###
    def _getKeyName(self):
        return self.__keyName
    keyName = property(fget=_getKeyName)


    def _getValue(self):
        return self.__connectedOutputChannels
    value = property(fget=_getValue)


    def _inputPos(self):
        return QPointF(0.0, 0.0)
    inputPos = property(fget=_inputPos)


    def addConnectedOutputChannel(self, connectedOutputChannel):
        if connectedOutputChannel in self.__connectedOutputChannels:
            return
        
        self.__connectedOutputChannels.append(str(connectedOutputChannel))
        self.signalValueChanged.emit(self.connectedOutputChannelsKey, self.__connectedOutputChannels)


    def removeConnecteOutputChannel(self, connectedOutputChannel):
        if not connectedOutputChannel in self.__connectedOutputChannels:
            return
        self.__connectedOutputChannels.remove(connectedOutputChannel)


    def addChannelConnectionItem(self, item):
        self.__channelConnectionItems.append(item)


    def removeChannelConnectionItem(self, item):
        self.__channelConnectionItems.remove(item)


    def trackChannelConnectionItems(self):
        for channelConnectionItem in self.__channelConnectionItems:
            channelConnectionItem.trackItems()


### private ###
    def _getConnectedOutputChannelsKey(self):
        return self.__internalKey + "." + self.__connectionType + ".connectedOutputChannels"
    connectedOutputChannelsKey = property(fget=_getConnectedOutputChannelsKey)


    def _getDataDistributionKey(self):
        return self.__internalKey + "." + self.__connectionType + ".dataDistribution"
    dataDistributionKey = property(fget=_getDataDistributionKey)


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
        
        outputItem = self.scene().itemAt(self.mapToScene(event.pos()), QTransform())
        if outputItem and (type(outputItem) == layoutcomponents.graphicsoutputchannelitem.GraphicsOutputChannelItem):
            self.addConnectedOutputChannel(outputItem.outputChannelConnection)

        # Remove connection line again (gets drawn in GraphicsInputChannelItem...
        self.scene().removeItem(self.__connection)
        self.__connection = None
        
        #QGraphicsItem.mouseReleaseEvent(self, event)


    def boundingRect(self):
        margin = 1
        return QRectF(0, 0, 40+margin, 5+margin)


    def paint(self, painter, option, widget=None):
        painter.setBrush(QBrush(Qt.white))
        painter.drawLine(self.inputPos, QPointF(40, 0))
        if self.__connectionType == "Network-Hash":
            painter.drawEllipse(self.inputPos, 5, 5)
        elif self.__connectionType == "BinaryFile":
            painter.drawEllipse(self.inputPos, 5, 5)
        elif self.__connectionType == "Hdf5File":
            painter.drawEllipse(self.inputPos, 5, 5)
        elif self.__connectionType == "TextFile":
            painter.drawEllipse(self.inputPos, 5, 5)


### slots ###
    # Triggered by DataNotifier signalUpdateComponent
    def onValueChanged(self, box, value):
        if self.connectedOutputChannelsKey == key:
            if self.scene():
                items = self.scene().items()
                for channelItem in items:
                    if type(channelItem) == layoutcomponents.graphicsoutputchannelitem.GraphicsOutputChannelItem:
                        for v in value:
                            vSplit = v.split('@', 1)
                            if (channelItem.predefinedDevInstId == vSplit[0]) and (channelItem.keyName == vSplit[1]):
                                channelConnectionItem = ChannelConnection(channelItem, self)
                                self.scene().addItem(channelConnectionItem)
                                self.addChannelConnectionItem(channelConnectionItem)
                            # Add connected output channel to list
                            self.addConnectedOutputChannel(v)
        elif self.dataDistributionKey == key:
            for channelConnectionItem in self.__channelConnectionItems:
                if value == "copy":
                    channelConnectionItem.setStyle(Qt.SolidLine)
                elif value == "shared":
                    channelConnectionItem.setStyle(Qt.DashLine)


    def onConnectedOutputChannelChanged(self, oldConnectedOutputChannel, newConnectedOutputChannel):
        self.removeConnecteOutputChannel(oldConnectedOutputChannel)
        # Add new connected output channel to list
        self.addConnectedOutputChannel(newConnectedOutputChannel)

