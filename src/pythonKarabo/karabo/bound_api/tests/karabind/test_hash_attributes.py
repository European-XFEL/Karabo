# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
import unittest

import karabind

import karathon


class Tests(unittest.TestCase):
    def test_hashAttributes(self):
        def inner(Hash, HashAttributes, HashAttributesNode, Types):
            # Test "has", "__contains__", "isType", "getType", "erase",
            #      "__delitem__", "size", "__len__", "empty", "bool",
            #      "clear",  "getNode", "get", "__getitem__", "getAs", "set",
            #      "__setitem__", "__iter__"

            h = Hash("a.b.c", "value")
            attrs = h.getAttributes("a.b.c")
            self.assertTrue(isinstance(attrs, HashAttributes))

            # Test 'empty'
            self.assertTrue(attrs.empty())

            # Test 'bool'
            self.assertTrue(not attrs)

            # Test 'has' ...
            self.assertTrue(not attrs.has('a.b.c'))

            # Test 'set'...
            attrs.set("aq", 12345678993758)
            self.assertTrue(attrs.has("aq"))

            # Test 'get' ...
            self.assertEqual(attrs.get("aq"), 12345678993758)

            # Test "__len__"
            self.assertEqual(len(attrs), 1)
            # add the second attribute ...
            attrs.set("ua", "interesting")

            # Test "__getitem__"
            self.assertEqual(attrs["ua"], "interesting")
            self.assertEqual(len(attrs), 2)

            # Test "isType"
            # self.assertTrue(attrs.isType("ua", "STRING"))
            # self.assertTrue(attrs.isType("ua", Types.STRING))

            # Test "size"
            self.assertEqual(attrs.size(), 2)

            # add one more attribute ...
            attrs['minerale'] = [True, False]
            self.assertEqual(attrs["minerale"], [True, False])

            # Test "getType", "isType"
            if Types is karabind.Types:
                # check attribute types ...
                self.assertEqual(attrs.getType('minerale'), Types.VECTOR_BOOL)
                # self.assertTrue(attrs.isType('minerale', "VECTOR_BOOL"))

            # Test "__contains__ ..."
            self.assertTrue("aq" in attrs)

            # Test "erase" ...
            attrs.erase("aq")
            self.assertFalse("aq" in attrs)

            # Test "getNode"
            node = attrs.getNode("minerale")
            self.assertTrue(isinstance(node, HashAttributesNode))
            # Tests for node can be found in "test_hash_attributes_node.py".

            # Test "__delitem__" ... reference to "minerale" node
            del(node)
            self.assertTrue("minerale" in attrs)
            # delete attribute
            del(attrs['minerale'])
            self.assertTrue('minerale' not in attrs)
            # attribute 'ua' is still in attrs container
            self.assertFalse(attrs.empty())

            # Test "clear"
            attrs.clear()
            self.assertTrue(attrs.empty())
            self.assertFalse(bool(attrs))

            # set new attributes ...
            # Test 'set'
            attrs.set("a1", 12)
            attrs.set("a2", True)

            # Test "__setitem__" ...
            attrs["a3"] = 120_333_666
            attrs["a4"] = 3.141592
            attrs["a5"] = [1, 2, 3]
            attrs.set("a6", 'abrakadabra')
            attrs["a7"] = ['abra', 'kadabra']
            self.assertEqual(len(attrs), 7)
            self.assertEqual(attrs.size(), 7)

            # Test "__iter__"
            it = iter(attrs)
            # Test first item
            n = next(it)
            self.assertTrue(isinstance(n, HashAttributesNode))
            self.assertEqual(n.getKey(), 'a1')
            self.assertEqual(n.getValue(), 12)
            self.assertEqual(n.getType(), Types.INT32)
            n = next(it)
            self.assertEqual(n.getKey(), 'a2')
            n = next(it)
            self.assertEqual(n.getKey(), 'a3')
            n = next(it)
            self.assertEqual(n.getKey(), 'a4')
            n = next(it)
            self.assertEqual(n.getKey(), 'a5')
            n = next(it)
            self.assertEqual(n.getKey(), 'a6')
            self.assertEqual(n.getValue(), 'abrakadabra')
            self.assertEqual(n.getType(), Types.STRING)
            n = next(it)
            self.assertEqual(n.getKey(), 'a7')
            self.assertEqual(n.getValue(), ['abra', 'kadabra'])
            self.assertEqual(n.getType(), Types.VECTOR_STRING)
            # Test end of iteration: "StopIteration"
            with self.assertRaises(StopIteration):
                n = next(it)

            attrs = h.getAttributes("a.b.c")

            # We do not need to test `getAs` here for each possible source and
            # target type since `getAs` uses the same conversion method as for
            # 'getValueAs'
            #
            # Test "getAs"
            self.assertEqual(attrs.getAs("a2", "BOOL"), True)
            self.assertEqual(attrs.getAs("a2", Types.VECTOR_BOOL), [True])
            self.assertEqual(attrs.getAs("a2", "VECTOR_UINT64"), [1])
            self.assertEqual(attrs.getAs("a2", "FLOAT"), 1.0)
            if Types is karabind.Types:
                self.assertEqual(attrs.getAs("a2", "COMPLEX_FLOAT"), (1+0j))
                self.assertEqual(attrs.getAs("a2", "VECTOR_COMPLEX_DOUBLE"),
                                 [(1+0j)])

            self.assertEqual(attrs.getAs("a1", Types.UINT32), 12)
            self.assertEqual(attrs.getAs("a1", "STRING"), '12')

        # Run test in karabind version
        inner(karabind.Hash, karabind.HashAttributes,
              karabind.HashAttributesNode, karabind.Types)
        # Run test in karathon version
        inner(karathon.Hash, karathon.HashAttributes,
              karathon.HashAttributesNode, karathon.Types)
