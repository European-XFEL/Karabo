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
from traits.api import Any, ArrayOrNone, Enum, HasStrictTraits, Int

from karabo.native import Encoding
from karabogui.controllers.arrays import (
    DIMENSIONS, get_dimensions_and_encoding, get_image_data, get_jpeg_data)
from karabogui.graph.common.api import Axes

# Special image dimensions
YUV422 = 2
YUV444 = 3
YUV_TYPES = {Encoding.YUV444, Encoding.YUV422_YUYV,
             Encoding.YUV422_UYVY}


class KaraboImageNode(HasStrictTraits):
    dim_x = Int
    dim_y = Int
    dim_z = Int
    encoding = Enum(*Encoding)
    stack_axis = Any

    # Internal traits
    _data = ArrayOrNone

    def set_value(self, image_node):
        dim_x, dim_y, dim_z, encoding = get_dimensions_and_encoding(image_node)

        self.dim_x = dim_x if dim_x is not None else 0
        self.dim_y = dim_y if dim_y is not None else 0
        self.dim_z = dim_z if dim_z is not None else 0
        self.encoding = encoding
        if encoding == Encoding.JPEG:
            self._data = get_jpeg_data(image_node, self.dim_z)
        else:
            self._data = get_image_data(image_node,
                                        self.dim_x, self.dim_y, self.dim_z)

        stack_axis = None
        if "stackAxis" in image_node:
            stack_axis = image_node.stackAxis.value
        self.stack_axis = stack_axis

    def get_data(self):
        # Get data depending on the image schema and encoding
        if self.stack_axis is not None:
            return self.get_slice(self.stack_axis, cell=0)
        elif self.encoding == Encoding.GRAY:
            return self.get_slice(DIMENSIONS['Z'], cell=0)
        else:
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

    def get_slice(self, axis, cell):
        """Returns the cell slice from the given axis"""
        if not self.dim_z or self._data is None:
            return self._data
        else:
            # Numpy arrays are received as x, y, z
            if axis == Axes.Y:
                return self._data[cell, :, :]
            if axis == Axes.X:
                return self._data[:, cell, :]
            if axis == Axes.Z:
                return self._data[:, :, cell]

    @property
    def is_valid(self):
        if self._data is None or self.dim_x < 1 or self.dim_y < 1:
            return False
        # Handle YUV encoding type
        if self.encoding in YUV_TYPES:
            if self.dim_z == YUV422:
                # input image is (u1, y1, v1, y2, u2, y3, v2, y4, ...)
                # only display "luma" (Y') component in the GUI
                self._data = self._data[:, :, 1]
                self.dim_z = 0
                self.encoding = Encoding.GRAY
            elif self.dim_z == YUV444:
                # input image is (u1, y1, v1, u2, y2, v2, ...)
                # only display "luma" (Y') component in the GUI
                self._data = self._data[:, :, 1]
                self.dim_z = 0
                self.encoding = Encoding.GRAY
            else:
                return False

        return True
