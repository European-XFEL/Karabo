import numpy as np
from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import QPainterPathStroker, QPainterPath
from pyqtgraph import Point

from graph.common.roi.base import KaraboROI
from graph.image.utils import float_to_string

from .utils import ROI_CENTER_HTML, ImageRegion


class CrosshairROI(KaraboROI):

    def __init__(self, pos=None, size=(1, 1),
                 scale_snap=False, translate_snap=False):
        super(CrosshairROI, self).__init__(pos, size,
                                           scale_snap, translate_snap)
        self._shape = None
        self._path = None
        self._selected = False
        self.sigRegionChanged.connect(self.invalidate)
        self.aspectLocked = True

    @pyqtSlot()
    def _change_roi_text_item_details(self):
        if not self.textItem:
            return

        self.textItem.setPos(self.pos())
        x, y = [float_to_string(coord) for coord in self.center]
        self.textItem.setHtml(ROI_CENTER_HTML.format(x, y))

    def invalidate(self):
        self._shape = None
        self.prepareGeometryChange()

    @property
    def center(self):
        return self.pos() - (self._scaling / 2)

    @property
    def coords(self):
        return (*self._absolute_position - (self._scaling / 2),)

    # ---------------------------------------------------------------------
    # Public methods

    def get_region(self, imageItem, x_data, y_data):
        image = imageItem.image
        x, y = np.floor(self._absolute_position).astype(int)

        if 0 <= x < image.shape[1]:
            y_region = image[:, x]
        else:
            y_region = np.array([])
            y_data = np.array([])

        if 0 <= y < image.shape[0]:
            x_region = image[y, :]
        else:
            x_region = np.array([])
            x_data = np.array([])

        details = ImageRegion([x_region, y_region], ImageRegion.Line,
                              x_data, y_data)

        return details

    def select(self, update=False):
        """Reimplemented because we recalculate the shape and repaint then"""
        super(CrosshairROI, self).select(update)
        self._selected = True
        self.invalidate()

    def unselect(self, update=False):
        """Reimplemented because we recalculate the shape and repaint then"""
        super(CrosshairROI, self).unselect(update)
        self._selected = False
        self.invalidate()

    # ---------------------------------------------------------------------
    # PyQt methods

    def boundingRect(self):
        return self.shape().boundingRect()

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
        path = self.mapToDevice(path)
        stroker = QPainterPathStroker()
        stroker.setWidth(10)
        outline = stroker.createStroke(path)
        return self.mapFromDevice(outline)

    def _create_path(self):
        path = QPainterPath()
        rect = self.mapRectFromView(self._viewBox().viewRect())

        # Factor is the desired width/height wrt viewbox divided by 2
        factor = 0.1 if self._selected else 0.05

        half_width = rect.width() * factor
        half_height = rect.height() * factor

        path.moveTo(Point(0, -half_height))
        path.lineTo(Point(0, half_height))

        path.moveTo(Point(-half_width, 0))
        path.lineTo(Point(half_width, 0))

        return path
