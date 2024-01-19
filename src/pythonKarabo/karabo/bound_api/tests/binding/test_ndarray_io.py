import weakref

import karabind
import numpy as np
import pytest

import karathon


@pytest.mark.parametrize(
    "BinarySerializerHash, Hash, fullyEqual",
    [
     (karathon.BinarySerializerHash, karathon.Hash, karathon.fullyEqual),
     (karabind.BinarySerializerHash, karabind.Hash, karabind.fullyEqual)
     ])
def test_ndarray_io(BinarySerializerHash, Hash, fullyEqual):

    ser = BinarySerializerHash.create('Bin')

    def func(i):
        arr = np.full((100, 200), 88+1, dtype=np.int16)
        data = Hash('node', Hash())
        node = data['node']
        node.set('ndarray', arr)
        node.set('index', i)
        archive = ser.save(data)
        dcopy = ser.load(archive)
        assert fullyEqual(dcopy, data)

    for i in range(100):
        func(i)


@pytest.mark.parametrize(
    "Hash, ImageData",
    [
     (karathon.Hash, karathon.ImageData),
     (karabind.Hash, karabind.ImageData)
     ])
def test_ndarray_refcount(Hash, ImageData):
    arr = np.arange(20000, dtype=np.int16).reshape(100, 200)
    arr_weak = weakref.ref(arr)
    h = Hash('a', arr)
    g = h['a']
    img = ImageData(arr)
    assert arr_weak() is not None
    del arr
    assert arr_weak() is not None
    del h
    assert arr_weak() is not None
    del g
    assert arr_weak() is not None
    del img
    assert arr_weak() is None
