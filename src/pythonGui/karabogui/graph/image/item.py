from collections import Callable

import numpy as np
from PyQt4.QtCore import pyqtSignal, QPointF, Qt
from PyQt4.QtGui import QTransform
from pyqtgraph import functions as fn, ImageItem, Point
from scipy.ndimage import zoom

from karabogui.graph.common.api import MouseMode
from .utils import map_rect_to_transform


DIMENSION_DOWNSAMPLE = [(500, 1.5), (1000, 2)]  # [(dimension, min downsample)]


class KaraboImageItem(ImageItem):
    clicked = pyqtSignal(float, float)
    hovered = pyqtSignal(object, object)

    def __init__(self, image=np.zeros((50, 50), dtype=int)):
        super(KaraboImageItem, self).__init__(image)

        self._origin = np.array([0, 0])
        self.auto_levels = True
        self._rect = None

    # ---------------------------------------------------------------------
    # Public methods

    def get_qimage(self):
        if self.qimage is None:
            self.render()
        return self.qimage

    def set_transform(self, scaling=(1, 1), translation=(0, 0), rotation=0):
        transform = QTransform()
        transform.rotate(rotation)
        transform.scale(*scaling)
        transform.translate(*translation)
        self.setTransform(transform)

        # Calculate new image item geometry
        self._origin = np.multiply(translation, scaling)
        self._rect = map_rect_to_transform(self.boundingRect(),
                                           scaling, self._origin)

    def rect(self):
        if self._rect is None:
            return self.boundingRect()
        return self._rect

    # ---------------------------------------------------------------------
    # Events

    def mouseClickEvent(self, event):
        if (self._viewBox().mouse_mode is MouseMode.Picker
                and event.button() == Qt.LeftButton):
            image_pos, view_pos = self._get_mouse_positions(event.pos())
            self.clicked.emit(image_pos.x(), image_pos.y())
        super(KaraboImageItem, self).mouseClickEvent(event)

    def hoverEvent(self, event):
        if self._viewBox().mouse_mode is MouseMode.Picker:
            self._hover_pointer_mode(event)
        super(KaraboImageItem, self).hoverEvent(event)

    # ---------------------------------------------------------------------
    # Private methods

    def _hover_pointer_mode(self, event):
        if event.isExit():
            self.hovered.emit(None, None)
            return

        event.acceptDrags(Qt.LeftButton)
        event.acceptClicks(Qt.LeftButton)

        (image_pos, view_pos) = self._get_mouse_positions(event.pos())
        self.hovered.emit(image_pos.x(), image_pos.y())

    def _get_mouse_positions(self, pos):
        image_pos = self.mapToParent(*np.floor(pos))
        view_pos = image_pos - QPointF(*self._origin)
        return image_pos, view_pos

    # ---------------------------------------------------------------------
    # Render patches

    def render(self):
        """Reimplementing for performance improvements"""

        # Convert data to QImage for display.

        # profile = debug.Profiler()
        if self.image is None or self.image.size == 0:
            return
        if isinstance(self.lut, Callable):
            lut = self.lut(self.image)
        else:
            lut = self.lut

        image = self.image

        if self.autoDownsample:
            # reduce dimensions of image based on screen resolution
            o = self.mapToDevice(QPointF(0, 0))
            x = self.mapToDevice(QPointF(1, 0))
            y = self.mapToDevice(QPointF(0, 1))
            w = Point(x - o).length()
            h = Point(y - o).length()
            if w == 0 or h == 0:
                self.qimage = None
                return

            # --- Start patching ---
            # Calculate scale on the nearest hundredths. Don't scale if the
            # item dimension is bigger than the image dimensions.
            image_y, image_x = image.shape

            # Swap dimensions if ImageItem is in row-major
            if self.axisOrder == 'row-major':
                w, h = h, w

            # Calculate scaling factor for each axis.
            # - if dimension >= 1000, min scale is 2
            # - if dimension in [500, 1000), min scale is 2
            # - if dimension is smaller than 500, no scaling is done
            xds = 1
            yds = 1
            for dim, min_ds in DIMENSION_DOWNSAMPLE:
                if image_x > dim:
                    xds = max(min_ds, round(1.0 / w * 100) / 100)
                if image_y > dim:
                    yds = max(min_ds, round(1.0 / h * 100) / 100)

            # Scale the image only if the dimension(s) is greater than 1
            if (xds, yds) != (1, 1):
                scale = [1/yds, 1/xds]
                image = zoom(self.image, scale, order=1)

            self._lastDownsample = (xds, yds)
            # --- End patching ---

        # if the image data is a small int, then we can combine levels + lut
        # into a single lut for better performance
        levels = self.levels
        if (levels is not None and levels.ndim == 1
                and image.dtype in (np.ubyte, np.uint16)):
            if self._effectiveLut is None:
                eflsize = 2 ** (image.itemsize * 8)
                ind = np.arange(eflsize)
                minlev, maxlev = levels
                levdiff = maxlev - minlev
                # don't allow division by 0
                levdiff = 1 if levdiff == 0 else levdiff
                if lut is None:
                    efflut = fn.rescaleData(ind,
                                            scale=255. / levdiff,
                                            offset=minlev,
                                            dtype=np.ubyte)
                else:
                    lutdtype = np.min_scalar_type(lut.shape[0] - 1)
                    efflut = fn.rescaleData(ind,
                                            scale=(lut.shape[0] - 1) / levdiff,
                                            offset=minlev,
                                            dtype=lutdtype,
                                            clip=(0, lut.shape[0] - 1))
                    efflut = lut[efflut]

                self._effectiveLut = efflut
            lut = self._effectiveLut
            levels = None

        # Assume images are in column-major order for backward compatibility
        # (most images are in row-major order)

        if self.axisOrder == 'col-major':
            image = image.transpose((1, 0, 2)[:image.ndim])

        argb, alpha = fn.makeARGB(image, lut=lut, levels=levels)
        self.qimage = fn.makeQImage(argb, alpha, transpose=False)
