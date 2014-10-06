__author__="Irina Kozlova <irina.kozlova at xfel.eu>"
__date__ ="$Sep 30, 2013$"


import unittest
from karabo.karathon import *
from karabo.decorators import *
from configuration_test_classes import SomeClass
from PIL import Image

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
            img = CpuImageUINT8(self.resourcesdir+"european-xfel-logo-greyscales.tif")
            self.assertEqual(img.dimensionality(), 2)
            self.assertEqual(img.dimX(), 331)
            self.assertEqual(img.dimY(), 331)
            self.assertEqual(img.dimZ(), 1)
            self.assertEqual(img.pixelType(), "UINT8")
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

    def test_cpuimage_read_tif(self):
        try:
            img = CpuImageUINT8()
            img.read(self.resourcesdir+"european-xfel-logo-greyscales.tif")
            self.assertEqual(img.dimensionality(), 2)
            self.assertEqual(img.dimX(), 331)
            self.assertEqual(img.dimY(), 331)
            self.assertEqual(img.dimZ(), 1)
            self.assertEqual(img.pixelType(), "UINT8")
            self.assertEqual(img.getStatistics().getMin(), 37.0)
            self.assertEqual(img.getStatistics().getMax(), 255.0)
        except Exception as e:
            self.fail("test_cpuimage_read_tif exception: " + str(e))
            
    def test_cpuimage_save(self):
        try:
            img = CpuImageUINT8(self.resourcesdir+"european-xfel-logo-greyscales.tif")
            Image.fromarray(img.getData()).save("/tmp/logo_savedFile.tif")
        except Exception as e:
            self.fail("test_cpuimage_save exception: " + str(e))                 
    
    def test_cpuimage_getheader(self):  
        try:
            img = CpuImageUINT8()
            img.read(self.resourcesdir+"european-xfel-logo-greyscales.tif")
            h = img.getHeader()
            self.assertEqual(h.get("__dimX"), 331)
            self.assertEqual(h.get("__dimY"), 331)
            self.assertEqual(h.get("__dimZ"), 1)
        except Exception as e:
            self.fail("test_cpuimage_getheader exception: " + str(e)) 
    
    def test_cpuimage_getstatistics(self):
        try:
            img = CpuImageUINT8()
            img.read(self.resourcesdir+"european-xfel-logo-greyscales.tif")
            self.assertEqual(img.pixelType(), "UINT8")
            self.assertEqual(img.size(), 109561)
            self.assertEqual(img.byteSize(), 109561)
            statistics = img.getStatistics()
            self.assertEqual(statistics.getMin(), 37.0)
            self.assertEqual(statistics.getMax(), 255.0)
            self.assertEqual(statistics.getXmin(), 0)
            self.assertEqual(statistics.getYmin(), 0)
            self.assertEqual(statistics.getZmin(), 0)
            self.assertEqual(statistics.getXmax(), 198)
            self.assertEqual(statistics.getYmax(), 0)
            self.assertEqual(statistics.getZmax(), 0)
            ##print statistics:
            #statistics.printStatistics()
        except Exception as e:
            self.fail("test_cpuimage_getstatistics exception: " + str(e))
            
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
          
    def test_cpuimage_getData(self):
        try:
            img = CpuImageUINT8(self.resourcesdir+"european-xfel-logo-greyscales.tif")
            pix = img.getData()
            self.assertEqual(str(pix.dtype), 'uint8')
            self.assertEqual(pix.ndim, 2)
            self.assertEqual(pix.shape[0], 331)
            self.assertEqual(pix.shape[1], 331)
            self.assertEqual(pix.size, 109561)        
        except Exception as e:
            self.fail("test_cpuimage_getData exception: " + str(e))
            
    def test_cpuimage_assign_ndarray(self):
        try:
            img = CpuImageUINT8(self.resourcesdir+"european-xfel-logo-greyscales.tif")
            pix = img.getData()
            
            img2 = CpuImageUINT8()
            img2.assign(pix)
            
            self.assertEqual(img.dimensionality(), img2.dimensionality())
            self.assertEqual(img.dimX(), img2.dimX())
            self.assertEqual(img.dimY(), img2.dimY())
            self.assertEqual(img.dimZ(), img2.dimZ())
            self.assertEqual(img.pixelType(), img2.pixelType())
            self.assertEqual(img.byteSize(), img2.byteSize())
            
            self.assertEqual(img2.dimensionality(), 2)
            self.assertEqual(img2.dimX(), 331)
            self.assertEqual(img2.dimY(), 331)
            self.assertEqual(img2.dimZ(), 1)
            self.assertEqual(img2.pixelType(), "UINT8")
            self.assertEqual(img2.byteSize(), 109561)
            self.assertEqual(img2.getStatistics().getMin(), 37.0)
            self.assertEqual(img2.getStatistics().getMax(), 255.0)
        
        except Exception as e:
            self.fail("test_cpuimage_assign_ndarray exception: " + str(e))         
        
    def test_cpuimage_read_png(self):
        try:
            img = CpuImageUINT8()
            img.read(self.resourcesdir+"image_0001A.png")
            self.assertEqual(img.dimensionality(), 2)
            self.assertEqual(img.dimX(), 800)
            self.assertEqual(img.dimY(), 600)
            self.assertEqual(img.dimZ(), 1)
            self.assertEqual(img.pixelType(), "UINT8")
            self.assertEqual(img.getStatistics().getMin(), 0.0)
            self.assertEqual(img.getStatistics().getMax(), 255.0)
        except Exception as e:
            self.fail("test_cpuimage_read_png exception: " + str(e))        
            
if __name__ == '__main__':
    unittest.main()



