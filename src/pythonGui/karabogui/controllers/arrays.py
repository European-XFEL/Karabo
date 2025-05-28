#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on October 4, 2016
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
#############################################################################
import numpy as np

from karabo.native import Encoding
from karabogui.binding.api import (
    REFERENCE_TYPENUM_TO_DTYPE, get_binding_array_value)

try:
    import cv2

    has_open_cv = True
except ImportError:
    has_open_cv = False

# Maps image physical dimension to ndarray dimension
DIMENSIONS = {
    'X': 1,
    'Y': 0,
    'Z': 2
}


def _extract_npy_data(image_node):
    """Internal method to extract npy data from an image node"""
    pixels = image_node.pixels.value
    if pixels.data.value is None:
        return None
    if len(pixels.data.value) == 0:
        return None
    arr_type = REFERENCE_TYPENUM_TO_DTYPE[pixels.type.value]
    npy = np.frombuffer(pixels.data.value, dtype=arr_type)
    return npy


def get_image_data(image_node, dimX, dimY, dimZ):
    """ Calculate a numpy array from the given ``image_node`` depending on the
    given dimensions and return it to use for image display.
    In case no data is included, a ``NoneType`` is returned.
    """
    npy = _extract_npy_data(image_node)
    if npy is None:
        return

    if dimZ:
        try:
            npy.shape = dimY, dimX, dimZ
        except ValueError:
            msg = 'Image has improper shape ({}, {}, {}) for size {}'.format(
                dimX, dimY, dimZ, len(npy))
            raise RuntimeError(msg)
    else:
        try:
            npy.shape = dimY, dimX
        except ValueError:
            msg = 'Image has improper shape ({}, {}, {}) for size {}'.format(
                dimX, dimY, dimZ, len(npy))
            raise RuntimeError(msg)

    return npy


def get_jpeg_data(image_node, dimZ):
    """ Calculate a numpy array from the given ``image_node`` depending on the
    given dimensions for a jpeg encoded stream
    """
    if not has_open_cv:
        return None

    npy = _extract_npy_data(image_node)
    if npy is None:
        return

    if dimZ == 3:
        npy = cv2.imdecode(npy, cv2.IMREAD_COLOR)
        # Default decode is BGR ...
        npy = cv2.cvtColor(npy, cv2.COLOR_BGR2RGB)
    else:
        npy = cv2.imdecode(npy, cv2.IMREAD_GRAYSCALE)

    return npy


def get_dimensions_and_encoding(image_node):
    """ The dimensions and the encoding of the given ``image_node`` are
    returned."""
    dimX = None
    dimY = None
    dimZ = None
    encoding = image_node.encoding.value

    if len(image_node.dims.value) == 2:
        # Shape
        dimX = image_node.dims.value[DIMENSIONS['X']]
        dimY = image_node.dims.value[DIMENSIONS['Y']]
        if encoding == Encoding.UNDEFINED:
            encoding = Encoding.GRAY  # assume it's gray
    elif len(image_node.dims.value) == 3:
        # Shape
        dimX = image_node.dims.value[DIMENSIONS['X']]
        dimY = image_node.dims.value[DIMENSIONS['Y']]
        dimZ = image_node.dims.value[DIMENSIONS['Z']]
        if encoding == Encoding.UNDEFINED:
            if dimZ == 3:
                encoding = Encoding.RGB  # assume it's RGB
            elif dimZ == 4:
                encoding = Encoding.RGBA  # assume it's RGBA
            else:
                encoding = Encoding.GRAY  # assume it's a stack of GRAY

    return dimX, dimY, dimZ, encoding


def get_array_data(proxy, default=None):
    """Retrieve the array and timestamp data from a property proxy belonging
    to an array binding

    :param default: default value to be returned if no value is available

    This function checks for `Undefined` on the proxy value and `None` data.
    If not data is available the `default` is returned with actual timestamp.

    :returns: data, timestamp
    """
    binding = proxy.binding
    return get_binding_array_value(binding, default)
