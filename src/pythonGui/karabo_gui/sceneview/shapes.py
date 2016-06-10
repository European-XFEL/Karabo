#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 6, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from abc import ABCMeta, abstractmethod

from PyQt4.QtCore import QLine, QRect, Qt
from PyQt4.QtGui import QBrush, QColor, QPen

from karabo_gui.pathparser import Parser
from .const import QT_PEN_CAP_STYLE, QT_PEN_JOIN_STYLE


class BaseShape(object, metaclass=ABCMeta):
    """ A shape base class
    """

    def __init__(self, model):
        super(BaseShape, self).__init__()
        self.model = model

        self.pen = QPen()
        self.pen.setWidth(1)
        self.brush = QBrush()

        self.shape = None
        self.selected = False

        self.set_pen()

    def set_pen(self):
        pen = QPen()
        if self.model.stroke == "none" or self.model.stroke_width == 0:
            pen.setStyle(Qt.NoPen)
        else:
            c = QColor(self.model.stroke)
            c.setAlphaF(self.model.stroke_opacity)
            pen.setColor(c)
            pen.setCapStyle(QT_PEN_CAP_STYLE[self.model.stroke_linecap])
            pen.setWidthF(self.model.stroke_width)
            pen.setDashOffset(self.model.stroke_dashoffset)

            if self.model.stroke_dasharray:
                pen.setDashPattern(self.model.stroke_dasharray)

            pen.setStyle(self.model.stroke_style)
            pen.setJoinStyle(QT_PEN_JOIN_STYLE[self.model.stroke_linejoin])
            pen.setMiterLimit(self.model.stroke_miterlimit)
        self.pen = pen

        if self.model.fill == "none":
            self.brush = QBrush()
        else:
            color = QColor(self.model.fill)
            color.setAlphaF(self.model.fill_opacity)
            self.brush = QBrush(color)

    @abstractmethod
    def geometry(self):
        """ Needs to be reimplemented in the inherited classes to get the
            geometry for the bounding rectangle.
        """

    def draw(self, painter):
        if self.selected:
            black = QPen(Qt.black)
            black.setStyle(Qt.DashLine)
            white = QPen(Qt.white)
            painter.setPen(white)
            painter.drawRect(self.geometry())
            painter.setPen(black)
            painter.drawRect(self.geometry())


class LineShape(BaseShape):
    """ A line which can appear in a scene
    """

    def __init__(self, model):
        super(LineShape, self).__init__(model)
        self.shape = QLine(self.model.x1, self.model.y1, self.model.x2,
                           self.model.y2)

    def geometry(self):
        return QRect(self.shape.p1(), self.shape.p2())

    def draw(self, painter):
        """ The line gets drawn.
        """
        painter.setPen(self.pen)
        painter.drawLine(self.shape)
        super(LineShape, self).draw(painter)


class RectangleShape(BaseShape):
    """ A rectangle which can appear in a scene
    """

    def __init__(self, model):
        super(RectangleShape, self).__init__(model)
        self.shape = QRect(self.model.x, self.model.y, self.model.width,
                           self.model.height)

    def geometry(self):
        return self.shape

    def draw(self, painter):
        """ The rectangle gets drawn.
        """
        painter.setPen(self.pen)
        painter.setBrush(self.brush)
        painter.drawRect(self.shape)
        super(RectangleShape, self).draw(painter)


class PathShape(BaseShape):
    """ A path which can appear in a scene
    """

    def __init__(self, model):
        super(PathShape, self).__init__(model)
        parser = Parser(self.model.svg_data)
        self.shape = parser.parse()

    def geometry(self):
        return self.shape.boundingRect().toRect()

    def draw(self, painter):
        """ The path gets drawn.
        """
        painter.setPen(self.pen)
        painter.setBrush(self.brush)
        painter.drawPath(self.shape)
        super(PathShape, self).draw(painter)
