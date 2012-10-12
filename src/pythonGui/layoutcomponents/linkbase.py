#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 27, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a base class for all link items
   of the scene.
   
   Inherited by: Link, Arrow
"""

__all__ = ["LinkBase"]

import functools

from layoutcomponents.linedialog import LineDialog

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class LinkBase(QGraphicsLineItem):

    def __init__(self, fromItem, toItem):
        super(LinkBase, self).__init__()
        
        print "LinkBase.__init__"
        
        self.__fromItem = fromItem
        self.__toItem = toItem
        
        self.__fromItem.addLink(self)
        self.__toItem.addLink(self)
        
        self.setFlags(QGraphicsItem.ItemIsSelectable)
        self.setZValue(-1)
        self.setColor(Qt.black)
        self.setWidthF(2.0)
        self.trackItems()


    def __del__(self):
        print "LinkBase.__del__"
        self.__fromItem.removeLink(self)
        self.__toItem.removeLink(self)


    def _fromItem(self):
        return self.__fromItem
    fromItem = property(fget=_fromItem)


    def _toItem(self):
        return self.__toItem
    toItem = property(fget=_toItem)


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


    def setWidthF(self, width):
        pen = self.pen()
        pen.setWidthF(width)
        self.setPen(pen)


    def widthF(self):
        return self.pen().widthF()


    def trackItems(self):
        pass


### protected ###
    def mouseDoubleClickEvent(self, event):
        lineDialog = LineDialog(None, self)
        if lineDialog.exec_() == QDialog.Rejected:
            return
        
        self.setWidthF(lineDialog.lineWidthF())
        self.setColor(lineDialog.lineColor())
        self.setStyle(lineDialog.penStyle())


    def contextMenuEvent(self, event):
        wrapped = []
        menu = QMenu(self.parentWidget())
        for text, param in (
                ("&Solid", Qt.SolidLine),
                ("&Dashed", Qt.DashLine),
                ("D&otted", Qt.DotLine),
                ("D&ashDotted", Qt.DashDotLine),
                ("DashDo&tDotted", Qt.DashDotDotLine)):
            wrapper = functools.partial(self.setStyle, param)
            wrapped.append(wrapper)
            menu.addAction(text, wrapper)
        menu.exec_(event.screenPos())

