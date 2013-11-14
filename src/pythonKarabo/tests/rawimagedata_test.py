__author__="Irina Kozlova <irina.kozlova at xfel.eu>"
__date__ ="$Oct 9, 2013$"


import unittest
from karabo.karathon import *
import numpy as np
import matplotlib.pyplot as plt

class  RawImageData_TestCase(unittest.TestCase):
    def setUp(self):
        self.resourcesdir = "../../../src/pythonKarabo/tests/resources/"
    
    def test_rawimage_to_png(self):
        try:
            imgArr = np.fromfile(self.resourcesdir+"image_0001.raw", dtype=np.uint32)            
            self.assertEqual(imgArr.size, 1048576)
  
            imgArr.shape = (1024, 1024)
            
            plt.imshow(imgArr)
            plt.savefig(self.resourcesdir+"image_0001A.png")
            #plt.show()
            
        except Exception as e:
            self.fail("test_rawimage_to_png exception group 1: " + str(e))

        try:
            f = open(self.resourcesdir+"image_0001.raw")
            imgStr = f.read()
            f.close()
            
            imgArr = np.array(bytearray(imgStr), dtype=np.uint32)
            self.assertEqual(imgArr.size, 4194304)
             
            imgArr.shape = (1024, 1024, 4)
            
            plt.imshow(imgArr)
            plt.savefig(self.resourcesdir+"image_0001B.png")
            #plt.show()
            
        except Exception as e:
            self.fail("test_rawimage_to_png exception group 2: " + str(e))
    

    def test_rawimagedata(self):
        
        try:
            imgArr = np.fromfile(self.resourcesdir+"image_0001.raw", dtype=np.uint32)            
            self.assertEqual(imgArr.size, 1048576)
            image = imgArr.reshape(1024, 1024)
            
            #               numpy array,    encoding,    optional header, optional endianess
            rdata = RawImageData(image, EncodingType.BGR)
            
            self.assertEqual(rdata.size(), 1048576)
            self.assertEqual(rdata.getByteSize(), 4194304)
            self.assertEqual(rdata.getEncoding(), EncodingType.BGR)
            self.assertEqual(rdata.getChannelSpace(), ChannelSpaceType.u_32_4)
            dims = rdata.getDimensions()
            self.assertEqual(dims, [1024L, 1024L, 1L])
            
            h = rdata.toHash()
            self.assertFalse(h["isBigEndian"])   # default setting     
            self.assertEqual(h["encoding"], 3)
            self.assertEqual(h["channelSpace"], 11)
            self.assertEqual(h["dims"], [1024L, 1024L, 1L])
            rdata.setIsBigEndian(True)
            self.assertTrue(h["isBigEndian"])
        except Exception as e:
            self.fail("test_rawimagedata exception group 1: " + str(e))
            
    def test_setData_bytearray(self):
        try:       
            imgNumpy = np.fromfile(self.resourcesdir+"image_0001.raw", dtype=np.uint32)
            imgArray = bytearray(imgNumpy);
            
            dimensions = Dims(1024, 1024, 1)
            header = Hash('a', 10, 'info', 99);
            rdata = RawImageData(imgArray, dimensions, EncodingType.BGR, ChannelSpaceType.u_32_4, header, False)
            
            data = rdata.getData()
            self.assertEqual(len(data), 4194304)
            h = rdata.toHash()

            self.assertFalse(h['isBigEndian'])
            self.assertEqual(h['header.info'], 99)
            
        except Exception as e:
            self.fail("test_setData_bytearray exception: " + str(e))
            
            
if __name__ == '__main__':
    unittest.main()   
