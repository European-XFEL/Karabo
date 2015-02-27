import unittest
from karabo.hash import (Hash, XMLWriter, XMLParser, BinaryWriter,
                         BinaryParser)
from karabo.hashtypes import Schema_ as Schema
import numpy
from numpy.testing import assert_equal

from karabo.karathon import BinarySerializerHash
from zlib import adler32

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
        h["string"] = "bla"
        h["chars"] = b"bla"
        h["vector"] = numpy.arange(7, dtype=numpy.int64)
        h["emptyvector"] = numpy.array([])
        h["hash"] = Hash("a", 3, "b", 7.1)

        h["bool", "bool"] = False
        h["int", "float"] = 7.3
        h["hash", "int"] = 3
        h["string", "chars"] = b"blub"
        h["chars", "string"] = "laber"
        sh = Hash()
        sh["a"] = Hash()
        sh["a", "nodeType"] = 0
        h["schema"] = Schema("blub", hash=sh)
        return h


    def check_hash(self, h):
        keys = ["bool", "int", "string", "chars", "vector", "emptyvector",
                "hash", "schema"]
        self.assertEqual(list(h.keys()), keys)
        self.assertTrue(h["bool"] is True)
        self.assertEqual(h["int"], 4)
        self.assertEqual(h["string"], "bla")
        self.assertTrue(isinstance(h["string"], str))
        self.assertEqual(h["chars"], b"bla")
        self.assertTrue(isinstance(h["chars"], bytes))
        self.assertEqual(h["hash.a"], 3)
        self.assertEqual(h["hash.b"], 7.1)
        assert_equal(h["vector"], numpy.arange(7))
        assert_equal(h["emptyvector"], numpy.array([]))

        self.assertTrue(h["bool", "bool"] is False)
        self.assertEqual(h["int", "float"], 7.3)
        self.assertEqual(h["hash", "int"], 3)
        self.assertEqual(h["string", "chars"], b"blub")
        self.assertTrue(isinstance(h["string", "chars"], bytes))
        self.assertEqual(h["chars", "string"], "laber")
        self.assertTrue(isinstance(h["chars", "string"], str))
        self.assertEqual(h["schema"].name, "blub")
        sh = h["schema"].hash
        self.assertFalse(sh["a"].keys())
        self.assertEqual(sh["a", "nodeType"], 0)


    def test_xml(self):
        w = XMLWriter()
        r = XMLParser()
        s = w.write(self.create_hash())
        self.check_hash(r.read(s))

        h = Hash("bla", self.create_hash())
        s = w.write(h)
        self.check_hash(r.read(s)["bla"])


    def test_binary(self):
        w = BinaryWriter()
        r = BinaryParser()
        s = w.write(self.create_hash())
        self.assertEqual(adler32(s), 2002923287)
        self.check_hash(r.read(s))


    def test_cpp(self):
        w = BinaryWriter()
        r = BinaryParser()
        s = w.write(self.create_hash())
        ser = BinarySerializerHash.create("Bin")
        h = ser.load(bytearray(s))
        ret = Hash.decode(ser.save(h), "Bin")
        self.check_hash(ret)


if __name__ == '__main__':
    unittest.main()

