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
import unittest
from zlib import adler32

import numpy
from numpy.testing import assert_equal

from karabo.bound import (
    NODE_ELEMENT, BinarySerializerHash, Hash as BoundHash,
    Schema as BoundSchema, TextSerializerHash, VectorHash as BoundVectorHash)
from karabo.middlelayer import Configurable
from karabo.native import (
    AccessMode, Hash, HashByte, HashList, Int32, Int64, NodeType, Schema,
    String, VectorHash as VectorHash, decodeBinary, decodeXML, encodeBinary,
    encodeXML)


class Hash_TestCase(unittest.TestCase):
    def test_constructors(self):
        h = Hash()
        self.assertEqual(len(h), 0)

        h = Hash('a', 1)
        self.assertEqual(len(h), 1)
        self.assertEqual(h['a'], 1)

        h = Hash('a', 1, 'b', 2.0)
        self.assertEqual(len(h), 2)
        self.assertEqual(h['a'], 1)
        self.assertEqual(h['b'], 2.0)

        h = Hash("a.b.c", 1, "b.c", 2.0, "c", 3.7, "d.e", "4",
                 "e.f.g.h", [5, 5, 5, 5, 5], "F.f.f.f.f", Hash("x.y.z", 99))
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

        h = Hash({"a": 1, "b": 2, "c": 3})
        self.assertEqual(h["b"], 2)
        self.assertEqual(len(h), 3)

    def test_getSet(self):
        h = Hash()
        h.set("a.b.c1.d", 1)
        self.assertTrue(h.get("a").has("b"))
        self.assertTrue(h.get("a.b").has("c1"))
        self.assertTrue(h.get("a.b.c1").has("d"))
        self.assertTrue(h.has("a.b.c1.d"))
        self.assertIn("a.b.c1.d", h)
        self.assertTrue(h.get("a").has("b.c1"))
        self.assertIn("b.c1", h["a"])

        h.set("a.b.c2.d", 2.0)
        self.assertTrue(h.get("a.b").has("c2.d"))
        self.assertTrue(h.get("a.b").has("c1.d"))
        self.assertEqual(h.get("a.b.c1.d"), 1)
        self.assertEqual(h.get("a.b.c2.d"), 2.0)

        h = Hash()
        h["a.b.c"] = 1  # statement is equivalent to h.set("a.b.c", 1)
        h["a.b.c"] = 2
        self.assertEqual(h["a.b.c"], 2)
        self.assertTrue(h.has("a.b"))
        self.assertFalse(h.has("a.b.c.d"))
        self.assertEqual(h["a"]["b"]["c"], 2)
        h["a"]["b"]["c"] = 77
        self.assertEqual(h["a"]["b"]["c"], 77)
        self.assertEqual(h["a.b.c"], 77)

        self.assertIs(h.get("laber"), None)
        self.assertEqual(h.get("laber", "whatever"), "whatever")

    def test_getSetVectorHash(self):
        h = Hash('a', [])
        g = [Hash('b', 1), Hash('b', 2)]  # python list of Hashes
        vh = h['a']  # get the reference because value is VectorHash
        vh.extend(g)
        g1 = (Hash('c', 10), Hash('c', 20),)  # python tuple of Hashes
        vh.extend(g1)  # "extend" lists, tuples, VectorHash objects
        vh.append(Hash('d', 100))  # "append" Hash object
        self.assertEqual(len(vh), 5)

    def test_iteration(self):
        h = Hash("should", 1, "be", 2, "iterated", 3, "in", 4, "correct", 5,
                 "order", 6)
        insertionOrder = [k for k in h]
        self.assertEqual(insertionOrder, ["should", "be", "iterated", "in",
                                          "correct", "order"])
        h.set("be", "2")  # Has no effect on order

        insertionOrder = [k for k in h]
        self.assertEqual(insertionOrder, ["should", "be", "iterated", "in",
                                          "correct", "order"])

        h.erase("be")  # Remove
        h.set("be", "2")  # Must be last element in sequence now

    def test_attributes(self):
        h = Hash("a.b.a.b", 42)
        h.setAttribute("a.b.a.b", "attr1", "someValue")
        self.assertEqual(h.getAttribute("a.b.a.b", "attr1"), "someValue")

        h = Hash("a.b.a.b", 42)
        h.setAttribute("a.b.a.b", "attr1", "someValue")
        self.assertEqual(h.getAttribute("a.b.a.b", "attr1"), "someValue")

        h.setAttribute("a.b.a.b", "attr2", 42)
        self.assertEqual(h.getAttribute("a.b.a.b", "attr1"), "someValue")
        self.assertEqual(h.getAttribute("a.b.a.b", "attr2"), 42)

        h.setAttribute("a.b.a.b", "attr2", 43)
        self.assertEqual(h.getAttribute("a.b.a.b", "attr1"), "someValue")
        self.assertEqual(h.getAttribute("a.b.a.b", "attr2"), 43)

        h.setAttribute("a.b.a.b", "attr1", True)
        self.assertEqual(h.getAttribute("a.b.a.b", "attr1"), True)
        self.assertEqual(h.getAttribute("a.b.a.b", "attr2"), 43)
        attrs = h.getAttributes("a.b.a.b")
        self.assertEqual(attrs.get("attr1"), True)
        self.assertEqual(attrs["attr1"], True)
        self.assertEqual(attrs.get("attr2"), 43)
        self.assertEqual(attrs["attr2"], 43)

    def test_copy(self):
        h = self.create_hash()
        c = Hash(h)
        self.check_hash(c)

    @staticmethod
    def _init_hash(h):
        """Initializes a given hash (can be either a MDL or a Bound Hash)."""

        use_bound = isinstance(h, BoundHash)

        h["bool"] = True
        h["int"] = 4
        h["string"] = "bla ä < / \\ \"' > ]]> & <![CDATA[ =_! (|}"
        h["stringlist"] = ["bla", "blub"]
        h["chars"] = b"bla"
        h["emptystringlist"] = []
        h["char"] = HashByte("c")
        h["none"] = None
        if use_bound:
            h["vector"] = [i for i in range(7)]
            h["emptyvector"] = []
            h["vectorbool"] = [True, False, True]
            # VECTOR_NONE serialisation not supported in MDL (and funny anyway)
            #  h["vectornone"] = [None] * 3
            h["hash"] = BoundHash("a", 3, "b", 7.1)
            h["hashlist"] = [BoundHash("a", 3), BoundHash()]
            h["emptyhashlist"] = BoundVectorHash()
            # Bound uses setAttribute for setting attributes.
            # TODO: Add one attribute of type VectorHash and
            # one of type Schema.
            h.setAttribute("bool", "bool", False)
            h.setAttribute("int", "float", 7.3)
            h.setAttribute("hash", "int", 3)
            h.setAttribute("string", "chars", b"blub")
            h.setAttribute("chars", "string",
                           "laber &quot; ö \\ \"' ]]> <![CDATA[ (|}")
            # Bound way for building a similar schema to the one build for
            # MDL below
            sch = BoundSchema('blub')
            (
                NODE_ELEMENT(sch)
                .key('a')
                .commit()
            )
            h['schema'] = sch
        else:
            h["vector"] = numpy.arange(7, dtype=numpy.int64)
            h["emptyvector"] = numpy.array([])
            h["vectorbool"] = numpy.array([True, False, True])
            h["hash"] = Hash("a", 3, "b", 7.1)
            h["hashlist"] = [Hash("a", 3), Hash()]
            h["emptyhashlist"] = HashList()
            # This idiom for assigning attributes is only supported in MDL.
            # TODO: add an attribute of type HashList.
            h["bool", "bool"] = False
            h["int", "float"] = 7.3
            h["hash", "int"] = 3
            h["string", "chars"] = b"blub"
            h["chars", "string"] = "laber &quot; ö \\ \"' ]]> <![CDATA[ (|}"
            # MDL specific: constructing a schema from its underlying Hash.
            sh = Hash()
            sh["a"] = Hash()
            sh["a", "nodeType"] = NodeType.Node
            h["schema"] = Schema("blub", hash=sh)

    def create_hash(self):
        h = Hash()
        Hash_TestCase._init_hash(h)
        return h

    def create_bound_hash(self):
        h = BoundHash()
        Hash_TestCase._init_hash(h)
        return h

    def check_hash_simple(self, h):
        """check that the hash *h* is the same as created by `create_hash`

        This method is simple enough that it works for both C++ and
        Python-only hashes."""
        keys = ["bool", "int", "string", "stringlist", "chars",
                "emptystringlist", "char", "none", "vector", "emptyvector",
                "vectorbool", "hash", "hashlist", "emptyhashlist", "schema"]
        self.assertEqual(list(h.keys()), keys)
        self.assertIs(h["bool"], True)
        self.assertEqual(h["int"], 4)
        self.assertEqual(h["string"],
                         "bla ä < / \\ \"' > ]]> & <![CDATA[ =_! (|}")
        self.assertTrue(isinstance(h["string"], str))
        self.assertIs(h["none"], None)
        self.assertEqual(h["stringlist"], ["bla", "blub"])
        self.assertEqual(h["chars"], b"bla")
        self.assertEqual(h["hash.a"], 3)
        self.assertEqual(h["hash.b"], 7.1)
        self.assertEqual(len(h["hashlist"]), 2)
        self.assertEqual(h["hashlist"][0]["a"], 3)
        self.assertEqual(len(h["hashlist"][1]), 0)
        assert_equal(h["vector"], [i for i in range(7)])
        assert_equal(h["emptyvector"], [])
        assert_equal(h["vectorbool"], [True, False, True])
        self.assertEqual(h["emptystringlist"], [])
        self.assertEqual(len(h["emptyhashlist"]), 0)
        self.assertIsInstance(h["emptyhashlist"], (HashList, BoundVectorHash))
        self.assertEqual(h["char"], "c")

    def check_hash(self, h):
        """check that the hash *h* is the same as created by `create_hash`

        This method does advanced checking only available for
        Python-only hashes."""
        self.check_hash_simple(h)
        self.assertIsInstance(h["chars"], bytes)
        self.assertTrue(h["bool", "bool"] is False)
        self.assertEqual(h["int", "float"], 7.3)
        self.assertEqual(h["hash", "int"], 3)
        self.assertEqual(h["string", "chars"], b"blub")
        self.assertTrue(isinstance(h["string", "chars"], bytes))
        self.assertEqual(h["chars", "string"],
                         "laber &quot; ö \\ \"' ]]> <![CDATA[ (|}")
        self.assertTrue(isinstance(h["chars", "string"], str))

        self.assertEqual(h["schema"].name, "blub")
        sh = h["schema"].hash
        self.assertFalse(sh["a"].keys())
        self.assertEqual(sh["a", "nodeType"], NodeType.Node)

    def test_xml(self):
        s = encodeXML(self.create_hash())
        self.check_hash(decodeXML(s))

        # hash XML encoding is pretty different if there is only
        # one key on the first level
        h = Hash("bla", self.create_hash())
        s = encodeXML(h)
        self.check_hash(decodeXML(s)["bla"])

    def test_binary(self):
        s = encodeBinary(self.create_hash())
        self.check_hash(decodeBinary(s))
        self.assertEqual(adler32(s), 379085425)

    def test_bin_roundtrip(self):
        """Tests compatibility between the Binary Serializers for MDL and Bound
        Python.

        The test is analogous to the one in 'test_xml_roundtrips' and has the
        same two parts.
        """
        # Starting from MDL Hash
        mdl_hash_bin = encodeBinary(self.create_hash())
        bound_ser = BinarySerializerHash.create("Bin")
        bound_hash = bound_ser.load(mdl_hash_bin)
        self.check_hash_simple(bound_hash)
        bound_hash_bin = bound_ser.save(bound_hash)
        mdl_hash = decodeBinary(bound_hash_bin)
        self.check_hash(mdl_hash)
        # Starting from Bound Hash
        bound_hash_bin = bound_ser.save(self.create_bound_hash())
        mdl_hash = decodeBinary(bound_hash_bin)
        self.check_hash(mdl_hash)
        mdl_hash_bin = encodeBinary(mdl_hash)
        bound_hash = bound_ser.load(mdl_hash_bin)
        self.check_hash_simple(bound_hash)

    def test_xml_roundtrip(self):
        """Tests compatibility between the XML Serializers for MDL and Bound
        Python.

        The test has two parts: the first builds a Bound Hash from the XML
        serialized form of an MDL Hash, checks the Bound Hash and then rebuilds
        the original MDL Hash from the XML serialized form of the Bound Hash.
        The second part is similar to the first, but it starts building an
        MDL Hash from the XML serialized form of a Bound Hash.
        """
        # Starting from MDL Hash
        mdl_hash_xml = encodeXML(self.create_hash())
        bound_ser = TextSerializerHash.create("Xml")
        bound_hash = bound_ser.load(mdl_hash_xml)
        self.check_hash_simple(bound_hash)
        bound_hash_xml = bound_ser.save(bound_hash)
        mdl_hash = decodeXML(bound_hash_xml)
        self.check_hash(mdl_hash)
        # Starting from Bound Hash
        bound_hash_xml = bound_ser.save(self.create_bound_hash())
        mdl_hash = decodeXML(bound_hash_xml)
        self.check_hash(mdl_hash)
        mdl_hash_xml = encodeXML(mdl_hash)
        bound_hash = bound_ser.load(mdl_hash_xml)
        self.check_hash_simple(bound_hash)

    def test_ndarray(self):
        bh = BoundHash()
        arr = numpy.arange(12).reshape((3, 4))
        bh.set("array", arr)
        ser = BinarySerializerHash.create("Bin")
        h = decodeBinary(ser.save(bh))
        self.assertEqual(h["array", "__classId"], "NDArray")
        assert_equal(h["array"]["shape"], [3, 4])
        self.assertEqual(h["array"]["type"], Int64.number)
        self.assertEqual(len(h["array"]["data"]), arr.nbytes)
        bh = ser.load(encodeBinary(h))
        assert_equal(bh["array"], arr)

    def test_mdl_xml_table(self):
        """Tests that a xml for an MDL Hash with vector of hash and schema
        attributes can be successfully loaded by the xml serializer.

        Those kind of attributes appear in the serialization of schemas with
        TableElements and support for them currently differ between Bound and
        MDL.
        """
        class Row(Configurable):
            anInt = Int32(defaultValue=33)
            aString = String(defaultValue="Spaghetti")

        class WithTable(Configurable):
            """A Configurable with a Table Element."""
            table = VectorHash(
                rows=Row,
                displayedName="Table",
                accessMode=AccessMode.READONLY)

        schema = WithTable.getClassSchema()
        schema_xml = encodeXML(schema.hash)
        schema_hash = decodeXML(schema_xml)
        self.assertTrue(schema_hash.has('table'))
        self.assertTrue(isinstance(schema_hash['table'], Hash))
        self.assertEqual(schema_hash['table', 'accessMode'], 2)
        self.assertTrue('rowSchema' in schema_hash.getAttributes('table'))
        self.assertTrue(isinstance(schema_hash['table', 'rowSchema'], str))

        # TODO: make encodeXML compatible with the Bound XML serializer from
        #       Karabo version 2.6.0 and above. Once that is done, 'rowSchema'
        #       should become an instance of Schema and 'table' should become
        #       an instance of HashList.

    def test_paths(self):
        h = self.create_hash()
        paths = h.paths(intermediate=False)
        self.assertEqual(len(paths), 16, paths)
        self.assertIn('bool', paths)
        self.assertIn('hash.a', paths)
        self.assertNotIn('hash', paths)
        self.assertNotIn('hashlist.a', paths)
        self.assertIn('emptyhashlist', paths)
        self.assertIn('schema', paths)
        self.assertNotIn('schema.blub', paths)

        all_paths = h.paths(intermediate=True)
        self.assertEqual(len(all_paths), 17, paths)
        self.assertIn('hash', all_paths)


if __name__ == '__main__':
    unittest.main()
