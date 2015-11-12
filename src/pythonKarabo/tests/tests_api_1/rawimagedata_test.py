__author__="Irina Kozlova <irina.kozlova at xfel.eu>"
__date__ ="$Oct 9, 2013$"

import os.path as op
import unittest

import numpy

from karathon import *


class  RawImageData_TestCase(unittest.TestCase):
    def setUp(self):
        self.resourcesdir = op.join(op.dirname(__file__), 'resources')

    def test_rawimagedata(self):
        
        try:
            image = numpy.fromfile(op.join(self.resourcesdir, "image_0001.raw"),
                                   dtype=numpy.uint32)
            self.assertEqual(image.size, 1048576)
            image = image.reshape(1024, 1024)
            
            # params: numpy array, optional copy flag (True), optional
            # dimensions (guessed from ndarray), optional encoding (guessed
            # from ndarray), optional endianness (CPU endianness)
            rdata = RawImageData(image)
            
            self.assertEqual(rdata.getData().size, 1048576)
            self.assertEqual(rdata.getByteSize(), 4194304)
            self.assertEqual(rdata.getEncoding(), EncodingType.GRAY)
            self.assertEqual(rdata.getChannelSpace(), ChannelSpaceType.u_32_4)
            self.assertFalse(rdata.isBigEndian())
            dims = rdata.getDimensions()
            self.assertEqual(dims, [1024, 1024])
            roiOffsets = rdata.getROIOffsets()
            self.assertEqual(roiOffsets, [0, 0]) # default setting
            
            rdata.setROIOffsets([1, 2])
            roiOffsets = rdata.getROIOffsets()
            self.assertEqual(roiOffsets, [1, 2])
            
            h = rdata.hash()
            self.assertFalse(h["isBigEndian"])   # default setting     
            self.assertEqual(h["encoding"], 0)
            self.assertEqual(h["channelSpace"], 11)
            self.assertEqual(h["dims"], [1024, 1024])
            rdata.setIsBigEndian(True)
            self.assertTrue(h["isBigEndian"])
        except Exception as e:
            self.fail("test_rawimagedata exception group 1: " + str(e))
            
            
if __name__ == '__main__':
    unittest.main()

