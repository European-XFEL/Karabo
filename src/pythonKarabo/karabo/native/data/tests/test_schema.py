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
from copy import deepcopy
from unittest import TestCase, main

from ..enums import (
    AccessLevel, AccessMode, ArchivePolicy, Assignment, MetricPrefix, NodeType,
    Unit)
from ..hash import Hash
from ..schema import Schema


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
        h["a", "metricPrefixSymbol"] = "m"
        h["a", "accessMode"] = AccessMode.RECONFIGURABLE.value
        h["a", "requiredAccessLevel"] = AccessLevel.OPERATOR.value
        h["a", "assignment"] = Assignment.INTERNAL.value
        h["a", "archivePolicy"] = ArchivePolicy.EVERY_EVENT.value
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
        # Python deepcopy is working
        self.assertNotEqual(deep["node.b", ...], {})
        self.assertNotEqual(h["node.b", ...], {})
        self.assertTrue(h.fullyEqual(deep))

        # Hash quick deepcopy test
        mutable = [1, 2, 3, 4]
        h["mutable"] = mutable
        s = h.deepcopy()
        deep = deepcopy(h)
        self.assertTrue(h.fullyEqual(s))
        self.assertEqual(h["mutable"], mutable)
        self.assertEqual(s["mutable"], mutable)
        self.assertEqual(deep["mutable"], mutable)
        mutable.append(6)
        self.assertEqual(h["mutable"], mutable)
        self.assertNotEqual(s["mutable"], mutable)
        self.assertNotEqual(deep["mutable"], mutable)
        self.assertTrue(h["mutable"] is mutable)
        self.assertFalse(s["mutable"] is mutable)
        self.assertFalse(deep["mutable"] is mutable)
        self.assertTrue(deep.fullyEqual(s))

    def test_helpers(self):
        attrs = self.schema.hash["a", ...]
        self.assertEqual(
            AccessLevel.OPERATOR, AccessLevel.fromAttributes(attrs))
        self.assertEqual(
            AccessMode.RECONFIGURABLE, AccessMode.fromAttributes(attrs))
        self.assertEqual(
            ArchivePolicy.EVERY_EVENT, ArchivePolicy.fromAttributes(attrs))
        self.assertEqual(
            Assignment.INTERNAL, Assignment.fromAttributes(attrs))
        self.assertEqual(
            MetricPrefix.MILLI, MetricPrefix.fromAttributes(attrs))
        self.assertEqual(
            Unit.AMPERE, Unit.fromAttributes(attrs))
        attrs = self.schema.hash["c", ...]
        self.assertIsNone(AccessLevel.fromAttributes(attrs))
        self.assertIsNone(AccessMode.fromAttributes(attrs))
        self.assertIsNone(ArchivePolicy.fromAttributes(attrs))
        self.assertIsNone(Assignment.fromAttributes(attrs))
        self.assertIsNone(MetricPrefix.fromAttributes(attrs))


if __name__ == "__main__":
    main()
