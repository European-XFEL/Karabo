__author__="Irina Kozlova <irina.kozlova at xfel.eu>"
__date__ ="$Sep 30, 2013$"


import unittest
from karabo.karathon import *
from karabo.decorators import *
from configuration_test_classes import SomeClass

class  CpuImage_TestCase(unittest.TestCase):
    def setUp(self):
        self.resourcesdir = "../../../src/pythonKarabo/tests/resources/"
    
    def test_cpuimage_float(self):
        try:
            img = CpuImageFLOAT()
            self.assertTrue(img.isEmpty())
            self.assertEqual(img.byteSize(), 0)
        except Exception as e:
            self.fail("test_cpuimage_float exception group 1: " + str(e))
        
        try:
            img = CpuImageFLOAT(self.resourcesdir+"european-xfel-logo-greyscales.tif")
            self.assertEqual(img.dimensionality(), 2)
            self.assertEqual(img.dimX(), 331)
            self.assertEqual(img.dimY(), 331)
            self.assertEqual(img.dimZ(), 1)
            self.assertEqual(img.pixelType(), "FLOAT")
            self.assertEqual(img.getStatistics().getMin(), 37.0)
            self.assertEqual(img.getStatistics().getMax(), 255.0)
        except Exception as e:
            self.fail("test_image_double exception group 2: " + str(e))    
 
        try:
            img = CpuImageFLOAT(1024, 1024)
            self.assertEqual(img.dimensionality(), 2)
            self.assertEqual(img.dimX(), 1024)
            self.assertEqual(img.dimY(), 1024)
            self.assertEqual(img.dimZ(), 1)
        except Exception as e:
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
        except Exception as e:
            self.fail("test_image_double exception group 5: " + str(e)) 

    def test_cpuimage_read(self):
        try:
            img = CpuImageDOUBLE()
            img.read(self.resourcesdir+"european-xfel-logo-greyscales.tif")
            self.assertEqual(img.dimensionality(), 2)
            self.assertEqual(img.dimX(), 331)
            self.assertEqual(img.dimY(), 331)
            self.assertEqual(img.dimZ(), 1)
            self.assertEqual(img.pixelType(), "DOUBLE")
            self.assertEqual(img.getStatistics().getMin(), 37.0)
            self.assertEqual(img.getStatistics().getMax(), 255.0)
        except Exception as e:
            self.fail("test_cpuimage_read exception: " + str(e))            
            
    def test_cpuimage_assign(self):
        try:
            fileName = self.resourcesdir+"european-xfel-logo-greyscales.tif"
            img = CpuImageUINT8(fileName)
            img2 = CpuImageUINT8()
            img2.assign(img)
            self.assertEqual(img2.dimensionality(), 2)
            self.assertEqual(img2.dimX(), 331)
            self.assertEqual(img2.dimY(), 331)
            self.assertEqual(img2.dimZ(), 1)
            self.assertEqual(img2.pixelType(), "UINT8")
            self.assertEqual(img2.getStatistics().getMin(), 37.0)
            self.assertEqual(img2.getStatistics().getMax(), 255.0)
        except Exception as e:
            self.fail("test_cpuimage_assign exception: " + str(e)) 
        
    def test_cpuimage_png_read(self):
        try:
            img = CpuImageDOUBLE()
            img.read(self.resourcesdir+"image_0001A.png")
            self.assertEqual(img.dimensionality(), 2)
            self.assertEqual(img.dimX(), 800)
            self.assertEqual(img.dimY(), 600)
            self.assertEqual(img.dimZ(), 1)
            self.assertEqual(img.pixelType(), "DOUBLE")
            self.assertEqual(img.getStatistics().getMin(), 0.0)
            self.assertEqual(img.getStatistics().getMax(), 255.0)
        except Exception,e:
            self.fail("test_cpuimage_png_read exception: " + str(e))        
            
if __name__ == '__main__':
    unittest.main()



