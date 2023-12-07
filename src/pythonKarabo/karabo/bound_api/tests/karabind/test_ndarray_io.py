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
