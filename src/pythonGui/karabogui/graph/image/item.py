import numpy as np
from PyQt5.QtCore import pyqtSignal, pyqtSlot, QPointF, QRectF, Qt
from PyQt5.QtGui import QColor, QImage, qRgb, QTransform
from pyqtgraph import functions as fn, ImageItem, Point
from scipy.ndimage import zoom

from karabogui.graph.common.api import MouseMode
from .utils import map_rect_to_transform


DIMENSION_DOWNSAMPLE = [(500, 1.5), (1000, 2)]  # [(dimension, min downsample)]
NULL_COLOR = QColor(255, 255, 255, 70)


class KaraboImageItem(ImageItem):
    clicked = pyqtSignal(float, float)
    hovered = pyqtSignal(object, object)

    def __init__(self, image=np.zeros((50, 50), dtype=int)):
        super(KaraboImageItem, self).__init__(image)

        self._origin = np.array([0, 0])
        self.auto_levels = True
        self._rect = None
        self.autoDownsample = True

        self.downsample_order = 0  # nearest-neighbors downsampling
        self._qlut = None
        self._lastDownsample = None
        self._slice_rect = None
        self._view_rect = None

    # ---------------------------------------------------------------------
    # Public methods

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

    def set_lookup_table(self, lut, update=True):
        self.lut = lut
        self._effectiveLut = None
        stride = self.lut.shape[0] // 256
        self._qlut = [qRgb(*v) for v in lut[::stride, :]]

        if update:
            self.updateImage()

    def get_color(self, x, y):
        """Get color of selected pixel. Return NULL color if there's
           no qimage or the selected pixel is out of view."""
        qimage = self._get_qimage()
        color = NULL_COLOR

        if qimage is None or self._lastDownsample is None:
            return color

        # Map coords to qimage coords
        diff = self._slice_rect.topLeft()
        x -= int(diff.x())
        y -= int(diff.y())

        # Account downsampling (shrunk qimage)
        xds, yds = self._lastDownsample
        if xds != 1:
            x = np.ceil(x / xds)
        if yds != 1:
            y = np.ceil(y / yds)

        # Check if calculated coord is inside the qimage
        if qimage.rect().contains(x, y):
            color = QColor(qimage.pixel(x, y))

        return color

    @pyqtSlot()
    def reset_downsampling(self, update=True):
        self._lastDownsample = None
        if update:
            self.updateImage()

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

    def _get_qimage(self):
        if self.qimage is None:
            self.render()
        return self.qimage

    # ---------------------------------------------------------------------
    # Render patches

    def render(self):
        """Reimplementing for performance improvements patches"""

        # 0. Check if image is valid
        image = self.image
        if is_image_invalid(image):
            return

        # 1. Get image according to view geometry
        image = self._slice(image)
        if is_image_invalid(image):
            return

        # 2. Downsample image according to the image geometry ratio
        #  and interpolation order
        if self.autoDownsample:
            image = self._downsample(image)
            if is_image_invalid(image):
                return

        # 3. Clip values according to levels
        low, high = (0, 255)  # default color range
        if self.levels is not None and self.levels.ndim == 1:
            level_min, level_max = self.levels
            image = np.clip(image, level_min, level_max)

            low, high = (0, 0)  # default to 0 in case levels are zero
            if level_min != 0:
                low = abs((image.min() - level_min) / level_min) * 255
            if level_max != 255:
                high = abs(image.max() / level_max) * 255

        # 4. Rescale values to 0-255 relative to the image min/max
        # for the QImage
        image = rescale(image, low, high)

        # 5. Transpose image array to match axis orientation. There is a need
        # to copy since QImage need the array copy (pointers), not the view.
        if self.axisOrder == 'col-major':
            image = image.transpose((1, 0, 2)[:image.ndim]).copy()

        # 6. Create QImage
        ny, nx = image.shape[:2]
        stride = image.strides[0]
        self.qimage = QImage(image, nx, ny, stride, QImage.Format_Indexed8)

        # 7. Set color table
        if self._qlut is not None:
            self.qimage.setColorTable(self._qlut)

    def paint(self, p, *args):
        """Reimplementing for performance improvements patches"""
        if self.image is None:
            return
        if self.qimage is None:
            self.render()
            if self.qimage is None:
                return
        if self.paintMode is not None:
            p.setCompositionMode(self.paintMode)

        p.drawImage(self._slice_rect, self.qimage)
        if self.border is not None:
            p.setPen(self.border)
            p.drawRect(self.boundingRect())

    def _slice(self, image):
        """Slice the image to be rendered wrt to the view. There's no need to
           render everything, especially if the selected view is precise and
           only shows a small part of the image. This saves us computation
           time."""

        # 0. Slice image with existing geometry
        if self._slice_rect is not None:
            x1, y1, x2, y2 = [int(c) for c in self._slice_rect.getCoords()]
            return image[y1:y2, x1:x2]

        # Start calculating the view geometry
        # 1. Get image shape wrt to axis order
        shape = image.shape[:2]
        if self.axisOrder == 'row-major':
            shape = shape[::-1]

        # 2. Check if coords is in the view rect.
        # If not, get new image geometry from view coords
        x1, y1 = (0, 0)
        width, height = shape
        if not self._view_rect.contains(QRectF(x1, y1, width, height)):
            # Calculate image from view geometry
            x1, y1, x2, y2 = self._view_rect.getCoords()
            new_width, new_height = x2 - x1, y2 - y1
            x1 = max(int(x1 - np.ceil(new_width * 0.1)), 0)
            y1 = max(int(y1 - np.ceil(new_height * 0.1)), 0)
            x2 = min(int(x2 + np.ceil(new_width * 0.1)), width)
            y2 = min(int(y2 + np.ceil(new_height * 0.1)), height)
            width, height = x2 - x1, y2 - y1

            # Slice image according to view geometry
            image = image[y1:y2, x1:x2]

        self._slice_rect = QRectF(x1, y1, width, height)
        return image

    def _downsample(self, image):
        """Downsample the image with respect to its size and the view size
           (by calculating the pixel ratio). If the image size is larger than
           the view size, where one couldn't see the fine details, we
           downsample heavily. On the other hand, when the image size is
           smaller than the view, where one could see the pixels, we limit the
           downsampling until it's not anymore."""

        # Get downsample factor
        if self._lastDownsample is None:
            # Calculate downsample image
            self._lastDownsample = self._calculate_downsample_factor(
                x_dim=image.shape[1], y_dim=image.shape[0])

            # If still None, do nothing
            if self._lastDownsample is None:
                return
        xds, yds = self._lastDownsample

        # Scale only if downsampling of (one of) the axis is greater than 1
        if xds > 1 or yds > 1:
            scale = [1 / yds, 1 / xds]
            image = zoom(image, scale, order=self.downsample_order)

        return image

    def _calculate_downsample_factor(self, x_dim, y_dim):
        """Calculate downsampling factor based of the ratio of the screen and
           the image geometry"""

        # Reduce dimensions of image based on screen resolution
        o = self.mapToDevice(QPointF(0, 0))
        x = self.mapToDevice(QPointF(1, 0))
        y = self.mapToDevice(QPointF(0, 1))
        w = Point(x - o).length()
        h = Point(y - o).length()
        if w == 0 or h == 0:
            return

        # Calculate scaling factor for each axis.
        # - if dimension >= 1000, min scale is 2
        # - if dimension in [500, 1000), min scale is 2
        # - if dimension is smaller than 500, no scaling is done
        x_scale = np.floor(1.0 / w * 100) / 100
        y_scale = np.floor(1.0 / h * 100) / 100

        # Get minimum downsampling wrt image dimensions
        x_min_ds, y_min_ds = (1, 1)
        for dim, min_ds in DIMENSION_DOWNSAMPLE:
            if x_dim > dim:
                x_min_ds = min_ds
            if y_dim > dim:
                y_min_ds = min_ds

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

    def viewTransformChanged(self):
        """Reimplemented to control image downsampling/slicing to view.

           When the imageItem is zoomed, we recalculate the image slice for the
           new view ranges and the downsampling of the image. If panned, we
           only consider calculating the image slice by checking if the view
           exceeds the current slice rect."""
        self.qimage = None
        zoomed = True
        view_rect = self.viewRect()

        # Check if image is zoomed. Checking with width is already sufficient.
        if self._view_rect is not None:
            diff = abs(view_rect.width() - self._view_rect.width())
            zoomed = diff > 0.01 * view_rect.width()
        self._view_rect = view_rect

        if zoomed:
            self._lastDownsample = None
            self._slice_rect = None
        elif self._slice_rect is not None:
            # Image is just panned.
            # Check if there is a need to get a new slice of QImage
            view_x1, view_y1, view_x2, view_y2 = \
                view_rect.getCoords()
            slice_x1, slice_y1, slice_x2, slice_y2 = \
                self._slice_rect.getCoords()

            # Get current image dimensions
            # (via boundingRect to consider axis major)
            im_x1, im_y1, im_x2, im_y2 = self.boundingRect().getCoords()

            # Get to-be-rendered image dimensions (wrt view)
            im_x1 = max(view_x1, im_x1)
            im_y1 = max(view_y1, im_y1)
            im_x2 = min(view_x2, im_x2)
            im_y2 = min(view_y2, im_y2)

            # Check if new required image is out of bounds of the slice
            x_out = im_x1 < slice_x1 or im_y1 < slice_y1
            y_out = im_x2 > slice_x2 or im_y2 > slice_y2
            out_of_bounds = x_out or y_out

            # Reset slice if out of bounds,
            # Else do nothing, sliced QImage is still sufficient for the view
            if out_of_bounds:
                self._slice_rect = None
            else:
                return

        self.updateImage()

    def informViewBoundsChanged(self):
        super(KaraboImageItem, self).informViewBoundsChanged()
        self.reset_downsampling(update=False)
        self._slice_rect = None


def rescale(image, low=0.0, high=100.0):
    min_value, max_value = np.min(image), np.max(image)
    value_range = np.subtract(max_value, min_value)

    if value_range == 0:
        return image

    rescaled = high - ((high - low) * ((max_value - image) / value_range))

    return rescaled.astype(np.uint8)


def is_image_invalid(image):
    return image is None or image.size == 0
