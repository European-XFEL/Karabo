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
            
        except Exception,e:
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
            
        except Exception,e:
            self.fail("test_rawimage_to_png exception group 2: " + str(e))
    

    def test_rawimagedata(self):
        
        try:
            d=Dims(1024, 1024, 1)
            byteSize = 4194304
            rdata = RawImageData(byteSize, d, EncodingType.BGR, ChannelSpaceType.u_32_4)  
            self.assertEqual(rdata.size(), 1048576)
            self.assertEqual(rdata.getByteSize(), 4194304)
            self.assertEqual(rdata.getEncoding(), EncodingType.BGR)
            self.assertEqual(rdata.getChannelSpace(), ChannelSpaceType.u_32_4)
        
            dims = rdata.getDimensions()
            self.assertEqual(dims, [1024L, 1024L, 1L])
            
            imgArr = np.fromfile(self.resourcesdir+"image_0001.raw", dtype=np.uint32)            
            self.assertEqual(imgArr.size, 1048576)
            
            imgArr2 = bytearray(imgArr)
            self.assertEqual(len(imgArr2), 4194304)
            
            rdata.setData(imgArr2)
            
            getData = rdata.getData()
            self.assertEqual(len(getData), 4194304)
            
            h = rdata.toHash()
            self.assertEqual(h.get("isBigEndian"), False)          
            self.assertEqual(h.get("encoding"), 3)
            self.assertEqual(h.get("channelSpace"), 11)
            self.assertEqual(h.get("dims"), [1024L, 1024L, 1L] )
            
            #construct RawImageData from given Hash and check its properties
            rdatanew = RawImageData(h)
            self.assertEqual(rdatanew.size(), 1048576)
            self.assertEqual(rdatanew.getByteSize(), 4194304)
            self.assertEqual(rdatanew.getEncoding(), EncodingType.BGR)
            self.assertEqual(rdatanew.getChannelSpace(), ChannelSpaceType.u_32_4)
            getDataNew = rdatanew.getData()
            self.assertEqual(len(getDataNew), 4194304)
            
        except Exception,e:
            self.fail("test_rawimagedata exception: " + str(e))


if __name__ == '__main__':
    unittest.main()   