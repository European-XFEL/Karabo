#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 25, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a rectangle component for the middle
   panel.
"""

__all__ = ["Rectangle"]

from layoutcomponents.nodebase import NodeBase
from layoutcomponents.rectangledialog import RectangleDialog

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class Rectangle(NodeBase, QGraphicsRectItem):

    def __init__(self, isEditable):
        super(Rectangle, self).__init__(isEditable)
        
        self.setBrush(QBrush(Qt.white, Qt.SolidPattern))
        
        # Always start from origin
        self.setRect(0, 0, 0, 0)
        
        self.setFlags(QGraphicsItem.ItemIsMovable | QGraphicsItem.ItemIsSelectable)


    def __del__(self):
        NodeBase.__del__(self)


    def setOutlineColor(self, color):
        pen = self.pen()
        pen.setColor(color)
        self.setPen(pen)


    def outlineColor(self):
        return self.pen().color()


    def setBackgroundColor(self, color):
        brush = self.brush()
        brush.setColor(color)
        self.setBrush(brush)


    def backgroundColor(self):
        return self.brush().color()


### protected ###
    def mouseMoveEvent(self, event):
        if self.isDesignMode == False:
            return
        QGraphicsRectItem.mouseMoveEvent(self, event)


    def mousePressEvent(self, event):
        if self.isDesignMode == False:
            return
        QGraphicsRectItem.mousePressEvent(self, event)


    def mouseReleaseEvent(self, event):
        if self.isDesignMode == False:
            return
        QGraphicsRectItem.mouseReleaseEvent(self, event)


    def mouseDoubleClickEvent(self, event):
        rectDialog = RectangleDialog(None, self)
        if rectDialog.exec_() == QDialog.Rejected:
            return
        
        self.setBackgroundColor(rectDialog.backgroundColor())
        self.setOutlineColor(rectDialog.outlineColor())
 
        QGraphicsRectItem.mouseDoubleClickEvent(self, event)

