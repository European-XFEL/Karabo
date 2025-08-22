#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 6, 2016
# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
#############################################################################
from abc import abstractmethod

from qtpy.QtCore import QLine, QMargins, QPoint, QRect, QSize, Qt
from qtpy.QtGui import QBrush, QColor, QPainterPath, QPen, QPolygon, QTransform
from qtpy.QtWidgets import QDialog
from traits.api import (
    ABCHasStrictTraits, Bool, Constant, Float, Instance, Property, Tuple,
    cached_property)

from karabo.common.scenemodel.api import BaseShapeObjectData
from karabo.common.utils import get_arrowhead_points
from karabogui.dialogs.pen_dialogs import PenDialog
from karabogui.pathparser import Parser

from .const import (
    GRID_STEP, QT_PEN_CAP_STYLE_FROM_STR, QT_PEN_CAP_STYLE_TO_STR,
    QT_PEN_JOIN_STYLE_FROM_STR, QT_PEN_JOIN_STYLE_TO_STR, SCREEN_MAX_VALUE)

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
        if dialog.exec() == QDialog.Rejected:
            return

        brush = dialog.brush
        if brush is not None and brush.style() == Qt.SolidPattern:
            fill = brush.color().name()
            fill_opacity = brush.color().alphaF()
        else:
            fill = 'none'
            fill_opacity = 1.0

        pen = dialog.pen
        self.model.trait_set(
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

    def sizeHint(self):
        return self.geometry()


class LineShape(BaseShape):
    """A line which can appear in a scene
    """
    # The line that we're drawing
    shape = Property(Instance(QLine), depends_on=['model.x1', 'model.x2',
                                                  'model.y1', 'model.y2'])

    # The geometry rect of the line. This includes margins that is independent
    # to the model values
    rect = Property(Instance(QRect), depends_on="shape")

    # There is a need to put margins to avoid mouse interaction problems.
    # This does not affect the model, but will most probably have an effect on
    # line shapes on a group/layout. This is an unavoidable evil.
    _margins = (GRID_STEP, GRID_STEP, GRID_STEP, GRID_STEP)
    _qmargins = Constant(QMargins(*_margins))

    # Record the vector direction on class instantiation. This is used to get
    # the correct line coords as we use its normalized rect in the scene.
    has_negative_width = Bool
    has_negative_height = Bool

    def _get_brush(self):
        """Reimplement the base-class property getter. No brushes here! """
        return None

    @cached_property
    def _get_shape(self):
        model = self.model
        return QLine(model.x1, model.y1, model.x2, model.y2)

    @cached_property
    def _get_rect(self):
        line = self.shape
        return QRect(line.p1(), line.p2())

    def _has_negative_width_default(self):
        model = self.model
        return model.x2 < model.x1

    def _has_negative_height_default(self):
        model = self.model
        return model.y2 < model.y1

    def draw(self, painter):
        """The line gets drawn.
        """
        painter.setPen(self.pen)
        painter.drawLine(self.shape)

    def geometry(self):
        # We have to return the normalized rect as this is used for mapping
        # the mouse position.
        return self.rect.normalized() + self._qmargins

    def set_geometry(self, rect):
        """Sets the effective geometry in the model which is without
           the margins and the normalization."""

        # Correct the rect first by reverting our changes:
        # 1. margins - we remove them
        rect -= self._qmargins

        # 2. normalization - we do it manually
        x1, y1, x2, y2 = rect.getCoords()
        if self.has_negative_width:
            x1, x2 = x2, x1
        if self.has_negative_height:
            y1, y2 = y2, y1

        self.model.trait_set(x1=x1, y1=y1, x2=x2, y2=y2)

    def translate(self, offset):
        x1, y1 = self.model.x1, self.model.y1
        x2, y2 = self.model.x2, self.model.y2
        xoff, yoff = offset.x(), offset.y()
        self.model.trait_set(x1=x1 + xoff, y1=y1 + yoff,
                             x2=x2 + xoff, y2=y2 + yoff)

    def minimumSize(self):
        """We use the margins for the minimum size. This just means that the
           effective rect has zero width/height"""
        left, top, right, bottom = self._margins
        return QSize(left + right, top + bottom)


class ArrowShape(LineShape):
    """Arrow has an additional arrowhead painted at the end of the line"""

    def draw(self, painter):
        super().draw(painter)
        model = self.model
        hp1 = QPoint(model.hx1, model.hy1)
        hp3 = QPoint(model.hx2, model.hy2)
        hp2 = QPoint(model.x2, model.y2)

        # Paint arrowhead as a polygon of three points
        painter.setBrush(QBrush(painter.pen().color()))
        painter.drawPolygon(QPolygon([hp1, hp2, hp3]))

    def translate(self, offset):
        super().translate(offset)
        hx1, hy1, hx2, hy2 = get_arrowhead_points(
            self.model.x1, self.model.y1, self.model.x2, self.model.y2)
        self.model.trait_set(hx1=hx1, hy1=hy1, hx2=hx2, hy2=hy2)

    def set_geometry(self, rect):
        super().set_geometry(rect)
        hx1, hy1, hx2, hy2 = get_arrowhead_points(
            self.model.x1, self.model.y1, self.model.x2, self.model.y2)
        self.model.trait_set(hx1=hx1, hy1=hy1, hx2=hx2, hy2=hy2)


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
        self.model.trait_set(x=rect.x(), y=rect.y(),
                             width=rect.width(), height=rect.height())

    def translate(self, offset):
        x, y = self.model.x, self.model.y
        self.model.trait_set(x=x + offset.x(), y=y + offset.y())


class PathShape(BaseShape):
    """A path which can appear in a scene
    """
    # The path object
    path = Instance(QPainterPath)
    shape = Property(Instance(QPainterPath), depends_on=["path", "transform"])
    _angle = Float(0.0)
    _offset = Tuple(0.0, 0.0)
    _scale = Float(1.0)
    transform = Property(Instance(QTransform),
                         depends_on=["_angle", "_offset", "_scale"])

    def _path_default(self):
        parser = Parser(self.model.svg_data)
        return parser.parse()

    @cached_property
    def _get_shape(self):
        return self.transform.map(self.path)

    def draw(self, painter):
        painter.setPen(self.pen)
        painter.setBrush(self.brush)
        painter.drawPath(self.shape)

    def geometry(self):
        return self.shape.boundingRect().toRect()

    def set_geometry(self, rect):
        pass

    def translate(self, offset):
        self._offset = offset.x(), offset.y()

    def rotate(self, angle):
        self._angle = -angle

    def scale(self, scale):
        self._scale = scale

    @cached_property
    def _get_transform(self):
        transform = QTransform()
        transform.translate(*self._offset)
        transform.scale(self._scale, self._scale)
        transform.rotate(self._angle)
        return transform
