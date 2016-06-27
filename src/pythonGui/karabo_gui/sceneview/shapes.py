#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 6, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from abc import ABCMeta, abstractmethod

from PyQt4.QtCore import QLine, QRect, QSize, Qt
from PyQt4.QtGui import QBrush, QColor, QDialog, QPen

from karabo_gui.dialogs.dialogs import PenDialog
from karabo_gui.pathparser import Parser
from .const import QT_PEN_CAP_STYLE, QT_PEN_JOIN_STYLE, SCREEN_MAX_VALUE


class BaseShape(object, metaclass=ABCMeta):
    """ A shape base class
    """

    def __init__(self, model, brush=None):
        super(BaseShape, self).__init__()
        self.pen = QPen()
        self.pen.setWidth(1)
        self.brush = brush

        self.shape = None
        self.selected = False
        self._hide_from_view = False

        self.set_model(model)

    def set_model(self, model):
        """ Set the new ``model`` and update the shape properties.
        """
        self.model = model
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

        if self.model.fill != "none" and self.brush is not None:
            color = QColor(self.model.fill)
            color.setAlphaF(self.model.fill_opacity)
            self.brush.setColor(color)
            self.brush.setStyle(Qt.SolidPattern)

    def hide(self):
        self._hide_from_view = True

    def show(self):
        self._hide_from_view = False

    def is_visible(self):
        return not self._hide_from_view

    @abstractmethod
    def draw(self, painter):
        """ Needs to be reimplemented in inherited classes to draw the shape.
        """

    @abstractmethod
    def geometry(self):
        """ Needs to be reimplemented in inherited classes to get the geometry
        for the bounding rectangle.
        """

    @abstractmethod
    def set_geometry(self, rect):
        """ Needs to be reimplemented in inherited classes to set the geometry
        for the bounding rectangle.
        """

    @abstractmethod
    def translate(self, offset):
        """ Needs to be reimplemented in inherited classes to move the shape.
        """

    def minimumSize(self):
        return QSize(0, 0)

    def maximumSize(self):
        return QSize(SCREEN_MAX_VALUE, SCREEN_MAX_VALUE)

    def edit(self):
        """ Edits the pen of the shape."""
        dialog = PenDialog(self.pen, self.brush)
        if dialog.exec_() == QDialog.Rejected:
            return

        pen = dialog.pen
        brush = dialog.brush

        if pen.style() == Qt.NoPen:
            self.model.stroke = 'none'
        else:
            self.model.stroke = pen.color().name()
        self.model.stroke_opacity = pen.color().alphaF()
        self.model.stroke_linecap = QT_PEN_CAP_STYLE[pen.capStyle()]
        self.model.stroke_dashoffset = pen.dashOffset()
        self.model.stroke_width = pen.widthF()
        self.model.stroke_dasharray = pen.dashPattern()
        self.model.stroke_style = pen.style()
        self.model.stroke_linejoin = QT_PEN_JOIN_STYLE[pen.joinStyle()]
        self.model.stroke_miterlimit = pen.miterLimit()
        if brush.style() == Qt.SolidPattern:
            self.model.fill = brush.color().name()
            self.model.fill_opacity = brush.color().alphaF()
        else:
            self.model.fill = 'none'

        self.set_pen()


class LineShape(BaseShape):
    """ A line which can appear in a scene
    """

    def __init__(self, model):
        super(LineShape, self).__init__(model)
        self.shape = QLine(self.model.x1, self.model.y1, self.model.x2,
                           self.model.y2)

    def draw(self, painter):
        """ The line gets drawn.
        """
        painter.setPen(self.pen)
        painter.drawLine(self.shape)

    def geometry(self):
        return QRect(self.shape.p1(), self.shape.p2())

    def set_geometry(self, rect):
        self.shape = QLine(rect.topLeft(), rect.bottomRight())
        self._update_model_values()

    def translate(self, offset):
        self.shape.translate(offset)
        self._update_model_values()

    def _update_model_values(self):
        self.model.set(x1=self.shape.x1(), y1=self.shape.y1(),
                       x2=self.shape.x2(), y2=self.shape.y2())


class RectangleShape(BaseShape):
    """ A rectangle which can appear in a scene
    """

    def __init__(self, model):
        super(RectangleShape, self).__init__(model, QBrush())
        self.shape = QRect(self.model.x, self.model.y, self.model.width,
                           self.model.height)

    def draw(self, painter):
        """ The rectangle gets drawn.
        """
        painter.setPen(self.pen)
        painter.setBrush(self.brush)
        painter.drawRect(self.shape)

    def geometry(self):
        return self.shape

    def set_geometry(self, rect):
        self.shape = rect
        self.model.set(x=rect.x(), y=rect.y(),
                       width=rect.width(), height=rect.height())

    def translate(self, offset):
        self.shape.translate(offset)
        self.model.set(x=self.shape.x(), y=self.shape.y())


class PathShape(BaseShape):
    """ A path which can appear in a scene
    """

    def __init__(self, model):
        super(PathShape, self).__init__(model, QBrush())
        parser = Parser(self.model.svg_data)
        self.shape = parser.parse()

    def draw(self, painter):
        """ The path gets drawn.
        """
        painter.setPen(self.pen)
        painter.setBrush(self.brush)
        painter.drawPath(self.shape)

    def geometry(self):
        return self.shape.boundingRect().toRect()

    def set_geometry(self, rect):
        pass

    def translate(self, offset):
        self.shape.translate(offset)
