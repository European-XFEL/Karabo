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

from manager import Manager


class GraphicsInputChannelItem(QGraphicsObject):
    # signals
    signalValueChanged = pyqtSignal(str, object) # key, value


    def __init__(self, parentItem, connectionType, isEditable=False):
        super(GraphicsInputChannelItem, self).__init__(parentItem)

        self.__internalKey = parentItem.internalKey() + ".input"
        self.__connectionType = connectionType
        
        # List of strings with connected output channel entries
        self.__connectedOutputChannels = []
        
        # List of connection line items
        self.__channelConnections = set()
        
        # Connect customItem signal to Manager, DEVICE_CLASS
        self.signalValueChanged.connect(Manager().onDeviceClassValueChanged)
        # Register for value changes of connectedOutputChannels
        Manager().registerEditableComponent(self.connectedOutputChannelsKey, self)


    def _getValue(self):
        return self.__connectedOutputChannels
    value = property(fget=_getValue)


    def _inputPos(self):
        return QPointF(0.0, 0.0)
    inputPos = property(fget=_inputPos)


### public ###
    def addConnectedOutputChannel(self, connectedOutputChannel):
        if connectedOutputChannel in self.__connectedOutputChannels:
            return
        
        self.__connectedOutputChannels.append(str(connectedOutputChannel))
        self.signalValueChanged.emit(self.connectedOutputChannelsKey, self.__connectedOutputChannels)


    def addChannelConnection(self, channelConnection):
        self.__channelConnections.add(channelConnection)


    def removeChannelConnection(self, channelConnection):
        self.__channelConnections.remove(channelConnection)


    def trackChannelConnection(self):
        for channelConnection in self.__channelConnections:
            channelConnection.trackItems()


### private ###
    def _getConnectedOutputChannelsKey(self):
        return self.__internalKey + "." + self.__connectionType + ".connectedOutputChannels"
    connectedOutputChannelsKey = property(fget=_getConnectedOutputChannelsKey)


    def _getDataDistributionKey(self):
        return self.__internalKey + "." + self.__connectionType + ".dataDistribution"
    dataDistributionKey = property(fget=_getDataDistributionKey)


### protected ###
    def boundingRect(self):
        margin = 1
        return QRectF(0, 0, 40+margin, 5+margin)


    def paint(self, painter, option, widget=None):
        painter.setBrush(QBrush(Qt.white))
        painter.drawLine(self.inputPos, QPointF(40, 0))
        if self.__connectionType == "NetworkInput-Hash":
            painter.drawEllipse(self.inputPos, 5, 5)


### slots ###
    # Triggered by DataNotifier signalUpdateComponent
    def onValueChanged(self, key, value):
        if self.connectedOutputChannelsKey == key:
            if self.scene():
                items = self.scene().items()
                for item in items:
                    if type(item) == layoutcomponents.graphicsoutputchannelitem.GraphicsOutputChannelItem:
                        for v in value:
                            vSplit = v.split('@', 1)
                            if item.predefinedDevInstId == vSplit[0]:
                                #connection = QGraphicsLineItem(QLineF(item.mapToScene(item.outputPos), self.mapToScene(self.inputPos)))
                                connection = ChannelConnection(item, self)
                                self.scene().addItem(connection)
                                self.addChannelConnection(connection)
                            # Add connected output channel to list
                            self.addConnectedOutputChannel(v)
            
        elif self.dataDistributionKey == key:
            print "change dataDistribution", value

