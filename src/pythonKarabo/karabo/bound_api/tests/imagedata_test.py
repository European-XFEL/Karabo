import weakref

import numpy as np

from karabo.bound import ImageData, Encoding


def _array_mem_addr(arr):
    """ Return the memory address of the buffer backing a numpy array.
    """
    return arr.__array_interface__['data'][0]


def test_imagedata_from_ndarray():
    a = np.arange(20000, dtype='uint8').reshape(100, 200)
    imageData = ImageData(a)
    assert np.all(imageData.getData() == a.flat)

    assert imageData.getDimensionTypes() == [0, 0]
    assert imageData.getDimensions() == [200, 100]
    assert imageData.getEncoding() == Encoding.GRAY
    assert imageData.getROIOffsets() == [0, 0]

    b = np.arange(60000, dtype='uint8').reshape(100, 200, 3)
    imageData = ImageData(b, copy=True)
    assert np.all(imageData.getData() == b.flat)
    assert _array_mem_addr(imageData.getData()) != _array_mem_addr(b)

    assert imageData.getDimensionTypes() == [0, 0, 0]
    assert imageData.getDimensions() == [200, 100, 3]
    assert imageData.getEncoding() == Encoding.RGB
    assert imageData.getROIOffsets() == [0, 0, 0]

    c = np.arange(80000, dtype='uint8').reshape(100, 200, 4)
    imageData = ImageData(c, copy=False)
    assert np.all(imageData.getData() == c)
    assert _array_mem_addr(imageData.getData()) == _array_mem_addr(c)

    assert imageData.getDimensionTypes() == [0, 0, 0]
    assert imageData.getDimensions() == [200, 100, 4]
    assert imageData.getEncoding() == Encoding.RGBA
    assert imageData.getROIOffsets() == [0, 0, 0]


def test_ndarry_refcounting():
    arr = np.arange(20000, dtype='uint8').reshape(100, 200)
    arr_weak = weakref.ref(arr)
    img = ImageData(arr, copy=False)

    del arr
    assert arr_weak() is not None
    del img
    assert arr_weak() is None

    arr = np.arange(20000, dtype='uint8').reshape(100, 200)
    arr_weak = weakref.ref(arr)
    img_data = ImageData(arr, copy=False).getData()

    del arr
    assert arr_weak() is not None
    del img_data
    assert arr_weak() is None


def test_imagedata_from_hash():
    a = np.arange(20000, dtype='uint8').reshape(100, 200)
    imageData = ImageData(a)
    h = imageData.hash()
    imageData2 = ImageData(h)
    assert np.all(imageData.getData() == imageData2.getData())

    assert imageData.getDimensionTypes() == imageData2.getDimensionTypes()
    assert imageData.getDimensions() == imageData2.getDimensions()
    assert imageData.getEncoding() == imageData2.getEncoding()
    assert imageData.getROIOffsets() == imageData2.getROIOffsets()


def test_imagedata_set_and_get():
    a = np.arange(20000, dtype='uint8').reshape(100, 200)
    imageData = ImageData()
    # Set
    imageData.setData(a)  # Also set dataType
    imageData.setDimensionTypes([0, 1])
    imageData.setDimensions([200, 100])  # width, height
    imageData.setEncoding(Encoding.GRAY)
    imageData.setROIOffsets([20, 10])  # x, y

    # Get
    assert np.all(imageData.getData() == a.flat)
    assert imageData.getDimensionTypes() == [0, 1]
    assert imageData.getDimensions() == [200, 100]
    assert imageData.getEncoding() == Encoding.GRAY
    assert imageData.getROIOffsets() == [20, 10]
