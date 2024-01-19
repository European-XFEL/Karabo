# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
'''
Created on Oct 17, 2012

@author: Sergey Esenov <serguei.essenov@xfel.eu>
'''

import copy
import unittest
import weakref

import numpy as np

import karathon
from karabo.bound import (
    Hash, HashMergePolicy, Types, VectorHash, fullyEqual,
    isStdVectorDefaultConversion, setStdVectorDefaultConversion, similar)
from karabo.testing.utils import compare_ndarray_data_ptrs


class Hash_TestCase(unittest.TestCase):
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
                     "e.f.g.h", [5, 5, 5, 5, 5], "F.f.f.f.f",
                     Hash("x.y.z", 99))
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

    def test_get(self):
        h = Hash()
        self.assertIsNone(h.get("missing"))
        self.assertEqual(h.get("missing", default="value"), "value")

    def test_getSet(self):

        try:
            h = Hash()
            h.set("a.b.c1.d", 1)
            self.assertTrue(h.get("a").has("b"), '"b" not found')
            self.assertTrue(h.get("a.b").has("c1"), '"c1" not found')
            self.assertTrue(h.get("a.b.c1").has("d"), '"d" not found')
            self.assertEqual(h.get("a.b.c1.d"), 1, '"get" should return 1')
            self.assertTrue(h.has("a.b.c1.d"), '"a.b.c1.d" key not found')
            self.assertIn("a.b.c1.d", h,
                          "test_getSet group 1: h __contains__ "
                          "\"a.b.c1.d\" failed")
            self.assertTrue(h.get("a").has("b.c1"), '"b.c1" key not found')
            self.assertIn("b.c1", h["a"],
                          "test_getSet group 1: h['a'] __contains__ "
                          "\"b.c1\" failed")

            h.set("a.b.c2.d", 2.0)
            self.assertTrue(h.get("a.b").has("c2.d"), '"c2.d" not found')
            self.assertTrue(h.get("a.b").has("c1.d"), '"c1.d" not found')
            self.assertEqual(h.get("a.b.c1.d"), 1, '"get" should return 1')
            self.assertEqual(h.get("a.b.c2.d"), 2.0, '"get" should return 2.0')

            h.set("a.b[0]", Hash("a", 1))
            self.assertTrue(h.get("a").has("b"), "'b' not found")
            self.assertTrue(h["a"].has("b"), "'b' not found")
            self.assertIn("b", h["a"],
                          "test_getSet group 1: h['a'] __contains__ "
                          "\"b\" failed")
            self.assertEqual(len(h.get("a")), 1, "'len' should give 1")
            self.assertEqual(len(h["a"]), 1, "'len' should give 1")
            self.assertEqual(len(h.get("a.b")), 1, "'len' should give 1")
            self.assertEqual(len(h["a.b"]), 1, "'len' should give 1")
            self.assertEqual(len(h.get("a.b")[0]), 1, "'len' should give 1")
            self.assertEqual(len(h["a.b"][0]), 1, "'len' should give 1")
            self.assertEqual(h["a.b"][0]["a"], 1, '"get" should return 1')
            self.assertEqual(h["a.b[0].a"], 1, '"get" should return 1')
            self.assertEqual(h.get("a.b")[0].get("a"), 1,
                             '"get" should return 1')
            self.assertEqual(h.get("a.b[0].a"), 1, '"get" should return 1')

            h.set("a.b[2]", Hash("a", 1))
            self.assertTrue(h.get("a").has("b"), "'b' not found")
            self.assertEqual(len(h["a"]), 1, "'len' should give 1")
            self.assertEqual(len(h["a.b"]), 3,
                             "'len' should give 3")  # 0, 1, 2
            self.assertEqual(h["a.b[0].a"], 1, '"get" should return 1')
            self.assertEqual(h["a.b[2].a"], 1, '"get" should return 1')
            self.assertEqual(h["a.b"][0]["a"], 1, '"get" should return 1')
            self.assertEqual(h["a.b"][2]["a"], 1, '"get" should return 1')
            self.assertTrue(h["a.b"][1].empty(),
                            'h["a.b"][1] should be empty Hash')

        except Exception as e:
            self.fail("test_getSet exception group 1: " + str(e))

        try:
            h = Hash()
            h["a.b.c"] = 1  # statement is equivalent to h.set("a.b.c", 1)
            h["a.b.c"] = 2
            self.assertEqual(h["a.b.c"], 2, "Value should be overwritten by 2")
            self.assertTrue(h.has("a.b"), "Key 'a.b' not found")
            self.assertFalse(h.has("a.b.c.d"),
                             "Key 'a.b.c.d' should not be found")
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
            self.assertEqual(h["x[1].y[0].b"], "green",
                             "Failure in array case")
        except Exception as e:
            self.fail("test_getSet exception group 4: " + str(e))

        try:
            h1 = Hash("a[0].b[0]", Hash("a", 1))
            h2 = Hash("a[0].b[0]", Hash("a", 2))
            self.assertEqual(h1["a[0].b[0].a"], 1, "Value should be equal 1")
            h1["a[0]"] = h2
            self.assertEqual(h1["a[0].a[0].b[0].a"], 2,
                             "Value should be equal 2")
            h1["a"] = h2
            self.assertEqual(h1["a.a[0].b[0].a"], 2, "Value should be equal 2")
        except Exception as e:
            self.fail("test_getSet exception group 5: " + str(e))

        try:
            h = Hash()
            b = True
            h["a"] = b
            self.assertEqual(str(h.getType("a")), "BOOL",
                             'The type should be "BOOL"')
            self.assertEqual(h.getType("a"), Types.BOOL,
                             'The type ID for "BOOL" should be Types.BOOL')
        except Exception as e:
            self.fail("test_getSet exception group 6: " + str(e))

        if Hash is karathon.Hash:
            try:
                h = Hash('a.b.c', [1, 2, 3, 4, 5, 6, 7], 'b.c.d',
                         [False, False, True, True, True, False, True])
                self.assertEqual(isStdVectorDefaultConversion(Types.PYTHON),
                                 True)
                self.assertEqual(h.isType('a.b.c', Types.VECTOR_INT32), True)
                self.assertIsInstance(h['a.b.c'], list)
                try:
                    setStdVectorDefaultConversion(Types.VECTOR_INT32)
                except RuntimeError:
                    pass
                self.assertEqual(isStdVectorDefaultConversion(Types.PYTHON),
                                 True)
                setStdVectorDefaultConversion(Types.NUMPY)
                self.assertEqual(isStdVectorDefaultConversion(Types.PYTHON),
                                 False)
                self.assertEqual(isStdVectorDefaultConversion(Types.NUMPY),
                                 True)
                self.assertIsInstance(h['a.b.c'], np.ndarray)
                self.assertIsInstance(h['b.c.d'], np.ndarray)
                setStdVectorDefaultConversion(Types.PYTHON)
                self.assertEqual(isStdVectorDefaultConversion(Types.PYTHON),
                                 True)
            except Exception as e:
                self.fail("test_getSet exception group 7: " + str(e))

    def test_getSetVectorHash(self):
        try:
            h = Hash('a', VectorHash())
            g = [Hash('b', 1), Hash('b', 2)]  # python list of Hashes
            vh = h['a']  # get the reference because value is VectorHash
            vh.extend(g)
            g1 = (Hash('c', 10), Hash('c', 20),)  # python tuple of Hashes
            vh.extend(g1)  # "extend" lists, tuples, VectorHash objects
            vh.append(Hash('d', 100))  # "append" Hash object
            self.assertEqual(len(vh), 5)
            self.assertEqual(h['a[4].d'], 100)
            h2 = copy.copy(h)
            self.assertTrue(h == h2)
            vh[4]['d'] = 999
            self.assertEqual(h['a[4].d'], 999)
            # self.assertFalse(h == h2)

        except Exception as e:
            self.fail("test_getSetVectorHash exception group 1: " + str(e))

    def test_getAs(self):

        try:
            h = Hash("a", True)
            self.assertEqual(h.getAs("a", Types.STRING), "1",
                             'Should return "1" as python string')
            self.assertEqual(h.getAs("a", Types.INT32), 1,
                             'Should return 1 as an python int')
            self.assertEqual(h.getAs("a", Types.INT64), 1,
                             'Should return 1L as python long')
            self.assertEqual(h.getAs("a", Types.FLOAT), 1.0,
                             'Should return 1.0 as python float')
            self.assertEqual(h.getAs("a", Types.DOUBLE), 1.0,
                             'Should return 1.0 as python float')
        except Exception as e:
            self.fail("test_getAs exception group 1: " + str(e))

        try:
            h = Hash("a", True)
            h.setAttribute("a", "a", True)
            self.assertEqual(h.getAttributeAs("a", "a", Types.STRING), "1",
                             'Should return "1" as python string')
            self.assertEqual(h.getAttributeAs("a", "a", Types.INT32), 1,
                             'Should return 1 as python int')
            self.assertEqual(h.getAttributeAs("a", "a", Types.DOUBLE), 1.0,
                             'Should return 1.0 as python float')
            h.setAttribute("a", "b", 12)
            h.setAttribute("a", "c", 1.23)
            attrs = h.getAttributes("a")
            g = Hash("Z.a.b.c", "value")
            g.setAttributes("Z.a.b.c", attrs)
            self.assertEqual(g.getAttributeAs("Z.a.b.c", "a", Types.STRING),
                             "1", 'Should return "1" as python string')
            self.assertEqual(g.getAttributeAs("Z.a.b.c", "a", Types.INT32), 1,
                             'Should return 1 as python int')
            self.assertEqual(g.getAttributeAs("Z.a.b.c", "a", Types.DOUBLE),
                             1.0, 'Should return 1.0 as python float')

        except Exception as e:
            self.fail("test_getAs exception group 2: " + str(e))

        try:
            # value is python list of boolean -> std::vector<bool>
            h = Hash("a", [False, False, False, False])
            self.assertEqual(h.getAs("a", Types.STRING), "0,0,0,0",
                             'Should return "0,0,0,0" as python string')
            self.assertEqual(h.getAs("a", Types.VECTOR_INT32)[3], 0,
                             "Should return 0")
        except Exception as e:
            self.fail("test_getAs exception group 4: " + str(e))

        # TODO
        try:
            # value is python bytearray -> std::vector<char>
            h = Hash("a", bytearray([52, 52, 52]))
            self.assertEqual(h.getAs("a", Types.STRING), "NDQ0",
                             'Should return "NDQO" as python string because '
                             'it assumes vector to contain binary data and '
                             'does a base64 encode')
        except Exception as e:
            self.fail("test_getAs exception group 5: " + str(e))

        try:
            h = Hash("a", ['1', '2', '3',
                           '4'])  # value is python list -> std::vector<char>
            self.assertEqual(h.getAs("a", Types.STRING), "1,2,3,4",
                             'Should return "1,2,3,4" as python string')
            self.assertEqual(h.getAs("a", Types.VECTOR_INT32)[3], 4,
                             "Should return 4")
        except Exception as e:
            self.fail("test_getAs exception group 6: " + str(e))

        try:
            h = Hash("a", [13, 13, 13, 13])
            self.assertEqual(h.getAs("a", Types.STRING), "13,13,13,13")
        except Exception as e:
            self.fail("test_getAs exception group 7: " + str(e))

        try:
            h = Hash("a", -42)
        except Exception as e:
            self.fail("test_getAs exception group 8: " + str(e))

        try:
            h = Hash("a", [-42])
            self.assertEqual(h.getAs("a", Types.STRING), "-42",
                             'Should return "-42" as str')
        except Exception as e:
            self.fail("test_getAs exception group 9: " + str(e))

        try:
            h = Hash("a", -2147483647)
            self.assertEqual(h.getAs("a", Types.STRING), "-2147483647",
                             'Should return "-2147483647" str')
        except Exception as e:
            self.fail("test_getAs exception group 12: " + str(e))

        try:
            h = Hash("a", 1234567890)
            self.assertEqual(h.getAs("a", Types.STRING), "1234567890",
                             'Should return "1234567890" str')
            self.assertEqual(h.getType("a"), Types.INT32)
            self.assertEqual(str(h.getType("a")), "INT32")
        except Exception as e:
            self.fail("test_getAs exception group 13: " + str(e))

        try:
            h = Hash("a", 0.123456789123456)
            self.assertEqual(h.getAs("a", Types.STRING), "0.123456789123456",
                             'Should return "0.123456789123456" str')
            self.assertEqual(h.getType("a"), Types.DOUBLE)
        except Exception as e:
            self.fail("test_getAs exception group 14: " + str(e))

    def test_setAs(self):
        testTypes = (Types.INT8, Types.UINT8, Types.INT16, Types.UINT16,
                     Types.INT32, Types.UINT32, Types.INT64, Types.UINT64,
                     Types.FLOAT, Types.DOUBLE)

        h = Hash()
        for t in testTypes:
            h.setAs("a", 5, t)
            self.assertEqual(h.getType("a"), t,
                             "Setting as type " + str(t) + " failed")
            self.assertEqual(h["a"], 5, "Equality failed for type " + str(t))

    def test_intUnboxingEdgeCases(self):
        values_and_types = {
            -(2 ** 31): (Types.INT32, Types.VECTOR_INT32),
            2 ** 31 - 1: (Types.INT32, Types.VECTOR_INT32),
            2 ** 32 - 1: (Types.UINT32, Types.VECTOR_UINT32),
            2 ** 32: (Types.INT64, Types.VECTOR_INT64),
            -(2 ** 63): (Types.INT64, Types.VECTOR_INT64),
            2 ** 63 - 1: (Types.INT64, Types.VECTOR_INT64),
            2 ** 64 - 1: (Types.UINT64, Types.VECTOR_UINT64),
        }
        # Why not OverflowError below? If I try so, my traceback is
        # OverflowError: int too big to convert
        #
        # The above exception was the direct cause of the following exception:
        #
        # Traceback (most recent call last):
        #   File "<snip>/tests/hash_test.py", line 397, in test_<snip>
        #  h = Hash("b", 2 ** 64)
        # SystemError: <Boost.Python.function object at 0x242c6c0> returned a
        #              result with an error set
        exc = SystemError if Hash.__module__ == "karathon" else RuntimeError
        with self.assertRaises(exc):
            h = Hash("b", 2 ** 64)
        exc = OverflowError if Hash.__module__ == "karathon" else RuntimeError
        with self.assertRaises(exc):
            h = Hash("vb", [2 ** 64])
        for value, type_ in values_and_types.items():
            msg = f"Failed to unbox {value}: "
            try:
                h = Hash("a", value)
                self.assertEqual(h.getType("a"), type_[0], msg=msg)
                self.assertEqual(h["a"], value, msg=msg)
            except Exception as e:
                self.fail(msg + repr(e))
            # Now for vectors:
            # first most broad int type first
            msg = f"Failed to unbox {value} in list: "
            try:
                h = Hash("a", [value, 0])
                self.assertEqual(h.getType("a"), type_[1], msg=msg)
                self.assertEqual(h["a"], [value, 0], msg=msg + " - first")
            except Exception as e:
                self.fail(msg + repr(e))
            # then most broad int type last
            try:
                h = Hash("a", [0, value])
                self.assertEqual(h.getType("a"), type_[1], msg=msg)
                self.assertEqual(h["a"], [0, value], msg=msg + " - last")
            except Exception as e:
                self.fail(msg + repr(e))

    def test_numpy_arrays(self):
        arr = np.ones((100,), dtype=np.uint32)
        arr_weak = weakref.ref(arr)

        h = Hash("a", arr)
        assert compare_ndarray_data_ptrs(h["a"], arr)  # no copy

        # Make sure references are cleaned up appropriately
        del arr
        assert arr_weak() is not None  # h is still holding a reference
        del h
        assert arr_weak() is None

    def test_find(self):
        try:
            h = Hash("a.b.c1.d", 1)
            node = h.find("a.b.c1.d")
            self.assertIsNotNone(node,
                                 "The 'node' object should be not 'None'")
            self.assertEqual(node.getKey(), "d",
                             'Bad key returned by "getKey" method')
            self.assertEqual(node.getValue(), 1, 'Should return 1')
            self.assertIsNone(h.find("a.b.c1.f"),
                              "The resulting object should be 'None'")
        except Exception as e:
            self.fail("test_find exception group 1: " + str(e))

        try:
            h = Hash("a.b.c", "1")
            node = h.find("a.b.c")
            self.assertIsNotNone(node, "The 'node' should be not 'None")
            node.setValue(2)
            self.assertEqual(h["a.b.c"], 2)
            node = h.find("a.b.c", '/')
            self.assertIsNone(node)
        except Exception as e:
            self.fail("test_find exception group 2: " + str(e))

    def test_iteration(self):
        h = Hash("should", 1, "be", 2, "iterated", 3, "in", 4, "correct", 5,
                 "order", 6)
        try:
            insertionOrder = list()
            for k in h:
                insertionOrder.append(str(k))
            self.assertEqual(insertionOrder,
                             ["should", "be", "iterated", "in", "correct",
                              "order"])

        except Exception as e:
            self.fail("test_iteration exception group 1: " + str(e))

        try:
            alphaNumericOrder = list()
            for k in h:
                alphaNumericOrder.append(k.getKey())
            alphaNumericOrder.sort()
            self.assertEqual(alphaNumericOrder,
                             ["be", "correct", "in", "iterated", "order",
                              "should"])
        except Exception as e:
            self.fail("test_iteration exception group 2: " + str(e))

        h.set("be", "2")  # Has no effect on order

        try:
            insertionOrder = list()
            for k in h:
                insertionOrder.append(str(k.getKey()))
            self.assertEqual(insertionOrder,
                             ["should", "be", "iterated", "in", "correct",
                              "order"])
        except Exception as e:
            self.fail("test_iteration exception group 3: " + str(e))

        try:
            alphaNumericOrder = list()
            for k in h:
                alphaNumericOrder.append(str(k.getKey()))
            alphaNumericOrder.sort()
            self.assertEqual(alphaNumericOrder,
                             ["be", "correct", "in", "iterated", "order",
                              "should"])
        except Exception as e:
            self.fail("test_iteration exception group 4: " + str(e))

        h.erase("be")  # Remove
        h.set("be", "2")  # Must be last element in sequence now

        try:
            insertionOrder = list()
            for k in h:
                insertionOrder.append(str(k.getKey()))
            self.assertEqual(insertionOrder,
                             ["should", "iterated", "in", "correct", "order",
                              "be"])
        except Exception as e:
            self.fail("test_iteration exception group 5: " + str(e))

        try:
            alphaNumericOrder = list()
            for k in h:
                alphaNumericOrder.append(str(k.getKey()))
            alphaNumericOrder.sort()
            self.assertEqual(alphaNumericOrder,
                             ["be", "correct", "in", "iterated", "order",
                              "should"])
        except Exception as e:
            self.fail("test_iteration exception group 6: " + str(e))

        # getKeys(...) to ...
        #         "list" and sort it  ... like C++ set
        try:
            tmp = list()  # create empty set
            h.getKeys(tmp)  # fill set by keys
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

        # "list" ... like C++ vector
        try:
            tmp = list()  # create empty vector
            h.getKeys(tmp)  # fill vector by keys
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
            h = Hash("b", "bla-la-la", "a.b.c", 1, "abc.2", 2.2222222,
                     "a.b.c1.d", "abc1d", "abc.1", 1.11111111)

            ll = []
            h.getKeys(ll)  # use top level keys
            self.assertEqual(len(ll), 3)
            i = iter(ll)  # "canonical" order: on every level insertion order
            self.assertEqual(str(next(i)), "b")
            self.assertEqual(str(next(i)), "a")
            self.assertEqual(str(next(i)), "abc")

            ll = []
            h.getPaths(ll)  # use full keys
            self.assertEqual(len(ll), 5)
            i = iter(ll)
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
                self.assertEqual(next(h.__iter__()).getValue()['d'],
                                 "bla-la-la")
        except Exception as e:
            self.fail("test_iteration exception group 10: " + str(e))

    def test_attributes(self):
        try:
            h = Hash("a.b.a.b", 42)
            h.setAttribute("a.b.a.b", "attr1", "someValue")
            self.assertEqual(h.getAttribute("a.b.a.b", "attr1"), "someValue",
                             'Should return "someValue"')
        except Exception as e:
            self.fail("test_attributes exception group 1: " + str(e))

        try:
            h = Hash("a.b.a.b", 42)
            h.setAttribute("a.b.a.b", "attr1", "someValue")
            self.assertEqual(h.getAttribute("a.b.a.b", "attr1"), "someValue",
                             'Should return "someValue"')

            h.setAttribute("a.b.a.b", "attr2", 42)
            self.assertEqual(h.getAttribute("a.b.a.b", "attr1"), "someValue",
                             'Should return "someValue"')
            self.assertEqual(h.getAttribute("a.b.a.b", "attr2"), 42,
                             'Should return 42')

            h.setAttribute("a.b.a.b", "attr2", 43)
            self.assertEqual(h.getAttribute("a.b.a.b", "attr1"), "someValue",
                             'Should return "someValue"')
            self.assertEqual(h.getAttribute("a.b.a.b", "attr2"), 43,
                             'Should return 42')

            h.setAttribute("a.b.a.b", "attr3", 1 + 2j)
            self.assertEqual(h.getAttribute("a.b.a.b", "attr3"), 1 + 2j,
                             'Should return 1+2j')

            h.setAttribute("a.b.a.b", "attr1", True)
            self.assertEqual(h.getAttribute("a.b.a.b", "attr1"), True,
                             'Should return "someValue"')
            self.assertEqual(h.getAttribute("a.b.a.b", "attr2"), 43,
                             'Should return 42')

            attrs = h.getAttributes("a.b.a.b")
            self.assertEqual(attrs.size(), 3)
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
            h.setAttribute("a.b.a.b", "attr1", [1, 2, 3])
            self.assertEqual(h.getAttribute("a.b.a.b", "attr1")[1], 2)
        except Exception as e:
            self.fail("test_attributes exception group 3: " + str(e))

        try:
            h = Hash("a.b.a.b", 42)
            h.setAttribute("a.b.a.b", "attr1", "someValue")
            self.assertEqual(h.getAttribute("a.b.a.b", "attr1"), "someValue",
                             'Should return "someValue"')
        except Exception as e:
            self.fail("test_attributes exception group 4: " + str(e))

        try:
            h = Hash("a.b.a.b", 42)
            h.setAttribute("a.b.a.b", "attr1", "someValue")
            self.assertEqual(h.getAttribute("a.b.a.b", "attr1"), "someValue",
                             'Should return "someValue"')
        except Exception as e:
            self.fail("test_attributes exception group 5: " + str(e))

        if Hash is karathon.Hash:
            try:
                h = Hash("a.b.a.b", 42)
                h.setAttribute("a.b.a.b", "attr1", [1, 2, 3, 4, 5, 6, 7])

                setStdVectorDefaultConversion(Types.PYTHON)
                if isStdVectorDefaultConversion(Types.PYTHON):
                    self.assertEqual(h.getAttribute("a.b.a.b", "attr1"),
                                     [1, 2, 3, 4, 5, 6, 7])
                if isStdVectorDefaultConversion(Types.NUMPY):
                    self.assertEqual(h.getAttribute("a.b.a.b", "attr1").all(),
                                     np.array([1, 2, 3, 4, 5, 6, 7],
                                              dtype=np.int32).all())

                setStdVectorDefaultConversion(Types.NUMPY)
                if isStdVectorDefaultConversion(Types.PYTHON):
                    self.assertEqual(h.getAttribute("a.b.a.b", "attr1"),
                                     [1, 2, 3, 4, 5, 6, 7])
                if isStdVectorDefaultConversion(Types.NUMPY):
                    self.assertEqual(h.getAttribute("a.b.a.b", "attr1").all(),
                                     np.array([1, 2, 3, 4, 5, 6, 7],
                                              dtype=np.int32).all())

                setStdVectorDefaultConversion(Types.PYTHON)
            except Exception as e:
                self.fail("test_attributes exception group 6: " + str(e))

    def test_attributes_get_copy(self):
        try:
            h = Hash("a.b.a.b", 42)
            h.setAttribute("a.b.a.b", "attr1", True)
            self.assertEqual(h.getAttribute("a.b.a.b", "attr1"), True)

            attrs = h.copyAttributes("a.b.a.b")
            attrs["attr1"] = False
            self.assertEqual(h.getAttribute("a.b.a.b", "attr1"), True)
        except Exception as e:
            self.fail("test_attributes_get_copy exception group 1: " + str(e))

        try:
            h = Hash("a.b.a.b", 42)
            h.setAttribute("a.b.a.b", "attr1", True)
            self.assertEqual(h.getAttribute("a.b.a.b", "attr1"), True)

            attrs = h.getAttributes("a.b.a.b")
            attrs["attr1"] = False
            self.assertEqual(h.getAttribute("a.b.a.b", "attr1"), False)
        except Exception as e:
            self.fail("test_attributes_get_copy exception group 2: " + str(e))

        try:
            h = Hash("a.b.a.b", 42)
            h.setAttribute("a.b.a.b", "attr1", True)
            self.assertEqual(h.getAttribute("a.b.a.b", "attr1"), True)
            node = h.getNode("a.b.a.b")
            attrs = node.copyAttributes()
            attrs["attr1"] = False
            self.assertEqual(h.getAttribute("a.b.a.b", "attr1"), True)
        except Exception as e:
            self.fail("test_attributes_get_copy exception group 3: " + str(e))

        try:
            h = Hash("a.b.a.b", 42)
            h.setAttribute("a.b.a.b", "attr1", True)
            self.assertEqual(h.getAttribute("a.b.a.b", "attr1"), True)
            node = h.getNode("a.b.a.b")
            attrs = node.getAttributes()
            attrs["attr1"] = False
            self.assertEqual(h.getAttribute("a.b.a.b", "attr1"), False)
        except Exception as e:
            self.fail("test_attributes_get_copy exception group 4: " + str(e))

    def test_merge(self):

        h1 = Hash("a", 1,
                  "b", 2,
                  "c.b[0].g", 3,
                  "c.c[0].d", 4,
                  "c.c[1]", Hash("a.b.c", 6),
                  "d.e", 7)
        h1.set("f.g", 99)
        h1.set("h", -1)
        h1.setAttribute("a", "attrKey", "Just a number")
        h1.setAttribute("c.b", "attrKey2", 3)
        h1.setAttribute("c.b[0].g", "attrKey3", 4.)
        h1.setAttribute("f", "attrKey6", "buaah!")

        h1b = Hash(h1)
        h1c = Hash(h1)
        h1d = Hash(h1)

        h2 = Hash("a", 21,
                  "b.c", 22,
                  "c.b[0]", Hash("key", "value"),
                  "c.b[1].d", 24,
                  "e", 27,
                  "f", Hash())
        h2.set("g.h.i", -88)
        h2.set("g.h.j", -188)
        h2.set("h.i", -199)
        h2.set("h.j", 200)
        h2.set("i[3]", Hash())
        h2.set("i[1].j", 200)
        h2.set("i[2]", Hash("k.l", 5.))
        h2.set("j", Hash("k", 5.))
        h2.setAttribute("a", "attrKey", "Really just a number")
        h2.setAttribute("e", "attrKey4", -1)
        h2.setAttribute("e", "attrKey5", -11.)
        h2.setAttribute("f", "attrKey7", 77)
        # attribute on new vector<Hash> node
        h2.setAttribute("i", "attrKey8", 123)
        h2.setAttribute("j", "attrKey9", 12.3)  # ... and new Hash node

        h1.merge(h2)  # HashMergePolicy.REPLACE_ATTRIBUTES is the default
        h1b.merge(h2, HashMergePolicy.MERGE_ATTRIBUTES)
        # same as h1d.merge(h2): only check similarity and once
        # attribute replacement below
        h1d += h2

        self.assertTrue(similar(h1, h1b),
                        "Replace or merge attributes influenced "
                        "resulting paths")
        self.assertTrue(similar(h1, h1d), "merge and += don't do the same")

        self.assertTrue(h1.has("a"))
        self.assertEqual(h1.get("a"), 21)  # new value
        #  Attribute kept, but value overwritten:
        self.assertTrue(h1.hasAttribute("a", "attrKey"),
                        "Attribute on node not kept")
        self.assertEqual("Really just a number",
                         h1.getAttribute("a", "attrKey"),
                         "Attribute not overwritten")
        self.assertEqual(1, h1.getAttributes("a").size(),
                         "Attribute added out of nothing")

        self.assertTrue(h1b.hasAttribute("a", "attrKey"),
                        "Attribute on node not kept (MERGE)")
        self.assertEqual("Really just a number",
                         h1b.getAttribute("a", "attrKey"),
                         "Attribute not overwritten (MERGE)")
        self.assertEqual(1, h1b.getAttributes("a").size(),
                         "Attribute added out of nothing (MERGE)")

        self.assertTrue(h1.has("b"))
        self.assertTrue(h1.isType("b", Types.HASH))  # switch to new type...
        self.assertTrue(h1.has("b.c"))  # ...and as Hash can hold a child

        # Attributes overwritten by nothing or kept
        self.assertEqual(0, h1.getAttributes("c.b").size(),
                         "Attributes on node kept")
        self.assertEqual(1, h1.getAttributes("c.b[0].g").size(),
                         "Attributes on untouched leaf not kept")
        self.assertTrue(h1.hasAttribute("c.b[0].g", "attrKey3"))
        self.assertEqual(4., h1.getAttribute("c.b[0].g", "attrKey3"),
                         "Attribute on untouched leaf changed")

        self.assertEqual(1, h1b.getAttributes("c.b").size(),
                         "Number of attributes on node changed (MERGE)")
        self.assertEqual(1, h1b.getAttributes("c.b[0].g").size(),
                         "Number of attributes on leaf changed (MERGE)")
        self.assertTrue(h1b.hasAttribute("c.b", "attrKey2"),
                        "Attribute on node not kept (MERGE)")
        self.assertEqual(3, h1b.getAttribute("c.b", "attrKey2"),
                         "Attribute on node changed (MERGE)")
        self.assertTrue(h1b.hasAttribute("c.b[0].g", "attrKey3"),
                        "Attribute on untouched leaf not kept (MERGE)")
        self.assertEqual(4., h1b.getAttribute("c.b[0].g", "attrKey3"),
                         "Attribute on untouched leaf changed (MERGE)")

        self.assertFalse(h1.has("c.b.d"))
        self.assertTrue(h1.has("c.b[0]"))
        self.assertTrue(h1.has("c.b[1]"))
        self.assertEqual(h1.get("c.b[1].d"), 24)
        self.assertTrue(h1.has("c.c[0].d"))
        self.assertTrue(h1.has("c.c[1].a.b.c"))
        self.assertTrue(h1.has("d.e"))
        self.assertTrue(h1.has("e"))
        self.assertTrue(h1.has("g.h.i"))
        self.assertTrue(h1.has("g.h.j"))
        self.assertTrue(h1.has("h.i"))
        self.assertTrue(h1.has("h.j"))
        self.assertTrue(h1.has("i[1].j"))
        self.assertTrue(h1.has("i[2].k.l"))
        self.assertTrue(h1.has("i[3]"))
        self.assertTrue(h1.has("j.k"))

        # Just add attributes with leaf (identical for REPLACE or MERGE)
        self.assertEqual(2, h1.getAttributes("e").size(),
                         "Not all attributes on leaf added")
        self.assertEqual(-1, h1.getAttribute("e", "attrKey4"),
                         "Int attribute value incorrect")
        self.assertEqual(-11., h1.getAttribute("e", "attrKey5"),
                         "Float attribute value incorrect")
        self.assertEqual(2, h1b.getAttributes("e").size(),
                         "Not all attributes on leaf added (MERGE)")
        self.assertEqual(-1, h1b.getAttribute("e", "attrKey4"),
                         "Int attribute value incorrect (MERGE)")
        self.assertEqual(-11., h1b.getAttribute("e", "attrKey5"),
                         "Float attribute value incorrect (MERGE)")
        # Just add attributes for new Hash/vector<Hash>
        # (identical for REPLACE or MERGE)
        self.assertEqual(1, h1.getAttributes("i").size(),
                         "Not all attributes on vector<Hash> added")
        self.assertEqual(123, h1.getAttribute("i", "attrKey8"),
                         "attributes on vector<Hash> wrong")
        self.assertEqual(1, h1.getAttributes("j").size(),
                         "Not all attributes on Hash added")
        self.assertEqual(12.3, h1.getAttribute("j", "attrKey9"),
                         "Attribute on Hash wrong")

        self.assertEqual(1, h1b.getAttributes("i").size(),
                         "Not all attributes on vector<Hash> added (MERGE)")
        self.assertEqual(123, h1b.getAttribute("i", "attrKey8"),
                         "Attributes on vector<Hash> wrong  (MERGE)")
        self.assertEqual(1, h1b.getAttributes("j").size(),
                         "Not all attributes on Hash added (MERGE)")
        self.assertEqual(12.3, h1b.getAttribute("j", "attrKey9"),
                         "A attributes on Hash wrong (MERGE)")

        self.assertTrue(h1b.hasAttribute("c.b", "attrKey2"),
                        "Attribute on node not kept (MERGE)")

        self.assertTrue(h1.has("f"))
        self.assertTrue(h1.has(
            "f.g"))  # merging does not overwrite h1["f"] with empty Hash

        self.assertEqual(1, h1.getAttributes("f").size(),
                         "Attributes not replaced")
        self.assertEqual(77, h1.getAttribute("f", "attrKey7"),
                         "Attribute value incorrect")
        # += is merge with REPLACE_ATTRIBUTES
        self.assertEqual(1, h1d.getAttributes("f").size(),
                         "Attributes not replaced (+=)")
        self.assertEqual(77, h1d.getAttribute("f", "attrKey7"),
                         "Attribute value incorrect (+=)")
        # here is MERGE_ATTRIBUTES
        self.assertEqual(2, h1b.getAttributes("f").size(),
                         "Attributes not merged")
        self.assertEqual("buaah!", h1b.getAttribute("f", "attrKey6"),
                         "Attribute value incorrect (MERGE)")
        self.assertEqual(77, h1b.getAttribute("f", "attrKey7"),
                         "Attribute value incorrect (MERGE)")

        # Now check the 'selectedPaths' feature (no extra test
        # for attribute merging needed):
        selectedPaths = ["a", "b.c", "g.h.i", "h.i", "i[2]", "i[5]"]
        # check that we tolerate to select path with invalid index

        h1c.merge(h2, HashMergePolicy.MERGE_ATTRIBUTES, selectedPaths)

        # Keep everything it had before merging:
        self.assertTrue(h1c.has("a"))
        self.assertTrue(h1c.has("b"))
        self.assertTrue(h1c.has("c.b[0].g"))
        self.assertTrue(h1c.has("c.c[0].d"))
        self.assertTrue(h1c.has("c.c[1].a.b.c"))
        self.assertTrue(h1c.has("d.e"))
        self.assertTrue(h1c.has("f.g"))
        # The additionally selected ones from h2:
        self.assertTrue(h1c.has("b.c"))
        self.assertTrue(h1c.has("g.h.i"))
        self.assertTrue(h1c.has("h.i"))
        self.assertTrue(h1c.has("i[2].k.l"))
        # But not the other ones from h2:
        self.assertFalse(
            h1c.has("c.b[0].key"))  # neither at old position of h2
        self.assertFalse(
            h1c.has("c.b[2]"))  # nor an extended vector<Hash> at all
        self.assertFalse(h1c.has("e"))
        # Take care that adding path "g.h.i" does not trigger
        # that other children of "g.h"
        # in h2 are taken as well:
        self.assertFalse(h1c.has("g.h.j"))
        self.assertFalse(h1c.has("h.j"))
        # Adding i[2] should not trigger to add children of i[1] nor i[3]]
        self.assertFalse(h1c.has("i[1].j"))
        self.assertFalse(h1c.has("i[3]"))

        # Some further small tests for so far untested cases with
        # selected paths...
        hashTarget = Hash("a.b", 1, "a.c", Hash(), "c", "so so!")
        hashSource = Hash("a.d", 8., "ha", 9)
        selectedPaths = ("a",)
        # trigger merging a.d - tuple should work as well
        hashTarget.merge(hashSource, HashMergePolicy.MERGE_ATTRIBUTES,
                         selectedPaths)
        self.assertTrue(hashTarget.has("a.d"))

        hashTargetB = Hash("a[1].b", 1, "c", "Does not matter")
        hashTargetC = Hash(hashTargetB)
        hashTargetD = Hash(hashTargetB)
        hashSourceBCD = Hash("a[2]", Hash("a", 33, "b", 4.4), "ha", 9,
                             "c[1]", Hash("k", 5, "l", 6),
                             "c[2]", Hash("b", -3), "d[2].b", 66,
                             "e[1]", Hash("1", 1, "2", 2, "3", 3))
        selectedPaths = ["a",  # trigger merging full vector
                         # trigger selecting first HashVec item overwriting
                         # what was not a hashVec before, but only keep
                         # selected items
                         "c[1].l",
                         "d",  # trigger adding full new vector
                         "e[1].2",
                         # selective adding of hashVec where there
                         #  was no node before
                         "e[1].3"]
        hashTargetB.merge(hashSourceBCD, HashMergePolicy.MERGE_ATTRIBUTES,
                          selectedPaths)
        self.assertTrue(hashTargetB.has("a[1].b"))
        self.assertTrue(hashTargetB.has("a[2].a"))
        self.assertTrue(hashTargetB.has("a[2].b"))
        self.assertFalse(hashTargetB.has("a[5]"))
        self.assertTrue(hashTargetB.has("c[0]"))
        self.assertFalse(hashTargetB.has("c[0].k"))
        self.assertTrue(hashTargetB.has("c[1].l"))
        self.assertTrue(hashTargetB.has("d[2].b"))
        self.assertFalse(hashTargetB.has("d[3]"))
        self.assertFalse(hashTargetB.has("e[0].1"))
        self.assertTrue(hashTargetB.has("e[1].2"))
        self.assertTrue(hashTargetB.has("e[1].3"))

        selectedPaths = ("a[0]",
                         "a[2].b",  # trigger selective vector items
                         "c")  # trigger overwriting with complete vector
        hashTargetC.merge(hashSourceBCD, HashMergePolicy.MERGE_ATTRIBUTES,
                          selectedPaths)
        self.assertTrue(hashTargetC.has("a[1].b"))
        self.assertFalse(hashTargetC.has("a[2].a"))
        self.assertTrue(hashTargetC.has("a[2].b"))
        self.assertFalse(hashTargetC.has("a[3]"))
        self.assertTrue(hashTargetC.has("c[1].k"))
        self.assertTrue(hashTargetC.has("c[1].l"))
        self.assertTrue(hashTargetC.has("c[2].b"))
        self.assertFalse(hashTargetC.has("c[3]"))

        # Now select only invalid indices - nothing should be added
        selectedPaths = ["a[10]",  # to existing vector
                         "c[10]",  # where there was another node
                         "d[10]",  # where there was no node at all
                         "ha[0]"]  # for leaves, all indices are invalid
        copyD = Hash(hashTargetD)
        # also test use of keyword parameter:
        hashTargetD.merge(hashSourceBCD, selectedPaths=selectedPaths)
        self.assertTrue(similar(copyD, hashTargetD),
                        "Selecting only invalid indices changed something")

        h = Hash('a[0].a.b.c', 1, 'a[1].a.b.d', 2)
        g = Hash('a[0].x.y.w', 77, 'a[0].a.c', 33, 'a[2].abc', 12)
        h += g
        self.assertTrue(h['a[0].a.b.c'] == 1)
        self.assertTrue(h['a[0].x.y.w'] == 77)
        self.assertTrue(h['a[0].a.c'] == 33)
        self.assertTrue(h['a[1].a.b.d'] == 2)
        self.assertTrue(h['a[2].abc'] == 12)

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

            self.assertTrue("a.b" in h3)
            self.assertFalse(h3["a"].empty())
            self.assertTrue("c" in h3)
            self.assertTrue(h3["b.c.d"] == 22)

        except Exception as e:
            self.fail("test_subtract exception group 2: " + str(e))

    def test_dict(self):
        try:
            h = Hash("a", {"b": {"c": {"d": [1, 2, 3, 4, 5]}}})

            self.assertEqual(h["a.b.c.d"], [1, 2, 3, 4, 5])
            self.assertEqual(h["a"]["b"]["c"]["d"], [1, 2, 3, 4, 5])

            h.set('x', {'y': {'z': True}})

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
            h.setAttribute("a.b.c", "attr1", 15)
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
            h.setAttribute("a.b.c", "attr1", 10)
            h.setAttribute("a.b.c", "attr2", "test")
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
            h.setAttribute("a.b.c", "attr1", 12)
            h.setAttribute("a.b.c", "attr2", True)

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

        # first testing erasePath
        try:
            h = Hash('a[0].b[0].c', 1, 'b[0].c.d', 2, 'c.d[0].e', 3, 'd.e', 4,
                     'e', 5, 'f.g.h.i.j.k', 6)
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

        # now testing erase
        h1 = Hash('a', 1, 'b', 2, 'c.d', 31, 'e.f.g', 411, 'e.f.h', 412, 'e.i',
                  42)

        try:
            self.assertEqual(len(h1), 4)

            # erase existing key on first level => size decreases
            self.assertTrue(h1.erase('a'))
            self.assertFalse('a' in h1)
            self.assertEqual(len(h1), 3)

            # non-existing key - return false and keep size:
            self.assertFalse(h1.erase('a'))
            self.assertEqual(len(h1), 3)

            # 'c.d': composite key without siblings
            self.assertTrue(h1.erase('c.d'))
            self.assertFalse('c.d' in h1)
            self.assertTrue('c' in h1)
            self.assertEqual(len(h1), 3)  # 'c' still in!

            # 'e.f': composite key with two children and a sibling
            self.assertTrue(h1.erase('e.f'))
            self.assertFalse('e.f.g' in h1)
            self.assertFalse('e.f.h' in h1)
            self.assertFalse('e.f' in h1)
            self.assertTrue('e' in h1)  # stays
            self.assertEqual(len(h1), 3)
        except Exception as e:
            self.fail("test_erase exception 3: " + str(e))

    def test_fullEqual(self):

        h = Hash('a', 1, 'b', 2.2, 'c.d', "text",
                 'e.f.g', VectorHash())
        h.setAttribute('a', "attr", 42)

        # A copy fully equals:
        h1 = copy.copy(h)
        self.assertTrue(fullyEqual(h1, h), "h1: " + str(h1))
        self.assertTrue(fullyEqual(h1, h, False), "h1: " + str(h1))

        # A changed value leads to being different
        h2 = copy.copy(h)
        h2["c.d"] = "TEXT"
        self.assertFalse(fullyEqual(h2, h), "h2: " + str(h2))
        self.assertFalse(fullyEqual(h2, h, False), "h2: " + str(h2))

        # An added value leads to being different
        h3 = copy.copy(h)
        h3["newKey"] = -11
        self.assertFalse(fullyEqual(h3, h), "h3: " + str(h3))
        self.assertFalse(fullyEqual(h3, h, False), "h3: " + str(h3))

        # A changed attribute leads to being different
        h4 = copy.copy(h)
        h4.setAttribute("a", "attr", -42)
        self.assertFalse(fullyEqual(h4, h), "h4: " + str(h4))
        self.assertFalse(fullyEqual(h4, h, False), "h4: " + str(h4))

        # An added attribute leads to being different
        h5 = copy.copy(h)
        h5.setAttribute("a", "newAttr", 0)
        self.assertFalse(fullyEqual(h5, h), "h5: " + str(h5))
        self.assertFalse(fullyEqual(h5, h, False), "h5: " + str(h5))

        # A changed order of keys leads to being different,
        # except if flag to ignore order is given
        h6 = copy.copy(h)
        bValue = h6["b"]
        h6.erase("b")
        h6["b"] = bValue
        self.assertFalse(fullyEqual(h6, h), "h6: " + str(h6))
        self.assertTrue(fullyEqual(h6, h, False), "h6: " + str(h6))

        # A changed order of attribute keys leads to being different,
        # except if flag to ignore order is given
        h1.setAttribute("a", "attr2", 43)  # 2nd attribute
        h7 = copy.copy(h1)
        attr2Value = h7.getAttribute("a", "attr")
        h7.getAttributes("a").erase("attr")  # erase 1st attribute
        h7.setAttribute("a", "attr", attr2Value)  # re-add after previous 2nd
        self.assertFalse(fullyEqual(h7, h1), "h7: " + str(h7))
        self.assertTrue(fullyEqual(h7, h1, False), "h7: " + str(h7))


if __name__ == '__main__':
    unittest.main()
