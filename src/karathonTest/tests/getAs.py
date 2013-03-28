import unittest
import numpy as np
from libkarathon import *


class  GetAsTestCase(unittest.TestCase):

    def test_getAs(self):
        
        try:
            h = Hash("a", True)
            self.assertEqual(h.getAs("a", Types.STRING), "1", 'Should return "1" as python string')
            self.assertEqual(h.getAs("a", Types.INT32), 1, 'Should return 1 as an python int')
            self.assertEqual(h.getAs("a", Types.INT64), 1L, 'Should return 1L as python long')
            self.assertEqual(h.getAs("a", Types.FLOAT), 1.0, 'Should return 1.0 as python float')
            self.assertEqual(h.getAs("a", Types.DOUBLE), 1.0, 'Should return 1.0 as python float')
        except Exception,e:
            self.fail("test_getAs exception group 1: " + str(e))

        try:
            h = Hash("a", True)
            h.setAttribute("a", "a", True)
            self.assertEqual(h.getAttributeAs("a","a",Types.STRING), "1", 'Should return "1" as python string')
            self.assertEqual(h.getAttributeAs("a","a", Types.INT32), 1, 'Should return 1 as python int')
            self.assertEqual(h.getAttributeAs("a","a", Types.DOUBLE), 1.0, 'Should return 1.0 as python float')
            h.setAttribute("a", "b", 12)
            h.setAttribute("a", "c", 1.23)
            attrs = h.getAttributes("a")
            g = Hash("Z.a.b.c", "value")
            g.setAttributes("Z.a.b.c", attrs)
            self.assertEqual(g.getAttributeAs("Z.a.b.c","a", Types.STRING), "1", 'Should return "1" as python string')
            self.assertEqual(g.getAttributeAs("Z.a.b.c","a", Types.INT32), 1, 'Should return 1 as python int')
            self.assertEqual(g.getAttributeAs("Z.a.b.c","a", Types.DOUBLE), 1.0, 'Should return 1.0 as python float')
        
        except Exception,e:
            self.fail("test_getAs exception group 2: " + str(e))

        try:
            h = Hash("a", np.array([False,False,False,False])) # value is numpy array of boolean -> std::vector<bool>
            self.assertEqual(h.getAs("a", Types.STRING), "0,0,0,0", 'Should return "0,0,0,0" as python string')
            self.assertEqual(h.getAs("a", Types.VECTOR_INT32)[3], 0, "Should return 0")
        except Exception,e:
            self.fail("test_getAs exception group 3: " + str(e))

        try:
            h = Hash("a", [False,False,False,False])     # value is python list of boolean -> std::vector<bool>
            self.assertEqual(h.getAs("a", Types.STRING), "0,0,0,0", 'Should return "0,0,0,0" as python string')
            self.assertEqual(h.getAs("a", Types.VECTOR_INT32)[3], 0, "Should return 0")
        except Exception,e:
            self.fail("test_getAs exception group 4: " + str(e))

        try:
            h = Hash("a", bytearray(['1','2','3','4']))   # value is python bytearray -> std::vector<char>
            self.assertEqual(h.getAs("a", Types.STRING), "1,2,3,4", 'Should return "1,2,3,4" as python string')
            self.assertEqual(h.getAs("a", Types.VECTOR_INT32)[3], 4, "Should return 4")
        except Exception,e:
            self.fail("test_getAs exception group 4: " + str(e))

        try:
            h = Hash("a", ['1','2','3','4'])              # value is python list -> std::vector<char>
            self.assertEqual(h.getAs("a", Types.STRING), "1,2,3,4", 'Should return "1,2,3,4" as python string')
            self.assertEqual(h.getAs("a", Types.VECTOR_INT32)[3], 4, "Should return 4")
        except Exception,e:
            self.fail("test_getAs exception group 5: " + str(e))
            
        try:
            h = Hash("a", [13,13,13,13])
            self.assertEqual(h.getAs("a", Types.STRING), "13,13,13,13")
        except Exception,e:
            self.fail("test_getAs exception group 6: " + str(e))
            
        try:
            h = Hash("a", -42L)
        except Exception,e:
            self.fail("test_getAs exception group 7: " + str(e))
            
        try:
            h = Hash("a", [-42L])
            self.assertEqual(h.getAs("a", Types.STRING), "-42", 'Should return "-42" as str')
        except Exception,e:
            self.fail("test_getAs exception group 8: " + str(e))
            
        try:
            h = Hash("a", np.array([-42L]))
            self.assertEqual(h.getAs("a", Types.STRING), "-42", 'Should return "-42" as str')
        except Exception,e:
            self.fail("test_getAs exception group 9: " + str(e))
            
        try:
            h = Hash("a", np.array([], dtype=int))
            self.assertEqual(h.getAs("a", Types.STRING), "", 'Should return empty str')
        except Exception,e:
            self.fail("test_getAs exception group 10: " + str(e))
            
        try:
            h = Hash("a", -2147483647L)
            self.assertEqual(h.getAs("a", Types.STRING), "-2147483647", 'Should return "-2147483647" str')
        except Exception,e:
            self.fail("test_getAs exception group 11: " + str(e))
            
        try:
            h = Hash("a", 1234567890123456789L)
            self.assertEqual(h.getAs("a", Types.STRING), "1234567890123456789", 'Should return "1234567890123456789" str')
            self.assertEqual(h.getType("a"), "INT64")
        except Exception,e:
            self.fail("test_getAs exception group 12: " + str(e))
            
        try:
            h = Hash("a", 0.123456789123456)
            self.assertEqual(h.getAs("a", Types.STRING), "0.123456789123456", 'Should return "0.123456789123456" str')
            self.assertEqual(h.getTypeAsId("a"), Types.DOUBLE)
        except Exception,e:
            self.fail("test_getAs exception group 13: " + str(e))
            
        

if __name__ == '__main__':
    unittest.main()

