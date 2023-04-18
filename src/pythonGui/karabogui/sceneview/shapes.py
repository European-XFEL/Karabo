#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 6, 2016
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#############################################################################
from abc import abstractmethod

from qtpy.QtCore import (
    QLine, QLineF, QMargins, QPoint, QPointF, QRect, QSize, Qt)
from qtpy.QtGui import QBrush, QColor, QPainterPath, QPen, QTransform
from qtpy.QtWidgets import QDialog
from traits.api import (
    ABCHasStrictTraits, Bool, Constant, Float, Instance, List, Property, Tuple,
    cached_property, on_trait_change)

from karabo.common.scenemodel.api import BaseShapeObjectData, XMLElementModel
from karabogui.dialogs.pen_dialogs import PenDialog
from karabogui.pathparser import Parser

from .const import (
    GRID_STEP, QT_PEN_CAP_STYLE_FROM_STR, QT_PEN_CAP_STYLE_TO_STR,
    QT_PEN_JOIN_STYLE_FROM_STR, QT_PEN_JOIN_STYLE_TO_STR, SCREEN_MAX_VALUE)
from .utils import calc_rotated_point

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


class MarkerShape(BaseShape):
    """This assumes that the child marker element is a path.
       Rects, circles, etc. are not yet supported."""

    model = Instance(XMLElementModel)

    children = List
    _offset = Instance(QPoint, args=())
    _scale = Float(1.0)
    _angle = Float(0.0)

    ref_point = Property(Instance(QPointF),
                         depends_on=["_angle", "_offset", "_scale"])

    use_stroke_width = Bool

    def _children_default(self):
        return [PathShape(model=model) for model in self.model.children]

    def draw(self, painter):
        for child in self.children:
            child.draw(painter)

    def geometry(self):
        rect = QRect()
        for child in self.children:
            rect = rect.united(child.geometry())
        return rect

    def set_geometry(self, rect):
        """Marker geometry depend on the union of the children geometries.
           We do not support setting the geometry from here, hence the
           blank reimplementation"""

    def translate(self, offset):
        self._offset = offset

    @on_trait_change("_offset, ref_point")
    def _translate(self):
        for child in self.children:
            child.translate(self._offset - self.ref_point)

    def rotate(self, angle):
        if self.model.orient == "auto":
            self._angle = angle
            for child in self.children:
                child.rotate(angle)

    def scale(self, scale):
        """SVG markers are scaled wrt strokeWidth or userSpaceOnUse.
           Currently, we only support strokeWidth scaling."""
        if self.use_stroke_width:
            self._scale = scale

            for child in self.children:
                # Correct scale if stroke width is 0
                child.scale(scale or 1)

    @cached_property
    def _get_ref_point(self):
        return calc_rotated_point(x=self.model.refX, y=self.model.refY,
                                  angle=self._angle, scale=self._scale)

    def _use_stroke_width_default(self):
        # This is used for scaling the the arrow and the marker by either the
        # stroke width or by user space. The current implementation is build by
        # around stroke width scaling, but we still have to check against
        # `useSpaceOnUse`. If markerUnits is not specified, we default to
        # using stroke width.
        return self.model.markerUnits != "userSpaceOnUse"


class ArrowShape(LineShape):

    line = Property(Instance(QLineF), depends_on="shape")
    marker = Instance(MarkerShape)

    def _marker_default(self):
        return MarkerShape(model=self.model.marker)

    @cached_property
    def _get_line(self):
        return QLineF(self.shape)

    def draw(self, painter):
        """Draw the line and the marker shapes"""
        super(ArrowShape, self).draw(painter)  # line
        self.marker.draw(painter)  # marker

    def geometry(self):
        """The geometry of the arrow shape depends on the union of the line
           shape and marker shape geometries."""
        line_geom = super(ArrowShape, self).geometry()
        return line_geom.united(self.marker.geometry())

    def set_geometry(self, rect):
        """Set the geometry by accounting the difference of the rects with the
           markers and applying the difference on the plain the rect"""
        diff = tuple(new - old for new, old in
                     zip(rect.getCoords(), self.geometry().getCoords()))

        # Adjust line rect.
        line_rect = super(ArrowShape, self).geometry()
        super(ArrowShape, self).set_geometry(line_rect.adjusted(*diff))

    @on_trait_change("line")
    def _transform_marker(self):
        """Rotate the marker with the angle made by the line"""
        self.marker.translate(self.shape.p2())
        if self.line.length() != 0:
            self.marker.rotate(self.line.angle())

    @on_trait_change("model.stroke_width")
    def _scale_marker(self):
        self.marker.scale(self.model.stroke_width)

    def minimumSize(self):
        """We use the line and the marker sizes for the effective
           minimum size."""
        line_size = super(ArrowShape, self).minimumSize()
        marker_size = self.marker.geometry().size()
        return QSize(max(line_size.width(), marker_size.width()),
                     max(line_size.height(), marker_size.height()))


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
