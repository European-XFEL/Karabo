#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 6, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtCore import QLine, QRect, Qt
from PyQt4.QtGui import QBrush, QLabel, QPen


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


class Label(BaseShape):
    """ A label which can appear in a scene
    """

    def __init__(self, model):
        super(Label, self).__init__(model)
        self.shape = QLabel(model.text)


class LineShape(BaseShape):
    """ A line which can appear in a scene
    """

    def __init__(self, model):
        super(LineShape, self).__init__(model)
        self.shape = QLine(model.x1, model.y1, model.x2, model.y2)

    def draw(self, painter):
        """ The line gets drawn.
        """
        painter.setPen(self.pen)
        painter.drawLine(self.shape)
        super(LineShape, self).draw(self, painter)


class RectangleShape(BaseShape):
    """ A rectangle which can appear in a scene
    """

    def __init__(self, model):
        super(RectangleShape, self).__init__(model)
        self.shape = QRect(model.x, model.y, model.width, model.height)

    def draw(self, painter):
        """ The rectangle gets drawn.
        """
        painter.setPen(self.pen)
        painter.setBrush(self.brush)
        painter.drawRect(self.shape)
        super(RectangleShape, self).draw(self, painter)
