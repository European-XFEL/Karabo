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
from copy import deepcopy

import numpy as np
from pyqtgraph import ColorMap, PlotItem
from qtpy.QtCore import QRectF, Signal, Slot
from qtpy.QtGui import QFont

from karabogui.graph.common.api import (
    COLORMAPS, ArrayExporter, AspectRatio, ExportTool, ImageExporter,
    create_axis_items)
from karabogui.graph.common.const import (
    AXIS_ITEMS, AXIS_X, AXIS_Y, DEFAULT_LABEL_X, DEFAULT_LABEL_Y,
    DEFAULT_OFFSET_X, DEFAULT_OFFSET_Y, DEFAULT_SCALE_X, DEFAULT_SCALE_Y,
    DEFAULT_UNITS_X, DEFAULT_UNITS_Y, ROTATION_FACTOR, TF_FLIPPED, TF_ROTATION,
    TF_SCALING, TF_TRANSLATION)

from .item import KaraboImageItem
from .viewbox import KaraboImageViewBox


def axis_tick_font():
    font = QFont("Source Sans Pro")
    font.setPixelSize(10)
    return font


TICK_AXIS = ["top", "left"]
AXIS_ORDER = "row-major"


class KaraboImagePlot(PlotItem):
    imageTransformed = Signal()
    imageLevelsChanged = Signal(object)
    imageAxesChanged = Signal()

    def __init__(self, parent=None):
        super().__init__(
            viewBox=KaraboImageViewBox(parent=None),
            axisItems=create_axis_items(axes_with_ticks=TICK_AXIS),
            enableMenu=False,
            parent=parent)

        # Initialize widgets
        self.imageItem = KaraboImageItem(parent=self)
        # aligning the image coords to view coords
        self.imageItem.axisOrder = AXIS_ORDER

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

        self._base_transform = {}
        self._base_units = []

        # Container for ranges
        self._axes_data = [np.array([]), np.array([])]
        self.transformed_axes = None

        # Plot items
        self.printer = None

        x_label = {"units": DEFAULT_UNITS_X, "label": DEFAULT_LABEL_X}
        y_label = {"units": DEFAULT_UNITS_Y, "label": DEFAULT_LABEL_Y}
        self._labels = [x_label, y_label]

        # Set default values for the transform
        self._set_default_transform()
        self._flip()

        # Connect signals
        self.vb.sigResized.connect(self.imageItem.reset_downsampling)

    # ---------------------------------------------------------------------
    # PyQt slots

    @Slot()
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
            x_scale, y_scale = self._transform[TF_SCALING]
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

        self.imageItem.setImage(image)
        if should_update and update:
            self._restore_view()

    def set_transform(self, x_scale, y_scale, x_translate, y_translate,
                      aspect_ratio=AspectRatio.PixelDependent, default=False):
        scaling = np.array([x_scale, y_scale], dtype=np.float64)
        translation = np.array([x_translate, y_translate], dtype=np.float64)

        if default:
            self._base_transform[TF_SCALING] = scaling
            self._base_transform[TF_TRANSLATION] = translation
            self._revert_transform()
        else:
            self._transform[TF_SCALING] = scaling
            self._transform[TF_TRANSLATION] = translation

        self.aspect_ratio = aspect_ratio

        if not default:
            self._apply_transform()

    def set_translation(self, x_translate=None, y_translate=None, update=True):
        """Public method to translate the image. Used by extensions"""
        if x_translate is None and y_translate is None:
            return

        if x_translate is not None:
            self._transform[TF_TRANSLATION][0] = x_translate
        if y_translate is not None:
            self._transform[TF_TRANSLATION][1] = y_translate

        self._apply_transform(update)

    def set_colormap(self, cmap):
        """Sets the colormap of the image by setting the image item LUT.
            Also updates the information of the selected pixel.

        :param str cmap:
            Colormap name. Should be among COLORMAPS.
        """
        lut = ColorMap(*zip(*COLORMAPS[cmap])).getLookupTable(
            alpha=False)
        self.imageItem.setLookUpTable(lut)

    @Slot(object)
    def set_image_levels(self, levels):
        """Sets the image levels. Also updates the information of the
        selected pixel.

        :param object levels:
             The image levels. Can be [float, float] or None
        """
        self.imageItem.auto_levels = levels is None
        self.imageItem.setLevels(levels)
        self.imageLevelsChanged.emit(levels)

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

    def set_viewbox_menu(self, menu=None):
        enabled = menu is not None
        if menu is not None:
            self.vb.menu = menu
        self.vb.setMenuEnabled(enabled)

    def set_range(self, x_range, y_range, padding=0):
        self.vb.setXRange(*x_range, padding=padding)
        self.vb.setYRange(*y_range, padding=padding)

    def set_label(self, axis=0, text='', units=''):
        formatted_units = f" ({units})" if units else units

        show = True if (text != "" or units != "") else False

        for ax in self._axes[axis]:
            # set AxisItem labels
            self.getAxis(ax).setLabel(text + formatted_units)
            # XXX: We circumvent a PyQtGraph bug here and set the visibility
            # ourselves! Setting a label and removing it, gives PyQtGraph
            # problems.
            self.showLabel(ax, show=show)

        labels = self._labels[axis]
        labels["label"] = text
        labels["units"] = units

    @Slot(ExportTool)
    def export(self, export_type):
        """Exports the image according to the desired format"""
        if export_type == ExportTool.Image:
            exporter = ImageExporter(self.scene())
        elif export_type == ExportTool.Data:
            exporter = ArrayExporter(self.imageItem.image)
        else:
            raise LookupError(f"No exporter found for {export_type}")

        exporter.export()

    def _set_default_transform(self):
        self._base_transform[TF_SCALING] = np.array(
            [DEFAULT_SCALE_X, DEFAULT_SCALE_Y], dtype=np.float64)
        self._base_transform[TF_TRANSLATION] = np.array(
            [DEFAULT_OFFSET_X, DEFAULT_OFFSET_Y], dtype=np.float64)
        self._base_transform[TF_ROTATION] = 0
        # x-axis is not flipped, while y-axis is. This is due to the axis order
        self._base_transform[TF_FLIPPED] = [False, True]

        self._revert_transform()

    def _revert_transform(self):
        self._transform = deepcopy(self._base_transform)
        self._units = deepcopy(self._base_units)

    # ---------------------------------------------------------------------
    # Transform methods

    def _axes_changed(self, image):
        """Returns True/False if the axes information changed in relation to
        an image"""
        size_x = self._axes_data[0].size
        size_y = self._axes_data[1].size
        new_size_y, new_size_x = image.shape[:2]

        return (size_x != new_size_x) or (size_y != new_size_y)

    def _update_axes_transforms(self):
        """Updates the transformed axes"""
        translation = self._transform[TF_TRANSLATION]
        scaling = self._transform[TF_SCALING]

        self.transformed_axes = []
        for i, axis in enumerate(self._axes_data):
            self.transformed_axes.append((axis * scaling[i]) + translation[i])

        self.imageAxesChanged.emit()

    def _apply_transform(self, update=True):
        """Applies transformation on the image item such as scale, translate,
           and rotation. Other plot items such as ROIs and indicators are
           transformed as well."""
        rotation = self._transform[TF_ROTATION]
        translation = self._transform[TF_TRANSLATION]
        scaling = self._transform[TF_SCALING]

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
        translation = self._transform[TF_TRANSLATION]
        scaling = self._transform[TF_SCALING]

        pos = np.array([rect.x(), rect.y()])
        abs_pos = ((pos - translation) / scaling).astype(int)

        size = np.array([rect.width(), rect.height()])
        abs_size = np.round(size / scaling)

        return QRectF(*(tuple(abs_pos) + tuple(abs_size)))

    def mapRectToTransform(self, rect):
        translation = self._transform[TF_TRANSLATION]
        scaling = self._transform[TF_SCALING]

        pos = np.array([rect.x(), rect.y()])
        trans_pos = pos * scaling + translation

        size = np.array([rect.width(), rect.height()])
        trans_size = size * scaling

        return QRectF(*(tuple(trans_pos) + tuple(trans_size)))

    def _flip(self):
        """Flips the image by inverting the viewbox
           and adjusting the shown axes"""
        flipped_x = self._transform[TF_FLIPPED][0]
        self.vb.invertX(flipped_x)

        flipped_y = self._transform[TF_FLIPPED][1]
        self.vb.invertY(flipped_y)

        major_x = "right" if flipped_x else "left"
        major_y = "top" if flipped_y else "bottom"
        major_axes = [major_x, major_y]

        for axis in AXIS_ITEMS:
            axis_item = self.getAxis(axis)
            show_values = axis in major_axes
            axis_item.setStyle(**{"showValues": show_values})

    # ---------------------------------------------------------------------
    # Reimplemented `PyQtGraph` methods

    def setMenuEnabled(self, enableMenu=True, enableViewBoxMenu=None):
        """Reimplemented function to circumvent behavior of pyqtgraph

        Note: This patch is not required in pyqtgraph > 0.11.1
        """
        self._menuEnabled = enableMenu
