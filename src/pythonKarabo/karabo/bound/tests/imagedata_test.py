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
import weakref

import numpy as np

from karabo.bound import Encoding, Hash, ImageData, Rotation, Types
from karabo.testing.utils import compare_ndarray_data_ptrs


def test_imagedata_from_ndarray():
    arr = np.arange(20000, dtype='uint16').reshape(100, 200)
    imageData = ImageData(arr)
    assert np.all(imageData.getData() == arr)

    assert imageData.getBitsPerPixel() == 16
    assert imageData.getDimensions() == (100, 200)
    assert imageData.getEncoding() == Encoding.GRAY
    assert imageData.isIndexable()
    assert imageData.getROIOffsets() == (0, 0)
    assert imageData.getBinning() == (1, 1)
    assert imageData.getRotation() == Rotation.ROT_0
    assert imageData.getFlipX() is False
    assert imageData.getFlipY() is False

    # Two additional tests on bitsPerPixel
    # 1. set it in constructor
    imageData = ImageData(arr, bitsPerPixel=12)
    assert imageData.getBitsPerPixel() == 12
    # 2. change it later
    imageData.setBitsPerPixel(10)
    assert imageData.getBitsPerPixel() == 10

    # Make sure conversion from Fortran order doesn't harm dimensions
    arr = np.asarray(np.arange(20000, dtype='uint8').reshape(100, 200),
                     order='F')
    imageData = ImageData(arr)
    assert imageData.getDimensions() == (100, 200)

    arr = np.arange(60000, dtype='uint8').reshape(100, 200, 3)
    imageData = ImageData(arr)
    assert np.all(imageData.getData() == arr)
    assert compare_ndarray_data_ptrs(imageData.getData(), arr)

    assert imageData.getDimensions() == (100, 200, 3)
    assert imageData.getEncoding() == Encoding.RGB
    assert imageData.getROIOffsets() == (0, 0, 0)
    assert imageData.getBinning() == (1, 1, 1)

    arr = np.arange(80000, dtype='uint8').reshape(100, 200, 4)
    imageData = ImageData(arr)
    assert np.all(imageData.getData() == arr)
    assert compare_ndarray_data_ptrs(imageData.getData(), arr)

    assert imageData.getDimensions() == (100, 200, 4)
    assert imageData.getEncoding() == Encoding.RGBA
    assert imageData.getROIOffsets() == (0, 0, 0)
    assert imageData.getBinning() == (1, 1, 1)

    arr = np.arange(220000, dtype='uint8').reshape(100, 200, 11)
    imageData = ImageData(arr)
    assert np.all(imageData.getData() == arr)
    assert compare_ndarray_data_ptrs(imageData.getData(), arr)
    assert imageData.getDimensions() == (100, 200, 11)
    assert imageData.getEncoding() == Encoding.GRAY
    assert imageData.getROIOffsets() == (0, 0, 0)
    assert imageData.getBinning() == (1, 1, 1)


def test_ndarry_refcounting():
    arr = np.arange(20000, dtype='uint8').reshape(100, 200)
    arr_weak = weakref.ref(arr)
    img = ImageData(arr)

    del arr
    assert arr_weak() is not None
    del img
    assert arr_weak() is None

    arr = np.arange(20000, dtype='uint8').reshape(100, 200)
    arr_weak = weakref.ref(arr)
    img_data = ImageData(arr).getData()

    del arr
    assert arr_weak() is not None
    del img_data
    assert arr_weak() is None

    # Make sure conversion from Fortran order creates a copy
    arr = np.asarray(np.arange(20000, dtype='uint8').reshape(100, 200),
                     order='F')
    img_data = ImageData(arr).getData()
    assert not compare_ndarray_data_ptrs(img_data, arr)

    img_data_weak = weakref.ref(img_data)
    assert img_data_weak() is not None
    del img_data
    assert img_data_weak() is None


def test_ndarry_in_hash():
    arr = np.arange(200, dtype='uint8').reshape(10, 20)
    assert arr.shape == (10, 20)
    h = Hash("arr", arr)
    arr2 = h["arr"]
    # still the same
    assert compare_ndarray_data_ptrs(arr, arr2)
    assert "arr.data" in h
    assert h["arr.type"] == Types.UINT8
    assert h["arr.shape"] == [10, 20]

    # Fake NDArray with wrong size of underlying BYTE_ARRAY
    # Do not try to mimic NDArrays like that in real life, this is just
    # to check that an inconsistently created NDArray in the Hash will fail
    array_node = Hash()
    # BYTE_ARRAY is on purpose not exposed to bound API via Types...
    array_node.setAs("data", b'12345', "BYTE_ARRAY")  # 5 bytes only
    array_node.setAs("type", int(Types.UINT16), Types.INT32)
    array_node.setAs("shape", [2, 3], Types.VECTOR_UINT64)
    array_node.setAs("isBigEndian", False, Types.BOOL)
    outer_hash = Hash("badArray", array_node)
    outer_hash.setAttribute("badArray", "__classId", "NDArray")
    try:
        outer_hash["badArray"]
    except RuntimeError as e:
        msg = ("Inconsistent NDArray: 5 are too few bytes for shape [2,3] "
               "of UINT16")
        assert msg in str(e)
    else:
        assert False


# This is the original test...
def test_imagedata_set_and_get():
    a = np.arange(20000, dtype='uint8').reshape(100, 200)
    imageData = ImageData()

    imageData.setData(a)  # Also set dataType, shape, dimensions and encoding
    imageData.setEncoding(Encoding.GRAY)
    imageData.setROIOffsets((20, 10))  # y, x
    imageData.setBinning((8, 3))
    imageData.setRotation(Rotation.ROT_180)
    # True/False combination is tested in C++
    imageData.setFlipX(False)
    imageData.setFlipY(True)

    assert np.all(imageData.getData() == a)
    # Now we set dimensions which trickle down to getData.
    # Before doing so, dimensions are undefined/empty (NOT taken from a!).
    # Don't test for that since not sure whether this is really desired.
    imageData.setDimensions((200, 100))

    assert np.all(imageData.getData() == a.reshape(200, 100))
    assert imageData.getDimensions() == (200, 100)
    assert imageData.getEncoding() == Encoding.GRAY
    assert imageData.getROIOffsets() == (20, 10)
    assert imageData.getBinning() == (8, 3)
    assert imageData.getRotation() == Rotation.ROT_180
    assert imageData.getFlipX() is False
    assert imageData.getFlipY() is True


def test_imagedata_type_set_and_get():
    a = np.arange(20000, dtype='float32').reshape(100, 200)
    imageData = ImageData()
    imageData.setData(a)  # Also set dataType, shape, dimensions and encoding

    # Check the default data type from the NumPy array...
    assert (imageData.getType() == Types.FLOAT)

    # Setting data type to int32
    imageData.setType(Types.INT32)
    assert (imageData.getType() == Types.INT32)

    # Setting data type to uint32
    imageData.setType(Types.UINT32)
    assert (imageData.getType() == Types.UINT32)


def test_imagedata_in_hash():
    arr = np.arange(20000, dtype='uint8').reshape(100, 200)
    imageData = ImageData(arr)
    h = Hash("img", imageData)

    assert isinstance(h["img"], ImageData)
    assert compare_ndarray_data_ptrs(h["img"].getData(), arr)
