'''
Created on Oct 17, 2012

@author: irinak
'''

import unittest
from libkarabo import *

class  Hash_TestCase(unittest.TestCase):

    def test_setget_(self):
        #======= Testing h.set() ========
        h = Hash()
        h.set("integerValue", 5)
        h.set("stringValue", "Hello World")
        h.set("intMinusValue", -5)
        h.setAsBool("boolValue", True)
        h.set("doubleValue", 1.5)
        #print "printing hash 'h' :\n", h

        #======= Testing h.get() ======= 
        hGetIntValue = h.get("integerValue")
        self.assertEqual(hGetIntValue, 5)
        
        hGetStringValue = h.get("stringValue")
        self.assertEqual(hGetStringValue, "Hello World")

        hGetDoubleAsString = h.getAsString("doubleValue")
        self.assertEqual(hGetDoubleAsString, "1.500000")
        
        self.assertEqual(h.get("doubleValue"), 1.500000)

        hGetBoolAsString = h.getAsString("boolValue")
        self.assertEqual(hGetBoolAsString, "true")
        
        self.assertEqual(h.get("boolValue"), True)

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
        
        #======= Testing h.getLeaves() =======
        hVec = h.getLeaves()
        self.assertEqual(hVec, ['a.b.c', 'a.b.d', 'a.b.e', 'a.b.myVecDouble', 'a.b.myVecInt', 'a.b.myVecString'])

        #======= Testing h.getKeys() =======
        h.set("x", 10)
        h.set("y", 15)
        hKeys = h.getKeys()
        self.assertEqual(hKeys, ['a', 'x', 'y'])
        
        #======= Testing h.has() =======
        self.assertEqual(h.has("a"), True)
        self.assertEqual(h.has("x"), True)
        self.assertEqual(h.has("y"), True)
        hSub = h.getFromPath("a.b")
        self.assertEqual(hSub.has("c"), True)
        self.assertEqual(hSub.has("myVecString"), True)
        
    def test_append_update_(self):

        #===== Test functions: append, update =====
        h1=Hash()
        h1.setFromPath("a.b.c", 1)
        h1.setFromPath("a.b.d", 2)
        h1.setFromPath("a.b.e", 3)
        h1.set("x", 15)
        
        h2=Hash()
        h2.setFromPath("a.b.f", 4)
        h2.setFromPath("a.b.d", 22)
        h2.setFromPath("y", 10)

        hApp = h1.append(h2)
        self.assertEqual(hApp.getFromPath("a.b.d"), 22)  #value of key 'a.b.d' in now 22
        self.assertEqual(hApp.getFromPath("a.b.f"), 4) 
        self.assertEqual(hApp.get("x"), 15)
        self.assertEqual(hApp.get("y"), 10)
        self.assertEqual(hApp.hasFromPath("a.b.c"), False) #key 'a.b.c' does not exist
        self.assertEqual(hApp.hasFromPath("a.b.e"), False) #key 'a.b.e' does not exist

        #print 'hApp=h1.append(h2) , print hApp : \n', hApp
        
        #hash 'h2' as above. we create new hash 'h3' and update it by 'h2'
        h3=Hash()
        h3.setFromPath("a.b.c", 1)
        h3.setFromPath("a.b.d", 2)
        h3.setFromPath("a.b.e", 3)
        h3.set("z", 7)

        h3.update(h2)
        #check updated 'h3'
        self.assertEqual(h3.getFromPath("a.b.c"), 1)
        self.assertEqual(h3.getFromPath("a.b.d"), 22)
        self.assertEqual(h3.getFromPath("a.b.e"), 3)
        self.assertEqual(h3.getFromPath("a.b.f"), 4)
        self.assertEqual(h3.get("y"), 10)
        self.assertEqual(h3.get("z"), 7)

    def test_vectorhash1_(self):
        #===== Test VECTOR_HASH ======
        h4 = Hash()
        h4.setFromPath("a[0].b", 10)
        h4.setFromPath("a[0].c", 20)
        h4.setFromPath("a[1].c", "Hallo World")
        
        #print 'h4: \n', h4
        #v4=h4.get("a")
        #print "\nShow v4 in loop:"
        #for x in v4: print x
        
        self.assertEqual(h4.getFromPath("a[0].b"), 10)
        self.assertEqual(h4.getFromPath("a[0].c"), 20)
        self.assertEqual(h4.getFromPath("a[1].c"), "Hallo World")
        
        self.assertEqual(h4.isFromPath("a[1].c", Types.STRING), True)
        self.assertEqual(h4.getTypeAsString("a"), "VECTOR_HASH")

        h5 = Hash()
        h5.setFromPath("a.b.c", 15)
        h5.setFromPath("a.b.d", "myString")
        h5.setFromPath("a.b.e", 5.5)
        h5.setFromPath("a.b.v[0].first", 10)
        h5.setFromPath("a.b.v[0].next", 7.7)
        h5.setFromPath("a.b.v[1].second", -30)
        h5.setFromPath("a.b.v[1].doubleVal", -5.5)
        
        self.assertEqual(h5.isFromPath("a.b.v[0].first", Types.INT32), True)
        self.assertEqual(h5.getFromPath("a.b.v[0].first"), 10)
        
        self.assertEqual(h5.isFromPath("a.b.v", Types.VECTOR_HASH), True)
        self.assertEqual(h5.getFromPath("a.b.v[1].doubleVal"), -5.5)
        
        #print 'h5: \n', h5
        #v5=h5.getFromPath("a.b.v")
        #print "\nShow v5 in loop:"
        #for x in v5: print x

    def test_hasfrompath_isfrompath_(self):
        #======= Test functions: hasFromPath, isFromPath =======
        h=Hash()
        h.setFromPath("a.b.c", 44)
        r1 = h.hasFromPath("a") 
        self.assertEqual(r1, True)
        
        r2 = h.hasFromPath("a.b") 
        self.assertEqual(r2, True)
        
        self.assertTrue(h.hasFromPath("a.b.c"))
        
        self.assertFalse(h.hasFromPath("a.b.w"))

        #add additional elements to hash 'h'
        h.setFromPath("a.b.s", "Hallo")
        h.setFromPath("a.b.doubleVal", -5.5)
        
        t1 = h.isFromPath("a.b", Types.HASH)
        self.assertEqual(t1, True)
        
        t2 = h.isFromPath("a.b.c", Types.INT32)
        self.assertEqual(t2, True)

        self.assertTrue(h.isFromPath("a.b.s", Types.STRING))
        
        #element 'a.b.s' is not of type INT32
        self.assertEqual(h.isFromPath("a.b.s", Types.INT32), False)
        
        self.assertEqual(h.isFromPath("a.b.doubleVal", Types.DOUBLE), True)
        

    def test_hasfrompath_isfrompath_(self):
        #======= Test function eraseFromPath =======
        '''
        Erase leaf of Hash tree represented by path
        returns 0 if no erase, and 1 if the leaf was erased
        '''
        h = Hash()
        h.setFromPath("a.b.c", 15)
        h.setFromPath("a.b.d", "myString")
        h.setFromPath("a.b.e", 5.5)
        
        #Erase 'a.b.e'
        check = h.eraseFromPath("a&b&e", "&")
        self.assertEqual(check, 1)
        self.assertFalse(h.hasFromPath("a.b.e"))
    
        #Erase 'a.b.d' and check that 'h' has no key 'a.b.d' any more
        h.eraseFromPath("a.b.d")
        self.assertFalse(h.hasFromPath("a.b.d"))
        
        #Erase not existing element 'x' 
        self.assertEqual(h.eraseFromPath("a.b.x"), 0)
         
        self.assertEqual(h.eraseFromPath("a.b"), 1)
        
        self.assertEqual(h.eraseFromPath("a"), 1)
        self.assertTrue(h.empty())

if __name__ == '__main__':
    unittest.main()

