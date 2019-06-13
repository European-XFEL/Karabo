from traits.api import Any, ArrayOrNone, Enum, Int, HasStrictTraits

from karabo.native import EncodingType
from karabogui.controllers.images import (
    get_dimensions_and_encoding, get_image_data, DIMENSIONS)

# Special image dimensions
YUV422 = 2
YUV444 = 3


class KaraboImageNode(HasStrictTraits):
    dim_x = Int
    dim_y = Int
    dim_z = Int
    encoding = Enum(*EncodingType)
    node = Any

    # Internal traits
    _data = ArrayOrNone

    def __init__(self, image_node):
        super(KaraboImageNode, self).__init__()
        self.node = image_node

        dim_x, dim_y, dim_z, encoding = get_dimensions_and_encoding(image_node)

        self.dim_x = dim_x
        self.dim_y = dim_y
        self.dim_z = dim_z if dim_z is not None else 0
        self.encoding = encoding
        self._data = get_image_data(self.node,
                                    self.dim_x, self.dim_y, self.dim_z)

    def get_data(self):
        return self._data

    def get_axis_dimension(self, axis):
        """Return the correct dimension from a given axis"""
        if axis == DIMENSIONS['Y']:
            return self.dim_y
        if axis == DIMENSIONS['X']:
            return self.dim_x
        if axis == DIMENSIONS['Z']:
            return self.dim_z
        else:
            raise ValueError('Invalid axis value: {}'.format(axis))

    def get_slice(self, axis, cell):
        """Returns the cell slice from the given axis"""
        if not self.dim_z or self._data is None:
            return self._data
        else:
            if axis == DIMENSIONS['Y']:
                return self._data[:, cell, :]
            if axis == DIMENSIONS['X']:
                return self._data[cell, :, :]
            if axis == DIMENSIONS['Z']:
                return self._data[:, :, cell]

    @property
    def is_valid(self):
        if self.dim_x is None and self.dim_y is None:
            return False

        if self.dim_x < 1 or self.dim_y < 1:
            return False

        # Handle YUV encoding type
        if self.encoding == EncodingType.YUV:
            if self.dim_z == YUV422:
                # input image is (u1, y1, v1, y2, u2, y3, v2, y4, ...)
                # only display "luma" (Y') component in the GUI
                self._data = self._data[:, :, 1]
                self.dim_z = 0
                self.encoding = EncodingType.GRAY
            elif self.dim_z == YUV444:
                # input image is (u1, y1, v1, u2, y2, v2, ...)
                # only display "luma" (Y') component in the GUI
                self._data = self._data[:, :, 1]
                self.dim_z = 0
                self.encoding = EncodingType.GRAY
            else:
                return False

        return True
