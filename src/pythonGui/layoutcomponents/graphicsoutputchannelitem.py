#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 1, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a class which is an output channel
   connected to a GraphicsCustomItem"""

__all__ = ["GraphicsOutputChannelItem"]

from layoutcomponents.graphicsinputchannelitem import GraphicsInputChannelItem
from layoutcomponents.line import Line

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class GraphicsOutputChannelItem(QGraphicsObject):
    # signals
    signalValueChanged = pyqtSignal(str, object) # key, value


    def __init__(self, parentItem, connectionType, isEditable=False):
        super(GraphicsOutputChannelItem, self).__init__(parentItem)
        
        self.__internalKey = parentItem.internalKey() + ".output"
        
        self.__connectionType = connectionType
        self.__connection = None


    def mousePressEvent(self, event):
        #print "GraphicsOutputChannelItem.mousePressEvent"
        pos = self.mapToScene(event.pos())
        self.__connection = Line(False) # TODO
        self.__connection.setWidthF(1.0)
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
        if inputItem and isinstance(inputItem, GraphicsInputChannelItem):
            inputItem.setConnectedOutputChannel(self.parentItem().additionalText() + "@output")

            if self.__connection:
                centerPos = self.__connection.boundingRect().center()
                self.__connection.setTransformOriginPoint(centerPos)
                self.__connection.setSelected(True)
        else:
            self.scene().removeItem(self.__connection)
        self.__connection = None
        
        #QGraphicsItem.mouseReleaseEvent(self, event)


    def boundingRect(self):
        margin = 5
        return QRectF(0, 0, 40+margin, 5+margin)


    def paint(self, painter, option, widget=None):
        painter.setBrush(QBrush(Qt.white))
        painter.drawLine(QPoint(0, 0), QPoint(40, 0))
        if self.__connectionType == "NetworkOutput-Hash":
            painter.drawEllipse(QPoint(40, 0), 5, 5)

