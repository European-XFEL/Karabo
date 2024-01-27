# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.

import numpy as np

from karabo.bound import Dims, Encoding, Hash, ImageData, Rotation


def test_imagedata_buildUp():
    # Create numpy array ...
    arr = np.array([[1, 2], [3, 4], [5, 6]])
    # By default: 64 bits
    assert arr.dtype == np.dtype('int64')
    # check shape...
    assert arr.shape == (3, 2)
    # Python is an owner
    assert arr.flags.owndata is True
    # Create default image data from 'arr'
    img = ImageData(arr)
    # image data is not an owner
    assert img.getData().flags.owndata is False
    assert np.all(arr == img.getData())
    # check binning
    assert img.getBinning() == (1, 1)
    # check deduced bits per pixel ...
    assert img.getBitsPerPixel() == 64
    # Dimension scales do not set
    assert img.getDimensionScales() == ''
    # check dimension types ...
    assert img.getDimensionTypes() == (0, 0)
    # check deduced dimensions ...
    assert img.getDimensions() == (3, 2)
    # check deduced encoding ...
    assert img.getEncoding() == Encoding.GRAY
    # check flip in X scale
    assert img.getFlipX() is False
    # ... and Y scale
    assert img.getFlipY() is False
    # Get ROI offesets ...
    assert img.getROIOffsets() == (0, 0)
    # No rotation ... (0 grad)
    assert img.getRotation() == 0
    # check type ...
    assert img.getType() == 16
    # is indexable?
    assert img.isIndexable() is True
    # create image with defined dimensions and encoding...
    # one-dimensional array ...
    arr = np.array([1, 2, 3, 4, 5, 6])
    # define required shape...
    dims = Dims(3, 2)
    img = ImageData(arr, dims, Encoding.RGBA)
    assert img.getEncoding() == Encoding.RGBA
    img.setEncoding(Encoding.GRAY)
    assert img.getEncoding() == Encoding.GRAY
    img.setBitsPerPixel(12)
    assert img.getBitsPerPixel() == 12
    # check shape of image
    assert img.getData().shape == (3, 2)
    # rotate 180 grad
    img.setRotation(Rotation.ROT_180)
    assert img.getRotation() == Rotation.ROT_180


def test_imagedata_hash():
    # Create numpy array
    arr = np.array([1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12])
    # Python is an owner of data
    assert arr.flags.owndata is True
    # create image data object...
    # NOTE: Dims defines shape
    img = ImageData(arr, Dims(3, 4), Encoding.RGBA, 32)
    # delete original array
    del arr
    # Store image data into Hash
    h = Hash('image', img, 'a', 12)
    assert h['image'].getData().flags.owndata is False
    assert isinstance(h['image'], ImageData) is True
    assert h['image'].getData().shape == (3, 4)
