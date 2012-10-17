#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 27, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a line component for the middle
   panel.
"""

__all__ = ["Line"]

from layoutcomponents.linedialog import LineDialog
from layoutcomponents.nodebase import NodeBase

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class Line(NodeBase, QGraphicsLineItem):

    def __init__(self, isEditable):
        super(Line, self).__init__(isEditable)
        
        # Always start from origin
        line = QLineF(QPointF(), QPointF())
        self.setLine(line)
        
        self.setWidthF(2.0)
        
        self.setFlags(QGraphicsItem.ItemIsMovable | QGraphicsItem.ItemIsSelectable)


    def setLength(self, length):
        self.prepareGeometryChange()
        line = self.line()
        line.setLength(length)
        self.setLine(line)
        self.update()


    def length(self):
        return self.line().length()


    def setWidthF(self, width):
        pen = self.pen()
        pen.setWidthF(width)
        self.setPen(pen)


    def widthF(self):
        return self.pen().widthF()


    def setStyle(self, style):
        pen = self.pen()
        pen.setStyle(style)
        self.setPen(pen)


    def style(self):
        return self.pen().style()


    def setColor(self, color):
        pen = self.pen()
        pen.setColor(color)
        self.setPen(pen)


    def color(self):
        return self.pen().color()


### protected ###
    def mouseMoveEvent(self, event):
        if self.isEditable == True:
            return
        QGraphicsLineItem.mouseMoveEvent(self, event)


    def mousePressEvent(self, event):
        if self.isEditable == True:
            return
        QGraphicsLineItem.mousePressEvent(self, event)


    def mouseReleaseEvent(self, event):
        if self.isEditable == True:
            return
        QGraphicsLineItem.mouseReleaseEvent(self, event)


    def mouseDoubleClickEvent(self, event):
        lineDialog = LineDialog(None, self)
        if lineDialog.exec_() == QDialog.Rejected:
            return
        
        self.setLength(lineDialog.lineLength())
        self.setWidthF(lineDialog.lineWidthF())
        self.setStyle(lineDialog.penStyle())
        self.setColor(lineDialog.lineColor())

        QGraphicsLineItem.mouseDoubleClickEvent(self, event)
