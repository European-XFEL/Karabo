__author__="Irina Kozlova <irina.kozlova at xfel.eu>"
__date__ ="$Sep 30, 2013$"


import unittest
from karabo.karathon import *

class  CpuImage_TestCase(unittest.TestCase):
    def setUp(self):
        self.resourcesdir = "../../../src/pythonKarabo/tests/resources/"
    
    def test_cpuimage_float(self):
        try:
            img = CpuImageFLOAT()
            self.assertTrue(img.isEmpty())
            self.assertEqual(img.byteSize(), 0)
        except Exception,e:
            self.fail("test_cpuimage_float exception group 1: " + str(e))
        
        try:
            img = CpuImageFLOAT(self.resourcesdir+"european-xfel-logo-greyscales.tif")
            self.assertEqual(img.dimensionality(), 2)
            self.assertEqual(img.dimX(), 331)
            self.assertEqual(img.dimY(), 331)
            self.assertEqual(img.dimZ(), 1)
        except Exception,e:
            self.fail("test_image_double exception group 2: " + str(e))    
 
        try:
            img = CpuImageFLOAT(1024, 1024)
            self.assertEqual(img.dimensionality(), 2)
            self.assertEqual(img.dimX(), 1024)
            self.assertEqual(img.dimY(), 1024)
            self.assertEqual(img.dimZ(), 1)
        except Exception,e:
            self.fail("test_image_double exception group 3: " + str(e))

        try:
            img = CpuImageFLOAT(4, 1, 1, "0,1,2,3", True)
            self.assertEqual(img.dimensionality(), 1)
            self.assertEqual(img.dimX(), 4)
            self.assertEqual(img.dimY(), 1)
            self.assertEqual(img.dimZ(), 1)
            n = img.dimX()
            for i in range(0,n):
                self.assertEqual(img[i], float(i))
        except Exception,e:
            self.fail("test_image_double exception group 5: " + str(e))    

if __name__ == '__main__':
    unittest.main()



