import unittest
from zlib import adler32

import numpy
from numpy.testing import assert_equal

from karabo.bound import (BinarySerializerHash, TextSerializerHash,
                          Hash as BoundHash, VectorHash)
from karabo.middlelayer import Hash, NodeType, Schema, HashList, Int64
from karabo.middlelayer_api.hash import _Byte


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
                "e.f.g.h", [5,5,5,5,5], "F.f.f.f.f", Hash("x.y.z", 99))
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
        h["a.b.c"] = 1    # statement is equivalent to h.set("a.b.c", 1)
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
        g = [Hash('b', 1), Hash('b',2)]      # python list of Hashes
        vh = h['a']    # get the reference because value is VectorHash
        vh.extend(g)
        g1 = (Hash('c',10), Hash('c',20),)   # python tuple of Hashes
        vh.extend(g1)  # "extend" lists, tuples, VectorHash objects
        vh.append(Hash('d',100))  # "append" Hash object
        self.assertEqual(len(vh), 5)


    def test_iteration(self):
        h = Hash("should", 1, "be", 2, "iterated", 3, "in", 4, "correct", 5, "order", 6)
        insertionOrder = [k for k in h]
        self.assertEqual(insertionOrder, ["should","be","iterated","in",
                                          "correct","order"])
        h.set("be", "2") # Has no effect on order

        insertionOrder = [k for k in h]
        self.assertEqual(insertionOrder, ["should","be","iterated","in",
                                          "correct","order"])

        h.erase("be")  # Remove
        h.set("be", "2")   # Must be last element in sequence now


    def test_attributes(self):
        h = Hash("a.b.a.b", 42)
        h.setAttribute("a.b.a.b","attr1", "someValue")
        self.assertEqual(h.getAttribute("a.b.a.b","attr1"), "someValue")

        h = Hash("a.b.a.b", 42)
        h.setAttribute("a.b.a.b","attr1", "someValue")
        self.assertEqual(h.getAttribute("a.b.a.b","attr1"), "someValue")

        h.setAttribute("a.b.a.b", "attr2", 42)
        self.assertEqual(h.getAttribute("a.b.a.b","attr1"), "someValue")
        self.assertEqual(h.getAttribute("a.b.a.b","attr2"), 42)

        h.setAttribute("a.b.a.b", "attr2", 43)
        self.assertEqual(h.getAttribute("a.b.a.b","attr1"), "someValue")
        self.assertEqual(h.getAttribute("a.b.a.b","attr2"), 43)

        h.setAttribute("a.b.a.b", "attr1", True)
        self.assertEqual(h.getAttribute("a.b.a.b","attr1"), True)
        self.assertEqual(h.getAttribute("a.b.a.b","attr2"), 43)
        attrs = h.getAttributes("a.b.a.b")
        self.assertEqual(attrs.get("attr1"), True)
        self.assertEqual(attrs["attr1"], True)
        self.assertEqual(attrs.get("attr2"), 43)
        self.assertEqual(attrs["attr2"], 43)


    def test_copy(self):
        h = self.create_hash()
        c = Hash(h)
        self.check_hash(c)


    def create_hash(self):
        h = Hash()
        h["bool"] = True
        h["int"] = 4
        h["string"] = "bla ä < / \\ \"' > ]]> & <![CDATA[ =_! (|}"
        h["complex"] = complex(1.0, 0.42)
        h["stringlist"] = ["bla", "blub"]
        h["chars"] = b"bla"
        h["vector"] = numpy.arange(7, dtype=numpy.int64)
        h["emptyvector"] = numpy.array([])
        h["hash"] = Hash("a", 3, "b", 7.1)
        h["hashlist"] = [Hash("a", 3), Hash()]
        h["emptystringlist"] = []
        h["emptyhashlist"] = HashList()
        h["vectorbool"] = numpy.array([True, False, True])
        h["char"] = _Byte("c")

        h["bool", "bool"] = False
        h["int", "float"] = 7.3
        h["hash", "int"] = 3
        h["string", "chars"] = b"blub"
        h["chars", "string"] = "laber &quot; ö \\ \"' ]]> <![CDATA[ (|}"
        h["vector", "complex"] = complex(1.0, 2.0)
        sh = Hash()
        sh["a"] = Hash()
        sh["a", "nodeType"] = NodeType.Leaf
        h["schema"] = Schema("blub", hash=sh)
        return h


    def check_hash_simple(self, h):
        """check that the hash *h* is the same as created by `create_hash`

        This method is simple enough that it works for both C++ and
        Python-only hashes."""
        keys = ["bool", "int", "string", "complex", "stringlist", "chars",
                "vector", "emptyvector", "hash", "hashlist", "emptystringlist",
                "emptyhashlist", "vectorbool", "char", "schema"]
        self.assertEqual(list(h.keys()), keys)
        self.assertIs(h["bool"], True)
        self.assertEqual(h["int"], 4)
        self.assertEqual(h["string"],
                         "bla ä < / \\ \"' > ]]> & <![CDATA[ =_! (|}")
        self.assertTrue(isinstance(h["string"], str))
        self.assertEqual(h["complex"], complex(1.0, 0.42))
        self.assertEqual(h["stringlist"], ["bla", "blub"])
        self.assertEqual(h["chars"], b"bla")
        self.assertEqual(h["hash.a"], 3)
        self.assertEqual(h["hash.b"], 7.1)
        self.assertEqual(len(h["hashlist"]), 2)
        self.assertEqual(h["hashlist"][0]["a"], 3)
        self.assertEqual(len(h["hashlist"][1]), 0)
        assert_equal(h["vector"], numpy.arange(7))
        assert_equal(h["emptyvector"], numpy.array([]))
        assert_equal(h["vectorbool"], numpy.array([True, False, True]))
        self.assertEqual(h["emptystringlist"], [])
        self.assertEqual(len(h["emptyhashlist"]), 0)
        self.assertIsInstance(h["emptyhashlist"], (HashList, VectorHash))
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
        self.assertEqual(h["vector", "complex"], complex(1.0, 2.0))
        self.assertTrue(isinstance(h["chars", "string"], str))
        self.assertEqual(h["schema"].name, "blub")
        sh = h["schema"].hash
        self.assertFalse(sh["a"].keys())
        self.assertEqual(sh["a", "nodeType"], NodeType.Leaf)


    def test_xml(self):
        s = self.create_hash().encode("XML")
        self.check_hash(Hash.decode(s, "XML"))

        # hash XML encoding is pretty different if there is only
        # one key on the first level
        h = Hash("bla", self.create_hash())
        s = h.encode("XML")
        self.check_hash(Hash.decode(s, "XML")["bla"])


    def test_binary(self):
        s = self.create_hash().encode("Bin")
        self.check_hash(Hash.decode(s, "Bin"))
        self.assertEqual(adler32(s), 963145160)


    def test_cpp_bin(self):
        s = self.create_hash().encode("Bin")
        ser = BinarySerializerHash.create("Bin")
        h = ser.load(s)
        self.check_hash_simple(h)
        ret = Hash.decode(ser.save(h), "Bin")
        self.check_hash(ret)


    def test_cpp_xml(self):
        s = self.create_hash().encode("XML")
        ser = TextSerializerHash.create("Xml")
        h = ser.load(s)
        self.check_hash_simple(h)
        ret = Hash.decode(ser.save(h), "XML")
        self.check_hash(ret)

    def test_ndarray(self):
        bh = BoundHash()
        arr = numpy.arange(12).reshape((3, 4))
        bh.set("array", arr)
        ser = BinarySerializerHash.create("Bin")
        h = Hash.decode(ser.save(bh), "Bin")
        self.assertEqual(h["array", "__classId"], "NDArray")
        assert_equal(h["array"]["shape"], [3, 4])
        self.assertEqual(h["array"]["type"], Int64.number)
        self.assertEqual(len(h["array"]["data"]), arr.nbytes)
        bh = ser.load(h.encode("Bin"))
        assert_equal(bh["array"], arr)


if __name__ == '__main__':
    unittest.main()

