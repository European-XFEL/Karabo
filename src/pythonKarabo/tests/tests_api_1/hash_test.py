'''
Created on Oct 17, 2012

@author: Sergey Esenov <serguei.essenov@xfel.eu>, Irina Kozlova <irina.kozlova@xfel.eu>
'''

import unittest
import numpy as np
from karabo.karathon import *
import copy

class  Hash_TestCase(unittest.TestCase):

    def test_constructors(self):
        
        # Check properties of empty Hash
        try:
            h = Hash()
            self.assertEqual(len(h), 0)
            self.assertTrue(h.empty())
        except Exception as e:
            self.fail("test_constructors exception group 1: " + str(e))

        # Check Hash with one property
        try:
            h = Hash('a', 1)
            self.assertEqual(len(h), 1)
            self.assertFalse(h.empty())
            self.assertEqual(h.get('a'), 1)
            self.assertEqual(h['a'], 1)
        except Exception as e:
            self.fail("test_constructors exception group 2: " + str(e))
            
        # Check Hash with 2 properties
        try:
            h = Hash('a', 1, 'b', 2.0)
            self.assertFalse(h.empty())
            self.assertEqual(len(h), 2)
            self.assertEqual(h['a'], 1)
            self.assertEqual(h['b'], 2.0)
        except Exception as e:
            self.fail("test_constructors exception group 3: " + str(e))
        
        # Check Hash with 6 properties of different types
        try:
            h = Hash("a.b.c", 1, "b.c", 2.0, "c", 3.7, "d.e", "4",
                    "e.f.g.h", [5,5,5,5,5], "F.f.f.f.f", Hash("x.y.z", 99))
            self.assertFalse(h.empty())
            self.assertEqual(len(h), 6)
            self.assertEqual(h['a.b.c'], 1)
            self.assertEqual(h['b.c'], 2.0)
            self.assertEqual(h['c'], 3.7)
            self.assertEqual(h['d.e'], "4")
            self.assertEqual(h['e.f.g.h'][0], 5)
            self.assertEqual(len(h['e.f.g.h']), 5)
            self.assertEqual(h['F.f.f.f.f']['x.y.z'], 99)
            self.assertEqual(h['F.f.f.f.f.x.y.z'], 99)
            self.assertEqual(h['F']['f']['f']['f']['f']['x']['y']['z'], 99)
            # Make Hash flat
            flat = Hash()
            Hash.flatten(h, flat)
            self.assertFalse(flat.empty())
            self.assertEqual(len(flat), 6)
            self.assertEqual(flat.get('a.b.c', ' '), 1)
            self.assertEqual(flat.get('b.c', ' '), 2.0)
            self.assertEqual(flat.get('c', ' '), 3.7)
            self.assertEqual(flat.get('d.e', ' '), "4")
            self.assertEqual(flat.get('e.f.g.h', ' ')[0], 5)
            self.assertEqual(len(flat.get('e.f.g.h', ' ')), 5)
            self.assertEqual(flat.get('F.f.f.f.f.x.y.z', ' '), 99)
            
            # Make flat Hash unflatten again
            tree = Hash()
            flat.unflatten(tree)
            self.assertFalse(tree.empty())
            self.assertEqual(len(tree), 6)
            self.assertEqual(tree['a.b.c'], 1)
            self.assertEqual(tree['b.c'], 2.0)
            self.assertEqual(tree['c'], 3.7)
            self.assertEqual(tree['d.e'], "4")
            self.assertEqual(tree['e.f.g.h'][0], 5)
            self.assertEqual(len(tree['e.f.g.h']), 5)
            self.assertEqual(tree['F.f.f.f.f']['x.y.z'], 99)
            self.assertEqual(tree['F.f.f.f.f.x.y.z'], 99)
        except Exception as e:
            self.fail("test_constructors exception group 4: " + str(e))
        

    def test_getSet(self):
        
        try:
            h = Hash()
            h.set("a.b.c1.d", 1)
            self.assertTrue(h.get("a").has("b"), '"b" not found')
            self.assertTrue(h.get("a.b").has("c1"), '"c1" not found')
            self.assertTrue(h.get("a.b.c1").has("d"), '"d" not found')
            self.assertEqual(h.get("a.b.c1.d"), 1, '"get" should return 1')
            self.assertTrue(h.has("a.b.c1.d"), '"a.b.c1.d" key not found')
            self.assertIn("a.b.c1.d", h, "test_getSet group 1: h __contains__ \"a.b.c1.d\" failed")
            self.assertTrue(h.get("a").has("b.c1"), '"b.c1" key not found')
            self.assertIn("b.c1", h["a"], "test_getSet group 1: h['a'] __contains__ \"b.c1\" failed")
        
            h.set("a.b.c2.d", 2.0)
            self.assertTrue(h.get("a.b").has("c2.d"), '"c2.d" not found')
            self.assertTrue(h.get("a.b").has("c1.d"), '"c1.d" not found')
            self.assertEqual(h.get("a.b.c1.d"), 1, '"get" should return 1')
            self.assertEqual(h.get("a.b.c2.d"), 2.0, '"get" should return 2.0')
            
            h.set("a.b[0]", Hash("a", 1))
            self.assertTrue(h.get("a").has("b"), "'b' not found")
            self.assertTrue(h["a"].has("b"), "'b' not found")
            self.assertIn("b", h["a"], "test_getSet group 1: h['a'] __contains__ \"b\" failed")
            self.assertEqual(len(h.get("a")), 1, "'len' should give 1")
            self.assertEqual(len(h["a"]), 1, "'len' should give 1")
            self.assertEqual(len(h.get("a.b")), 1, "'len' should give 1")
            self.assertEqual(len(h["a.b"]), 1, "'len' should give 1")
            self.assertEqual(len(h.get("a.b")[0]), 1, "'len' should give 1")
            self.assertEqual(len(h["a.b"][0]), 1, "'len' should give 1")
            self.assertEqual(h["a.b"][0]["a"], 1, '"get" should return 1')
            self.assertEqual(h["a.b[0].a"], 1, '"get" should return 1')
            self.assertEqual(h.get("a.b")[0].get("a"), 1, '"get" should return 1')
            self.assertEqual(h.get("a.b[0].a"), 1, '"get" should return 1')
            
            h.set("a.b[2]", Hash("a", 1))
            self.assertTrue(h.get("a").has("b"), "'b' not found")
            self.assertEqual(len(h["a"]), 1, "'len' should give 1")
            self.assertEqual(len(h["a.b"]), 3, "'len' should give 3")   # 0, 1, 2
            self.assertEqual(h["a.b[0].a"], 1, '"get" should return 1')
            self.assertEqual(h["a.b[2].a"], 1, '"get" should return 1')
            self.assertEqual(h["a.b"][0]["a"], 1, '"get" should return 1')
            self.assertEqual(h["a.b"][2]["a"], 1, '"get" should return 1')
            self.assertTrue(h["a.b"][1].empty(), 'h["a.b"][1] should be empty Hash')
            
        except Exception as e:
            self.fail("test_getSet exception group 1: " + str(e))
            
        try:
            h = Hash()
            h["a.b.c"] = 1    # statement is equivalent to h.set("a.b.c", 1)
            h["a.b.c"] = 2
            self.assertEqual(h["a.b.c"], 2, "Value should be overwritten by 2")
            self.assertTrue(h.has("a.b"), "Key 'a.b' not found")
            self.assertFalse(h.has("a.b.c.d"), "Key 'a.b.c.d' should not be found")
            # similarity with python dictionary...
            self.assertEqual(h["a"]["b"]["c"], 2)
            h["a"]["b"]["c"] = 77
            self.assertEqual(h["a"]["b"]["c"], 77)
            self.assertEqual(h["a.b.c"], 77)
        except Exception as e:
            self.fail("test_getSet exception group 2: " + str(e))
            
        try:
            h = Hash("a[0]", Hash("a", 1), "a[1]", Hash("a", 1))
            self.assertEqual(h["a[0].a"], 1, "Value should be 1")
            self.assertEqual(h["a[1].a"], 1, "Value should be 1")
            self.assertEqual(h["a"][0]["a"], 1, "Value should be 1")
            self.assertEqual(h["a"][1]["a"], 1, "Value should be 1")
        except Exception as e:
            self.fail("test_getSet exception group 3: " + str(e))
            
        try:
            h = Hash()
            h["x[0].y[0]"] = Hash("a", 4.2, "b", "red", "c", True)
            h["x[1].y[0]"] = Hash("a", 4.0, "b", "green", "c", False)
            self.assertTrue(h["x[0].y[0].c"], "Failure in array case")
            self.assertFalse(h["x[1].y[0].c"], "Failure in array case")
            self.assertEqual(h["x[0].y[0].b"], "red", "Failure in array case")
            self.assertEqual(h["x[1].y[0].b"], "green", "Failure in array case")
        except Exception as e:
            self.fail("test_getSet exception group 4: " + str(e))
            
        try:
            h1 = Hash("a[0].b[0]", Hash("a", 1))
            h2 = Hash("a[0].b[0]", Hash("a", 2))
            self.assertEqual(h1["a[0].b[0].a"], 1, "Value should be equal 1")
            h1["a[0]"] = h2
            self.assertEqual(h1["a[0].a[0].b[0].a"], 2, "Value should be equal 2")
            h1["a"] = h2
            self.assertEqual(h1["a.a[0].b[0].a"], 2, "Value should be equal 2")
        except Exception as e:
            self.fail("test_getSet exception group 5: " + str(e))
            
        try:
            h = Hash()
            b = True
            h["a"] = b
            self.assertEqual(str(h.getType("a")), "BOOL", 'The type should be "BOOL"')
            self.assertEqual(h.getType("a"), Types.BOOL, 'The type ID for "BOOL" should be Types.BOOL')
        except Exception as e:
            self.fail("test_getSet exception group 6: " + str(e))
            
        try:
            h = Hash('a.b.c', [1, 2, 3, 4, 5, 6, 7], 'b.c.d', [False, False, True, True, True, False, True])
            self.assertEqual(isStdVectorDefaultConversion(Types.PYTHON), True)
            self.assertEqual(h.isType('a.b.c', Types.VECTOR_INT32), True)
            self.assertEqual(str(type(h['a.b.c'])), "<class 'list'>")
            try:
                setStdVectorDefaultConversion(Types.VECTOR_INT32)
            except RuntimeError as e:
                pass
            self.assertEqual(isStdVectorDefaultConversion(Types.PYTHON), True)
            setStdVectorDefaultConversion(Types.NUMPY)
            self.assertEqual(isStdVectorDefaultConversion(Types.PYTHON), False)
            self.assertEqual(isStdVectorDefaultConversion(Types.NUMPY), True)
            self.assertEqual(str(type(h['a.b.c'])), "<class 'numpy.ndarray'>")
            self.assertEqual(str(type(h['b.c.d'])), "<class 'numpy.ndarray'>")
            setStdVectorDefaultConversion(Types.PYTHON)
            self.assertEqual(isStdVectorDefaultConversion(Types.PYTHON), True)
        except Exception as e:
            self.fail("test_getSet exception group 7: " + str(e))
            
    def test_getSetVectorHash(self):
        try:
            h = Hash('a', VectorHash())
            g = [Hash('b', 1), Hash('b',2)]      # python list of Hashes
            vh = h['a']    # get the reference because value is VectorHash
            vh.extend(g)
            g1 = (Hash('c',10), Hash('c',20),)   # python tuple of Hashes
            vh.extend(g1)  # "extend" lists, tuples, VectorHash objects
            vh.append(Hash('d',100))  # "append" Hash object
            self.assertEqual(len(vh), 5)
            self.assertEqual(h['a[4].d'], 100)
            h2 = copy.copy(h)
            self.assertTrue(h == h2)
            vh[4]['d'] = 999
            self.assertEqual(h['a[4].d'], 999)
            #self.assertFalse(h == h2)
            
        except Exception as e:
            self.fail("test_getSetVectorHash exception group 1: " + str(e))
        
        
    def test_getAs(self):
        
        try:
            h = Hash("a", True)
            self.assertEqual(h.getAs("a", Types.STRING), "1", 'Should return "1" as python string')
            self.assertEqual(h.getAs("a", Types.INT32), 1, 'Should return 1 as an python int')
            self.assertEqual(h.getAs("a", Types.INT64), 1, 'Should return 1L as python long')
            self.assertEqual(h.getAs("a", Types.FLOAT), 1.0, 'Should return 1.0 as python float')
            self.assertEqual(h.getAs("a", Types.DOUBLE), 1.0, 'Should return 1.0 as python float')
        except Exception as e:
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
        
        except Exception as e:
            self.fail("test_getAs exception group 2: " + str(e))

        try:
            h = Hash("a", np.array([False,False,False,False])) # value is numpy array of boolean -> std::vector<bool>
            self.assertEqual(h.getAs("a", Types.STRING), "0,0,0,0", 'Should return "0,0,0,0" as python string')
            self.assertEqual(h.getAs("a", Types.VECTOR_INT32)[3], 0, "Should return 0")
        except Exception as e:
            self.fail("test_getAs exception group 3: " + str(e))

        try:
            h = Hash("a", [False,False,False,False])     # value is python list of boolean -> std::vector<bool>
            self.assertEqual(h.getAs("a", Types.STRING), "0,0,0,0", 'Should return "0,0,0,0" as python string')
            self.assertEqual(h.getAs("a", Types.VECTOR_INT32)[3], 0, "Should return 0")
        except Exception as e:
            self.fail("test_getAs exception group 4: " + str(e))

        #TODO
        try:
            h = Hash("a", bytearray([52,52,52]))   # value is python bytearray -> std::vector<char>
            self.assertEqual(h.getAs("a", Types.STRING), "NDQ0", 'Should return "NDQO" as python string because it assumes vector to contain binary data and does a base64 encode')
        except Exception as e:
            self.fail("test_getAs exception group 5: " + str(e))

        try:
            h = Hash("a", ['1','2','3','4'])              # value is python list -> std::vector<char>
            self.assertEqual(h.getAs("a", Types.STRING), "1,2,3,4", 'Should return "1,2,3,4" as python string')
            self.assertEqual(h.getAs("a", Types.VECTOR_INT32)[3], 4, "Should return 4")
        except Exception as e:
            self.fail("test_getAs exception group 6: " + str(e))
            
        try:
            h = Hash("a", [13,13,13,13])
            self.assertEqual(h.getAs("a", Types.STRING), "13,13,13,13")
        except Exception as e:
            self.fail("test_getAs exception group 7: " + str(e))
            
        try:
            h = Hash("a", -42)
        except Exception as e:
            self.fail("test_getAs exception group 8: " + str(e))
            
        try:
            h = Hash("a", [-42])
            self.assertEqual(h.getAs("a", Types.STRING), "-42", 'Should return "-42" as str')
        except Exception as e:
            self.fail("test_getAs exception group 9: " + str(e))
            
        try:
            h = Hash("a", np.array([-42]))
            self.assertEqual(h.getAs("a", Types.STRING), "-42", 'Should return "-42" as str')
        except Exception as e:
            self.fail("test_getAs exception group 10: " + str(e))
            
        try:
            h = Hash("a", np.array([], dtype=int))
            self.assertEqual(h.getAs("a", Types.STRING), "", 'Should return empty str')
        except Exception as e:
            self.fail("test_getAs exception group 11: " + str(e))
            
        try:
            h = Hash("a", -2147483647)
            self.assertEqual(h.getAs("a", Types.STRING), "-2147483647", 'Should return "-2147483647" str')
        except Exception as e:
            self.fail("test_getAs exception group 12: " + str(e))
            
        try:
            h = Hash("a", 1234567890)
            self.assertEqual(h.getAs("a", Types.STRING), "1234567890", 'Should return "1234567890" str')
            self.assertEqual(h.getType("a"), Types.INT32)
            self.assertEqual(str(h.getType("a")), "INT32")
        except Exception as e:
            self.fail("test_getAs exception group 13: " + str(e))
            
        try:
            h = Hash("a", 0.123456789123456)
            self.assertEqual(h.getAs("a", Types.STRING), "0.123456789123456", 'Should return "0.123456789123456" str')
            self.assertEqual(h.getType("a"), Types.DOUBLE)
        except Exception as e:
            self.fail("test_getAs exception group 14: " + str(e))
            
        
    def test_find(self):
        try:
            h = Hash("a.b.c1.d", 1)
            node = h.find("a.b.c1.d")
            self.assertIsNotNone(node, "The 'node' object should be not 'None'")
            self.assertEqual(node.getKey(), "d", 'Bad key returned by "getKey" method')
            self.assertEqual(node.getValue(), 1, 'Should return 1');
            self.assertIsNone(h.find("a.b.c1.f"), "The resulting object should be 'None'")
        except Exception as e:
            self.fail("test_find exception group 1: " + str(e))
    

        try:
            h = Hash("a.b.c", "1")
            node = h.find("a.b.c")
            self.assertIsNotNone(node, "The 'node' should be not 'None")
            node.setValue(2)
            self.assertEqual(h["a.b.c"], 2);
            node = h.find("a.b.c", '/')
            self.assertIsNone(node)
        except Exception as e:
            self.fail("test_find exception group 2: " + str(e))
        

    def test_iteration(self):
        h = Hash("should", 1, "be", 2, "iterated", 3, "in", 4, "correct", 5, "order", 6)
        a = HashAttributes("should", 1, "be", 2, "iterated", 3, "in", 4, "correct", 5, "order", 6)
        try:
            insertionOrder = list()
            for k in h:
                insertionOrder.append(str(k))
            self.assertEqual(insertionOrder, ["should","be","iterated","in","correct","order"])
            
        except Exception as e:
            self.fail("test_iteration exception group 1: " + str(e))

        try:
            alphaNumericOrder = list()
            for k in h:
                alphaNumericOrder.append(k.getKey())
            alphaNumericOrder.sort()
            self.assertEqual(alphaNumericOrder,["be","correct","in","iterated","order","should"])
        except Exception as e:
            self.fail("test_iteration exception group 2: " + str(e))

        h.set("be", "2") # Has no effect on order

        try:
            insertionOrder = list()
            for k in h:
                insertionOrder.append(str(k.getKey()))
            self.assertEqual(insertionOrder,["should","be","iterated","in","correct","order"])
        except Exception as e:
            self.fail("test_iteration exception group 3: " + str(e))

        try:
            alphaNumericOrder = list()
            for k in h:
                alphaNumericOrder.append(str(k.getKey()))
            alphaNumericOrder.sort()
            self.assertEqual(alphaNumericOrder,["be","correct","in","iterated","order","should"])
        except Exception as e:
            self.fail("test_iteration exception group 4: " + str(e))

        h.erase("be")  # Remove
        h.set("be", "2")   # Must be last element in sequence now

        try:
            insertionOrder = list()
            for k in h:
                insertionOrder.append(str(k.getKey()))
            self.assertEqual(insertionOrder,["should","iterated","in","correct","order","be"])
        except Exception as e:
            self.fail("test_iteration exception group 5: " + str(e))

        try:
            alphaNumericOrder = list()
            for k in h:
                alphaNumericOrder.append(str(k.getKey()))
            alphaNumericOrder.sort()
            self.assertEqual(alphaNumericOrder,["be","correct","in","iterated","order","should"])
        except Exception as e:
            self.fail("test_iteration exception group 6: " + str(e))

        #  getKeys(...) to ...
        #         "list" and sort it  ... like C++ set
        try:
            tmp = list()           # create empty set
            h.getKeys(tmp)         # fill set by keys
            tmp.sort()
            it = iter(tmp)
            self.assertEqual(str(next(it)), "be")
            self.assertEqual(str(next(it)), "correct")
            self.assertEqual(str(next(it)), "in")
            self.assertEqual(str(next(it)), "iterated")
            self.assertEqual(str(next(it)), "order")
            self.assertEqual(str(next(it)), "should")
        except Exception as e:
            self.fail("test_iteration exception group 7: " + str(e))

        #        "list" ... like C++ vector
        try:
            tmp = list()           # create empty vector
            h.getKeys(tmp)         # fill vector by keys
            it = iter(tmp)
            self.assertEqual(str(next(it)), "should")
            self.assertEqual(str(next(it)), "iterated")
            self.assertEqual(str(next(it)), "in")
            self.assertEqual(str(next(it)), "correct")
            self.assertEqual(str(next(it)), "order")
            self.assertEqual(str(next(it)), "be")
        except Exception as e:
            self.fail("test_iteration exception group 8: " + str(e))

        try:
            h = Hash("b", "bla-la-la", "a.b.c", 1, "abc.2", 2.2222222, "a.b.c1.d", "abc1d", "abc.1", 1.11111111)
            
            l = [];
            h.getKeys(l)    # use top level keys
            self.assertEqual(len(l), 3)
            i = iter(l)                         # "canonical" order: on every level insertion order
            self.assertEqual(str(next(i)), "b")
            self.assertEqual(str(next(i)), "a")
            self.assertEqual(str(next(i)), "abc")
            
            l = [];
            h.getPaths(l)   # use full keys 
            self.assertEqual(len(l), 5)
            i = iter(l)
            self.assertEqual(str(next(i)), "b")
            self.assertEqual(str(next(i)), "a.b.c")
            self.assertEqual(str(next(i)), "a.b.c1.d")
            self.assertEqual(str(next(i)), "abc.2")
            self.assertEqual(str(next(i)), "abc.1")
        except Exception as e:
            self.fail("test_iteration exception group 9: " + str(e))
            
        try:
            hash = Hash("a.b[0].c.d", "bla-la-la", "a.b[1].c.d", "bla-la-la")
            vec = hash['a.b']
            for h in vec:
                self.assertEqual(next(h.__iter__()).getValue()['d'], "bla-la-la")
        except Exception as e:
            self.fail("test_iteration exception group 10: " + str(e))


    def test_attributes(self):
        try:
            h = Hash("a.b.a.b", 42)
            h.setAttribute("a.b.a.b","attr1", "someValue")
            self.assertEqual(h.getAttribute("a.b.a.b","attr1"), "someValue", 'Should return "someValue"')
        except Exception as e:
            self.fail("test_attributes exception group 1: " + str(e))

        try:
            h = Hash("a.b.a.b", 42)
            h.setAttribute("a.b.a.b","attr1", "someValue")
            self.assertEqual(h.getAttribute("a.b.a.b","attr1"), "someValue", 'Should return "someValue"')
            
            h.setAttribute("a.b.a.b", "attr2", 42)
            self.assertEqual(h.getAttribute("a.b.a.b","attr1"), "someValue", 'Should return "someValue"')
            self.assertEqual(h.getAttribute("a.b.a.b","attr2"), 42, 'Should return 42')
            
            h.setAttribute("a.b.a.b", "attr2", 43)
            self.assertEqual(h.getAttribute("a.b.a.b","attr1"), "someValue", 'Should return "someValue"')
            self.assertEqual(h.getAttribute("a.b.a.b","attr2"), 43, 'Should return 42')
            
            h.setAttribute("a.b.a.b", "attr1", True)
            self.assertEqual(h.getAttribute("a.b.a.b","attr1"), True, 'Should return "someValue"')
            self.assertEqual(h.getAttribute("a.b.a.b","attr2"), 43, 'Should return 42')
            
            attrs = h.getAttributes("a.b.a.b")
            self.assertEqual(attrs.size(), 2)
            self.assertEqual(attrs.get("attr1"), True)
            self.assertEqual(attrs["attr1"], True)
            self.assertEqual(attrs.get("attr2"), 43)
            self.assertEqual(attrs["attr2"], 43)
            
            node = attrs.getNode("attr1")
            self.assertEqual(node.getType(), Types.BOOL)

            node = attrs.getNode("attr2")
            self.assertEqual(node.getType(), Types.INT32)

        except Exception as e:
            self.fail("test_attributes exception group 2: " + str(e))

        try:
            h = Hash("a.b.a.b", 1)
            h.setAttribute("a.b.a.b","attr1", [1,2,3])
            self.assertEqual(h.getAttribute("a.b.a.b","attr1")[1], 2)
        except Exception as e:
            self.fail("test_attributes exception group 3: " + str(e))

        try:
            h = Hash("a.b.a.b", 42)
            h.setAttribute("a.b.a.b","attr1", "someValue")
            self.assertEqual(h.getAttribute("a.b.a.b","attr1"), "someValue", 'Should return "someValue"')
        except Exception as e:
            self.fail("test_attributes exception group 4: " + str(e))

        try:
            h = Hash("a.b.a.b", 42)
            h.setAttribute("a.b.a.b","attr1", "someValue")
            self.assertEqual(h.getAttribute("a.b.a.b","attr1"), "someValue", 'Should return "someValue"')
        except Exception as e:
            self.fail("test_attributes exception group 5: " + str(e))

        try:
            h = Hash("a.b.a.b", 42)
            h.setAttribute("a.b.a.b","attr1", [1,2,3,4,5,6,7])
            
            setStdVectorDefaultConversion(Types.PYTHON)
            if isStdVectorDefaultConversion(Types.PYTHON):
                self.assertEqual(h.getAttribute("a.b.a.b","attr1"), [1,2,3,4,5,6,7])
            if isStdVectorDefaultConversion(Types.NUMPY):
                self.assertEqual(h.getAttribute("a.b.a.b","attr1").all(), np.array([1,2,3,4,5,6,7], dtype=np.int32).all())
            
            setStdVectorDefaultConversion(Types.NUMPY)
            if isStdVectorDefaultConversion(Types.PYTHON):
                self.assertEqual(h.getAttribute("a.b.a.b","attr1"), [1,2,3,4,5,6,7])
            if isStdVectorDefaultConversion(Types.NUMPY):
                self.assertEqual(h.getAttribute("a.b.a.b","attr1").all(), np.array([1,2,3,4,5,6,7], dtype=np.int32).all())
                
            setStdVectorDefaultConversion(Types.PYTHON)
        except Exception as e:
            self.fail("test_attributes exception group 6: " + str(e))
            
        try:
            h = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])
            h.setAttribute('a.b.c','attr1',[1.234,2.987,5.555])
            if isStdVectorDefaultConversion(Types.PYTHON):
                self.assertEqual(h.getAttribute('a.b.c','attr1'), [1.234,2.987,5.555])
                self.assertEqual(h.getAttributeAs('a.b.c','attr1',Types.NDARRAY_DOUBLE).all(), np.array([1.234,2.987,5.555], dtype=np.double).all())
            if isStdVectorDefaultConversion(Types.NUMPY):
                self.assertEqual(h.getAttribute('a.b.c','attr1').all(), np.array([1.234,2.987,5.555], dtype=np.double).all())
                self.assertEqual(h.getAttributeAs('a.b.c','attr1',Types.VECTOR_DOUBLE), [1.234,2.987,5.555])
        except Exception as e:
            self.fail("test_attributes exception group 7: " + str(e))

    def test_attributes_get_copy(self):
        try:
            h = Hash("a.b.a.b", 42)
            h.setAttribute("a.b.a.b","attr1", True)
            self.assertEqual(h.getAttribute("a.b.a.b","attr1"), True)
            
            attrs = h.copyAttributes("a.b.a.b")
            attrs["attr1"] = False
            self.assertEqual(h.getAttribute("a.b.a.b","attr1"), True)
        except Exception as e:
            self.fail("test_attributes_get_copy exception group 1: " + str(e))

        try:
            h = Hash("a.b.a.b", 42)
            h.setAttribute("a.b.a.b","attr1", True)
            self.assertEqual(h.getAttribute("a.b.a.b","attr1"), True)
            
            attrs = h.getAttributes("a.b.a.b")
            attrs["attr1"] = False
            self.assertEqual(h.getAttribute("a.b.a.b","attr1"), False)
        except Exception as e:
            self.fail("test_attributes_get_copy exception group 2: " + str(e))

        try:
            h = Hash("a.b.a.b", 42)
            h.setAttribute("a.b.a.b","attr1", True)
            self.assertEqual(h.getAttribute("a.b.a.b","attr1"), True)
            node = h.getNode("a.b.a.b")
            attrs = node.copyAttributes()
            attrs["attr1"] = False
            self.assertEqual(h.getAttribute("a.b.a.b","attr1"), True)
        except Exception as e:
            self.fail("test_attributes_get_copy exception group 3: " + str(e))

        try:
            h = Hash("a.b.a.b", 42)
            h.setAttribute("a.b.a.b","attr1", True)
            self.assertEqual(h.getAttribute("a.b.a.b","attr1"), True)
            node = h.getNode("a.b.a.b")
            attrs = node.getAttributes()
            attrs["attr1"] = False
            self.assertEqual(h.getAttribute("a.b.a.b","attr1"), False)
        except Exception as e:
            self.fail("test_attributes_get_copy exception group 4: " + str(e))


    def test_merge(self):
        try:
            h1 = Hash("a", 1,
                      "b", 2,
                      "c.b[0].g", 3,
                      "c.c[0].d", 4,
                      "c.c[1]", Hash("a.b.c", 6),
                      "d.e", 7
                     )
            
            h2 = Hash("a", 21,
                      "b.c", 22,
                      "c.b[0]", Hash("key", "value"),
                      "c.b[1].d", 24,
                      "e", 27
                     )
            
            h1 += h2
            
            self.assertTrue(h1.has("a"))
            self.assertEqual(h1.get("a"), 21)
            self.assertEqual(h1["a"], 21)
            self.assertTrue(h1.has("b"))
            self.assertFalse(h1.has("c.b.d"))
            self.assertTrue(h1.has("c.b[0]"))
            self.assertTrue(h1.has("c.b[1]"))
            self.assertTrue(h1.has("c.b[2]"))
            self.assertEqual(h1.get("c.b[2].d"), 24)
            self.assertTrue(h1.has("c.c[0].d"))
            self.assertTrue(h1.has("c.c[1].a.b.c"))
            self.assertTrue(h1.has("d.e"))
            self.assertTrue(h1.has("e"))

            h3 = h1

            self.assertTrue(similar(h1, h3))
            
        except Exception as e:
            self.fail("test_iteration exception group 1: " + str(e))
            
    def test_subtract(self):
        try:
            h1 = Hash("a", 1,
                      "b", 2,
                      "c.b[0].g", 3,
                      "c.c[0].d", 4,
                      "c.c[1]", Hash("a.b.c", 6),
                      "d.e", 7
                     )
            
            h2 = Hash("a", 21,
                      "b.c", 22,
                      "c.b[0]", Hash("key", "value"),
                      "c.b[1].d", 24,
                      "e", 27
                     )
            
            h1 += h2
            h1 -= h2
            
            self.assertFalse("a" in h1)
            self.assertTrue(h1["b"].empty())
            self.assertEqual(h1["c.b[0].g"], 3)
            self.assertEqual(h1["c.b[1].key"], "value")
            self.assertEqual(h1["c.b[2].d"], 24)
            self.assertEqual(h1["c.c[0].d"], 4)
            self.assertEqual(h1["c.c[1].a.b.c"], 6)
            self.assertEqual(h1["d.e"], 7)
            
        except Exception as e:
            self.fail("test_subtract exception group 1: " + str(e))
            
        try:
            h3 = Hash("a.b.c", 1,
                      "a.b.d", 2,
                      "b.c.d", 22,
                      "c.a.b", 77,
                      "c.c", "blabla"
                     )
            h4 = Hash("a.b", Hash(), "c", Hash())
            h3 -= h4
            
            self.assertFalse("a.b" in h3)
            self.assertTrue(h3["a"].empty())
            self.assertFalse("c" in h3)
            self.assertTrue(h3["b.c.d"] == 22)
            
        except Exception as e:
            self.fail("test_subtract exception group 2: " + str(e))
            
            
    def test_dict(self):
        try:
            h = Hash("a", {"b" : { "c" : {"d" : [1, 2, 3, 4, 5]}}})
            
            self.assertEqual(h["a.b.c.d"], [1, 2, 3, 4, 5])
            self.assertEqual(h["a"]["b"]["c"]["d"], [1, 2, 3, 4, 5])
    
            h.set('x', {'y' : {'z' : True}})
            
            self.assertEqual(h["x.y.z"], True)
            
        except Exception as e:
            self.fail("test_dict exception group 1: " + str(e))
              
    def test_hashNode(self):     
        try:
            h = Hash("a.b.c", 42)
            node = h.getNode("a.b.c")
            self.assertEqual(node.getValue(), 42)
            
            self.assertEqual(node.getValueAs(Types.STRING), '42')
            self.assertEqual(node.getValueAs(Types.INT32), 42)
            
            self.assertEqual(node.getValueAs("STRING"), '42')
            self.assertEqual(node.getValueAs("INT32"), 42)
        except Exception as e:
            self.fail("test_node exception group 1: " + str(e))
           
        try:
            h = Hash("a.b.c", 42)
            h.setAttribute("a.b.c","attr1", 15)
            node = h.getNode("a.b.c")
            self.assertEqual(node.getAttribute("attr1"), 15)
            
            self.assertEqual(node.getAttributeAs("attr1", Types.STRING), '15')
            self.assertEqual(node.getAttributeAs("attr1", Types.INT32), 15)
        
            self.assertEqual(node.getAttributeAs("attr1", "STRING"), '15')
            self.assertEqual(node.getAttributeAs("attr1", "INT32"), 15)
        except Exception as e:
            self.fail("test_node exception group 2: " + str(e))
            
        try:
            h = Hash("a.b.c", 15)
            node = h.getNode("a.b.c")
            self.assertEqual(node.getType(), Types.INT32)
            self.assertEqual(node.getValue(), 15)
            
            node.setType(Types.STRING)
            self.assertEqual(node.getType(), Types.STRING)
            self.assertEqual(node.getValue(), '15')
            
            node.setType("UINT32")
            self.assertEqual(node.getType(), Types.UINT32)
            self.assertEqual(node.getValue(), 15)
        except Exception as e:
            self.fail("test_node exception group 3: " + str(e))
           
    def test_hashAttributesNode(self):     
        try:
            h = Hash("a.b.c", "value")
            h.setAttribute("a.b.c","attr1", 10)
            h.setAttribute("a.b.c","attr2", "test")
            attrs = h.getAttributes("a.b.c")
            
            node = attrs.getNode("attr1")
            self.assertEqual(node.getType(), Types.INT32)
            self.assertEqual(node.getValue(), 10)
            self.assertEqual(node.getValueAs(Types.STRING), '10')
            self.assertEqual(node.getValueAs("STRING"), '10')
            
            node.setType(Types.STRING)
            self.assertEqual(node.getType(), Types.STRING)
            self.assertEqual(node.getValue(), '10')
            self.assertEqual(node.getValueAs(Types.UINT32), 10)
            self.assertEqual(node.getValueAs("UINT32"), 10)
            
            node.setType("UINT32")
            self.assertEqual(node.getType(), Types.UINT32)
            self.assertEqual(node.getValue(), 10)
            
            node = attrs.getNode("attr2")
            self.assertEqual(node.getType(), Types.STRING)
            
        except Exception as e:
            self.fail("test_hashAttributesNode exception: " + str(e))             
           
           
    def test_hashAttributes(self):     
        try:
            h = Hash("a.b.c", "value")
            h.setAttribute("a.b.c","attr1", 12)
            h.setAttribute("a.b.c","attr2", True)
            
            attrs = h.getAttributes("a.b.c")
            
            self.assertEqual(attrs.getAs("attr1", Types.UINT32), 12)
            self.assertEqual(attrs.getAs("attr1", Types.STRING), '12')
            self.assertEqual(attrs.getAs("attr2", Types.BOOL), True)
            
            self.assertEqual(attrs.getAs("attr1", "UINT32"), 12)     
            self.assertEqual(attrs.getAs("attr1", "STRING"), '12')           
            self.assertEqual(attrs.getAs("attr2", "BOOL"), True)
               
        except Exception as e:
            self.fail("test_hashAttributes exception: " + str(e))


    def test_erase(self):
        try:
            h = Hash('a[0].b[0].c', 1, 'b[0].c.d', 2, 'c.d[0].e', 3, 'd.e', 4, 'e', 5, 'f.g.h.i.j.k', 6)
            self.assertTrue('a' in h)
            self.assertTrue('f' in h)
            self.assertTrue('b' in h)
            self.assertTrue('c' in h)
            self.assertTrue('d' in h)
        except Exception as e:
            self.fail("test_erase exception 1: " + str(e))
            
        try:
            paths = h.getPaths()
            for path in paths:
                if len(path) == 11:
                    h.erasePath(path)
            self.assertFalse('a' in h)
            self.assertFalse('f' in h)
            self.assertTrue('b' in h)
            self.assertTrue('c' in h)
            self.assertTrue('d' in h)
        except Exception as e:
            self.fail("test_erase exception 2: " + str(e))
            
        try:
            paths = h.getPaths()
            for path in paths:
                if len(path) == 8:
                    h.erasePath(path)
            self.assertFalse('a' in h)
            self.assertFalse('f' in h)
            self.assertFalse('b' in h)
            self.assertFalse('c' in h)
            self.assertEqual(len(h), 2)
            self.assertEqual(h['d.e'], 4)
            self.assertEqual(h['e'], 5)
        except Exception as e:
            self.fail("test_erase exception 3: " + str(e))
        
        
if __name__ == '__main__':
    unittest.main()

