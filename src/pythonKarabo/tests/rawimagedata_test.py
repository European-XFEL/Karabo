__author__="Irina Kozlova <irina.kozlova at xfel.eu>"
__date__ ="$Oct 9, 2013$"


import unittest
from karabo.karathon import *
import numpy

class  RawImageData_TestCase(unittest.TestCase):
    def setUp(self):
        self.resourcesdir = "../../../src/pythonKarabo/tests/resources/"
    
    def test_rawimagedata(self):
        
        try:
            image = numpy.fromfile(self.resourcesdir+"image_0001.raw", dtype=numpy.uint32)            
            self.assertEqual(image.size, 1048576)
            image = image.reshape(1024, 1024)
            
            # params: numpy array, optional copy flag (True), optional encoding (EncodingType.GRAY), optional endianness (CPU endianness)
            rdata = RawImageData(image, True, EncodingType.BGR)
            
            self.assertEqual(rdata.getData().size, 1048576)
            self.assertEqual(rdata.getByteSize(), 4194304)
            self.assertEqual(rdata.getEncoding(), EncodingType.BGR)
            self.assertEqual(rdata.getChannelSpace(), ChannelSpaceType.u_32_4)
            self.assertFalse(rdata.isBigEndian())
            dims = rdata.getDimensions()
            self.assertEqual(dims, [1024L, 1024L])
            
            h = rdata.hash()
            self.assertFalse(h["isBigEndian"])   # default setting     
            self.assertEqual(h["encoding"], 3)
            self.assertEqual(h["channelSpace"], 11)
            self.assertEqual(h["dims"], [1024L, 1024L])
            rdata.setIsBigEndian(True)
            self.assertTrue(h["isBigEndian"])
        except Exception as e:
            self.fail("test_rawimagedata exception group 1: " + str(e))
            
            
if __name__ == '__main__':
    unittest.main()   
