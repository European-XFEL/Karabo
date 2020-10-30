#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on October 4, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import numpy as np
from traits.api import Undefined

from karabo.native import EncodingType

# Ensure correct mapping of reference type to numpy dtype.
REFERENCE_TYPENUM_TO_DTYPE = {
    0: 'bool',
    2: 'char',
    4: 'int8',
    6: 'uint8',
    8: 'int16',
    10: 'uint16',
    12: 'int32',
    14: 'uint32',
    16: 'int64',
    18: 'uint64',
    20: 'float32',
    22: 'float64'
}

# Maps image physical dimension to ndarray dimension
DIMENSIONS = {
    'X': 1,
    'Y': 0,
    'Z': 2
}


def get_image_data(image_node, dimX, dimY, dimZ):
    """ Calculate a numpy array from the given ``image_node`` depending on the
    given dimensions and return it to use for image display.
    In case no data is included, a ``NoneType`` is returned.
    """
    pixels = image_node.pixels.value
    if pixels.data.value is None:
        return None
    if len(pixels.data.value) == 0:
        return None

    arr_type = REFERENCE_TYPENUM_TO_DTYPE[pixels.type.value]
    npy = np.frombuffer(pixels.data.value, dtype=arr_type)
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
        if encoding == EncodingType.UNDEFINED:
            encoding = EncodingType.GRAY  # assume it's gray
    elif len(image_node.dims.value) == 3:
        # Shape
        dimX = image_node.dims.value[DIMENSIONS['X']]
        dimY = image_node.dims.value[DIMENSIONS['Y']]
        dimZ = image_node.dims.value[DIMENSIONS['Z']]
        if encoding == EncodingType.UNDEFINED:
            if dimZ == 3:
                encoding = EncodingType.RGB  # assume it's RGB
            elif dimZ == 4:
                encoding = EncodingType.RGBA  # assume it's RGBA
            else:
                encoding = EncodingType.GRAY  # assume it's a stack of GRAY

    return dimX, dimY, dimZ, encoding


def get_array_data(proxy):
    """Retrieve the NDArray from a property proxy belonging an NDArray binding

    Check for `Undefined` on the proxy value and `None` data.
    """
    node = proxy.value
    if node is Undefined:
        return None
    pixels = node.data.value
    if pixels is None:
        return None

    arr_type = REFERENCE_TYPENUM_TO_DTYPE[node.type.value]
    value = np.frombuffer(pixels, dtype=arr_type)
    if value.ndim == 1:
        return value

    return None
