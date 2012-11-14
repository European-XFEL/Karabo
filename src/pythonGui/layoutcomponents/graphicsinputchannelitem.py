#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 1, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a class which is an input channel
   connected to a GraphicsCustomItem"""

__all__ = ["GraphicsInputChannelItem"]


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
        
        # Connect customItem signal to Manager, DEVICE_CLASS
        self.signalValueChanged.connect(Manager().onDeviceClassValueChanged)
        # Register for value changes of connectedOutputChannels
        Manager().registerEditableComponent(self.connectedOutputChannelsKey, self)


### public ###
    def setConnectedOutputChannel(self, connectedOutputChannel):
        self.signalValueChanged.emit(self.connectedOutputChannelsKey, [str(connectedOutputChannel)])


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
        painter.drawLine(QPoint(0, 0), QPoint(40, 0))
        if self.__connectionType == "NetworkInput-Hash":
            painter.drawEllipse(QPoint(0, 0), 5, 5)


### slots ###
    # Triggered by DataNotifier signalUpdateComponent
    def onValueChanged(self, key, value):
        # TODO: Draw line between input and output channel
        if self.connectedOutputChannelsKey == key:
            print "change outputChannels", value
        elif self.dataDistributionKey == key:
            print "change dataDistribution", value

