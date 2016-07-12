import asyncio
from unittest import TestCase, main

from karabo.middlelayer import (
    AccessMode, Assignment, Configurable, Hash, Int32, KaraboError, Node,
    Unit, unit)


class DummyConfigurable(Configurable):
    dummy_value = None
    dummy_child = None

    def setValue(self, desc, value):
        super().setValue(desc, value)
        assert self.dummy_value is None
        self.dummy_value = value

    def assertValue(self, value):
        assert value == self.dummy_value
        self.dummy_value = None

    def setChildValue(self, key, value, desc):
        super().setChildValue(key, value, desc)
        assert self.dummy_child is None
        self.key = key
        self.dummy_child = value

    def assertChild(self, key, value):
        assert value == self.dummy_child
        assert key == self.key
        self.dummy_child = None


def rehash(**kwargs):
    """assure hashes look exactly like they came over the network"""
    h = Hash(kwargs)
    return Hash.decode(h.encode("Bin"), "Bin")


def run_coro(coro):
    loop = asyncio.get_event_loop()
    return loop.run_until_complete(coro)


class Tests(TestCase):
    @classmethod
    def setUpClass(cls):
        asyncio.set_event_loop(asyncio.new_event_loop())

    def test_nodefault(self):
        class B(Configurable):
            value = Int32(unitSymbol=Unit.METER)

        class A(DummyConfigurable):
            value = Int32(unitSymbol=Unit.METER)
            node = Node(B)

        a = A()
        self.assertFalse(hasattr(a, "value"))
        self.assertFalse(hasattr(a.node, "value"))
        a = A(rehash(value=7, node=Hash("value", 3)))
        self.assertEqual(a.value, 7 * unit.meter)
        self.assertEqual(a.node.value, 3 * unit.meter)
        a.assertValue(7 * unit.meter)
        a.assertChild("node.value", 3 * unit.meter)
        a.value = 9 * unit.meter
        a.node.value = 4 * unit.meter
        self.assertEqual(a.value, 9 * unit.meter)
        self.assertEqual(a.node.value, 4 * unit.meter)
        a.assertValue(9 * unit.meter)
        a.assertChild("node.value", 4 * unit.meter)
        run_coro(a.slotReconfigure(rehash(value=10, node=Hash("value", 5))))
        self.assertEqual(a.value, 10 * unit.meter)
        self.assertEqual(a.node.value, 5 * unit.meter)

    def test_default(self):
        class B(Configurable):
            value = Int32(unitSymbol=Unit.METER, defaultValue=33)

        class A(DummyConfigurable):
            value = Int32(unitSymbol=Unit.METER, defaultValue=22)
            node = Node(B)

        a = A()
        self.assertEqual(a.value, 22 * unit.meter)
        self.assertEqual(a.node.value, 33 * unit.meter)
        a = A(rehash(value=7, node=Hash("value", 3)))
        self.assertEqual(a.value, 7 * unit.meter)
        self.assertEqual(a.node.value, 3 * unit.meter)
        a.assertValue(7 * unit.meter)
        a.assertChild("node.value", 3 * unit.meter)
        a.value = 9 * unit.meter
        a.node.value = 4 * unit.meter
        self.assertEqual(a.value, 9 * unit.meter)
        self.assertEqual(a.node.value, 4 * unit.meter)
        a.assertValue(9 * unit.meter)
        a.assertChild("node.value", 4 * unit.meter)
        run_coro(a.slotReconfigure(rehash(value=10, node=Hash("value", 5))))
        self.assertEqual(a.value, 10 * unit.meter)
        self.assertEqual(a.node.value, 5 * unit.meter)

    def test_mandatory(self):
        class B(Configurable):
            value = Int32(assignment=Assignment.MANDATORY,
                          unitSymbol=Unit.METER)

        class A(DummyConfigurable):
            value = Int32(assignment=Assignment.MANDATORY,
                          unitSymbol=Unit.METER)
            node = Node(B)

        with self.assertRaises(KaraboError):
            a = A(rehash(node=Hash("value", 3)))
        with self.assertRaises(KaraboError):
            a = A(rehash(value=7))

        a = A(rehash(value=7, node=Hash("value", 3)))
        self.assertEqual(a.value, 7 * unit.meter)
        self.assertEqual(a.node.value, 3 * unit.meter)
        a.assertValue(7 * unit.meter)
        a.assertChild("node.value", 3 * unit.meter)
        a.value = 9 * unit.meter
        a.node.value = 4 * unit.meter
        self.assertEqual(a.value, 9 * unit.meter)
        self.assertEqual(a.node.value, 4 * unit.meter)
        a.assertValue(9 * unit.meter)
        a.assertChild("node.value", 4 * unit.meter)
        run_coro(a.slotReconfigure(rehash(value=10, node=Hash("value", 5))))
        self.assertEqual(a.value, 10 * unit.meter)
        self.assertEqual(a.node.value, 5 * unit.meter)

    def test_readonly(self):
        class B(Configurable):
            value = Int32(accessMode=AccessMode.READONLY,
                          unitSymbol=Unit.METER)

        class A(DummyConfigurable):
            value = Int32(accessMode=AccessMode.READONLY,
                          unitSymbol=Unit.METER)
            node = Node(B)

        a = A()
        self.assertFalse(hasattr(a, "value"))
        self.assertFalse(hasattr(a.node, "value"))
        a = A(rehash(value=7, node=Hash("value", 3)))
        # we ignore read only parameters in configuration
        self.assertFalse(hasattr(a, "value"))
        self.assertFalse(hasattr(a.node, "value"))
        a.value = 9 * unit.meter
        a.node.value = 4 * unit.meter
        self.assertEqual(a.value, 9 * unit.meter)
        self.assertEqual(a.node.value, 4 * unit.meter)
        a.assertValue(9 * unit.meter)
        a.assertChild("node.value", 4 * unit.meter)
        with self.assertRaises(KaraboError):
            run_coro(a.slotReconfigure(rehash(node=Hash("value", 5))))
        with self.assertRaises(KaraboError):
            run_coro(a.slotReconfigure(rehash(value=10)))

    def test_readonly_default(self):
        class B(Configurable):
            value = Int32(defaultValue=9,
                          accessMode=AccessMode.READONLY,
                          unitSymbol=Unit.METER)

        class A(DummyConfigurable):
            value = Int32(defaultValue=5,
                          accessMode=AccessMode.READONLY,
                          unitSymbol=Unit.METER)
            node = Node(B)

        a = A()
        self.assertEqual(a.value, 5 * unit.meter)
        self.assertEqual(a.node.value, 9 * unit.meter)
        a = A(rehash(value=7, node=Hash("value", 3)))
        # we ignore read only parameters in configuration
        self.assertEqual(a.value, 5 * unit.meter)
        self.assertEqual(a.node.value, 9 * unit.meter)
        a.assertValue(5 * unit.meter)
        a.assertChild("node.value", 9 * unit.meter)
        a.value = 9 * unit.meter
        a.node.value = 4 * unit.meter
        self.assertEqual(a.value, 9 * unit.meter)
        self.assertEqual(a.node.value, 4 * unit.meter)
        a.assertValue(9 * unit.meter)
        a.assertChild("node.value", 4 * unit.meter)
        with self.assertRaises(KaraboError):
            run_coro(a.slotReconfigure(rehash(node=Hash("value", 5))))
        with self.assertRaises(KaraboError):
            run_coro(a.slotReconfigure(rehash(value=10)))

    def test_init_nodefault(self):
        class B(Configurable):
            value = Int32(accessMode=AccessMode.INITONLY,
                          unitSymbol=Unit.METER)

        class A(DummyConfigurable):
            value = Int32(accessMode=AccessMode.INITONLY,
                          unitSymbol=Unit.METER)
            node = Node(B)

        a = A()
        self.assertFalse(hasattr(a, "value"))
        self.assertFalse(hasattr(a.node, "value"))
        a = A(rehash(value=7, node=Hash("value", 3)))
        self.assertEqual(a.value, 7 * unit.meter)
        self.assertEqual(a.node.value, 3 * unit.meter)
        a.assertValue(7 * unit.meter)
        a.assertChild("node.value", 3 * unit.meter)
        a.value = 9 * unit.meter
        a.node.value = 4 * unit.meter
        self.assertEqual(a.value, 9 * unit.meter)
        self.assertEqual(a.node.value, 4 * unit.meter)
        a.assertValue(9 * unit.meter)
        a.assertChild("node.value", 4 * unit.meter)
        with self.assertRaises(KaraboError):
            run_coro(a.slotReconfigure(rehash(value=10)))
        with self.assertRaises(KaraboError):
            run_coro(a.slotReconfigure(rehash(node=Hash("value", 5))))

    def test_init_default(self):
        class B(Configurable):
            value = Int32(accessMode=AccessMode.INITONLY,
                          unitSymbol=Unit.METER, defaultValue=33)

        class A(DummyConfigurable):
            value = Int32(accessMode=AccessMode.INITONLY,
                          unitSymbol=Unit.METER, defaultValue=22)
            node = Node(B)

        a = A()
        self.assertEqual(a.value, 22 * unit.meter)
        self.assertEqual(a.node.value, 33 * unit.meter)
        a = A(rehash(value=7, node=Hash("value", 3)))
        self.assertEqual(a.value, 7 * unit.meter)
        self.assertEqual(a.node.value, 3 * unit.meter)
        a.assertValue(7 * unit.meter)
        a.assertChild("node.value", 3 * unit.meter)
        a.value = 9 * unit.meter
        a.node.value = 4 * unit.meter
        self.assertEqual(a.value, 9 * unit.meter)
        self.assertEqual(a.node.value, 4 * unit.meter)
        a.assertValue(9 * unit.meter)
        a.assertChild("node.value", 4 * unit.meter)
        with self.assertRaises(KaraboError):
            run_coro(a.slotReconfigure(rehash(value=10)))
        with self.assertRaises(KaraboError):
            run_coro(a.slotReconfigure(rehash(node=Hash("value", 5))))

    def test_init_mandatory(self):
        class B(Configurable):
            value = Int32(accessMode=AccessMode.INITONLY,
                          assignment=Assignment.MANDATORY,
                          unitSymbol=Unit.METER)

        class A(DummyConfigurable):
            value = Int32(accessMode=AccessMode.INITONLY,
                          assignment=Assignment.MANDATORY,
                          unitSymbol=Unit.METER)
            node = Node(B)

        with self.assertRaises(KaraboError):
            a = A(rehash(node=Hash("value", 3)))
        with self.assertRaises(KaraboError):
            a = A(rehash(value=7))

        a = A(rehash(value=7, node=Hash("value", 3)))
        self.assertEqual(a.value, 7 * unit.meter)
        self.assertEqual(a.node.value, 3 * unit.meter)
        a.assertValue(7 * unit.meter)
        a.assertChild("node.value", 3 * unit.meter)
        a.value = 9 * unit.meter
        a.node.value = 4 * unit.meter
        self.assertEqual(a.value, 9 * unit.meter)
        self.assertEqual(a.node.value, 4 * unit.meter)
        a.assertValue(9 * unit.meter)
        a.assertChild("node.value", 4 * unit.meter)
        with self.assertRaises(KaraboError):
            run_coro(a.slotReconfigure(rehash(value=10)))
        with self.assertRaises(KaraboError):
            run_coro(a.slotReconfigure(rehash(node=Hash("value", 5))))

    def test_setter(self):
        setter_value = init_value = 0

        class Setter(Int32):
            def setter(self, instance, value):
                nonlocal setter_value
                setter_value += value
                super().setter(instance, value + 1 * unit.meter)

            def initialize(self, instance, value):
                nonlocal init_value
                init_value += value
                super().initialize(instance, value + 2 * unit.meter)

        class A(Configurable):
            value = Setter(unitSymbol=Unit.METER)

        a = A(rehash(value=44))
        self.assertEqual(init_value, 44 * unit.meter)
        self.assertEqual(setter_value, (44 + 2) * unit.meter)
        self.assertEqual(a.value, (44 + 2 + 1) * unit.meter)
        run_coro(a.slotReconfigure(rehash(value=3)))
        self.assertEqual(setter_value, (44 + 3 + 2) * unit.meter)
        self.assertEqual(a.value, (3 + 1) * unit.meter)

    def test_setter_nested(self):
        setter_value = init_value = 0

        class Setter(Int32):
            def setter(self, instance, value):
                nonlocal setter_value
                setter_value += value
                super().setter(instance, value + 1 * unit.meter)

            def initialize(self, instance, value):
                nonlocal init_value
                init_value += value
                super().initialize(instance, value + 2 * unit.meter)

        class B(Configurable):
            value = Setter(unitSymbol=Unit.METER)

        class A(Configurable):
            node = Node(B)

        a = A(rehash(node=Hash("value", 44)))
        self.assertEqual(init_value, 44 * unit.meter)
        self.assertEqual(setter_value, (44 + 2) * unit.meter)
        self.assertEqual(a.node.value, (44 + 2 + 1) * unit.meter)
        run_coro(a.slotReconfigure(rehash(node=Hash("value", 3))))
        self.assertEqual(setter_value, (44 + 3 + 2) * unit.meter)
        self.assertEqual(a.node.value, (3 + 1) * unit.meter)

    def test_setter_coro(self):
        setter_value = init_value = 0

        class Setter(Int32):
            @asyncio.coroutine
            def setter(self, instance, value):
                nonlocal setter_value
                setter_value += value
                super().setter(instance, value + 1 * unit.meter)

            @asyncio.coroutine
            def initialize(self, instance, value):
                nonlocal init_value
                init_value += value
                yield from super().initialize(instance, value + 2 * unit.meter)

        class A(Configurable):
            value = Setter(unitSymbol=Unit.METER)

        a = A(rehash(value=44))
        self.assertEqual(init_value, 0)
        self.assertEqual(setter_value, 0)
        run_coro(a._run())
        self.assertEqual(init_value, 44 * unit.meter)
        self.assertEqual(setter_value, (44 + 2) * unit.meter)
        self.assertEqual(a.value, (44 + 2 + 1) * unit.meter)
        run_coro(a.slotReconfigure(rehash(value=3)))
        self.assertEqual(setter_value, (44 + 2 + 3) * unit.meter)
        self.assertEqual(a.value, 4 * unit.meter)

    def test_setter_coro_nested(self):
        setter_value = init_value = 0

        class Setter(Int32):
            @asyncio.coroutine
            def setter(self, instance, value):
                nonlocal setter_value
                setter_value += value
                super().setter(instance, value + 1 * unit.meter)

            @asyncio.coroutine
            def initialize(self, instance, value):
                nonlocal init_value
                init_value += value
                yield from super().initialize(instance, value + 2 * unit.meter)

        class B(Configurable):
            value = Setter(unitSymbol=Unit.METER)

        class A(Configurable):
            node = Node(B)

        a = A(rehash(node=Hash("value", 44)))
        self.assertEqual(init_value, 0)
        self.assertEqual(setter_value, 0)
        run_coro(a._run())
        self.assertEqual(init_value, 44 * unit.meter)
        self.assertEqual(setter_value, (44 + 2) * unit.meter)
        self.assertEqual(a.node.value, (44 + 2 + 1) * unit.meter)
        run_coro(a.slotReconfigure(rehash(node=Hash("value", 3))))
        self.assertEqual(setter_value, (44 + 2 + 3) * unit.meter)
        self.assertEqual(a.node.value, 4 * unit.meter)

    def test_cross_error(self):
        class B(Configurable):
            allowed = Int32(defaultValue=1)
            forbidden = Int32(accessMode=AccessMode.READONLY,
                              defaultValue=2)

        class A(Configurable):
            allowed = Int32(defaultValue=3)
            forbidden = Int32(accessMode=AccessMode.READONLY,
                              defaultValue=4)
            node = Node(B)

        a = A()
        with self.assertRaises(KaraboError):
            run_coro(a.slotReconfigure(
                rehash(allowed=5, forbidden=6)))
        self.assertEqual(a.allowed, 3)
        self.assertEqual(a.node.allowed, 1)
        with self.assertRaises(KaraboError):
            run_coro(a.slotReconfigure(
                rehash(node=Hash("allowed", 7, "forbidden", 8))))
        self.assertEqual(a.allowed, 3)
        self.assertEqual(a.node.allowed, 1)

    def test_nested(self):
        class C(Configurable):
            value = Int32(defaultValue=3)

        class B(Configurable):
            nested = Node(C)

        class A(DummyConfigurable):
            nested = Node(B)

        a = A()
        a.assertChild("nested.nested.value", 3)
        self.assertEqual(a.nested.nested.value, 3)
        a.nested.nested.value = 5
        a.assertChild("nested.nested.value", 5)
        self.assertEqual(a.nested.nested.value, 5)
        run_coro(a.slotReconfigure(rehash(
            nested=Hash("nested", Hash("value", 7)))))
        self.assertEqual(a.nested.nested.value, 7)
        a.assertChild("nested.nested.value", 7)


if __name__ == "__main__":
    main()
