# To change this license header, choose License Headers in Project Properties.
# To change this template file, choose Tools | Templates
# and open the template in the editor.

import unittest
from karabo.karathon import (Hash, NDArray, BinarySerializerHash)
import numpy as np


class  Ndarr1_TestCase(unittest.TestCase):
    #def setUp(self):
    #    self.foo = Ndarr1_()
    #

    #def tearDown(self):
    #    self.foo.dispose()
    #    self.foo = None

    def test_ndarr1_ndarray_copy(self):
        a = np.arange(30).reshape(2,5,3)
        nd = NDArray(a)
        try:
            assert nd.has('data')
            assert nd.has('dataType')
            assert nd.has('dims')
            assert nd.has('dimTypes')
            assert nd.has('isBigEndian')
            self.assertEqual(a.shape, tuple(nd.getDimensions()))
            self.assertEqual(len(nd.hash().get("data")), 240)
            self.assertEqual(nd.getDataType(), "INT64")
            self.assertEqual(str(nd.hash().getType("data")), "VECTOR_CHAR")
            b = nd.getData()
            c = (a == b).flatten().tolist()
            assert len(c) == 30
            assert len(c) == c.count(True)
        except Exception as e:
            self.fail("test_ndarr1_ndarray_copy: " + str(e))
        
    def test_ndarr1_ndarray_nocopy(self):
        a = np.arange(30).reshape(2,5,3)
        nd = NDArray()
        nd.setData(a, copy=False)
        try:
            assert nd.has('data')
            assert nd.has('dataType')
            assert nd.has('dims')
            assert nd.has('dimTypes')
            assert nd.has('isBigEndian')
            self.assertEqual(a.shape, tuple(nd.getDimensions()))
            self.assertEqual(nd.getDataType(), "INT64")
            self.assertEqual(str(nd.hash().getType("data")), "ARRAY_CHAR")
            b = nd.getData()     # it takes implicitly an ownership over ndarray (copying) 
            self.assertEqual(str(nd.hash().getType("data")), "VECTOR_CHAR")
            self.assertEqual(len(nd.hash().get("data")), 240)
            c = (a == b).flatten().tolist()
            assert len(c) == 30
            assert len(c) == c.count(True)
        except Exception as e:
            self.fail("test_ndarr1_ndarray_nocopy: " + str(e))
        
    def test_ndarr1_ndarray_archive(self):
        a = np.arange(30).reshape(2,5,3)
        nd = NDArray()
        nd.setData(a, copy=False)
        ser = BinarySerializerHash.create("Bin")
        try:
            self.assertEqual(str(nd.hash().getType("data")), "ARRAY_CHAR")
            archive = ser.save(nd.hash())
            assert(type(archive) == bytes)
            nd2 = NDArray(ser.load(archive))
            self.assertEqual(str(nd2.hash().getType("data")), "VECTOR_CHAR")
            b = nd2.getData()
            c = (a == b).flatten().tolist()
            assert len(c) == 30
            assert len(c) == c.count(True)
        except Exception as e:
            self.fail("test_ndarr1_ndarray_archive: " + str(e))
        
        
if __name__ == '__main__':
    unittest.main()

