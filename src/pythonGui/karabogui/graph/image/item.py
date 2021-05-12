import numpy as np
from pyqtgraph import GraphicsObject, Point, functions as fn, getConfigOption
from qtpy.QtCore import QPointF, QRectF, Qt, Signal, Slot
from qtpy.QtGui import QColor, QImage, QTransform, qRgb
from scipy.ndimage import zoom

from karabogui.graph.common.api import MouseMode

from .utils import bytescale, map_rect_to_transform, rescale

DIMENSION_DOWNSAMPLE = [(500, 1.5), (1000, 2)]  # [(dimension, min downsample)]
NULL_COLOR = QColor(255, 255, 255, 70)


class KaraboImageItem(GraphicsObject):
    clicked = Signal(float, float)
    hovered = Signal(object, object)

    sigImageChanged = Signal()

    def __init__(self, image=np.zeros((50, 50), dtype=np.uint8), parent=None):
        GraphicsObject.__init__(self)
        self.auto_levels = True

        self._origin = np.array([0, 0])
        self._rect = None
        self.autoDownsample = True

        self.downsample_order = 0  # nearest-neighbors downsampling
        self._qlut = None
        self._lastDownsample = None
        self._slice_rect = None
        self._view_rect = None
        self.lut = None
        self.image = None
        self.qimage = None

        self.levels = None
        self.axisOrder = getConfigOption('imageAxisOrder')
        self.setImage(image)

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
        stride = lut.shape[0] // 256
        self._qlut = [qRgb(*v) for v in lut[::stride, :]]
        if update:
            self.updateImage()

    def setOpts(self, update=True, **kwargs):
        if 'axisOrder' in kwargs:
            value = kwargs['axisOrder']
            if value not in ('row-major', 'col-major'):
                raise ValueError('axisOrder must be either "row-major" '
                                 'or "col-major"')
            self.axisOrder = value
        if 'lut' in kwargs:
            self.set_lookup_table(kwargs['lut'], update=update)
        if 'levels' in kwargs:
            self.setLevels(kwargs['levels'], update=update)
        if update:
            self.update()

    def updateImage(self, **kwargs):
        """Update the Image"""
        defaults = {
            'autoLevels': False,
        }
        defaults.update(kwargs)
        return self.setImage(**defaults)

    def width(self):
        if self.image is None:
            return None
        axis = 0 if self.axisOrder == 'col-major' else 1
        return self.image.shape[axis]

    def height(self):
        if self.image is None:
            return None
        axis = 1 if self.axisOrder == 'col-major' else 0
        return self.image.shape[axis]

    def boundingRect(self):
        if self.image is None:
            return QRectF(0, 0, 0, 0)
        return QRectF(0, 0, self.width(), self.height())

    def get_color(self, x, y):
        """Get color of selected pixel. Return NULL color if there's
           no qimage or the selected pixel is out of view."""
        qimage = self._get_qimage()
        color = NULL_COLOR
        if qimage is None:
            return color

        # Map coords to qimage coords
        diff = self._slice_rect.topLeft()
        x -= int(diff.x())
        y -= int(diff.y())

        # Account downsampling (shrunk qimage)
        if self._lastDownsample is not None:
            xds, yds = self._lastDownsample
            if xds != 1:
                x = np.ceil(x / xds)
            if yds != 1:
                y = np.ceil(y / yds)

        # Check if calculated coord is inside the qimage
        if qimage.rect().contains(x, y):
            color = QColor(qimage.pixel(x, y))

        return color

    @Slot()
    def reset_downsampling(self, update=True):
        self._lastDownsample = None
        if update:
            self.updateImage()

    # ---------------------------------------------------------------------
    # Events

    def mouseClickEvent(self, event):
        if (self.getViewBox().mouse_mode is MouseMode.Picker
                and event.button() == Qt.LeftButton):
            image_pos, view_pos = self._get_mouse_positions(event.pos())
            self.clicked.emit(image_pos.x(), image_pos.y())

    def hoverEvent(self, event):
        if self.getViewBox().mouse_mode is MouseMode.Picker:
            self._hover_pointer_mode(event)

    def mouseDragEvent(self, event):
        if event.button() != Qt.LeftButton:
            event.ignore()
            return

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

    def set_qimage(self, image=None, autoLevels=None, **kwargs):
        """Update the image displayed by this item."""
        new_data = False
        if image is None:
            if self.image is None:
                return
        else:
            new_data = True
            shapeChanged = (self.image is None
                            or image.shape != self.image.shape)
            image = image.view(np.ndarray)
            self.image = image
            if shapeChanged:
                self.prepareGeometryChange()
                self.informViewBoundsChanged()

        if autoLevels is None:
            if 'levels' in kwargs:
                autoLevels = False
            else:
                autoLevels = True

        if autoLevels:
            image = self.image
            while image.size > 2 ** 16:
                image = image[::2, ::2]
            mn, mx = np.nanmin(image), np.nanmax(image)
            # Note: mn and mx can still be NaN if the data is all-NaN
            if mn == mx:
                # We have a valid image but same levels.
                mnn = mn - 1
                # If the new min greater equal 0 take it!
                if mnn >= 0:
                    mn = mnn
                else:
                    mx += 1
            elif np.isnan(mn) or np.isnan(mx):
                mn = 0
                mx = 255
            kwargs['levels'] = [mn, mx]

        # Apply levels here!
        self.setOpts(update=False, **kwargs)
        self.qimage = None
        self.update()

        if new_data:
            self.sigImageChanged.emit()

    def setImage(self, image=None, **kwargs):
        """There could be cases that all image pixels have the same value.
        Since pyqtgraph defaults the levels to (0, 255) for such, we
        calculate the levels for our own sanity."""

        # Check if the image is valid
        if image is None:
            return

        # Convert single-channel image to 2D array
        if image.ndim == 3 and image.shape[-1] == 1:
            image = image[..., 0]

        if image.ndim == 2 and self.auto_levels:
            # Calculate levels only on pseudocolor images
            # (single channel or 2D images)
            image_min, image_max = image.min(), image.max()
            levels = [image_min, image_max]
            self.set_qimage(image=image, levels=levels, **kwargs)
        else:
            # No levels calculation needed for pseudocolor images with preset
            # levels and for RGB images
            self.set_qimage(image=image, autoLevels=False, **kwargs)

    def render(self):
        """Reimplementing for performance improvements patches"""
        # Check if image is valid
        image = self.image
        if is_image_invalid(image):
            return

        if image.ndim == 2:
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
            low, high = (0.0, 255.0)  # default color range
            if self.levels is None:
                image_min, image_max = image.min(), image.max()
            else:
                level_min, level_max = self.levels
                image = np.clip(image, level_min, level_max)
                image_min, image_max = image.min(), image.max()
                # Calculate new color ranges with the ratio of the image
                # extrema and the preset levels.
                low, high = rescale(np.array([image_min, image_max]),
                                    min_value=level_min, max_value=level_max,
                                    low=low, high=high)

            # 4. Rescale values to 0-255 relative to the image min/max
            # for the QImage
            image = bytescale(image, cmin=image_min, cmax=image_max,
                              low=low, high=high)

            # 6. Create QImage
            qimage = self._build_qimage(image,
                                        img_format=QImage.Format_Indexed8)

            # 7. Set color table
            if self._qlut is not None:
                qimage.setColorTable(self._qlut)
            self.qimage = qimage
        else:
            if image.shape[-1] == 3:
                # Add an opaque alpha channel to be QImage compatible
                img_format = QImage.Format_RGB888
            elif image.shape[-1] == 4:
                img_format = QImage.Format_ARGB32
            else:
                raise NotImplementedError(f"Formatting for image with shape "
                                          f"{image.shape} is not supported")

            # Convert data type to uint8 if otherwise
            # Note: Convert with QImage
            if image.dtype != np.uint8:
                image = image.astype(np.uint8)

            self._slice_rect = QRectF(0, 0, *image.shape[:2])
            self.qimage = self._build_qimage(image, img_format)

    def _build_qimage(self, image, img_format):
        # Transpose image array to match axis orientation.
        if self.axisOrder == 'col-major':
            image = image.transpose((1, 0, 2)[:image.ndim]).copy()

        ny, nx = image.shape[:2]
        if img_format in (QImage.Format_Indexed8, QImage.Format_RGB888):
            stride = image.strides[0]
            return QImage(image, nx, ny, stride, img_format)

        qimage = QImage(image, nx, ny, img_format)
        if img_format == QImage.Format_ARGB32:
            # Swap the RGB values to BGR. Seems like Qt uses BGR color space
            qimage = qimage.rgbSwapped()
        return qimage

    def paint(self, p, *args):
        """Reimplementing for performance improvements patches"""
        if self.image is None:
            return
        if self.qimage is None:
            self.render()
            if self.qimage is None:
                return

        p.drawImage(self._slice_rect, self.qimage)

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

        # Get a fresh viewrect
        self._cachedView = None
        self._view_rect = self.viewRect()
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
        if levels is not None:
            levels = np.asarray(levels)
        if not fn.eq(levels, self.levels):
            self.levels = levels
            if update:
                self.updateImage()

    def viewTransformChanged(self):
        """Reimplemented to control image downsampling/slicing to view.

           When the imageItem is zoomed, we recalculate the image slice for the
           new view ranges and the downsampling of the image. If panned, we
           only consider calculating the image slice by checking if the view
           exceeds the current slice rect."""
        super().viewTransformChanged()

        self.qimage = None
        zoomed = True

        # Note: Invalidate the cachedView before getting a new `viewRect`
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

    def clear(self):
        self.image = None
        self.prepareGeometryChange()
        self.informViewBoundsChanged()
        self.update()


def is_image_invalid(image):
    return image is None or image.size == 0
