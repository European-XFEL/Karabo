#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on October 4, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import numpy as np

from PyQt4.QtGui import QImage

# Ensure correct mapping of reference type to numpy dtype.
_REFERENCE_TYPENUM_TO_DTYPE = {
    0: 'bool',
    2: 'char',
    4: 'int8',
    6: 'uint8',
    8: 'int16',
    10: 'uint16',
    12: 'int32',
    14 : 'uint32',
    16: 'int64',
    18: 'uint64',
    20: 'float',
    22: 'float64'
}


def get_image_data(image_node):
    """ Calculate a numpy array from the given ``image_node`` and return it to
    use for image display. In case no data is included, a ``NoneType`` is
    returned.
    """
    pixels = image_node.pixels
    if len(pixels.data) == 0:
        return None
    arr_type = _REFERENCE_TYPENUM_TO_DTYPE[pixels.type]
    return np.frombuffer(pixels.data, dtype=arr_type)


def get_dimensions_and_format(image_node):
    """ The dimensions and the format of the given ``image_node`` are
    returned."""
    dimX = None
    dimY = None
    dimZ = None
    format = None
    if len(image_node.dims) == 2:
        # Shape
        dimX = image_node.dims[1]
        dimY = image_node.dims[0]
        # Format: Grayscale
        format = QImage.Format_Indexed8
    elif len(image_node.dims) == 3:
        # Shape
        dimX = image_node.dims[1]
        dimY = image_node.dims[0]
        dimZ = image_node.dims[2]
        if dimZ == 3:
            # Format: RGB
            format = QImage.Format_RGB888
    return dimX, dimY, dimZ, format
