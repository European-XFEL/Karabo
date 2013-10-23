__author__="Irina Kozlova <irina.kozlova at xfel.eu>"
__date__ ="$Jul 2, 2013$"

''' @@@Test is commented for now as it is commented in karathon.cc
    @@@(corresponds to test in karabo: xip_test/Image_Test.cc )
import unittest
from karabo.karathon import *

class  Image_TestCase(unittest.TestCase):
    def setUp(self):
        self.resourcesdir = "../../../src/pythonKarabo/tests/resources/"
    
    def test_image_double(self):
        try:
            img = ImageDOUBLE(CPU)
            self.assertTrue(img.isEmpty())
            self.assertEqual(img.byteSize(), 0)
        except Exception as e:
            self.fail("test_image_double exception group 1: " + str(e))
        
        try:
            img = ImageDOUBLE(CPU, self.resourcesdir+"in-3-3-3.asc")
            self.assertEqual(img.dimensionality(), 3)
            self.assertEqual(img.dimX(), 3)
            self.assertEqual(img.dimY(), 3)
            self.assertEqual(img.dimZ(), 3)
            self.assertEqual(img(2, 2, 2), 222)
            self.assertEqual(img(1, 0, 2), 102)
        except Exception as e:
            self.fail("test_image_double exception group 2: " + str(e))    
            
        try:
            img = ImageDOUBLE(CPU, 1024, 1024)
            self.assertEqual(img.dimensionality(), 2)
            self.assertEqual(img.dimX(), 1024)
            self.assertEqual(img.dimY(), 1024)
            self.assertEqual(img.dimZ(), 1)
        except Exception as e:
            self.fail("test_image_double exception group 3: " + str(e))
            
        try:
            img = ImageDOUBLE(CPU, 10, 1, 1, 5.5)
            self.assertEqual(img.dimensionality(), 1)
            self.assertEqual(img.dimX(), 10)
            self.assertEqual(img.dimY(), 1)
            self.assertEqual(img.dimZ(), 1)
            n = img.dimX()
            for i in range(0,n):
                self.assertEqual(img[i], 5.5)          
        except Exception as e:
            self.fail("test_image_double exception group 4: " + str(e))    
        
        try:
            img = ImageDOUBLE(CPU, 4, 1, 1, "0,1,2,3", True)
            self.assertEqual(img.dimensionality(), 1)
            self.assertEqual(img.dimX(), 4)
            self.assertEqual(img.dimY(), 1)
            self.assertEqual(img.dimZ(), 1)
            n = img.dimX()
            for i in range(0,n):
                self.assertEqual(img[i], float(i))
        except Exception as e:
            self.fail("test_image_double exception group 5: " + str(e))    
        

    def test_image_float(self):
        try:
            img = ImageFLOAT(CPU)
            self.assertTrue(img.isEmpty())
            self.assertEqual(img.byteSize(), 0)
        except Exception as e:
            self.fail("test_image_float exception group 1: " + str(e))
        
        try:
            img = ImageFLOAT(CPU, self.resourcesdir+"in-3-3-3.asc")
            self.assertEqual(img.dimensionality(), 3)
            self.assertEqual(img.dimX(), 3)
            self.assertEqual(img.dimY(), 3)
            self.assertEqual(img.dimZ(), 3)
            self.assertEqual(img(2, 2, 2), 222)
            self.assertEqual(img(1, 0, 2), 102)
        except Exception as e:
            self.fail("test_image_float exception group 2: " + str(e))       
        
        try:
            img = ImageFLOAT(CPU, 1024, 1024)
            self.assertEqual(img.dimensionality(), 2)
            self.assertEqual(img.dimX(), 1024)
            self.assertEqual(img.dimY(), 1024)
            self.assertEqual(img.dimZ(), 1)
        except Exception as e:
            self.fail("test_image_float exception group 3: " + str(e))
            
        try:
            img = ImageFLOAT(CPU, 10, 1, 1, 5.5)
            self.assertEqual(img.dimensionality(), 1)
            self.assertEqual(img.dimX(), 10)
            self.assertEqual(img.dimY(), 1)
            self.assertEqual(img.dimZ(), 1)
            n = img.dimX()
            for i in range(0,n):
                self.assertEqual(img[i], 5.5)          
        except Exception as e:
            self.fail("test_image_float exception group 4: " + str(e))    
        
        try:
            img = ImageFLOAT(CPU, 4, 1, 1, "0,1,2,3", True)
            self.assertEqual(img.dimensionality(), 1)
            self.assertEqual(img.dimX(), 4)
            self.assertEqual(img.dimY(), 1)
            self.assertEqual(img.dimZ(), 1)
            n = img.dimX()
            for i in range(0,n):
                self.assertEqual(img[i], float(i))
        except Exception as e:
            self.fail("test_image_float exception group 5: " + str(e))    
    
if __name__ == '__main__':
    unittest.main()
'''