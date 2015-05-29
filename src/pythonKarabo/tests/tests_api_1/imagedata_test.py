# To change this license header, choose License Headers in Project Properties.
# To change this template file, choose Tools | Templates
# and open the template in the editor.

import unittest
from karabo.karathon import (Hash, ImageData, ChannelSpace, Encoding)
import numpy as np


class  ImageData_TestCase(unittest.TestCase):
    #def setUp(self):
    #    self.foo = ImageData_()
    #

    #def tearDown(self):
    #    self.foo.dispose()
    #    self.foo = None

    def test_imagedata_from_ndarray(self):
        a = np.arange(20000, dtype='int64').reshape(100, 200)
        imageData = ImageData(a)
        try:
            self.assertEqual(imageData.getData(), a.tostring())

            self.assertEqual(imageData.getChannelSpace(), ChannelSpace.s_64_8)
            self.assertEqual(imageData.getDataType(), 'INT64')
            self.assertEqual(imageData.getDimensionTypes(), [0, 0])
            self.assertEqual(imageData.getDimensions(), [200, 100])
            self.assertEqual(imageData.getEncoding(), Encoding.GRAY)
            self.assertEqual(imageData.getROIOffsets(), [0, 0])
            
        except Exception as e:
            self.fail("test_imagedata_from_ndarray: " + str(e))
        
        
        b = np.arange(60000, dtype='float64').reshape(100, 200, 3)
        imageData = ImageData(b)
        try:
            self.assertEqual(imageData.getData(), b.tostring())

            self.assertEqual(imageData.getChannelSpace(), ChannelSpace.f_64_8)
            self.assertEqual(imageData.getDataType(), 'DOUBLE')
            self.assertEqual(imageData.getDimensionTypes(), [0, 0, 0])
            self.assertEqual(imageData.getDimensions(), [200, 100, 3])
            self.assertEqual(imageData.getEncoding(), Encoding.RGB)
            self.assertEqual(imageData.getROIOffsets(), [0, 0, 0])
            
        except Exception as e:
            self.fail("test_imagedata_from_ndarray: " + str(e))
        
        
        c = np.arange(80000, dtype='uint64').reshape(100, 200, 4)
        imageData = ImageData(c)
        try:
            self.assertEqual(imageData.getChannelSpace(), ChannelSpace.u_64_8)
            self.assertEqual(imageData.getDataType(), 'UINT64')
            self.assertEqual(imageData.getDimensionTypes(), [0, 0, 0])
            self.assertEqual(imageData.getDimensions(), [200, 100, 4])
            self.assertEqual(imageData.getEncoding(), Encoding.RGBA)
            self.assertEqual(imageData.getROIOffsets(), [0, 0, 0])
            self.assertEqual(imageData.getData(), c.tostring())
            
        except Exception as e:
            self.fail("test_imagedata_from_ndarray: " + str(e))
    
    
    def test_imagedata_from_hash(self):
        a = np.arange(20000, dtype='int64').reshape(100, 200)
        imageData = ImageData(a)
        h = imageData.hash()
        imageData2 = ImageData(h)
        try:
            self.assertEqual(imageData.getData(), imageData2.getData())

            self.assertEqual(imageData.getChannelSpace(), imageData2.getChannelSpace())
            self.assertEqual(imageData.getDataType(), imageData2.getDataType())
            self.assertEqual(imageData.getDimensionTypes(), imageData2.getDimensionTypes())
            self.assertEqual(imageData.getDimensions(), imageData2.getDimensions())
            self.assertEqual(imageData.getEncoding(), imageData2.getEncoding())
            self.assertEqual(imageData.getROIOffsets(), imageData2.getROIOffsets())
               
        except Exception as e:
            self.fail("test_imagedata_from_hash: " + str(e))
    
    
    def test_imagedata_set_and_get(self):
        a = np.arange(20000, dtype='int64').reshape(100, 200)
        imageData = ImageData()
        try:
            # Set
            imageData.setData(a) # Also set dataType
            imageData.setChannelSpace(ChannelSpace.s_64_8)
            imageData.setDimensionTypes([0, 1])
            imageData.setDimensions([200, 100]) # width, height
            imageData.setEncoding(Encoding.GRAY)
            imageData.setROIOffsets([20, 10]) # x, y
            
            # Get
            self.assertEqual(imageData.getData(), a.tostring())
            self.assertEqual(imageData.getChannelSpace(), ChannelSpace.s_64_8)
            self.assertEqual(imageData.getDataType(), 'INT64')
            self.assertEqual(imageData.getDimensionTypes(), [0, 1])
            self.assertEqual(imageData.getDimensions(), [200, 100])
            self.assertEqual(imageData.getEncoding(), Encoding.GRAY)
            self.assertEqual(imageData.getROIOffsets(), [20, 10])
            
        except Exception as e:
            self.fail("test_imagedata_set_and_get: " + str(e))
        
        
if __name__ == '__main__':
    unittest.main()

