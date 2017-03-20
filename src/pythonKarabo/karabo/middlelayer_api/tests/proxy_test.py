from unittest import TestCase, main

from karabo.middlelayer import (Hash, isSet, Proxy, ProxyNode,
                                ProxySlot, Schema, State, SubProxy, Unit, unit)
from karabo.middlelayer_api.enums import NodeType
from karabo.middlelayer_api.proxy import ProxyFactory


class Tests(TestCase):
    def test_setvalue(self):
        class TestFactory(ProxyFactory):
            class Proxy(Proxy):
                def setValue(self, desc, value):
                    calls.append((desc, value))
        h = Hash("node", Hash("b", None), "a", None, "setA", None)
        h["node", "nodeType"] = NodeType.Node.value
        h["node.b", "nodeType"] = NodeType.Leaf.value
        h["node.b", "valueType"] = "STRING"
        h["a", "nodeType"] = NodeType.Leaf.value
        h["a", "valueType"] = "INT32"
        h["a", "description"] = "a's description"
        h["a", "allowedStates"] = ["INIT", "UNKNOWN"]
        h["a", "unitSymbol"] = "A"
        h["a", "defaultValue"] = 22.5
        h["setA", "nodeType"] = NodeType.Node.value
        h["setA", "displayType"] = "Slot"
        schema = Schema("test", hash=h)
        cls = TestFactory.createProxy(schema)

        self.assertLess({"a", "node", "setA"}, set(dir(cls)))
        self.assertIsInstance(cls.node, ProxyNode)
        self.assertTrue(issubclass(cls.node.cls, SubProxy))
        self.assertEqual(cls.a.description, "a's description")
        self.assertEqual(cls.a.allowedStates, {State.INIT, State.UNKNOWN})
        self.assertEqual(cls.a.key, "a")
        self.assertEqual(cls.a.longkey, "a")
        self.assertIs(cls.a.unitSymbol, Unit.AMPERE)
        self.assertEqual(cls.a.defaultValue, 22.5)

        self.assertIn("b", dir(cls.node.cls))
        self.assertEqual(cls.node.cls.b.key, "b")
        self.assertEqual(cls.node.cls.b.longkey, "node.b")

        self.assertIsInstance(cls.setA, ProxySlot)

        obj = cls()
        self.assertFalse(isSet(obj.a))
        self.assertIs(obj.a.descriptor, cls.a)
        self.assertFalse(isSet(obj.node.b))

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


if __name__ == "__main__":
    main()
