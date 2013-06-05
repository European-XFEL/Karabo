#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 21, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a text component for the middle
   panel.
"""

__all__ = ["Text"]


from layoutcomponents.nodebase import NodeBase
from layoutcomponents.textdialog import TextDialog

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class Text(NodeBase, QGraphicsItem):

    def __init__(self, isEditable):
        super(Text, self).__init__(isEditable)
        
        self.__font = QFont()
        self.__text = str()
        self.__textColor = QColor(Qt.black)
        self.__backgroundColor = QColor(Qt.lightGray)
        self.__outlineColor = QColor(Qt.darkGray)
        
        self.setFlags(QGraphicsItem.ItemIsMovable | QGraphicsItem.ItemIsSelectable | QGraphicsItem.ItemSendsGeometryChanges)


    def __del__(self):
        NodeBase.__del__(self)


    def setFont(self, font):
        self.prepareGeometryChange()
        self.__font = font
        self.update()


    def font(self):
        return self.__font


    def setText(self, text):
        self.prepareGeometryChange()
        self.__text = text
        self.update()


    def text(self):
        return self.__text


    def setTextColor(self, color):
        self.__textColor = color
        self.update()


    def textColor(self):
        return self.__textColor


    def setOutlineColor(self, color):
        self.__outlineColor = color
        self.update()


    def outlineColor(self):
        return self.__outlineColor


    def setBackgroundColor(self, color):
        self.__backgroundColor = color
        self.update()


    def backgroundColor(self):
        return self.__backgroundColor


    def boundingRect(self):
        margin = 1
        return self.outlineRect().adjusted(-margin, -margin, +margin, +margin)


    def shape(self):
        rect = self.outlineRect()
        path = QPainterPath()
        path.addRoundRect(rect, self.roundness(rect.width()), self.roundness(rect.height()))
        return path


    def paint(self, painter, option, widget):
        pen = QPen(self.__outlineColor)
        if self.isSelected():
            pen.setStyle(Qt.DotLine)
            pen.setWidth(2)
        
        painter.setFont(self.__font)
        painter.setPen(pen)
        painter.setBrush(self.__backgroundColor)
        rect = self.outlineRect()
        painter.drawRoundRect(rect, self.roundness(rect.width()), self.roundness(rect.height()))
        painter.setPen(self.__textColor)
        painter.drawText(rect, Qt.AlignCenter, self.__text)


### protected ###
    def mouseMoveEvent(self, event):
        if self.isDesignMode == False:
            return
        QGraphicsItem.mouseMoveEvent(self, event)


    def mousePressEvent(self, event):
        if self.isDesignMode == False:
            return
        QGraphicsItem.mousePressEvent(self, event)


    def mouseReleaseEvent(self, event):
        if self.isDesignMode == False:
            return
        QGraphicsItem.mouseReleaseEvent(self, event)


    def mouseDoubleClickEvent(self, event):
        #text = QInputDialog.getText(event.widget(), "Edit Text", "Enter new text:", QLineEdit.Normal, self.__text)
        #if len(text[0]) > 0:
        #    self.setText(text[0])
        
        textDialog = TextDialog(None, self)
        if textDialog.exec_() == QDialog.Rejected:
            return
        
        self.setText(textDialog.text())
        self.setFont(textDialog.font())
        self.setTextColor(textDialog.textColor())
        self.setBackgroundColor(textDialog.backgroundColor())
        self.setOutlineColor(textDialog.outlineColor())

        QGraphicsItem.mouseDoubleClickEvent(self, event)


    def itemChange(self, change, value):
        if change == QGraphicsItem.ItemPositionHasChanged: # ItemPositionChange
            self.trackItems()
        return QGraphicsItem.itemChange(self, change, value)


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

