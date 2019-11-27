from copy import deepcopy

import numpy as np
from PyQt5.QtCore import pyqtSignal, pyqtSlot, QRectF
from PyQt5.QtGui import QFont
from pyqtgraph import ColorMap, PlotItem

from karabogui.graph.common.api import (
    ArrayExporter, AspectRatio, COLORMAPS, ExportTool, create_axis_items,
    ImageExporter)
from karabogui.graph.common.const import (
    AXIS_ITEMS, AXIS_X, AXIS_Y, DEFAULT_LABEL_X, DEFAULT_LABEL_Y,
    DEFAULT_OFFSET_X, DEFAULT_OFFSET_Y, DEFAULT_SCALE_X, DEFAULT_SCALE_Y,
    DEFAULT_UNITS_X, DEFAULT_UNITS_Y, IS_FLIPPED, LABEL, ROTATION,
    ROTATION_FACTOR, SCALING, TRANSLATION, UNITS)

from .item import KaraboImageItem
from .viewbox import KaraboImageViewBox


def axis_tick_font():
    font = QFont()
    font.setPixelSize(11)
    return font


class KaraboImagePlot(PlotItem):
    imageTransformed = pyqtSignal()
    imageLevelsChanged = pyqtSignal()
    imageAxesChanged = pyqtSignal()

    AXIS_ORDER = "row-major"  # aligning the image coords to view coords
    DEFAULT_ROTATION = 0  # rotated at 0 deg.. which is not rotated at all

    # x-axis is not flipped, while y-axis is. This is due to the axis order
    IS_FLIPPED = np.array([False, True])
    MAJOR_AXES = ["top", "left"]

    def __init__(self, parent=None):
        super(KaraboImagePlot, self).__init__(
            viewBox=KaraboImageViewBox(),
            axisItems=create_axis_items(axes_with_ticks=self.MAJOR_AXES),
            parent=parent)

        # Initialize widgets
        self.imageItem = KaraboImageItem()
        self.imageItem.axisOrder = KaraboImagePlot.AXIS_ORDER
        self.addItem(self.imageItem)

        # Improve axes rendering
        for axis in AXIS_ITEMS:
            axis_item = self.getAxis(axis)
            axis_item.setZValue(0)

        self.show_all_axis()

        # Initialize values
        self._axes = [AXIS_X, AXIS_Y]
        self._image_set = False
        self._transform = {}
        self._units = []
        self.aspect_ratio = AspectRatio.PixelDependent

        self._original_transform = {}
        self._original_units = []

        self._axes_data = [np.array([]), np.array([])]
        self.transformed_axes = None

        # Plot items
        self.printer = None

        x_label = {UNITS: DEFAULT_UNITS_X, LABEL: DEFAULT_LABEL_X}
        y_label = {UNITS: DEFAULT_UNITS_Y, LABEL: DEFAULT_LABEL_Y}
        self._labels = [x_label, y_label]

        # Setup plot item
        self.setMinimumWidth(200)
        self.setMinimumHeight(200)
        self._set_default_transform()
        self._flip()

        # Connect signals
        self.vb.sigResized.connect(self.imageItem.reset_downsampling)

    # ---------------------------------------------------------------------
    # PyQt slots

    @pyqtSlot()
    def _restore_view(self):
        """Restores the image item to the original transform, which is
        usually no scaling, translation, and rotation.
        """
        # Restore original transform
        self._revert_transform()

        # Only update the image after all operation
        self._apply_transform(update=False)

        # Flip axes if needed
        self._flip()

        self.vb.enableAutoRange()

    # ---------------------------------------------------------------------
    # Properties

    @property
    def axes_transform(self):
        return self._transform

    @property
    def axes_labels(self):
        return deepcopy(self._labels)

    @property
    def image(self):
        return self.imageItem.image

    @property
    def image_set(self):
        return self._image_set

    # ---------------------------------------------------------------------
    # Public methods

    def set_aspect_ratio(self, ratio=AspectRatio.NoAspectRatio):
        self.aspect_ratio = ratio
        if self.aspect_ratio == AspectRatio.NoAspectRatio:
            self.vb.setAspectLocked(False)
        elif self.aspect_ratio == AspectRatio.PixelDependent:
            x_scale, y_scale = self._transform[SCALING]
            self.vb.setAspectLocked(True, ratio=y_scale / x_scale)
        elif self.aspect_ratio == AspectRatio.ScaleDependent:
            self.vb.setAspectLocked(True, ratio=1)

    def setData(self, data, update=True):
        """Convenience wrapper for a generic interface with plot widgets"""
        self.set_image(data, update)

    def set_image(self, image, update=True):
        if image is None or image.size == 0:
            return

        if not self._image_set:
            self._image_set = True

        should_update = False
        if self._axes_changed(image):
            # Axes are different, need to remap them
            self._axes_data = [np.arange(image.shape[1]),
                               np.arange(image.shape[0])]
            self._update_axes_transforms()
            should_update = True

        self.imageItem.setImage(image, autoLevels=self.imageItem.auto_levels)
        if should_update and update:
            self._restore_view()

    def set_transform(self, x_scale, y_scale, x_translate, y_translate,
                      aspect_ratio=AspectRatio.PixelDependent, default=False):
        scaling = np.array([x_scale, y_scale])
        translation = np.array([x_translate, y_translate])

        if default:
            self._original_transform[SCALING] = scaling
            self._original_transform[TRANSLATION] = translation
            self._revert_transform()
        else:
            self._transform[SCALING] = scaling
            self._transform[TRANSLATION] = translation

        self.aspect_ratio = aspect_ratio

        if not default:
            self._apply_transform()

    def set_translation(self, x_translate=None, y_translate=None, update=True):
        if x_translate is None and y_translate is None:
            return

        if x_translate is not None:
            self._transform[TRANSLATION][0] = x_translate
        if y_translate is not None:
            self._transform[TRANSLATION][1] = y_translate

        self._apply_transform(update)

    def set_colormap(self, cmap):
        """Sets the colormap of the image by setting the image item LUT.
            Also updates the information of the selected pixel.

        :param str cmap:
            Colormap name. Should be among COLORMAPS.
        """
        lut = (ColorMap(*zip(*COLORMAPS[cmap]), mode="RGB")
               .getLookupTable(alpha=False, mode="RGB"))
        self.imageItem.set_lookup_table(lut)

    @pyqtSlot(object)
    def set_image_levels(self, levels):
        """Sets the image levels. Also updates the information of the
        selected pixel.

        :param object levels:
             The image levels. Can be [float, float] or None
        """
        self.imageItem.auto_levels = levels is None
        self.imageItem.setLevels(levels)
        self.imageLevelsChanged.emit()

    def hide_all_axis(self):
        for v in AXIS_ITEMS:
            self.hideAxis(v)

    def show_all_axis(self):
        for axis in AXIS_ITEMS:
            self.showAxis(axis)

    def rotate_axis(self, theta):
        raise NotImplementedError()

    def flip_axis(self, axis):
        raise NotImplementedError()

    def set_context_menu(self, menu):
        self.setMenuEnabled(menu is not None)

    def set_range(self, x_range, y_range, padding=0):
        self.vb.setXRange(*x_range, padding=padding)
        self.vb.setYRange(*y_range, padding=padding)

    def set_label(self, axis=0, text='', units=''):
        formatted_units = " ({})".format(units) if units else units

        # set AxisItem labels
        for ax in self._axes[axis]:
            self.getAxis(ax).setLabel(text + formatted_units)

        labels = self._labels[axis]
        labels[LABEL] = text
        labels[UNITS] = units

    @pyqtSlot(ExportTool)
    def export(self, export_type):
        """Exports the image according to the desired format"""
        if export_type == ExportTool.Image:
            exporter = ImageExporter(self.scene())
        elif export_type == ExportTool.Data:
            exporter = ArrayExporter(self.imageItem.image)
        else:
            raise LookupError("No exporter found for {}".format(export_type))

        exporter.export()

    def _set_default_transform(self):
        self._original_transform[SCALING] = np.array([DEFAULT_SCALE_X,
                                                      DEFAULT_SCALE_Y])
        self._original_transform[TRANSLATION] = np.array([DEFAULT_OFFSET_X,
                                                          DEFAULT_OFFSET_Y])
        self._original_transform[ROTATION] = KaraboImagePlot.DEFAULT_ROTATION
        self._original_transform[IS_FLIPPED] = KaraboImagePlot.IS_FLIPPED

        self._revert_transform()

    def _revert_transform(self):
        self._transform = deepcopy(self._original_transform)
        self._units = deepcopy(self._original_units)

    # ---------------------------------------------------------------------
    # Transform methods

    def _axes_changed(self, image=None):
        """Returns True/False if the axes information changed in relation to
        an image, which is either a new or current one"""
        if image is None:
            # Use current
            image = self.imageItem.image

        x_current_size = self._axes_data[0].size
        y_current_size = self._axes_data[1].size

        y_new_size, x_new_size = image.shape[:2]

        return x_current_size != x_new_size or y_current_size != y_new_size

    def _update_axes_transforms(self):
        """Updates the transformed axes"""
        translation = self._transform[TRANSLATION]
        scaling = self._transform[SCALING]

        self.transformed_axes = []
        for i, axis in enumerate(self._axes_data):
            self.transformed_axes.append((axis * scaling[i]) + translation[i])

        self.imageAxesChanged.emit()

    def _apply_transform(self, update=True):
        """Applies transformation on the image item such as scale, translate,
           and rotation. Other plot items such as ROIs and indicators are
           transformed as well."""
        rotation = self._transform[ROTATION]
        translation = self._transform[TRANSLATION]
        scaling = self._transform[SCALING]

        self.imageItem.set_transform(
            scaling=scaling * ROTATION_FACTOR[rotation],
            translation=translation / scaling,
            rotation=rotation)

        self._update_axes_transforms()

        # calculate new image aspect ratio
        if self.aspect_ratio == AspectRatio.NoAspectRatio:
            self.vb.setAspectLocked(False)
        elif self.aspect_ratio == AspectRatio.PixelDependent:
            x_scale, y_scale = scaling
            self.vb.setAspectLocked(True, ratio=y_scale / x_scale)
        elif self.aspect_ratio == AspectRatio.ScaleDependent:
            self.vb.setAspectLocked(True, ratio=1)

        self.imageTransformed.emit()
        if update:
            self.vb.enableAutoRange()

    def mapRectFromTransform(self, rect):
        translation = self._transform[TRANSLATION]
        scaling = self._transform[SCALING]

        pos = np.array([rect.x(), rect.y()])
        abs_pos = ((pos - translation) / scaling).astype(int)

        size = np.array([rect.width(), rect.height()])
        abs_size = np.round(size / scaling)

        return QRectF(*(tuple(abs_pos) + tuple(abs_size)))

    def mapRectToTransform(self, rect):
        translation = self._transform[TRANSLATION]
        scaling = self._transform[SCALING]

        pos = np.array([rect.x(), rect.y()])
        trans_pos = pos * scaling + translation

        size = np.array([rect.width(), rect.height()])
        trans_size = size * scaling

        return QRectF(*(tuple(trans_pos) + tuple(trans_size)))

    def _flip(self):
        """Flips the image by inverting the viewbox
           and adjusting the shown axes"""
        flipped_x = self._transform[IS_FLIPPED][0]
        self.vb.invertX(flipped_x)

        flipped_y = self._transform[IS_FLIPPED][1]
        self.vb.invertY(flipped_y)

        major_x = "right" if flipped_x else "left"
        major_y = "top" if flipped_y else "bottom"
        major_axes = [major_x, major_y]

        for axis in AXIS_ITEMS:
            axis_item = self.getAxis(axis)
            axis_item.has_ticks = axis in major_axes
