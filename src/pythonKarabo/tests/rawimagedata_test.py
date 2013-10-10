__author__="Irina Kozlova <irina.kozlova at xfel.eu>"
__date__ ="$Oct 9, 2013$"


import unittest
from karabo.karathon import *
import numpy as np
import matplotlib.pyplot as plt

class  RawImageData_TestCase(unittest.TestCase):
    def setUp(self):
        self.resourcesdir = "../../../src/pythonKarabo/tests/resources/"
    
    def test_rawimagedata_to_png(self):
        try:
            imgArr = np.fromfile(self.resourcesdir+"image_0001.raw", dtype=np.uint32)
            self.assertEqual(imgArr.size, 1048576)
  
            imgArr.shape = (1024, 1024)
            
            plt.imshow(imgArr)
            plt.savefig(self.resourcesdir+"image_0001.png")
            
        except Exception,e:
            self.fail("test_rawimagedata_to_png exception: " + str(e))
