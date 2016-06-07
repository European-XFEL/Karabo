#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 6, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtCore import QLine, QRect, Qt
from PyQt4.QtGui import QBrush, QColor, QPen


class BaseShape(object):
    """ A shape base class
    """

    def __init__(self, model):
        super(BaseShape, self).__init__()

        self.pen = QPen()
        self.pen.setWidth(1)
        self.brush = QBrush()

        self.shape = None
        self.selected = False

        self.set_pen(model)

    def set_pen(self, model):
        pen = QPen()
        if model.stroke == "none" or model.stroke_width == 0:
            pen.setStyle(Qt.NoPen)
        else:
            c = QColor(model.stroke)
            c.setAlphaF(model.stroke_opacity)
            pen.setColor(c)
            pen.setCapStyle(dict(
                butt=Qt.FlatCap, square=Qt.SquareCap, round=Qt.RoundCap)
                [model.stroke_linecap])
            pen.setWidthF(model.stroke_width)
            pen.setDashOffset(model.stroke_dashoffset)

            if model.stroke_dasharray:
                pen.setDashPattern(model.stroke_dasharray)

            pen.setStyle(model.stroke_style)
            pen.setJoinStyle(dict(miter=Qt.SvgMiterJoin, round=Qt.RoundJoin,
                             bevel=Qt.BevelJoin)[model.stroke_linejoin])
            pen.setMiterLimit(model.stroke_miterlimit)
        self.pen = pen

        if model.fill == "none":
            self.brush = QBrush()
        else:
            color = QColor(model.fill)
            color.setAlphaF(model.fill_opacity)
            self.brush = QBrush(color)

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
        self.shape = QLine(model.x1, model.y1, model.x2, model.y2)

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
        self.shape = QRect(model.x, model.y, model.width, model.height)

    def geometry(self):
        return self.shape

    def draw(self, painter):
        """ The rectangle gets drawn.
        """
        painter.setPen(self.pen)
        painter.setBrush(self.brush)
        painter.drawRect(self.shape)
        super(RectangleShape, self).draw(painter)
