import numpy as np


from karabo.native.configuration import attr_fast_deepcopy
from karabo.native.data import (
    dtype_from_number, get_image_data, Hash, is_equal, Schema,
    dictToHash)


def test_numpy_int32_dtype():
    dtype = dtype_from_number(12)
    assert dtype == np.int32


def test_numpy_object_dtype():
    dtype = dtype_from_number(1)
    assert dtype == np.object_


def test_numpy_int64_dtype():
    dtype = dtype_from_number(16)
    assert dtype == np.int64


def test_numpy_unknwon():
    dtype = dtype_from_number(39)
    assert dtype == np.object_


def test_get_image_data():
    h = Hash()
    h['data.image.pixels'] = Hash()
    h['data.image.pixels.data'] = np.array([[2, 4, 6], [6, 8, 10]],
                                            np.int64)
    h['data.image.pixels.type'] = 16
    h['data.image.pixels.shape'] = np.array([2, 3], np.uint64)
    h['data.image.pixels.isBigEndian'] = False
    image = get_image_data(h)
    np.testing.assert_array_equal(
        image, np.array([[2, 4, 6], [6, 8, 10]], np.int64))


def test_dict_hash():
    """TEST THAT A DICT CAN BE MOVED TO A HASH"""
    b = {"b": 2}
    e = ["a", "b", "c"]
    d = {"a": 1, "c": 3, "d": 4, "b": b, "e": e}

    h = dictToHash(d)
    assert h["a"] == 1
    node = h["b"]
    assert isinstance(node, Hash)
    assert h["b.b"] == 2
    assert h["e"] == e


def test_array_equal():
    assert is_equal(np.array([1, 2, 3]), np.array([1, 2, 3]))
