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

from karabo.bound import BinarySerializerHash, Hash, ImageData, fullyEqual


def test_ndarray_io():

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


def test_ndarray_refcount():
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
