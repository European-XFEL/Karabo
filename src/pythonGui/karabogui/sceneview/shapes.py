#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 6, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from abc import abstractmethod

from PyQt5.QtCore import QLine, QRect, QSize, Qt
from PyQt5.QtGui import QBrush, QColor, QPainterPath, QPen
from PyQt5.QtWidgets import QDialog
from traits.api import (ABCHasStrictTraits, Bool, Instance, Property,
                        cached_property)

from karabo.common.scenemodel.api import BaseShapeObjectData
from karabogui.dialogs.dialogs import PenDialog
from karabogui.pathparser import Parser
from .const import (QT_PEN_CAP_STYLE_FROM_STR, QT_PEN_CAP_STYLE_TO_STR,
                    QT_PEN_JOIN_STYLE_FROM_STR, QT_PEN_JOIN_STYLE_TO_STR,
                    SCREEN_MAX_VALUE)

_BRUSH_ATTRS = ('fill', 'fill_opacity')
_PEN_ATTRS = ('stroke', 'stroke_opacity', 'stroke_linecap',
              'stroke_dashoffset', 'stroke_width', 'stroke_dasharray',
              'stroke_style', 'stroke_linejoin', 'stroke_miterlimit')


class BaseShape(ABCHasStrictTraits):
    """A shape base class
    """
    # The scene data model backing this shape
    model = Instance(BaseShapeObjectData)
    # The brush used by this shape
    brush = Property(Instance(QBrush),
                     depends_on=['model.' + attr for attr in _BRUSH_ATTRS])
    # The pen used by this shape
    pen = Property(Instance(QPen),
                   depends_on=['model.' + attr for attr in _PEN_ATTRS])
    # Is the shape currently shown?
    _hidden = Bool(False)

    @cached_property
    def _get_brush(self):
        brush = QBrush()
        if self.model.fill != "none":
            color = QColor(self.model.fill)
            color.setAlphaF(self.model.fill_opacity)
            brush.setStyle(Qt.SolidPattern)
            brush.setColor(color)
        return brush

    @cached_property
    def _get_pen(self):
        pen = QPen()
        model = self.model
        if model.stroke == "none" or model.stroke_width == 0:
            pen.setStyle(Qt.NoPen)
        else:
            c = QColor(model.stroke)
            c.setAlphaF(model.stroke_opacity)
            pen.setColor(c)
            pen.setCapStyle(QT_PEN_CAP_STYLE_FROM_STR[model.stroke_linecap])
            pen.setWidthF(model.stroke_width)
            pen.setDashOffset(model.stroke_dashoffset)

            if model.stroke_dasharray:
                pen.setDashPattern(model.stroke_dasharray)

            pen.setStyle(model.stroke_style)
            join_style = QT_PEN_JOIN_STYLE_FROM_STR[model.stroke_linejoin]
            pen.setJoinStyle(join_style)
            pen.setMiterLimit(model.stroke_miterlimit)
        return pen

    def edit(self, scene_view):
        """ Edits the pen and brush of the shape."""
        dialog = PenDialog(self.pen, self.brush, parent=scene_view)
        if dialog.exec_() == QDialog.Rejected:
            return

        brush = dialog.brush
        if brush is not None and brush.style() == Qt.SolidPattern:
            fill = brush.color().name()
            fill_opacity = brush.color().alphaF()
        else:
            fill = 'none'
            fill_opacity = 1.0

        pen = dialog.pen
        self.model.set(
            stroke='none' if pen.style() == Qt.NoPen else pen.color().name(),
            stroke_opacity=pen.color().alphaF(),
            stroke_linecap=QT_PEN_CAP_STYLE_TO_STR[pen.capStyle()],
            stroke_dashoffset=pen.dashOffset(),
            stroke_width=0 if pen.style() == Qt.NoPen else pen.widthF(),
            stroke_dasharray=pen.dashPattern(),
            stroke_style=pen.style(),
            stroke_linejoin=QT_PEN_JOIN_STYLE_TO_STR[pen.joinStyle()],
            stroke_miterlimit=pen.miterLimit(),
            fill=fill,
            fill_opacity=fill_opacity
        )

    def hide(self):
        self._hidden = True

    def show(self):
        self._hidden = False

    def is_visible(self):
        return not self._hidden

    @abstractmethod
    def draw(self, painter):
        """Needs to be reimplemented in inherited classes to draw the shape.
        """

    @abstractmethod
    def geometry(self):
        """Needs to be reimplemented in inherited classes to get the geometry
        for the bounding rectangle.
        """

    @abstractmethod
    def set_geometry(self, rect):
        """Needs to be reimplemented in inherited classes to set the geometry
        for the bounding rectangle.
        """

    @abstractmethod
    def translate(self, offset):
        """Needs to be reimplemented in inherited classes to move the shape.
        """

    def minimumSize(self):
        return QSize(0, 0)

    def maximumSize(self):
        return QSize(SCREEN_MAX_VALUE, SCREEN_MAX_VALUE)


class LineShape(BaseShape):
    """A line which can appear in a scene
    """
    # The line that we're drawing
    shape = Property(Instance(QLine), depends_on=['model.x1', 'model.x2',
                                                  'model.y1', 'model.y2'])

    def _get_brush(self):
        """Reimplement the base-class property getter. No brushes here! """
        return None

    @cached_property
    def _get_shape(self):
        model = self.model
        return QLine(model.x1, model.y1, model.x2, model.y2)

    def draw(self, painter):
        """The line gets drawn.
        """
        painter.setPen(self.pen)
        painter.drawLine(self.shape)

    def geometry(self):
        return QRect(self.shape.p1(), self.shape.p2())

    def set_geometry(self, rect):
        start, end = rect.topLeft(), rect.bottomRight()
        self.model.set(x1=start.x(), y1=start.y(), x2=end.x(), y2=end.y())

    def translate(self, offset):
        x1, y1 = self.model.x1, self.model.y1
        x2, y2 = self.model.x2, self.model.y2
        xoff, yoff = offset.x(), offset.y()
        self.model.set(x1=x1 + xoff, y1=y1 + yoff, x2=x2 + xoff, y2=y2 + yoff)


class RectangleShape(BaseShape):
    """A rectangle which can appear in a scene
    """
    # The rectangle that we're drawing
    shape = Property(Instance(QRect),
                     depends_on=['model.x', 'model.y', 'model.height',
                                 'model.width'])

    @cached_property
    def _get_shape(self):
        model = self.model
        return QRect(model.x, model.y, model.width, model.height)

    def draw(self, painter):
        painter.setPen(self.pen)
        painter.setBrush(self.brush)
        painter.drawRect(self.shape)

    def geometry(self):
        return self.shape

    def set_geometry(self, rect):
        self.model.set(x=rect.x(), y=rect.y(),
                       width=rect.width(), height=rect.height())

    def translate(self, offset):
        x, y = self.model.x, self.model.y
        self.model.set(x=x + offset.x(), y=y + offset.y())


class PathShape(BaseShape):
    """A path which can appear in a scene
    """
    # The path object
    shape = Property(Instance(QPainterPath), depends_on=['model.svg_data'])

    @cached_property
    def _get_shape(self):
        parser = Parser(self.model.svg_data)
        return parser.parse()

    def draw(self, painter):
        painter.setPen(self.pen)
        painter.setBrush(self.brush)
        painter.drawPath(self.shape)

    def geometry(self):
        return self.shape.boundingRect().toRect()

    def set_geometry(self, rect):
        pass

    def translate(self, offset):
        self.shape.translate(offset)
