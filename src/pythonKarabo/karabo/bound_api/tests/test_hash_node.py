import unittest

import karabind

import karathon


class Tests(unittest.TestCase):
    def test_hashNode(self):
        def inner(Hash, HashNode, Types):
            # Test "getKey", "setValue", "getValue", "getValueAs",
            #      "setAttribute", "getAttribute", "getAttributeAs",
            #      "hasAttribute", "setAttributes", "getAttributes",
            #      "copyAttributes", "getType", "setType"

            # 'getValueAs' for HashNode is testing also the other methods:
            # `getAs`, `getAttributeAs` for Hash and HashAttributesNode.
            # Only one conversion version is used.
            # One exception to that is Hash's `getAs` for few types.

            # BOOL
            # Boolean is something special which can be converted
            # to any other type
            h = Hash("a.b.c", True)
            self.assertTrue(isinstance(h, Hash))
            node = h.getNode("a.b.c")
            self.assertTrue(isinstance(node, HashNode))

            # Test "getKey"
            self.assertEqual(node.getKey(), 'c')

            # Test "getValue"
            self.assertEqual(node.getValue(), True)

            # Test "getType"
            self.assertEqual(node.getType(), Types.BOOL)

            # Test "setType"
            node.setType("VECTOR_BOOL")
            self.assertEqual(node.getType(), Types.VECTOR_BOOL)
            self.assertEqual(node.getValue(), [True])
            node.setType(Types.VECTOR_INT32)
            self.assertEqual(node.getType(), Types.VECTOR_INT32)
            self.assertEqual(node.getValue(), [1])

            # Test "setValue"
            node.setValue([1, 2, 3])
            self.assertEqual(node.getValue(), [1, 2, 3])
            node.setValue(2.7818281828)
            self.assertEqual(node.getValue(), 2.7818281828)
            self.assertEqual(node.getType(), Types.DOUBLE)

            # Test "getValueAs"
            node.setValue(True)
            self.assertEqual(node.getValueAs(Types.CHAR), '1')
            self.assertEqual(node.getValueAs("INT8"), 1)
            # "getValueAs" uses default conversion tested in
            # "test_hash_attributes_node.py"
            # .... skipped tests
            self.assertEqual(node.getValueAs(Types.DOUBLE), 1.0)
            if Types is karabind.Types:
                self.assertEqual(node.getValueAs("COMPLEX_FLOAT"), (1+0j))
                self.assertEqual(node.getValueAs("COMPLEX_DOUBLE"), (1+0J))
            self.assertEqual(node.getValueAs(Types.STRING), '1')
            self.assertEqual(node.getValueAs(Types.VECTOR_BOOL), [True])
            self.assertEqual(node.getValueAs(Types.VECTOR_INT16), [1])
            # .... skipped tests ...
            if Types is karabind.Types:
                self.assertEqual(node.getValueAs(Types.VECTOR_FLOAT), [1.0])
                self.assertEqual(node.getValueAs(Types.VECTOR_DOUBLE), [1.0])
                self.assertEqual(node.getValueAs(Types.VECTOR_COMPLEX_FLOAT),
                                 [(1+0j)])
                self.assertEqual(node.getValueAs(Types.VECTOR_COMPLEX_DOUBLE),
                                 [(1+0j)])
            # Call is not supported ...
            # FIXME: VECTOR_STRING not supported in both cases

            # INT32
            # Special value 1 or 0 can be converted like boolean to any type...
            h = Hash('a.b.c', 1)
            n = h.getNode('a.b.c')
            self.assertEqual(n.getKey(), 'c')
            self.assertEqual(n.getValue(), 1)
            self.assertEqual(n.getType(), Types.INT32)
            self.assertEqual(n.getValueAs(Types.BOOL), True)
            self.assertEqual(n.getValueAs(Types.CHAR), '1')
            self.assertEqual(n.getValueAs(Types.INT8), 1)
            # ... skipped tests ...
            self.assertEqual(n.getValueAs(Types.UINT64), 1)
            self.assertEqual(n.getValueAs(Types.FLOAT), 1.0)
            self.assertEqual(n.getValueAs(Types.DOUBLE), 1.0)
            if Types is karabind.Types:
                self.assertEqual(n.getValueAs(Types.COMPLEX_FLOAT), (1+0j))
                self.assertEqual(n.getValueAs(Types.COMPLEX_DOUBLE), (1+0j))
            self.assertEqual(n.getValueAs(Types.STRING), '1')
            self.assertEqual(n.getValueAs(Types.VECTOR_BOOL), [True])
            # self.assertEqual(n.getValueAs(Types.VECTOR_CHAR), ['1'])
            # self.assertEqual(n.getValueAs(Types.VECTOR_INT8), [1])
            # self.assertEqual(n.getValueAs(Types.VECTOR_UINT8), [1])
            self.assertEqual(n.getValueAs(Types.VECTOR_INT16), [1])
            # ... skipped tests
            self.assertEqual(n.getValueAs(Types.VECTOR_UINT64), [1])
            if Types is karabind.Types:
                self.assertEqual(n.getValueAs(Types.VECTOR_FLOAT), [1.0])
                self.assertEqual(n.getValueAs(Types.VECTOR_DOUBLE), [1.0])
                self.assertEqual(n.getValueAs(Types.VECTOR_COMPLEX_FLOAT),
                                 [(1+0j)])
                self.assertEqual(n.getValueAs(Types.VECTOR_COMPLEX_DOUBLE),
                                 [(1+0j)])
            # Call is not supported ...
            # self.assertEqual(n.getValueAs(Types.VECTOR_STRING), ['1'])

            h = Hash("a.b.c", 42)
            n = h.getNode("a.b.c")
            self.assertEqual(n.getValue(), 42)
            self.assertEqual(n.getType(), Types.INT32)
            self.assertEqual(n.getValueAs(Types.INT8), 42)
            # ... skipped tests
            self.assertEqual(n.getValueAs(Types.DOUBLE), 42)
            if Types is karabind.Types:
                self.assertEqual(n.getValueAs(Types.COMPLEX_FLOAT), (42+0j))
                self.assertEqual(n.getValueAs(Types.COMPLEX_DOUBLE), (42+0j))
            self.assertEqual(n.getValueAs(Types.STRING), '42')
            # Base64 encoding
            # Since we use `karabo::util::StringTools.hh` for conversion
            # the following types are converted using Base64 encoding, so
            # we skip them here...
            # self.assertEqual(n.getValueAs(Types.VECTOR_INT8), [42])
            # self.assertEqual(n.getValueAs(Types.VECTOR_UINT8), [42])
            self.assertEqual(n.getValueAs(Types.VECTOR_INT16), [42])
            # ... skipped tests
            self.assertEqual(n.getValueAs(Types.VECTOR_UINT64), [42])
            if Types is karabind.Types:
                self.assertEqual(n.getValueAs(Types.VECTOR_FLOAT), [42.0])
                self.assertEqual(n.getValueAs(Types.VECTOR_DOUBLE), [42.0])
                self.assertEqual(
                    n.getValueAs("VECTOR_COMPLEX_FLOAT"), [(42+0j)])
                self.assertEqual(
                    n.getValueAs("VECTOR_COMPLEX_DOUBLE"), [(42+0j)])
            # Call is not supported...
            # self.assertEqual(n.getValueAs(Types.VECTOR_STRING), '42')

            # Test "setAttribute"
            h = Hash("a", 42)
            h.setAttribute("a", "a", 15)
            node = h.getNode("a")
            self.assertTrue(isinstance(node, HashNode))

            # Test "getAttribute"
            self.assertEqual(node.getAttribute("a"), 15)

            # Test "getAttributeAs"
            self.assertEqual(node.getAttributeAs("a", Types.INT8), 15)
            # ... skipped tests
            self.assertEqual(node.getAttributeAs("a", "DOUBLE"), 15)
            if Types is karabind.Types:
                self.assertEqual(
                    node.getAttributeAs("a", Types.COMPLEX_FLOAT), (15+0j))
                self.assertEqual(
                    node.getAttributeAs("a", Types.COMPLEX_DOUBLE), (15+0j))
            self.assertEqual(node.getAttributeAs("a", Types.STRING), '15')
            # Base64 encoding
            # self.assertEqual(
            #     node.getAttributeAs("a", Types.VECTOR_INT8), [15])
            # self.assertEqual(
            #     node.getAttributeAs("a", Types.VECTOR_UINT8), [15])
            if Types is karabind.Types:
                self.assertEqual(
                    node.getAttributeAs("a", "VECTOR_INT16"), [15])
                # ... skipped tests
                self.assertEqual(
                    node.getAttributeAs("a", Types.VECTOR_DOUBLE), [15])
                self.assertEqual(
                    node.getAttributeAs("a", Types.VECTOR_COMPLEX_FLOAT),
                    [(15+0j)])
                self.assertEqual(
                    node.getAttributeAs("a", Types.VECTOR_COMPLEX_DOUBLE),
                    [(15+0j)])
            # Not supported
            # self.assertEqual(
            #     node.getAttributeAs("a", Types.VECTOR_STRING), ['15'])

            # Test "hasAttribute"
            self.assertTrue(node.hasAttribute("a"))
            self.assertFalse(node.hasAttribute("b"))

            # Test "setAttributes", "getAttributes", "copyAttributes"
            h = Hash('a', 12)
            n1 = h.getNode('a')
            n1.setAttribute('a', 1)
            n1.setAttribute('b', True)
            n1.setAttribute('c', "test")
            n1.setAttribute('d', 3.141592)

            # Test "getAttributes"
            attrs = n1.getAttributes()
            self.assertEqual(attrs['a'], 1)
            self.assertEqual(attrs['d'], 3.141592)
            attrs['b'] = False
            self.assertFalse(n1.getAttribute('b'))

            h['b'] = "value"
            n2 = h.getNode('b')

            # Test "setAttributes"
            n2.setAttributes(attrs)  # copy
            n1.setAttribute('c', "newtest")
            self.assertNotEqual(n2.getAttribute('c'), "newtest")

            # Test "copyAttributes"
            attrs2 = n1.copyAttributes()
            attrs2['a'] = 2
            self.assertNotEqual(n1.getAttribute('a'), 2)

        # Run test in karabind version
        inner(karabind.Hash, karabind.HashNode, karabind.Types)
        # Run test in karathon version
        inner(karathon.Hash, karathon.HashNode, karathon.Types)
