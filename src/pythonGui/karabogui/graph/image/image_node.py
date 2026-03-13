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
import numpy as np
from traits.api import ArrayOrNone, Enum, HasStrictTraits, Int, Set

from karabo.native import Encoding
from karabogui.controllers.arrays import (
    DIMENSIONS, get_dimensions_and_encoding, get_image_data, get_jpeg_data)
from karabogui.graph.common.api import Axes
from karabogui.graph.image.enums import ColorMode
from karabogui.graph.image.utils import karabo_invalid_image

# Special image dimensions
YUV422 = 2
YUV444 = 3
YUV_TYPES = {Encoding.YUV444, Encoding.YUV422_YUYV,
             Encoding.YUV422_UYVY}
GRAY_ENCODINGS = {Encoding.GRAY}
STANDARD_ENCODINGS = {Encoding.RGB, *GRAY_ENCODINGS, *YUV_TYPES}
EXTENDED_COLOR_ENCODINGS = {
    Encoding.RGBA,
    Encoding.BGR,
    Encoding.BGRA,
    Encoding.BAYER_RG,
    Encoding.BAYER_BG,
    Encoding.BAYER_GR,
    Encoding.BAYER_GB,
    *YUV_TYPES,
    }

invalid_image = karabo_invalid_image()


class KaraboImageNode(HasStrictTraits):
    dim_x = Int
    dim_y = Int
    dim_z = Int
    encoding = Enum(*Encoding)
    # color_mode defines the supported encodings by the controller.
    color_mode = Enum(*ColorMode)

    # Internal traits
    _data = ArrayOrNone
    _supported_encodings = Set(GRAY_ENCODINGS)

    def set_color_mode(self, mode):
        self.color_mode = mode

    def set_value(self, image_node):
        """
        Extract the dimensions, encoding, and data from image proxy value and
        process the data according to the color_mode supported by the
        controller.

        Parameters:
            image_node: The image proxy value, from the device.
        """

        dim_x, dim_y, dim_z, encoding = get_dimensions_and_encoding(image_node)

        self.dim_x = dim_x if dim_x is not None else 0
        self.dim_y = dim_y if dim_y is not None else 0
        self.dim_z = dim_z if dim_z is not None else 0

        if encoding == Encoding.GRAY:
            self._data = get_image_data(
                image_node, self.dim_x, self.dim_y, self.dim_z)
            self.encoding = encoding
            return

        if encoding == Encoding.JPEG:
            self._data = get_jpeg_data(image_node, self.dim_z)
            self.encoding = encoding
            return

        data = get_image_data(
            image_node, self.dim_x, self.dim_y, self.dim_z)
        if self.color_mode is ColorMode.STANDARD:
            # For the 'STANDARD_COLOR' mode, let's display only the luma (Y')
            # component from the image. For 'GRAY_ONLY' mode, YUV encodings
            # are not supported. For 'EXTENDED_COLOR_ONLY' we need full data
            # so that it can be converted.
            if encoding == Encoding.YUV422_UYVY:
                # input image is (u1, y1, v1, y2, u2, y3, v2, y4, ...)
                data = data[:, :, 1]
                self.dim_z = 0
            elif encoding in (Encoding.YUV422_YUYV, Encoding.YUV444):
                # input image is (y1, u1, y2, v1,  y3, u2, y4, v2, ...) or
                # (y1, u1, v1,  y2, u2, v2, ...)
                data = data[:, :, 0]
                self.dim_z = 0

        self._data = data
        self.encoding = encoding

    def get_data(self, stack_axis=None, cell=None):
        """"""
        # Get data depending on the image schema and encoding

        if self._data is None:
            return np.array([])
        if not self.is_valid:
            return invalid_image
        if cell is None:
            cell = 0
        if stack_axis is not None:
            return self._get_slice(stack_axis, cell)
        if self.encoding == Encoding.GRAY:
            return self._get_slice(DIMENSIONS['Z'], cell)
        return self._data

    def get_axis_dimension(self, axis):
        """Return the correct dimension from a given axis"""
        if axis == Axes.Y:
            return self.dim_y
        if axis == Axes.X:
            return self.dim_x
        if axis == Axes.Z:
            return self.dim_z
        else:
            raise ValueError(f'Invalid axis value: {axis}')

    def _get_slice(self, axis, cell):
        """Returns the cell slice from the given axis"""
        if not self.dim_z:
            return self._data
        # Numpy arrays are received as x, y, z
        if axis == Axes.Y:
            return self._data[cell, :, :]
        if axis == Axes.X:
            return self._data[:, cell, :]
        if axis == Axes.Z:
            return self._data[:, :, cell]

    @property
    def is_valid(self):
        if self.dim_x < 1 or self.dim_y < 1:
            return False
        return self.encoding in self._supported_encodings

    def _color_mode_changed(self, mode):
        if mode == ColorMode.GRAY:
            self._supported_encodings = GRAY_ENCODINGS
        elif mode == ColorMode.STANDARD:
            self._supported_encodings = STANDARD_ENCODINGS
        elif mode == ColorMode.COLOR:
            self._supported_encodings = EXTENDED_COLOR_ENCODINGS
