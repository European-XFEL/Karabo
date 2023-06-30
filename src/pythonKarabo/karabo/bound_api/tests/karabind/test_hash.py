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
import copy
import unittest

import karabind
import numpy as np

import karathon


class Tests(unittest.TestCase):
    # Test "clear", "empty", "getKeys", "keys", "getValues", "values",
    #      "getPaths", "paths", "set", "__setitem__", "setAs", "get",
    #      "__getitem__", "has", "__contains__", "erase", "__iter__",
    #      "__delitem__", "erasePath", "__len__", "__bool__", "getAs",
    #      "getType", "merge", "__iadd__", "__isub__", "similar",
    #      "fullyEqual", "isType", "flatten", "unflatten", "find",
    #      "setNode", "getNode", "hasAttribute", "getAttribute",
    #      "getAttributeAs", "getAttributes", "copyAttributes",
    #      "setAttribute", "setAttributes", "__copy__",
    #      "__deepcopy__"

    def test_constructor(self):
        def inner(Hash, Node):
            h = Hash()

            # Test "__len__"
            self.assertEqual(len(h), 0)

            # Test "empty"
            self.assertTrue(h.empty())

            # Test "__bool__"
            self.assertFalse(h)
            # It is allowed upto 6 key/value pairs to be defined
            h = Hash('a', 1, 'b', 2, 'c', 3, 'd', 4, 'e', 5, 'f', 6)
            self.assertTrue(h)
            # For further populating, use set ...
            h.set('g', 7)
            self.assertEqual(len(h), 7)
            self.assertFalse(h.empty())

            # Test "clear"
            h.clear()
            self.assertTrue(h.empty())

            # Test constructors
            # Cannot construct Hash with more than 6 key/value pairs
            with self.assertRaises(TypeError):
                h = Hash(
                    'a', 1, 'b', 2, 'c', 3, 'd', 4, 'e', 5, 'f', 6, 'g', 7)
            # Cannot construct Node object in Python ...
            if Node is karabind.HashNode:
                with self.assertRaises(TypeError):
                    print(Node())

        # Run test in karabind version
        inner(karabind.Hash, karabind.HashNode)
        # Run test in karathon version
        inner(karathon.Hash, karathon.HashNode)

    def test_getset(self):
        def inner(Hash, Node, Types, similar, fullyEqual):

            # Test "get" .... default value
            h = Hash()
            self.assertIsNone(h.get("missing"))
            self.assertEqual(h.get("missing", default="value"), "value")

            # Test "set"
            h.set('a', "some text")
            h.set('b.c.d.e.f', 2.718281828)

            # Test "__setitem__"
            h['w.r.a.p'] = 123456789999
            h['c.c.c.c'] = True
            h['p.e.r.f'] = [1.1, 2.2, 3.3]

            # Test "get"
            self.assertEqual(h.get('a'), "some text")
            self.assertEqual(h.get('p.e.r.f'), [1.1, 2.2, 3.3])

            # Test "__getItem__"
            self.assertEqual(h['b.c.d.e.f'], 2.718281828)
            self.assertEqual(h['c.c.c.c'], True)

            # Test "getType"
            self.assertEqual(h.getType('b.c.d.e.f'), Types.DOUBLE)
            self.assertEqual(h.getType('p.e.r.f'), Types.VECTOR_DOUBLE)

            # Test "isType"
            self.assertTrue(h.isType('a', Types.STRING))
            self.assertTrue(h.isType('c.c.c.c', Types.BOOL))
            if Hash is karabind.Hash:
                self.assertTrue(h.isType('a', "STRING"))
                self.assertTrue(h.isType('c.c.c.c', "BOOL"))

            # Test "has"
            self.assertTrue(h.has('p.e.r.f'))
            self.assertFalse(h.has("miss.ing"))

            # Test "keys"
            self.assertEqual(h.keys(), ['a', 'b', 'w', 'c', 'p'])

            # Test "paths"
            self.assertEqual(h.paths(), ['a', 'b.c.d.e.f', 'w.r.a.p',
                                         'c.c.c.c', 'p.e.r.f'])
            # Test "getKeys"
            self.assertEqual(h.getKeys(), ['a', 'b', 'w', 'c', 'p'])

            # Test "getPaths"
            self.assertEqual(h.getPaths(), ['a', 'b.c.d.e.f', 'w.r.a.p',
                                            'c.c.c.c', 'p.e.r.f'])
            # Test C++-ish "getKeys"
            res = []
            h.getKeys(res)
            self.assertEqual(res, ['a', 'b', 'w', 'c', 'p'])

            # Test C++-ish "getPaths"
            res = []
            h.getPaths(res)
            self.assertEqual(res, ['a', 'b.c.d.e.f', 'w.r.a.p', 'c.c.c.c',
                                   'p.e.r.f'])
            # Test "values"
            self.assertEqual(h.values(), ["some text",
                                          Hash('c.d.e.f', 2.718281828),
                                          Hash('r.a.p', 123456789999),
                                          Hash('c.c.c', True),
                                          Hash('e.r.f', [1.1, 2.2, 3.3])])
            # Test "getValues"
            self.assertEqual(h.getValues(), ["some text",
                                             Hash('c.d.e.f', 2.718281828),
                                             Hash('r.a.p', 123456789999),
                                             Hash('c.c.c', True),
                                             Hash('e.r.f', [1.1, 2.2, 3.3])])

            # Test "flatten"
            g = Hash()
            h.flatten(g)
            self.assertEqual(g.keys(), ['a', 'b.c.d.e.f', 'w.r.a.p',
                                        'c.c.c.c', 'p.e.r.f'])
            self.assertEqual(g.keys(), g.paths())
            # ERROR: self.assertTrue('b.c.d.e.f' in g)
            self.assertTrue(g.has('b.c.d.e.f', '/'))
            self.assertFalse(g.has('b.c.d', '/'))
            self.assertEqual(g.values(), ["some text", 2.718281828,
                                          123456789999, True, [1.1, 2.2, 3.3]
                                          ])
            # NOTE: flatten Hash has keys containing '.' considered as default
            # separator.  The 'get with default separator will fail.  To get
            # the value, one can use another separator like
            self.assertEqual(g.get('b.c.d.e.f', '/'), 2.718281828)
            self.assertEqual(g.get('w.r.a.p', '|'), 123456789999)

            # Test "unflatten"
            f = Hash()
            g.unflatten(f)
            self.assertTrue(f == h)  # '==' is 'similar'
            self.assertTrue(similar(f, h))
            self.assertTrue(fullyEqual(f, h))
            g.clear()
            self.assertFalse(g)

            # Test "erase"
            f.erase('b.c.d.e')
            self.assertTrue('b' in f)
            self.assertFalse('b.c.d.e' in f)
            self.assertTrue('b.c.d' in f)
            self.assertEqual(f.getType('b.c.d'), Types.HASH)
            self.assertEqual(f['b.c.d'], Hash())

            # Test "erasePath"
            self.assertEqual(f['p.e.r.f'], [1.1, 2.2, 3.3])
            f.erasePath('p.e.r')
            self.assertFalse('p.e.r' in f)
            self.assertFalse('p.e' in f)
            self.assertFalse('p' in f)

            # Test "__delitem__"
            del f['w.r']
            self.assertFalse('w.r' in f)
            self.assertTrue('w' in f)
            self.assertEqual(f['w'], Hash())
            self.assertEqual(h['w'], Hash('r.a.p', 123456789999))

            del f

            # Test "__iter__"
            self.assertEqual(len(h), 5)
            it = iter(h)

            self.assertEqual(h['a'], "some text")
            n1 = next(it)
            self.assertTrue(isinstance(n1, Node))
            self.assertEqual(n1.getKey(), 'a')
            self.assertEqual(n1.getValue(), 'some text')
            self.assertEqual(n1.getType(), Types.STRING)

            self.assertEqual(h['b.c.d.e.f'], 2.718281828)
            n2 = next(it)
            self.assertTrue(isinstance(n2, Node))
            self.assertEqual(n2.getKey(), 'b')
            self.assertEqual(n2.getValue(), Hash('c.d.e.f', 2.718281828))
            self.assertEqual(n2.getType(), Types.HASH)

            self.assertEqual(h['w.r.a.p'], 123456789999)
            n3 = next(it)
            self.assertTrue(isinstance(n3, Node))
            self.assertEqual(n3.getKey(), 'w')
            self.assertEqual(n3.getValue(), Hash('r.a.p', 123456789999))
            self.assertEqual(n3.getType(), Types.HASH)

            self.assertEqual(h['c.c.c.c'], True)
            n4 = next(it)
            self.assertTrue(isinstance(n4, Node))
            self.assertEqual(n4.getKey(), 'c')
            self.assertEqual(n4.getValue(), Hash('c.c.c', True))
            self.assertEqual(n4.getType(), Types.HASH)

            self.assertEqual(h['p.e.r.f'], [1.1, 2.2, 3.3])
            n5 = next(it)
            self.assertTrue(isinstance(n5, Node))
            self.assertEqual(n5.getKey(), 'p')
            self.assertEqual(n5.getValue(), Hash('e.r.f', [1.1, 2.2, 3.3]))
            self.assertEqual(n5.getType(), Types.HASH)

            with self.assertRaises(StopIteration):
                next(it)

            # Test "find"
            n = h.find('b.c.d')
            self.assertTrue(n)
            self.assertEqual(n.getKey(), 'd')
            n = h.find('b.c.x')
            self.assertFalse(n)

            # Test "get" and/or "__getitem__" while pointing to the middle
            # of the parent tree 'h'
            # The resulting subtree behaves like a reference
            # Source "parent" tree is ...
            self.assertEqual(h['b.c.d.e.f'], 2.718281828)
            self.assertEqual(h.getType('b.c.d.e.f'), Types.DOUBLE)
            self.assertEqual(h.getType('b.c.d'), Types.HASH)
            # Test "__getitem__"
            g = h['b.c.d']  # reference to the subtree
            self.assertEqual(g['e.f'], 2.718281828)
            self.assertEqual(g.getType('e.f'), Types.DOUBLE)
            # set new value via subtree ref.
            g['e.f'] = 'new long string'
            # the type is changed ...
            self.assertEqual(g.getType('e.f'), Types.STRING)
            # Check that this is real reference, not a copy...
            self.assertEqual(h['b.c.d.e.f'], 'new long string')
            self.assertEqual(h.getType('b.c.d.e.f'), Types.STRING)

            # Reference from reference?
            # parent tree ... the owner
            h = Hash('a.b.c.d.e.f', 1)
            self.assertEqual(h['a.b.c.d.e.f'], 1)
            # reference to the node 'a.b.c' ... the proxy
            g = h['a.b.c']
            g['d.e.f'] = 55
            self.assertEqual(h['a.b.c.d.e.f'], 55)
            # reference to the node 'd.e' ... the proxy
            t = g['d.e']
            t['f'] = 88
            self.assertEqual(g['d.e.f'], 88)
            self.assertEqual(h['a.b.c.d.e.f'], 88)
            # drop the parent Hash in Python
            del h
            # attempt to access 'g' or 't' gives crash!!!
            # Now only in karathon! karabind is fixed using lifetime
            # control via 'keep_alive<...>()' magic
            if Hash is karabind.Hash:
                self.assertEqual(g['d.e.f'], 88)
                self.assertEqual(t['f'], 88)

            h = Hash("a[0]", Hash("a", 1), "a[1]", Hash("a", 1))
            self.assertEqual(h["a[0].a"], 1, "Value should be 1")
            self.assertEqual(h["a[1].a"], 1, "Value should be 1")
            self.assertEqual(h["a"][0]["a"], 1, "Value should be 1")
            self.assertEqual(h["a"][1]["a"], 1, "Value should be 1")
            # destroy parent Hash in C++
            h.clear()
            del g  # the subtree is invalidated. SEGFAULT if accessed.
            self.assertTrue(h.empty())

            # Comparison of 2 Hashes using operators: "==" and "!="
            h = Hash('a.b.c', True, 'x.y.z', [1, 2, 3])
            g = copy.copy(h)
            self.assertTrue(h == g)
            # Change some values in the copy
            g['a.b.c'] = False
            g['x.y.z'] = [9, 8, 7, 6, 5]
            # BTW, check that above `copy` is a real copy and not a reference
            self.assertEqual(h['a.b.c'], True)
            self.assertEqual(h['x.y.z'], [1, 2, 3])

            # Changing the value of leaf does not change the equality because
            # in C++ the operators '==' and '!=' use call (erroneously) to
            # 'similar(...)' method which checks only the "structure" equality
            # and does not compare the leaves ...
            self.assertTrue(h == g)
            # But if the structure is changed ...
            h['a.b.c'] = 12    # the type is changed from BOOL to INT32
            self.assertTrue(h != g)
            # ... or some new key/value was added ...
            g = copy.copy(h)   # restore equality
            g['h.e.l.p'] = 2.78
            self.assertTrue(h != g)

            # Test "getNode"
            h = Hash('a', 12)
            n = h.getNode('a')
            self.assertEqual(n.getKey(), 'a')
            self.assertEqual(n.getValue(), 12)
            self.assertEqual(n.getType(), Types.INT32)

            # Test "setNode"
            f = Hash('b', "text")
            f.setNode(n)
            self.assertTrue('b' in f)
            self.assertTrue('a' in f)
            self.assertEqual(f['a'], 12)
            self.assertEqual(n.getValue(), 12)
            self.assertEqual(f.getType('a'), Types.INT32)

        # Run test in karabind version
        inner(karabind.Hash, karabind.HashNode,
              karabind.Types, karabind.similar,
              karabind.fullyEqual)
        # Run test in karathon version
        inner(karathon.Hash, karathon.HashNode,
              karathon.Types, karathon.similar,
              karathon.fullyEqual)

    def test_getsetVectorHash(self):
        def inner(Hash, VectorHash):
            vh = VectorHash()
            vh.append(Hash('b', 1))
            vh.append(Hash('b', 2))
            h = Hash('a', vh)
            h['a'].extend((Hash('c', 3), Hash('c', 4),))
            h['a'].append(Hash('d', 5))
            self.assertEqual(len(h['a']), 5)
            self.assertEqual(h['a'][2]['c'], 3)
            self.assertEqual(h['a[2].c'], 3)
            self.assertTrue(isinstance(h, Hash))
            self.assertFalse(isinstance(h['a'], Hash))
            self.assertTrue(isinstance(h['a'], VectorHash))
            self.assertTrue(isinstance(h['a'][2], Hash))
            self.assertTrue(isinstance(h['a[2].c'], int))
            h['a'][1]['b'] = 222
            self.assertEqual(h['a[1].b'], 222)

            h2 = h  # h2 points to the same tree
            h2['a[3].c'] = 33
            self.assertEqual(h2['a[3].c'], 33)
            self.assertEqual(h['a[3].c'], 33)

            h3 = copy.copy(h)  # full copy
            h3['a[3].c'] = 77
            self.assertEqual(h3['a[3].c'], 77)
            self.assertEqual(h['a[3].c'], 33)
            self.assertTrue(h == h2)
            # self.assertFalse(h == h3)

            # Check implicitly conversion [hash, hash,...] to VectorHash
            h = Hash('a', [Hash('a', 1), Hash('b', 2), Hash('c', 3)])
            self.assertEqual(h['a[0].a'], 1)
            self.assertEqual(h['a[1].b'], 2)
            self.assertEqual(h['a[2].c'], 3)

        # Run test in karabind version
        inner(karabind.Hash, karabind.VectorHash)
        # Run test in karathon version
        inner(karathon.Hash, karathon.VectorHash)

    def test_getAs(self):
        def inner(Hash, Node, Types):
            # BOOL
            h = Hash("a", True)
            self.assertEqual(h.getType("a"), Types.BOOL)
            # The following tests are not needed since all conversion methods:
            # "getValueAs", "getAs", "getAttributeAs" use the same internal
            # conversion function.

            # Test "getAs"
            self.assertTrue(h.getAs("a", Types.BOOL))
            self.assertEqual(h.getAs("a", Types.CHAR), '1')
            self.assertEqual(h.getAs("a", Types.INT8), 1)
            self.assertEqual(h.getAs("a", Types.UINT8), 1)
            self.assertEqual(h.getAs("a", Types.INT16), 1)
            self.assertEqual(h.getAs("a", Types.UINT16), 1)
            self.assertEqual(h.getAs("a", Types.INT32), 1,
                             'Should return 1 as an python int')
            self.assertEqual(h.getAs("a", Types.UINT32), 1)
            self.assertEqual(h.getAs("a", Types.INT64), 1,
                             'Should return 1L as python long')
            self.assertEqual(h.getAs("a", Types.UINT64), 1)
            self.assertEqual(h.getAs("a", Types.FLOAT), 1.0,
                             'Should return 1.0 as python float')
            self.assertEqual(h.getAs("a", Types.DOUBLE), 1.0,
                             'Should return 1.0 as python float')
            if Hash is karabind.Hash:
                self.assertEqual(h.getAs("a", Types.COMPLEX_FLOAT), (1+0j))
                self.assertEqual(h.getAs("a", Types.COMPLEX_DOUBLE), (1+0j))
            self.assertEqual(h.getAs("a", Types.STRING), "1",
                             'Should return "1" as python string')
            self.assertEqual(h.getAs("a", Types.VECTOR_BOOL), [True])
            self.assertEqual(h.getAs("a", Types.VECTOR_INT8),
                             bytearray(b'\x01'))
            self.assertEqual(h.getAs("a", Types.VECTOR_INT16), [1])
            self.assertEqual(h.getAs("a", Types.VECTOR_UINT16), [1])
            self.assertEqual(h.getAs("a", Types.VECTOR_INT32), [1])
            self.assertEqual(h.getAs("a", Types.VECTOR_UINT32), [1])
            self.assertEqual(h.getAs("a", Types.VECTOR_INT64), [1])
            self.assertEqual(h.getAs("a", Types.VECTOR_UINT64), [1])
            self.assertEqual(h.getAs("a", Types.VECTOR_FLOAT), [1.0])
            self.assertEqual(h.getAs("a", Types.VECTOR_DOUBLE), [1.0])
            self.assertEqual(h.getAs("a", Types.VECTOR_COMPLEX_FLOAT),
                             [(1+0j)])
            self.assertEqual(h.getAs("a", Types.VECTOR_COMPLEX_DOUBLE),
                             [(1+0j)])
            if Hash is karabind.Hash:
                self.assertEqual(h.getAs("a", Types.VECTOR_STRING), ['1'])

        # Run test in karabind version
        inner(karabind.Hash, karabind.HashNode,
              karabind.Types)
        # Run test in karathon version
        inner(karathon.Hash, karathon.HashNode,
              karathon.Types)

    def test_getAttributeAs(self):
        def inner(Hash, Node, Types):

            # Test "getAttributesAs"
            h = Hash("a", True)
            h.setAttribute("a", "a", True)
            self.assertEqual(h.getAttributeAs("a", "a", Types.STRING), "1",
                             'Should return "1" as python string')
            self.assertEqual(h.getAttributeAs("a", "a", Types.CHAR), '1')
            self.assertEqual(h.getAttributeAs("a", "a", Types.INT32), 1,
                             'Should return 1 as python int')
            self.assertEqual(h.getAttributeAs("a", "a", Types.DOUBLE), 1.0,
                             'Should return 1.0 as python float')
            self.assertEqual(h.getAttributeAs("a", "a", Types.VECTOR_BOOL),
                             [True])
            self.assertEqual(
                h.getAttributeAs("a", "a", Types.VECTOR_INT8),
                bytearray(b'\x01'))
            self.assertEqual(
                h.getAttributeAs("a", "a", Types.VECTOR_INT32), [1])
            self.assertEqual(
                h.getAttributeAs("a", "a", Types.VECTOR_DOUBLE), [1.0])
            self.assertEqual(
                h.getAttributeAs("a", "a", Types.VECTOR_COMPLEX_FLOAT),
                [(1+0j)])
            self.assertEqual(
                h.getAttributeAs("a", "a", Types.VECTOR_COMPLEX_DOUBLE),
                [(1+0j)])
            if Hash is karabind.Hash:
                self.assertEqual(
                    h.getAttributeAs("a", "a", Types.VECTOR_STRING), ['1'])

            # DOUBLE
            h.setAttribute("a", "c", 1.23)
            self.assertEqual(h.getAttributes("a").getNode('c').getType(),
                             Types.DOUBLE)
            if Hash is karabind.Hash:
                self.assertEqual(h.getAttributeType('a', 'c'), Types.DOUBLE)

            h = Hash('a', 1)
            # Check that `getAttributes(...)` return reference internal
            attrs = h.getAttributes("a")
            self.assertEqual(len(attrs), 0)
            h.setAttribute('a', 'a', True)
            self.assertEqual(len(attrs), 1)

            # Check that attributes can be copied to another Hash tree
            g = Hash("Z.a.b.c", "value")
            g.setAttributes("Z.a.b.c", attrs)
            self.assertEqual(g.getAttributeAs("Z.a.b.c", "a", Types.STRING),
                             "1", 'Should return "1" as python string')
            self.assertEqual(g.getAttributeAs("Z.a.b.c", "a", Types.INT32), 1,
                             'Should return 1 as python int')
            self.assertEqual(g.getAttributeAs("Z.a.b.c", "a", Types.DOUBLE),
                             1.0, 'Should return 1.0 as python float')

        # Run test in karabind version
        inner(karabind.Hash, karabind.HashNode,
              karabind.Types)
        # Run test in karathon version
        inner(karathon.Hash, karathon.HashNode,
              karathon.Types)

    def test_setAs(self):
        def inner(Hash, Types):
            # Test "setAs"
            testTypes = (Types.INT8, Types.UINT8, Types.INT16, Types.UINT16,
                         Types.INT32, Types.UINT32, Types.INT64, Types.UINT64,
                         Types.FLOAT, Types.DOUBLE)

            h = Hash()

            for t in testTypes:
                h.setAs("a", 5, t)
                self.assertEqual(h.getType("a"), t,
                                 "Setting as type " + str(t) + " failed")
                self.assertEqual(h["a"], 5, "Equality failed for type "
                                 + str(t))
            # Special values for BOOL
            h.setAs("a", 1, "BOOL")
            self.assertEqual(h.getType("a"), Types.BOOL)
            self.assertEqual(h["a"], True)

            h.setAs("a", 1.0, "VECTOR_BOOL")
            self.assertEqual(h.getType("a"), Types.VECTOR_BOOL)
            self.assertEqual(h["a"], [True])

            # Other vectors ...

            # Small positive numbers ... < 128
            h.setAs("a", [127, 0, 77], Types.VECTOR_INT8)
            self.assertEqual(h.getType("a"), Types.VECTOR_INT8)
            # byte array: 0x7f -> 127, 0x00 -> 0, 'M' -> 77
            if Types is karathon.Types:
                self.assertEqual(h["a"], bytearray(b'\x7f\x00M'))
            elif Types is karabind.Types:
                self.assertEqual(h["a"], [127, 0, 77])

            h.setAs("a", [1260, -21170, 0, 1], Types.VECTOR_INT16)
            self.assertEqual(h.getType("a"), Types.VECTOR_INT16)
            self.assertEqual(h["a"], [1260, -21170, 0, 1])

            h.setAs("a", [1260, -21170, 0, 1], Types.VECTOR_DOUBLE)
            self.assertEqual(h.getType("a"), Types.VECTOR_DOUBLE)
            self.assertEqual(h["a"], [1260.0, -21170.0, 0.0, 1.0])

            if Types is karabind.Types:
                h.setAs("a", [1260, -21170, 0, 1], Types.VECTOR_COMPLEX_FLOAT)
                self.assertEqual(h.getType("a"), Types.VECTOR_COMPLEX_FLOAT)
                self.assertEqual(h["a"], [(1260+0j), (-21170+0j), 0j, (1+0j)])

                h.setAs("a", [1260, -21170, 0, 1], Types.VECTOR_STRING)
                self.assertEqual(h.getType("a"), Types.VECTOR_STRING)
                self.assertEqual(h["a"], ['1260', '-21170', '0', '1'])

        # Run test in karabind version
        inner(karabind.Hash, karabind.Types)
        # Run test in karathon version
        inner(karathon.Hash, karathon.Types)

    def test_merge(self):
        def inner(Hash, HashMergePolicy, similar, Types):
            h1 = Hash("a", 1,
                      "b", 2,
                      "c.b[0].g", 3,
                      "c.c[0].d", 4,
                      "c.c[1]", Hash("a.b.c", 6),
                      "d.e", 7)
            h1.set("f.g", 99)
            h1.set("h", -1)

            # Test "setAttribute"
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
            self.assertTrue(h1.isType("b", Types.HASH))  # switch to new type
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
                             "Not all attributes on vector<Hash> added (MERGE)"
                             )
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
                             # trigger selecting first HashVec item
                             # overwriting what was not a hashVec before, but
                             # only keep selected items
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
                            "Selecting only invalid indices changed something"
                            )

            h = Hash('a[0].a.b.c', 1, 'a[1].a.b.d', 2)
            g = Hash('a[0].x.y.w', 77, 'a[0].a.c', 33, 'a[2].abc', 12)
            h += g
            self.assertTrue(h['a[0].a.b.c'] == 1)
            self.assertTrue(h['a[0].x.y.w'] == 77)
            self.assertTrue(h['a[0].a.c'] == 33)
            self.assertTrue(h['a[1].a.b.d'] == 2)
            self.assertTrue(h['a[2].abc'] == 12)

        # Run test in karabind version
        inner(karabind.Hash, karabind.HashMergePolicy, karabind.similar,
              karabind.Types)
        # Run test in karathon version
        inner(karathon.Hash, karathon.HashMergePolicy, karathon.similar,
              karathon.Types)

    def test_subtract(self):
        def inner(Hash):
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

                # Test "__iadd__"
                h1 += h2

                # Test "__isub__"
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

        # Run test in karabind version
        inner(karabind.Hash)
        # Run test in karathon version
        inner(karathon.Hash)

    def test_erase(self):
        def inner(Hash):
            # first testing erasePath
            try:
                h = Hash('a[0].b[0].c', 1, 'b[0].c.d', 2, 'c.d[0].e', 3,
                         'd.e', 4, 'e', 5, 'f.g.h.i.j.k', 6)
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
            h1 = Hash('a', 1, 'b', 2, 'c.d', 31, 'e.f.g', 411, 'e.f.h', 412,
                      'e.i', 42)

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

        # Run test in karabind version
        inner(karabind.Hash)
        # Run test in karathon version
        inner(karathon.Hash)

    def test_fullEqual(self):
        def inner(Hash, VectorHash, fullyEqual):
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
            # re-add after previous 2nd
            h7.setAttribute("a", "attr", attr2Value)
            self.assertFalse(fullyEqual(h7, h1), "h7: " + str(h7))
            self.assertTrue(fullyEqual(h7, h1, False), "h7: " + str(h7))

        # Run test in karabind version
        inner(karabind.Hash, karabind.VectorHash, karabind.fullyEqual)
        # Run test in karathon version
        inner(karathon.Hash, karathon.VectorHash, karathon.fullyEqual)

    def test_ndarray(self):
        def inner(Hash):
            arr = np.array([[1, 2, 3, 4, 5], [6, 7, 8, 9, 10],
                            [11, 12, 13, 14, 15]])
            # The array owner is Python
            self.assertTrue(arr.flags.owndata)
            # Put array into Hash ...
            h = Hash('a', arr)
            # The array owner inside the Hash is not a C++ code ...
            self.assertFalse(h['a'].flags.owndata)
            # The other attributes are not changed ...
            self.assertTrue(h['a'].dtype == np.dtype('int64'))
            self.assertEqual(h['a'][0][0], 1)
            self.assertTrue(h['a'][0][3] == 4)
            self.assertTrue(h['a'][2][1] == 12)
            self.assertTrue(h['a'].size == 15)
            self.assertTrue(h['a'].itemsize == 8)
            self.assertTrue(h['a'].nbytes == 120)
            self.assertTrue(h['a'].ndim == 2)
            self.assertTrue(h['a'].shape == (3, 5))
            self.assertTrue(h['a'].strides == (40, 8))
            self.assertTrue(h['a'].flags.c_contiguous)
            self.assertTrue(h['a'].flags.carray)
            self.assertTrue(h['a'].flags.aligned)
            self.assertTrue(h['a'].flags.writeable)
            self.assertIsInstance(h['a'], np.ndarray)

            h2 = copy.copy(h)

            self.assertIsInstance(h2['a'], np.ndarray)
            self.assertEqual(h2['a'][0][0], 1)

            h['a'][0][0] = 255

            self.assertEqual(h['a'][0][0], 255)
            self.assertEqual(h2['a'][0][0], 255)
            self.assertEqual(arr[0][0], 255)

        # Run test in karabind version
        inner(karathon.Hash)
        # Run test in karathon version
        inner(karabind.Hash)
