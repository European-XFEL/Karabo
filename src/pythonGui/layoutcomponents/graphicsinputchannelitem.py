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


class GraphicsInputChannelItem(QGraphicsItem):


    def __init__(self, parent, connectionType, isEditable=False):
        super(GraphicsInputChannelItem, self).__init__(parent)

        self.__connectionType = connectionType


    def dragEnterEvent(self, event):
        print "GraphicsInputChannelItem.dragEnterEvent"


    def dragLeaveEvent(self, event):
        print "GraphicsInputChannelItem.dragLeaveEvent"
 
 
    def dropEvent(self, event):
        print "GraphicsInputChannelItem.dropEvent"


    def boundingRect(self):
        margin = 1
        return QRectF(0, 0, 40+margin, 5+margin)


    def paint(self, painter, option, widget=None):
        painter.setBrush(QBrush(Qt.white))
        painter.drawLine(QPoint(0, 0), QPoint(40, 0))
        if self.__connectionType == "NetworkInput-Hash":
            painter.drawEllipse(QPoint(0, 0), 5, 5)