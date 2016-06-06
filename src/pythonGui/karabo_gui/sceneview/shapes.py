#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 6, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtCore import QLine, QRect, Qt
from PyQt4.QtGui import QBrush, QPen


class Shape(object):
    """ A shape base class
    """

    def __init__(self, model):
        super(Shape, self).__init__()

        self.pen = QPen()
        self.pen.setWidth(1)
        self.brush = QBrush()

        self.shape = None
        self.selected = False

        self.set_pen(model)

    def set_pen(self, model):
        # TODO: get pen data of model
        pass

    def draw(self, painter):
        if self.selected:
            black = QPen(Qt.black)
            black.setStyle(Qt.DashLine)
            white = QPen(Qt.white)
            painter.setPen(white)
            painter.drawRect(self.geometry())
            painter.setPen(black)
            painter.drawRect(self.geometry())


class Line(Shape):
    """ A line which can appear in a scene
    """

    def __init__(self, model):
        super(Line, self).__init__(model)
        self.shape = QLine(model.x1, model.y1, model.x2, model.y2)

    def draw(self, painter):
        """ The line gets drawn.
        """
        painter.setPen(self.pen)
        painter.drawLine(self.shape)
        Shape.draw(self, painter)


class Rectangle(Shape):
    """ A rectangle which can appear in a scene
    """

    def __init__(self, model):
        super(Rectangle, self).__init__(model)
        self.shape = QRect(model.x, model.y, model.width, model.height)

    def draw(self, painter):
        """ The line gets drawn.
        """
        painter.setPen(self.pen)
        painter.setBrush(self.brush)
        painter.drawRect(self.shape)
        Shape.draw(self, painter)
