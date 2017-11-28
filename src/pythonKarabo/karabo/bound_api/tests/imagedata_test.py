import weakref

import numpy as np

from karabo.bound import Encoding, Hash, ImageData
from karabo.testing.utils import compare_ndarray_data_ptrs


def test_imagedata_from_ndarray():
    arr = np.arange(20000, dtype='uint16').reshape(100, 200)
    imageData = ImageData(arr)
    assert np.all(imageData.getData() == arr)

    assert imageData.getBitsPerPixel() == 16
    assert imageData.getDimensionTypes() == (0, 0)
    assert imageData.getDimensions() == (100, 200)
    assert imageData.getEncoding() == Encoding.GRAY
    assert imageData.isIndexable()
    assert imageData.getROIOffsets() == (0, 0)

    # Two additional tests on bitsPerPixel
    # 1. set it in constructor
    imageData = ImageData(arr, bitsPerPixel=12)
    assert imageData.getBitsPerPixel() == 12
    # 2. change it later
    imageData.setBitsPerPixel(10);
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

    assert imageData.getDimensionTypes() == (0, 0, 0)
    assert imageData.getDimensions() == (100, 200, 3)
    assert imageData.getEncoding() == Encoding.RGB
    assert imageData.getROIOffsets() == (0, 0, 0)

    arr = np.arange(80000, dtype='uint8').reshape(100, 200, 4)
    imageData = ImageData(arr)
    assert np.all(imageData.getData() == arr)
    assert compare_ndarray_data_ptrs(imageData.getData(), arr)

    assert imageData.getDimensionTypes() == (0, 0, 0)
    assert imageData.getDimensions() == (100, 200, 4)
    assert imageData.getEncoding() == Encoding.RGBA
    assert imageData.getROIOffsets() == (0, 0, 0)


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


def test_imagedata_set_and_get():
    a = np.arange(20000, dtype='uint8').reshape(100, 200)
    imageData = ImageData()
    # Set
    imageData.setData(a)  # Also set dataType
    imageData.setDimensionTypes((0, 1))
    imageData.setDimensions((200, 100))
    imageData.setEncoding(Encoding.GRAY)
    imageData.setROIOffsets((20, 10))  # x, y

    # Get
    assert np.all(imageData.getData() == a)
    assert imageData.getDimensionTypes() == (0, 1)
    assert imageData.getDimensions() == (200, 100)
    assert imageData.getEncoding() == Encoding.GRAY
    assert imageData.getROIOffsets() == (20, 10)


def test_imagedata_in_hash():
    arr = np.arange(20000, dtype='uint8').reshape(100, 200)
    imageData = ImageData(arr)
    h = Hash("img", imageData)

    assert isinstance(h["img"], ImageData)
    assert compare_ndarray_data_ptrs(h["img"].getData(), arr)
