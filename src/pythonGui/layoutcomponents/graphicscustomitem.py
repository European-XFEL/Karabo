#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on October 19, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a class which is a customwidget
   component for the middle panel."""

__all__ = ["GraphicsCustomItem"]


from PyQt4.QtCore import *
from PyQt4.QtGui import *


class GraphicsCustomItem(QGraphicsItem):


    def __init__(self, isEditable, text):
        super(GraphicsCustomItem, self).__init__()

        self.__font = QFont()
        self.__text = text

        self.setFlags(QGraphicsItem.ItemIsMovable | QGraphicsItem.ItemIsSelectable | QGraphicsItem.ItemIsFocusable)


    def boundingRect(self):
        margin = 1
        return self.outlineRect().adjusted(-margin, -margin, +margin, +margin)


    def shape(self):
        rect = self.outlineRect()
        path = QPainterPath()
        path.addRoundRect(rect, self.roundness(rect.width()), self.roundness(rect.height()))
        return path


    def paint(self, painter, option, widget):
        #pen = QPen(self.__outlineColor)
        pen = painter.pen()
        if self.isSelected():
            pen.setStyle(Qt.DotLine)
            pen.setWidth(2)
        
        painter.setFont(self.__font)
        painter.setPen(pen)
        #painter.setBrush(self.__backgroundColor)
        rect = self.outlineRect()
        painter.drawRoundRect(rect, self.roundness(rect.width()), self.roundness(rect.height()))
        #painter.setPen(self.__textColor)
        painter.drawText(rect, Qt.AlignCenter, self.__text)


### private ###
    def outlineRect(self):
        padding = 8
        metrics = QFontMetricsF(self.__font) #qApp.fontMetrics())
        rect = metrics.boundingRect(self.__text)
        rect.adjust(-padding, -padding, +padding, +padding)
        rect.translate(-rect.center())
        return rect


    def roundness(self, size):
        diameter = 12
        return 100 * diameter / int(size)

