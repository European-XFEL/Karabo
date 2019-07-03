import numpy as np
from PyQt4.QtCore import pyqtSignal, pyqtSlot, QPointF, Qt
from PyQt4.QtGui import QImage, qRgb, QTransform
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
        self.autoDownsample = False

        self.downsample_order = 0
        self._downsampling_enabled = False
        self._qlut = None
        self._lastDownsample = None

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

    def enable_downsampling(self, enable):
        self._downsampling_enabled = enable
        self.autoDownsample = enable

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
        """Reimplementing for performance improvements patches"""

        # 0. Check if image is valid
        image = self.image
        if image is None or image.size == 0:
            return

        # 1. Downsample image according to the image geometry ratio
        #  and interpolation order
        if self._downsampling_enabled:
            image = self._downsample_image(image)

            # Check if downsampling returns an image
            if image is None:
                return

        # 2. Clip values according to levels
        levels = self.levels
        if levels is not None and levels.ndim == 1:
            image = np.clip(image, levels[0], levels[1])

        # 3. Rescale values to 0-255 for the QImage
        image = rescale(image, low=0, high=255)

        # 4. Transpose image array to match axis orientation
        if self.axisOrder == 'col-major':
            image = image.transpose((1, 0, 2)[:image.ndim])

        # 5. Create QImage
        ny, nx = image.shape[:2]
        stride = image.strides[0]
        self.qimage = QImage(image, nx, ny, stride, QImage.Format_Indexed8)

        # 6. Set color table
        if self._qlut is not None:
            self.qimage.setColorTable(self._qlut)

    def _downsample_image(self, image):
        if self._lastDownsample is None:
            # Calculate downsample image
            self._lastDownsample = self._calculate_downsample_scale(image)

            # If still None, do nothing
            if self._lastDownsample is None:
                return

        xds, yds = self._lastDownsample

        # Scale only if downsampling of (one of) the axis is greater than 1
        if xds > 1 or yds > 1:
            scale = [1 / yds, 1 / xds]
            image = zoom(image, scale, order=self.downsample_order)

        return image

    def _calculate_downsample_scale(self, image):
        # Reduce dimensions of image based on screen resolution
        o = self.mapToDevice(QPointF(0, 0))
        x = self.mapToDevice(QPointF(1, 0))
        y = self.mapToDevice(QPointF(0, 1))
        w = Point(x - o).length()
        h = Point(y - o).length()
        if w == 0 or h == 0:
            self.qimage = None
            return

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
        xds, yds = (1, 1)
        x_scale = round(1.0 / w * 100) / 100
        y_scale = round(1.0 / h * 100) / 100

        # Get minimum downsampling wrt image dimensions
        x_min_ds, y_min_ds = (1, 1)
        for dim, min_ds in DIMENSION_DOWNSAMPLE:
            if image_x > dim:
                x_min_ds = min_ds
            if image_y > dim:
                y_min_ds = min_ds

        # Scale the image if autodownsampling is specified or (one of) the axis
        # has a bigger scale than the minimum
        if x_scale >= x_min_ds or y_scale >= x_min_ds or self.autoDownsample:
            # Correct downsampling wrt if less than prescribed minimum
            xds = x_scale if x_scale > x_min_ds else x_min_ds
            yds = y_scale if y_scale > y_min_ds else y_min_ds

        return xds, yds

    def setLevels(self, levels, update=True):
        """Reimplemented function for version conflict"""
        if levels is not None:
            levels = np.asarray(levels)
        if not fn.eq(levels, self.levels):
            self.levels = levels
            self._effectiveLut = None
            if update:
                self.updateImage()

    def setLookupTable(self, lut, update=True):
        """
        Set the lookup table (numpy array) to use for this image. (see
        :func:`makeARGB <pyqtgraph.makeARGB>` for more information on how this
        is used).
        Optionally, lut can be a callable that accepts the current image as an
        argument and returns the lookup table to use.

        Ordinarily, this table is supplied by a :class:`HistogramLUTItem
        <pyqtgraph.HistogramLUTItem>`
        or :class:`GradientEditorItem <pyqtgraph.GradientEditorItem>`.
        """
        if lut is not self.lut:
            self.lut = lut
            self._effectiveLut = None
            stride = self.lut.shape[0] // 256
            self._qlut = [qRgb(*v) for v in lut[::stride, :]]

            if update:
                self.updateImage()

    def viewTransformChanged(self):
        """Reimplemented because we do not want to recalculate downsample"""

    def informViewBoundsChanged(self):
        """Reimplemented because we want to catch image shape changes"""
        super(KaraboImageItem, self).informViewBoundsChanged()
        self.reset_downsampling_scale(update=False)

    def set_downsample_order(self, order):
        self.downsample_order = order
        self.reset_downsampling_scale()

    @pyqtSlot()
    def reset_downsampling_scale(self, update=True):
        self._lastDownsample = None
        if update:
            self.updateImage()


def rescale(image, low=0.0, high=100.0):
    min_value, max_value = np.min(image), np.max(image)
    value_range = max_value - min_value

    if value_range == 0:
        return image

    rescaled = high - ((high - low) * ((max_value - image) / value_range))

    return rescaled.astype(np.uint8)
