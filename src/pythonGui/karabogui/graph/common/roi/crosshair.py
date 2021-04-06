import numpy as np
from qtpy.QtCore import Slot, QRectF
from qtpy.QtGui import QPainterPathStroker, QPainterPath
from pyqtgraph import Point

from karabogui.graph.common.utils import float_to_string

from .base import KaraboROI
from .utils import ImageRegion, set_roi_html


class CrosshairROI(KaraboROI):

    def __init__(self, pos=None, size=(1, 1), name='',
                 scale_snap=False, translate_snap=False, pen=None):
        """Reimplementation of pyqtgraph CrosshairROI to support transforms
        and add a few fixes/performance improvements:

        - Correct position according to snap rules
          (+ scale/2, where scale = 1)
        - Force redraw the crosshair shape with sigRegionChanged"""

        super(CrosshairROI, self).__init__(pos + 0.5, size, name,
                                           scale_snap, translate_snap,
                                           pen=pen)
        self._shape = None
        self._path = None
        self.sigRegionChanged.connect(self.redraw)
        self.aspectLocked = True

    @Slot()
    def _change_roi_text_item_details(self):
        if not self.textItem:
            return

        self.textItem.setPos(self.pos())
        x, y = [float_to_string(coord) for coord in self.center]
        self.textItem.setHtml(set_roi_html(name=self.name, center=(x, y)))

    def invalidate(self):
        self._shape = None
        self.prepareGeometryChange()

    @property
    def center(self):
        return self.pos() - (self._scaling / 2)

    @property
    def coords(self):
        return tuple(self._absolute_position - (self._scaling / 2))

    # ---------------------------------------------------------------------
    # Public methods

    def get_region(self, imageItem):
        image = imageItem.image
        x, y = np.floor(self._absolute_position).astype(int)

        if 0 <= x < image.shape[1]:
            y_region = image[:, x]
            y_slice = slice(image.shape[0])
        else:
            y_region = np.array([])
            y_slice = slice(0)

        if 0 <= y < image.shape[0]:
            x_region = image[y, :]
            x_slice = slice(image.shape[1])
        else:
            x_region = np.array([])
            x_slice = slice(0)

        image_region = ImageRegion([x_region, y_region], ImageRegion.Line,
                                   x_slice, y_slice)

        return image_region

    def redraw(self):
        """Conventional Qt items use self.update() to redraw/repaint.
           CrosshairROI, on the other hand, needs to recalculate the paths
           prior to repainting. This can be done using self.invalidate()"""
        self.invalidate()

    # ---------------------------------------------------------------------
    # PyQt methods

    def boundingRect(self):
        shape = self.shape()
        return shape.boundingRect() if shape is not None else QRectF()

    def paint(self, p, *args):
        p.setPen(self.currentPen)
        p.drawPath(self._path)

    def shape(self):
        if self._shape is None:
            self._path = self._create_path()
            self._shape = self._create_shape(self._path)

        return self._shape

    def viewTransformChanged(self):
        super(CrosshairROI, self).viewTransformChanged()
        self.invalidate()

    # ---------------------------------------------------------------------
    # PyQtGraph methods

    def getSnapPosition(self, pos, snap=None):
        """Patching to support snapping based on scaling"""
        scaled_pos = np.floor(np.array(pos) / self._scaling) * self._scaling

        # This time, we want half-integer values (middle of the pixel).
        scaled_pos += self._scaling / 2

        return Point(*scaled_pos)

    # ---------------------------------------------------------------------
    # Private methods

    def _create_shape(self, path):
        """Creates the stroke from the input path. When the viewbox is not
           rendered yet, which happens during initialization,
           `mapToDevice` returns None"""
        path = self.mapToDevice(path)
        if path is not None:
            stroker = QPainterPathStroker()
            stroker.setWidth(10)
            outline = stroker.createStroke(path)
            return self.mapFromDevice(outline)
        return None

    def _create_path(self):
        # Factor is the desired width/height wrt viewbox divided by 2
        factor = 0.1 if self.selected else 0.05
        width, height = self._get_effective_dimensions()

        # Create the path from the half dimension
        path = QPainterPath()
        # paint x-axis
        half_width = width * factor
        path.moveTo(Point(-half_width, 0))
        path.lineTo(Point(half_width, 0))
        # paint y-axis
        half_height = height * factor
        path.moveTo(Point(0, -half_height))
        path.lineTo(Point(0, half_height))

        return path

    def _get_effective_dimensions(self):
        viewbox = self.getViewBox()
        rect = self.mapRectFromView(viewbox.viewRect())
        width, height = rect.width(), rect.height()

        # Get factor that generates least common multiple from the width/height
        min_dim, max_dim = width, height
        if min_dim > max_dim:
            min_dim, max_dim = max_dim, min_dim
        factor = max_dim // min_dim

        # Get effective dimension from the multiple factor
        if width < height:
            height = width * factor
        else:
            width = height * factor

        # Correct dimension with the screen geometry ratio (rect of the plot)
        geom = viewbox.screenGeometry()
        ratio = geom.height() / geom.width()
        if ratio > 1:
            height /= ratio
        else:
            width *= ratio

        return width, height
