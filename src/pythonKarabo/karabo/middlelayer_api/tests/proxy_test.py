from unittest import TestCase, main
from subprocess import PIPE, Popen
import sys

from karabo.middlelayer import (decodeBinary, ProxyNode, ProxySlot, State,
                                SubProxy, Unit)
from karabo.middlelayer_api.proxy import ProxyFactory


class TestProxyFactory(ProxyFactory):
    pass


class Tests(TestCase):
    def test_bound(self):
        process = Popen([sys.executable, "-m", "karabo.bound_api.launcher",
                         "schema", "karabo.bound_device_test", "TestDevice"],
                        stdout=PIPE)
        stdout, stderr = process.communicate()
        schema = decodeBinary(stdout)
        cls = ProxyFactory.createProxy(schema["TestDevice"])
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


if __name__ == "__main__":
    main()
