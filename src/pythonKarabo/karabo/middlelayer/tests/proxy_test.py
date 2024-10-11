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
from unittest import TestCase, main
from weakref import ref

from karabo.middlelayer import (
    Hash, Proxy, ProxyNode, ProxySlot, Schema, State, SubProxy, Timestamp,
    Unit, isSet, unit)
from karabo.middlelayer.device_client import (
    filterByTags, getAliasFromKey, getDescriptors, getKeyFromAlias)
from karabo.middlelayer.proxy import ProxyFactory
from karabo.native import NodeType


class Tests(TestCase):
    def setUp(self):
        h = Hash("node", Hash("b", None), "a", None, "setA", None)
        h["node", "nodeType"] = NodeType.Node.value
        h["node.b", "nodeType"] = NodeType.Leaf.value
        h["node.b", "valueType"] = "STRING"
        h["node.b", "alias"] = "AStop"
        h["node.b", "tags"] = ["plc"]
        h["a", "nodeType"] = NodeType.Leaf.value
        h["a", "valueType"] = "INT32"
        h["a", "description"] = "a's description"
        h["a", "allowedStates"] = ["INIT", "UNKNOWN"]
        h["a", "unitSymbol"] = "A"
        h["a", "defaultValue"] = 22.5
        h["a", "alias"] = "AStart"
        h["a", "tags"] = ["mpod", "plc"]
        h["setA", "nodeType"] = NodeType.Node.value
        h["setA", "displayType"] = "Slot"
        h["setA", "classId"] = "Slot"
        h["setA", "description"] = "setA's description"
        self.schema = Schema("test", hash=h)

    def test_class(self):
        cls = ProxyFactory.createProxy(self.schema)

        self.assertLess({"a", "node", "setA"}, set(dir(cls)))
        self.assertIsInstance(cls.node, ProxyNode)
        self.assertTrue(issubclass(cls.node.cls, SubProxy))
        self.assertEqual(cls.a.description, "a's description")
        self.assertEqual(cls.a.allowedStates, {State.INIT, State.UNKNOWN})
        self.assertEqual(cls.a.key, "a")
        self.assertEqual(cls.a.longkey, "a")
        self.assertIs(cls.a.unitSymbol, Unit.AMPERE)
        # The defaultValue in the schema is wrong, we have an int here!
        self.assertEqual(cls.a.defaultValue, 22)

        self.assertIn("b", dir(cls.node.cls))
        self.assertEqual(cls.node.cls.b.key, "b")
        self.assertEqual(cls.node.cls.b.longkey, "node.b")

        self.assertIsInstance(cls.setA, ProxySlot)
        self.assertEqual(cls.setA.description, "setA's description")

    def test_object(self):
        cls = ProxyFactory.createProxy(self.schema)

        obj = cls()
        self.assertFalse(isSet(obj.a))
        self.assertIs(obj.a.descriptor, cls.a)
        self.assertFalse(isSet(obj.node.b))
        self.assertEqual(obj.setA.__doc__, "setA's description")

    def test_setvalue(self):
        class TestFactory(ProxyFactory):
            class Proxy(Proxy):
                def setValue(self, desc, value):
                    calls.append((desc, value))
        cls = TestFactory.createProxy(self.schema)

        proxy = cls()
        calls = []
        self.assertFalse(isSet(proxy.a))
        proxy.a = 5
        self.assertEqual(calls, [(cls.a, 5 * unit.ampere)])
        self.assertFalse(isSet(proxy.a))

        calls = []
        self.assertFalse(isSet(proxy.node.b))
        proxy.node.b = "hallo"
        self.assertEqual(calls, [(cls.node.cls.b, "hallo")])
        self.assertFalse(isSet(proxy.node.b))

    def test_callslot(self):
        class TestFactory(ProxyFactory):
            class Proxy(Proxy):
                def _callSlot(self, desc):
                    calls.append(desc)
        cls = TestFactory.createProxy(self.schema)

        calls = []
        proxy = cls()
        proxy.setA()
        self.assertEqual(calls, [cls.setA])

    def test_onchanged(self):
        class TestFactory(ProxyFactory):
            class Proxy(Proxy):
                def _notifyChanged(self, descriptor, value):
                    calls.append((descriptor, value))

        cls = TestFactory.createProxy(self.schema)
        proxy = cls()

        calls = []
        proxy._onChanged(Hash("a", 7))
        self.assertEqual(proxy.a, 7 * unit.A)
        self.assertEqual(calls, [(cls.a, 7 * unit.A)])

        calls = []
        proxy._onChanged(Hash("node", Hash("b", "whatever")))
        self.assertEqual(proxy.node.b, "whatever")
        self.assertEqual(calls, [(cls.node.cls.b, "whatever")])

        calls = []
        proxy._onChanged(Hash("setA", 22, "inexistent", 33))
        self.assertEqual(calls, [])

        h = Hash("a", 8)
        ts = Timestamp("2017-03-11")
        h["a", ...] = ts.toDict()
        proxy._onChanged(h)
        self.assertEqual(proxy.a.timestamp, ts)

        # _onChanged used to leak a cyclic reference to the proxy.
        weakproxy = ref(proxy)
        del proxy
        self.assertIsNone(weakproxy())

    def test_schemaupdate(self):
        h = Hash("node", Hash("c", None), "a", Hash("c", None), "setB", None)
        h["node", "nodeType"] = NodeType.Node.value
        h["node.c", "nodeType"] = NodeType.Leaf.value
        h["node.c", "valueType"] = "STRING"
        h["a", "nodeType"] = NodeType.Node.value
        h["a.c", "nodeType"] = NodeType.Leaf.value
        h["a.c", "valueType"] = "STRING"
        h["setB", "nodeType"] = NodeType.Node.value
        h["setB", "displayType"] = "Slot"
        h["setB", "classId"] = "Slot"
        h["setB", "description"] = "setB's description"
        schema = Schema("test", hash=h)

        cls = ProxyFactory.createProxy(self.schema)
        proxy = cls()
        proxy._onChanged(Hash("node", Hash("b", "whatever"), "a", 3))
        self.assertEqual(proxy.a, 3 * unit.A)
        self.assertEqual(proxy.node.b, "whatever")

        self.assertNotIn("setB", proxy._schema_hash)
        ProxyFactory.updateSchema(proxy, schema)
        proxy._onChanged(Hash("node", Hash("c", "whatever"),
                              "a", Hash("c", "bla")))
        self.assertEqual(proxy.a.c, "bla")
        self.assertEqual(proxy.node.c, "whatever")
        self.assertIn("setB", proxy._schema_hash)

    def test_get_descriptors(self):
        cls = ProxyFactory.createProxy(self.schema)
        proxy = cls()

        descriptors = list(getDescriptors(proxy))
        paths = [k.longkey for k in descriptors]

        self.assertEqual(["node.b", "a", "setA"], paths)

    def test_filterByTag(self):
        cls = ProxyFactory.createProxy(self.schema)
        proxy = cls()

        descriptors = filterByTags(proxy, "mpod")
        paths = [k.longkey for k in descriptors]
        self.assertEqual(["a"], paths)

        descriptors = filterByTags(proxy, "plc")
        paths = [k.longkey for k in descriptors]
        self.assertEqual(["node.b", "a"], paths)

    def test_getAliasFromKey(self):
        cls = ProxyFactory.createProxy(self.schema)
        proxy = cls()

        alias = getAliasFromKey(proxy, "a")
        self.assertEqual("AStart", alias)

        alias = getAliasFromKey(proxy, "node.b")
        self.assertEqual("AStop", alias)

    def test_getKeyFromAlias(self):
        cls = ProxyFactory.createProxy(self.schema)
        proxy = cls()

        key = getKeyFromAlias(proxy, "AStart")
        self.assertEqual("a", key)

        alias = getKeyFromAlias(proxy, "AStop")
        self.assertEqual("node.b", alias)


if __name__ == "__main__":
    main()
