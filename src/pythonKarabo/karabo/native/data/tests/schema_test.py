from copy import deepcopy
from unittest import TestCase, main

from karabo.native import Hash, Schema
from karabo.native.data.enums import NodeType


class Tests(TestCase):
    def setUp(self):
        h = Hash("node", Hash("b", None), "a", None,
                 "c", None, "d", None, "e", None)

        h["a", "nodeType"] = NodeType.Leaf.value
        h["a", "valueType"] = "INT32"
        h["a", "description"] = "a's description"
        h["a", "allowedStates"] = ["INIT", "UNKNOWN"]
        h["a", "unitSymbol"] = "A"
        h["a", "defaultValue"] = 22.5
        h["a", "alias"] = "Karabo"
        h["node", "nodeType"] = NodeType.Node.value
        h["node.b", "nodeType"] = NodeType.Leaf.value
        h["node.b", "valueType"] = "STRING"
        h["node.b", "description"] = "b's description"
        h["node.b", "allowedStates"] = ["CHANGING", "UNKNOWN"]
        h["node.b", "unitSymbol"] = "A"
        h["node.b", "defaultValue"] = 22.5
        h["node.b", "tags"] = ["mpod"]
        h["c", "nodeType"] = NodeType.Leaf.value
        h["c", "valueType"] = "INT32"
        h["c", "description"] = "c's description"
        h["c", "allowedStates"] = ["INIT", "UNKNOWN"]
        h["c", "unitSymbol"] = "A"
        h["c", "defaultValue"] = 17.5
        h["c", "tags"] = ["mpod"]
        h["d", "nodeType"] = NodeType.Leaf.value
        h["d", "valueType"] = "INT32"
        h["d", "description"] = "d's description"
        h["d", "allowedStates"] = ["INIT", "UNKNOWN"]
        h["d", "unitSymbol"] = "A"
        h["d", "defaultValue"] = 27.5
        h["d", "tags"] = ["mpod", "plc"]

        h["e", "nodeType"] = NodeType.Leaf.value
        h["e", "valueType"] = "VECTOR_STRING"
        h["e", "description"] = "e's description"
        h["e", "unitSymbol"] = ""
        h["e", "defaultValue"] = ["One", "Two", "Three"]

        self.schema = Schema("XFEL", hash=h)

    def test_getKeyFromAlias(self):
        key = self.schema.getKeyFromAlias("Karabo")
        self.assertEqual(key, "a")

    def test_keyHasAlias(self):
        a = self.schema.keyHasAlias("a")
        self.assertTrue(a)
        b = self.schema.keyHasAlias("node.b")
        self.assertFalse(b)

    def test_getAliasAsString(self):
        alias = self.schema.getAliasAsString("a")
        self.assertEqual(alias, "Karabo")

    def test_filterByTags_1(self):
        h = self.schema.filterByTags("plc")
        path = h.paths()
        self.assertIn("d", path)

    def test_filterByTags_2(self):
        h = self.schema.filterByTags("mpod")
        path = h.paths()
        self.assertIn("node.b", path)
        self.assertIn("c", path)
        self.assertIn("d", path)

    def test_schema_equal(self):
        h = self.schema.hash
        self.assertTrue(h.fullyEqual(self.schema.hash))
        # Test schema value modification
        s = h.deepcopy()
        self.assertTrue(h.fullyEqual(s))
        s.erase("d")
        self.assertFalse(h.fullyEqual(s))
        self.assertFalse(s.fullyEqual(h))

        s = h.deepcopy()
        self.assertTrue(h.fullyEqual(s))
        s["e", "defaultValue"] = ["Three"]
        self.assertFalse(h.fullyEqual(s))

        # Test schema attr modification
        s = h.deepcopy()
        self.assertTrue(h.fullyEqual(s))
        s["d", "tags"] = ["mpod"]
        self.assertFalse(h.fullyEqual(s))

        deep = deepcopy(h)
        # Python deepcopy loses attributes
        self.assertEqual(deep["node.b", ...], {})
        self.assertNotEqual(h["node.b", ...], {})
        self.assertFalse(h.fullyEqual(deep))

        # Hash quick deepcopy test
        mutable = [1, 2, 3, 4]
        h["mutable"] = mutable
        s = h.deepcopy()
        self.assertTrue(h.fullyEqual(s))
        self.assertEqual(h["mutable"], mutable)
        self.assertEqual(s["mutable"], mutable)
        mutable.append(6)
        # h changes, while the quick deepcopy does not
        self.assertEqual(h["mutable"], mutable)
        self.assertNotEqual(s["mutable"], mutable)


if __name__ == "__main__":
    main()
