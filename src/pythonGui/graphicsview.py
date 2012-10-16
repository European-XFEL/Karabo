#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 21, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a QGraphicsView."""

__all__ = ["GraphicsView"]

from layoutcomponents.graphicsproxywidget import GraphicsProxyWidget
from layoutcomponents.line import Line
from layoutcomponents.nodebase import NodeBase
from layoutcomponents.rectangle import Rectangle
from userattributecustomframe import UserAttributeCustomFrame
from userdevicecustomframe import UserDeviceCustomFrame

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class GraphicsView(QGraphicsView):
    # Enums
    MoveItem, InsertText, InsertLine, InsertRect = range(4)
    # Signals
    lineInserted = pyqtSignal()
    rectInserted = pyqtSignal()

    def __init__(self):
        super(GraphicsView, self).__init__()

        # Current mode of the view (move, insert
        self.__mode = self.MoveItem
        
        self.__line = None
        self.__rect = None
        
        self.__isEditableMode = False
        
        self.setAcceptDrops(True)
        self.setDragMode(QGraphicsView.RubberBandDrag)
        self.setRenderHints(QPainter.Antialiasing | QPainter.TextAntialiasing)


    # Sets all items editable or not
    def setEditableMode(self, isEditableMode):
        self.__isEditableMode = isEditableMode
        for item in self.items():
            if isinstance(item, NodeBase):
                item.isEditable = isEditableMode


    def _getMode(self):
        return self.__mode
    def _setMode(self, mode):
        self.__mode = mode
    mode = property(fget=_getMode, fset=_setMode)


    def addItem(self, item):
        self.scene().addItem(item)
        self.scene().clearSelection()
        item.setSelected(True)


### protected ###
    def wheelEvent(self, event):
        #factor = 1.41 ** (-event.delta() / 240.0)
        factor = 1.0 + (0.2 * qAbs(event.delta()) / 120.0)
        if event.delta() > 0:
            self.scale(factor, factor)
        else:
            factor = 1.0/factor
            self.scale(factor, factor)


    def mousePressEvent(self, event):
        if (event.button() != Qt.LeftButton):
            return

        # Items are created in origin and must then be moved to the position to
        # set their position correctly for later purposes!!!
        pos = QPointF(self.mapToScene(event.pos()))
        if self.__mode == self.InsertLine:
            self.__line = Line()
            self.addItem(self.__line)
            self.__line.setPos(pos.x(), pos.y())
        elif self.__mode == self.InsertRect:
            self.__rect = Rectangle()
            self.addItem(self.__rect)
            self.__rect.setPos(pos.x(), pos.y())

        QGraphicsView.mousePressEvent(self, event)


    def mouseMoveEvent(self, event):
        pos = self.mapToScene(event.pos())
        if self.__mode == self.InsertLine and self.__line:
            linePos = self.__line.pos()
            pos = QPointF(pos.x()-linePos.x(), pos.y()-linePos.y())
            newLine = QLineF(QPointF(), QPointF(pos))
            self.__line.setLine(newLine)
        elif self.__mode == self.InsertRect and self.__rect:
            rectPos = self.__rect.pos()
            pos = QPointF(pos.x()-rectPos.x(), pos.y()-rectPos.y())
            newRect = QRectF(QPointF(), QPointF(pos))
            self.__rect.setRect(newRect)
        elif self.__mode == self.MoveItem:
            QGraphicsView.mouseMoveEvent(self, event)


    def mouseReleaseEvent(self, event):
        if self.__line and self.__mode == self.InsertLine:
            centerPos = self.__line.boundingRect().center()
            self.__line.setTransformOriginPoint(centerPos)
            self.__line.setSelected(True)
            self.lineInserted.emit()
        elif self.__rect and self.__mode == self.InsertRect:
            rect = self.__rect.boundingRect()
            centerPos = rect.center()
            self.__rect.setTransformOriginPoint(centerPos)
            self.__rect.setSelected(True)
            self.rectInserted.emit()

        self.__line = None
        self.__rect = None
        QGraphicsView.mouseReleaseEvent(self, event)


# Drag & Drop events
    def dragEnterEvent(self, event):
        #print "GraphicsView.dragEnterEvent"
        
        source = event.source()
        if (source is not None) and (source is not self):
            event.setDropAction(Qt.MoveAction)
            event.accept()
        
        QWidget.dragEnterEvent(self, event)


    def dragMoveEvent(self, event):
        #print "GraphicsView.dragMoveEvent"

    #    source = event.source()
    #    if source is not None and not event.mimeData().hasHtml():
    #        pos = self._getWidgetCenterPosition(self.mapFromGlobal(QCursor.pos()), source.width()/2, source.height()/2)
    #        source.move(pos)

        event.setDropAction(Qt.MoveAction)
        event.accept()
        
        QWidget.dragMoveEvent(self, event)


    def dropEvent(self, event):
        #print "GraphicsView.dropEvent"
        
        source = event.source()
        if source is not None:
            if event.mimeData().hasHtml():
                type = event.mimeData().html()
                data = event.mimeData().data("data")
                data = data.split(',')
                
                if len(data) < 2:
                    return
                
                # Drop from NavigationTreeView or AttributeTreeWidget?
                if type == "NavigationTreeView":
                    userCustomFrame = UserDeviceCustomFrame(key=data[0], displayName=data[1], parent=self)
                elif type == "AttributeTreeWidget":
                    key = data[0]
                    item = source.getAttributeTreeWidgetItemByKey(key)
                    navigationItemType = source.getNavigationItemType()
                    userCustomFrame = UserAttributeCustomFrame(item.classAlias, item=item, key=key, parent=self, navigationItemType=navigationItemType)
                    userCustomFrame.signalRemoveUserAttributeCustomFrame.connect(self.onRemoveUserCustomFrame)
                
                proxyWidget = GraphicsProxyWidget(userCustomFrame)
                proxyWidget.isEditable = self.__isEditableMode
                self.addItem(proxyWidget)

                bRect = proxyWidget.boundingRect()
                leftPos = bRect.topLeft()
                leftPos = proxyWidget.mapToScene(leftPos)
                centerPos = bRect.center()
                centerPos = proxyWidget.mapToScene(centerPos)

                offset = centerPos-leftPos

                pos = event.pos()
                scenePos = self.mapToScene(pos)
                scenePos = scenePos-offset
                
                proxyWidget.setPos(scenePos)
            #else:
            #    pos = self._getWidgetCenterPosition(self.mapFromGlobal(QCursor.pos()), source.width()/2, source.height()/2)
            #    source.move(pos)

        event.setDropAction(Qt.MoveAction)
        event.accept()


    def _getWidgetCenterPosition(self, pos, centerX, centerY):
        # QPointF pos, int centerX, int centerY
        pos.setX(pos.x()-centerX)
        pos.setY(pos.y()-centerY)
        return pos


### slots ###
    def onRemoveUserCustomFrame(self, userCustomFrame):
        print "onRemoveUserCustomFrame", userCustomFrame
        if userCustomFrame is None:
            return
        userCustomFrame.deleteLater()

