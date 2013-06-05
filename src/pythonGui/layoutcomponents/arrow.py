#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 25, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents an arrow component for the middle
   panel.
"""

__all__ = ["Arrow"]

import math

from layoutcomponents.linkbase import LinkBase

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class Arrow(LinkBase):

    def __init__(self, fromItem=None, toItem=None):
        super(Arrow, self).__init__(fromItem, toItem)
        
        self.__arrowHead = QPolygonF()


    def __del__(self):
        print "Arrow.__del__"
        Arrow.__del__(self)


    def boundingRect(self):
        extra = (self.pen().width() + 20) / 2.0
        p1 = self.line().p1()
        p2 = self.line().p2()
        return QRectF(p1, QSizeF(p2.x() - p1.x(), p2.y() - p1.y())).normalized().adjusted(-extra, -extra, extra, extra)


    def shape(self):
        path = super(Arrow, self).shape()
        path.addPolygon(self.__arrowHead)
        return path


    def paint(self, painter, option, widget=None):
        if (self.fromItem.collidesWithItem(self.toItem)):
            return

        startItem = self.fromItem
        endItem = self.toItem
        arrowSize = 10.0
        painter.setPen(self.pen())
        painter.setBrush(self.color())
        
        centerLine = QLineF(startItem.pos(), endItem.pos())
        endBoundingRect = endItem.boundingRect()
        endPolygon = QPolygonF()
        endPolygon << endBoundingRect.topRight() << endBoundingRect.topLeft() << endBoundingRect.bottomLeft() << endBoundingRect.bottomRight()
        
        p1 = endPolygon.first() + endItem.pos()

        intersectPos = QPointF()
        for i in endPolygon:
            p2 = i + endItem.pos()
            polyLine = QLineF(p1, p2)
            intersectType = polyLine.intersect(centerLine, intersectPos)
            if intersectType == QLineF.BoundedIntersection:
                break
            p1 = p2
        
        self.setLine(QLineF(intersectPos, startItem.pos()))
        line = self.line()

        angle = 0
        if line.length() > 0:
            angle = math.acos(line.dx() / line.length())
        if line.dy() >= 0:
            angle = (math.pi * 2.0) - angle

        arrowP1 = line.p1() + QPointF(math.sin(angle + math.pi / 3.0) * arrowSize,
                                      math.cos(angle + math.pi / 3.0) * arrowSize)
        arrowP2 = line.p1() + QPointF(math.sin(angle + math.pi - math.pi / 3.0) * arrowSize,
                                      math.cos(angle + math.pi - math.pi / 3.0) * arrowSize)

        self.__arrowHead.clear()
        for point in [line.p1(), arrowP1, arrowP2]:
            self.__arrowHead.append(point)

        painter.drawLine(line)
        painter.drawPolygon(self.__arrowHead)

        if self.isSelected():
            painter.setPen(QPen(self.color(), 1, Qt.DashLine))
            line = QLineF(self.line())
            line.translate(0, 5.0)
            painter.drawLine(line)
            line.translate(0,-10.0)
            painter.drawLine(line)

