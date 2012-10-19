'''
Created on Oct 17, 2012

@author: irinak
'''

import unittest
from libkarabo import *

class  Hash_TestCase(unittest.TestCase):
    #def setUp(self):
    #    self.foo = Hash_()
    #

    #def tearDown(self):
    #    self.foo.dispose()
    #    self.foo = None

    def test_setgetfrompath_(self):
        h = Hash()
        h.setFromPath("a.b.c", 15)
        h.setFromPath("a.b.d", "myString")
        h.setFromPath("a.b.e", 5.5)
        
        self.assertEqual(h.getFromPath("a.b.c"), 15, "getFromPath integer")
        self.assertEqual(h.getFromPath("a.b.d"), "myString", "getFromPath string")
        self.assertEqual(h.getFromPath("a.b.e"), 5.5, "getFromPath double")
        
        myVecInt=[5,10,15]
        h.setFromPath("a|b|myVecInt", myVecInt, "|")

        myVecString=["Hallo", "test1 and test2", "new text"]
        h.setFromPath("a-b-myVecString", myVecString, "-")
        
        myVecDouble=[1.1, 2.2, 5.5]
        h.setFromPath("a.b.myVecDouble", myVecDouble)
        
        self.assertEqual(h.getFromPath("a.b.myVecInt"), [5, 10, 15], "getFromPath vector integer")
        self.assertEqual(h.getFromPath("a.b.myVecString"), ["Hallo", "test1 and test2", "new text"], "getFromPath vector string")
        self.assertEqual(h.getFromPath("a.b.myVecDouble"), [1.1, 2.2, 5.5], "getFromPath vector double")
        self.assertEqual(h.getTypeAsId("a"), Types.HASH, "getTypeAsId type HASH" )
        self.assertEqual(h.getTypeAsString("a"), "HASH", "getTypeAsString type HASH")    
        
if __name__ == '__main__':
    unittest.main()

