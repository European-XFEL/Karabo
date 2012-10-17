#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 28, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a class which a customwidget component
   for the middle panel."""

__all__ = ["GraphicsProxyWidget"]


from layoutcomponents.nodebase import NodeBase

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class GraphicsProxyWidget(NodeBase, QGraphicsProxyWidget):


    def __init__(self, isEditable, widget=None):
        super(GraphicsProxyWidget, self).__init__(isEditable)

        self._setWidget(widget)
        self.setFlags(QGraphicsItem.ItemIsMovable | QGraphicsItem.ItemIsSelectable | QGraphicsItem.ItemIsFocusable)


    def __del__(self):
        NodeBase.__del__(self)


    def _setWidget(self, widget):
        self.setWidget(widget)
    def _getWidget(self):
        return self.widget()
    embeddedWidget = property(fget=_getWidget, fset=_setWidget)


### protected ###
    def paint(self, painter, option, widget):
        if self.isSelected():
            pen = painter.pen()
            pen.setStyle(Qt.DashLine)
            painter.setPen(pen)
            #rect = self.subWidgetRect(self.embeddedWidget)
            rect = self.boundingRect()
            painter.drawRect(rect)
        QGraphicsProxyWidget.paint(self, painter, option, widget)


    def mouseMoveEvent(self, event):
        if self.isEditable == True:
            QGraphicsProxyWidget.mouseMoveEvent(self, event)
        else:
            QGraphicsItem.mouseMoveEvent(self, event)


    def mousePressEvent(self, event):
        if self.isEditable == True:
            self.setFlag(QGraphicsItem.ItemIsFocusable, True)
            QGraphicsProxyWidget.mousePressEvent(self, event)
        else:
            self.setFlag(QGraphicsItem.ItemIsFocusable, False)
            QGraphicsItem.mousePressEvent(self, event)


    def mouseReleaseEvent(self, event):
        if self.isEditable == True:
            QGraphicsProxyWidget.mouseReleaseEvent(self, event)
        else:
            QGraphicsItem.mouseReleaseEvent(self, event)


    def contextMenuEvent(self, event):
        pos = event.pos()
        self.embeddedWidget.showContextMenu(QPoint(pos.x(), pos.y()))

