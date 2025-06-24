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

import gc
import unittest
import weakref

import numpy as np

from karabo.bound import Hash, Types
from karabo.testing.utils import compare_ndarray_data_ptrs


class Hash_TestCase(unittest.TestCase):

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
        with self.assertRaises(RuntimeError):
            h = Hash("b", 2 ** 64)
        with self.assertRaises(RuntimeError):
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

    def test_hash_update(self):
        h = Hash("a.b.a.b", 42)
        h2 = h.update({"a.b.a.b": 10})
        self.assertEqual(h["a.b.a.b"], 10)
        # test the correct reference
        self.assertTrue(h2 is h)
        del h
        gc.collect()
        self.assertEqual(h2["a.b.a.b"], 10)

        h = Hash()
        int_list = [1, 2, 3, 4]
        h.update(e=int_list)
        self.assertEqual(h["e"], int_list)
        self.assertEqual(h["e"][0], 1)
        int_list[0] = 10
        self.assertEqual(h["e"][0], 1)
        del int_list
        self.assertEqual(h["e"], [1, 2, 3, 4])
        with self.assertRaises(RuntimeError):
            h.update([1, 2, 3])

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


if __name__ == '__main__':
    unittest.main()
