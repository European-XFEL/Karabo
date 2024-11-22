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

from karabo.bound import (
    DimensionType, Dims, Encoding, Hash, ImageData, Rotation, Types)
from karabo.testing.utils import compare_ndarray_data_ptrs


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

    # Check that C-order is guaranteed in C++ NDArray
    # The original order is not kept and returned from C++ as C-style.

    # Input array is F_CONTIGUOUS array
    x = np.asarray(np.arange(20000, dtype=np.int16).reshape(100, 200),
                   order='F')
    assert x.flags.owndata is True
    assert x.flags.c_contiguous is False
    assert x.flags.f_contiguous is True
    ix = ImageData(x)
    # array inside ImageData is kept as C++ NDArray in c-order
    assert not ix.getData().flags.owndata
    assert ix.getData().flags.c_contiguous is True
    assert ix.getData().flags.f_contiguous is False
    assert compare_ndarray_data_ptrs(x, ix.getData()) is False

    # Input array is C_CONTIGUOUS array
    y = np.asarray(np.arange(20000, dtype=np.int16).reshape(100, 200),
                   order='C', copy=True)
    assert compare_ndarray_data_ptrs(x, y) is False
    assert y.flags.owndata is True
    assert y.flags.c_contiguous is True
    assert y.flags.f_contiguous is False
    assert np.all(x == y) is np.True_
    assert compare_ndarray_data_ptrs(x, y) is False
    iy = ImageData(y)
    assert iy.getData().flags.c_contiguous is True
    assert iy.getData().flags.f_contiguous is False
    assert compare_ndarray_data_ptrs(y, iy.getData()) is True

    assert np.all(ix.getData() == iy.getData())
    assert compare_ndarray_data_ptrs(ix.getData(), iy.getData()) is False

    # getDataCopy: the new python object will be created
    nx = ix.getDataCopy()
    assert nx.flags.owndata is True
    assert nx.flags.c_contiguous is True
    assert nx.flags.f_contiguous is False
    ny = iy.getDataCopy()
    assert ny.flags.owndata is True
    assert ny.flags.c_contiguous is True
    assert ny.flags.f_contiguous is False

    assert np.all(nx == ny) == np.True_
    assert np.all(ix.getData() == nx) == np.True_

    # getData/setData
    assert x.flags.owndata is True
    assert x.flags.f_contiguous is True
    img = ImageData()
    img.setData(x)
    assert img.getData().flags.owndata is False
    assert img.getData().flags.c_contiguous is True
    assert np.all(img.getData() == ix.getData()) == np.True_

    # setDataCopy
    img = ImageData()  # empty ImageData object
    img.setDataCopy(ix.getData())
    assert img.getData().flags.owndata is False
    assert img.getData().flags.c_contiguous is True
    assert img.getData().flags.f_contiguous is False
    assert np.all(img.getData() == ix.getData()) == np.True_

    # getType/setType
    assert img.getType() == Types.INT16
    img.setType(Types.UINT16)
    assert img.getType() == Types.UINT16

    # getDimensions/setDimensions
    assert img.getDimensions() == (100, 200)
    img.setDimensions([200, 100])
    assert img.getDimensions() == (200, 100)

    # getDimensionTypes/setDimensionTypes
    img.setDimensionTypes((DimensionType.STACK, DimensionType.STACK))
    assert img.getDimensionTypes() == (DimensionType.STACK,
                                       DimensionType.STACK)

    # getDimensionScales/setDimesionScales
    img.setDimensionScales("Power, kw per hour")
    assert img.getDimensionScales() == "Power, kw per hour"

    # getROIOffsets/setROIOffsets
    assert img.getROIOffsets() == (0,)
    img.setROIOffsets((30, 40))
    assert img.getROIOffsets() == (30, 40)

    # getBinning/setBinning
    binning = Dims(3, 8)
    img.setBinning(binning)  # or img,setBinning((3, 8))
    assert img.getBinning() == (3, 8)

    # setFlipX/Y
    img.setFlipY(True)
    img.setFlipX(False)
    assert img.getFlipX() is False
    assert img.getFlipY() is True


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
