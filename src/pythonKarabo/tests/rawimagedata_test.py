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
        
        except Exception,e:
            self.fail("test_rawimage_to_png exception group 2: " + str(e))

if __name__ == '__main__':
    unittest.main()   