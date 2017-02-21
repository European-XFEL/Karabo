import asyncio
from unittest import TestCase, main

from karabo.middlelayer import (
    AccessMode, Assignment, Configurable, Hash, Int32, isSet, KaraboError,
    Node, String, Unit, unit, VectorHash)


class StoreChanges(Configurable):
    """This test class keeps all changes for later inspection"""
    values_set = None
    children_set = None

    def setValue(self, desc, value):
        super().setValue(desc, value)
        if self.values_set is None:
            self.values_set = []
        self.values_set.append((desc, value))

    def setChildValue(self, key, value, desc):
        super().setChildValue(key, value, desc)
        if self.children_set is None:
            self.children_set = []
        self.children_set.append((key, value, desc))


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
            bvalue = Int32(unitSymbol=Unit.METER)

        class A(StoreChanges):
            value = Int32(unitSymbol=Unit.METER)
            node = Node(B)

        a = A()
        self.assertFalse(isSet(a.value))
        self.assertFalse(isSet(a.node.bvalue))
        self.assertEqual(a.values_set, [(A.value, None), (A.node, a.node)])
        a.values_set = []
        self.assertEqual(a.children_set, [("node", a.node, A.node) ])
        a.children_set = []
        a = A(rehash(value=7, node=Hash("bvalue", 3)))
        self.assertEqual(a.value, 7 * unit.meter)
        self.assertEqual(a.node.bvalue, 3 * unit.meter)
        self.assertEqual(a.values_set,
                         [(A.value, 7 * unit.meter), (A.node, a.node)])
        a.values_set = []
        self.assertEqual(a.children_set,
                         [("value", 7 * unit.meter, A.value),
                          ("node", a.node, A.node)])
        a.children_set = []
        a.value = 9 * unit.meter
        self.assertEqual(a.value, 9 * unit.meter)
        self.assertEqual(a.values_set, [(A.value, 9 * unit.meter)])
        a.values_set = []
        self.assertEqual(a.children_set, [("value", 9 * unit.meter, A.value)])
        a.children_set = []
        a.node.bvalue = 4 * unit.meter
        self.assertEqual(a.node.bvalue, 4 * unit.meter)
        self.assertEqual(a.values_set, [])
        self.assertEqual(a.children_set,
                         [("node.bvalue", 4 * unit.meter, B.bvalue)])
        a.children_set = []
        run_coro(a.slotReconfigure(rehash(value=10, node=Hash("bvalue", 5))))
        self.assertEqual(a.value, 10 * unit.meter)
        self.assertEqual(a.node.bvalue, 5 * unit.meter)

    def test_default(self):
        class B(Configurable):
            bvalue = Int32(unitSymbol=Unit.METER, defaultValue=33)

        class A(StoreChanges):
            value = Int32(unitSymbol=Unit.METER, defaultValue=22)
            node = Node(B)

        a = A()
        self.assertEqual(a.value, 22 * unit.meter)
        self.assertEqual(a.node.bvalue, 33 * unit.meter)
        self.assertEqual(a.values_set, [(A.value, 22 * unit.meter),
                                        (A.node, a.node)])
        a.values_set = []
        self.assertEqual(a.children_set, [("value", 22 * unit.meter, A.value),
                                          ("node", a.node, A.node)])
        a.children_set = []
        a = A(rehash(value=7, node=Hash("bvalue", 3)))
        self.assertEqual(a.value, 7 * unit.meter)
        self.assertEqual(a.node.bvalue, 3 * unit.meter)
        self.assertEqual(a.values_set,
                         [(A.value, 7 * unit.meter), (A.node, a.node)])
        a.values_set = []
        self.assertEqual(a.children_set,
                         [("value", 7 * unit.meter, A.value),
                          ("node", a.node, A.node)])
        a.children_set = []
        a.value = 9 * unit.meter
        self.assertEqual(a.value, 9 * unit.meter)
        self.assertEqual(a.values_set, [(A.value, 9 * unit.meter)])
        a.values_set = []
        self.assertEqual(a.children_set, [("value", 9 * unit.meter, A.value)])
        a.children_set = []
        a.node.bvalue = 4 * unit.meter
        self.assertEqual(a.node.bvalue, 4 * unit.meter)
        self.assertEqual(a.values_set, [])
        self.assertEqual(a.children_set,
                         [("node.bvalue", 4 * unit.meter, B.bvalue)])
        a.children_set = []
        run_coro(a.slotReconfigure(rehash(value=10, node=Hash("bvalue", 5))))
        self.assertEqual(a.value, 10 * unit.meter)
        self.assertEqual(a.node.bvalue, 5 * unit.meter)

    def test_mandatory(self):
        class B(Configurable):
            bvalue = Int32(assignment=Assignment.MANDATORY,
                           unitSymbol=Unit.METER)

        class A(StoreChanges):
            value = Int32(assignment=Assignment.MANDATORY,
                          unitSymbol=Unit.METER)
            node = Node(B)

        with self.assertRaises(KaraboError):
            a = A(rehash(node=Hash("bvalue", 3)))
        with self.assertRaises(KaraboError):
            a = A(rehash(value=7))

        a = A(rehash(value=7, node=Hash("bvalue", 3)))
        self.assertEqual(a.value, 7 * unit.meter)
        self.assertEqual(a.node.bvalue, 3 * unit.meter)
        self.assertEqual(a.values_set,
                         [(A.value, 7 * unit.meter), (A.node, a.node)])
        a.values_set = []
        self.assertEqual(a.children_set,
                         [("value", 7 * unit.meter, A.value),
                          ("node", a.node, A.node)])
        a.children_set = []
        a.value = 9 * unit.meter
        self.assertEqual(a.value, 9 * unit.meter)
        self.assertEqual(a.values_set, [(A.value, 9 * unit.meter)])
        a.values_set = []
        self.assertEqual(a.children_set, [("value", 9 * unit.meter, A.value)])
        a.children_set = []
        a.node.bvalue = 4 * unit.meter
        self.assertEqual(a.node.bvalue, 4 * unit.meter)
        self.assertEqual(a.values_set, [])
        self.assertEqual(a.children_set,
                         [("node.bvalue", 4 * unit.meter, B.bvalue)])
        a.children_set = []
        run_coro(a.slotReconfigure(rehash(value=10, node=Hash("bvalue", 5))))
        self.assertEqual(a.value, 10 * unit.meter)
        self.assertEqual(a.node.bvalue, 5 * unit.meter)

    def test_readonly(self):
        class B(Configurable):
            bvalue = Int32(accessMode=AccessMode.READONLY,
                           unitSymbol=Unit.METER)

        class A(StoreChanges):
            value = Int32(accessMode=AccessMode.READONLY,
                          unitSymbol=Unit.METER)
            node = Node(B)

        a = A()
        self.assertFalse(isSet(a.value))
        self.assertFalse(isSet(a.node.bvalue))
        self.assertEqual(a.values_set, [(A.value, None), (A.node, a.node)])
        self.assertEqual(a.children_set, [("node", a.node, A.node)])
        a = A(rehash(value=7, node=Hash("bvalue", 3)))
        # we ignore read only parameters in configuration
        self.assertFalse(isSet(a.value))
        self.assertFalse(isSet(a.node.bvalue))
        self.assertEqual(a.values_set, [(A.value, None), (A.node, a.node)])
        a.values_set = []
        self.assertEqual(a.children_set, [("node", a.node, A.node)])
        a.children_set = []
        a.value = 9 * unit.meter
        self.assertEqual(a.value, 9 * unit.meter)
        self.assertEqual(a.values_set, [(A.value, 9 * unit.meter)])
        a.values_set = []
        self.assertEqual(a.children_set, [("value", 9 * unit.meter, A.value)])
        a.children_set = []
        a.node.bvalue = 4 * unit.meter
        self.assertEqual(a.node.bvalue, 4 * unit.meter)
        self.assertEqual(a.values_set, [])
        self.assertEqual(a.children_set,
                         [("node.bvalue", 4 * unit.meter, B.bvalue)])
        a.children_set = []
        with self.assertRaises(KaraboError):
            run_coro(a.slotReconfigure(rehash(node=Hash("bvalue", 5))))
        with self.assertRaises(KaraboError):
            run_coro(a.slotReconfigure(rehash(value=10)))

    def test_readonly_default(self):
        class B(Configurable):
            bvalue = Int32(defaultValue=9,
                           accessMode=AccessMode.READONLY,
                           unitSymbol=Unit.METER)

        class A(StoreChanges):
            value = Int32(defaultValue=5,
                          accessMode=AccessMode.READONLY,
                          unitSymbol=Unit.METER)
            node = Node(B)

        a = A()
        self.assertEqual(a.value, 5 * unit.meter)
        self.assertEqual(a.node.bvalue, 9 * unit.meter)
        self.assertEqual(a.values_set, [(A.value, 5 * unit.meter),
                                        (A.node, a.node)])
        a.values_set = []
        self.assertEqual(a.children_set, [("value", 5 * unit.meter, A.value),
                                          ("node", a.node, A.node)])
        a.children_set = []
        a = A(rehash(value=7, node=Hash("bvalue", 3)))
        # we ignore read only parameters in configuration
        self.assertEqual(a.values_set, [(A.value, 5 * unit.meter),
                                        (A.node, a.node)])
        a.values_set = []
        self.assertEqual(a.children_set, [("value", 5 * unit.meter, A.value),
                                          ("node", a.node, A.node)])
        a.children_set = []
        a.value = 9 * unit.meter
        self.assertEqual(a.value, 9 * unit.meter)
        self.assertEqual(a.values_set, [(A.value, 9 * unit.meter)])
        a.values_set = []
        self.assertEqual(a.children_set, [("value", 9 * unit.meter, A.value)])
        a.children_set = []
        a.node.bvalue = 4 * unit.meter
        self.assertEqual(a.node.bvalue, 4 * unit.meter)
        self.assertEqual(a.values_set, [])
        self.assertEqual(a.children_set,
                         [("node.bvalue", 4 * unit.meter, B.bvalue)])
        a.children_set = []
        with self.assertRaises(KaraboError):
            run_coro(a.slotReconfigure(rehash(node=Hash("bvalue", 5))))
        with self.assertRaises(KaraboError):
            run_coro(a.slotReconfigure(rehash(value=10)))

    def test_init_nodefault(self):
        class B(Configurable):
            bvalue = Int32(accessMode=AccessMode.INITONLY,
                           unitSymbol=Unit.METER)

        class A(StoreChanges):
            value = Int32(accessMode=AccessMode.INITONLY,
                          unitSymbol=Unit.METER)
            node = Node(B)

        a = A()
        self.assertFalse(isSet(a.value))
        self.assertFalse(isSet(a.node.bvalue))
        self.assertEqual(a.values_set, [(A.value, None), (A.node, a.node)])
        self.assertEqual(a.children_set, [("node", a.node, A.node)])
        a = A(rehash(value=7, node=Hash("bvalue", 3)))
        self.assertEqual(a.value, 7 * unit.meter)
        self.assertEqual(a.node.bvalue, 3 * unit.meter)
        self.assertEqual(a.values_set,
                         [(A.value, 7 * unit.meter), (A.node, a.node)])
        a.values_set = []
        self.assertEqual(a.children_set,
                         [("value", 7 * unit.meter, A.value),
                          ("node", a.node, A.node)])
        a.children_set = []
        a.value = 9 * unit.meter
        self.assertEqual(a.value, 9 * unit.meter)
        self.assertEqual(a.values_set, [(A.value, 9 * unit.meter)])
        a.values_set = []
        self.assertEqual(a.children_set, [("value", 9 * unit.meter, A.value)])
        a.children_set = []
        a.node.bvalue = 4 * unit.meter
        self.assertEqual(a.node.bvalue, 4 * unit.meter)
        self.assertEqual(a.values_set, [])
        self.assertEqual(a.children_set,
                         [("node.bvalue", 4 * unit.meter, B.bvalue)])
        a.children_set = []
        with self.assertRaises(KaraboError):
            run_coro(a.slotReconfigure(rehash(node=Hash("bvalue", 5))))
        with self.assertRaises(KaraboError):
            run_coro(a.slotReconfigure(rehash(value=10)))

    def test_init_default(self):
        class B(Configurable):
            bvalue = Int32(accessMode=AccessMode.INITONLY,
                          unitSymbol=Unit.METER, defaultValue=33)

        class A(StoreChanges):
            value = Int32(accessMode=AccessMode.INITONLY,
                          unitSymbol=Unit.METER, defaultValue=22)
            node = Node(B)

        a = A()
        self.assertEqual(a.value, 22 * unit.meter)
        self.assertEqual(a.node.bvalue, 33 * unit.meter)
        self.assertEqual(a.values_set, [(A.value, 22 * unit.meter),
                                        (A.node, a.node)])
        a.values_set = []
        self.assertEqual(a.children_set, [("value", 22 * unit.meter, A.value),
                                          ("node", a.node, A.node)])
        a.children_set = []
        a = A(rehash(value=7, node=Hash("bvalue", 3)))
        self.assertEqual(a.values_set, [(A.value, 7 * unit.meter),
                                        (A.node, a.node)])
        a.values_set = []
        self.assertEqual(a.children_set, [("value", 7 * unit.meter, A.value),
                                          ("node", a.node, A.node)])
        a.children_set = []
        a.value = 9 * unit.meter
        self.assertEqual(a.value, 9 * unit.meter)
        self.assertEqual(a.values_set, [(A.value, 9 * unit.meter)])
        a.values_set = []
        self.assertEqual(a.children_set, [("value", 9 * unit.meter, A.value)])
        a.children_set = []
        a.node.bvalue = 4 * unit.meter
        self.assertEqual(a.node.bvalue, 4 * unit.meter)
        self.assertEqual(a.values_set, [])
        self.assertEqual(a.children_set,
                         [("node.bvalue", 4 * unit.meter, B.bvalue)])
        a.children_set = []
        with self.assertRaises(KaraboError):
            run_coro(a.slotReconfigure(rehash(node=Hash("bvalue", 5))))
        with self.assertRaises(KaraboError):
            run_coro(a.slotReconfigure(rehash(value=10)))


    def test_init_mandatory(self):
        class B(Configurable):
            bvalue = Int32(accessMode=AccessMode.INITONLY,
                           assignment=Assignment.MANDATORY,
                           unitSymbol=Unit.METER)

        class A(StoreChanges):
            value = Int32(accessMode=AccessMode.INITONLY,
                          assignment=Assignment.MANDATORY,
                          unitSymbol=Unit.METER)
            node = Node(B)

        with self.assertRaises(KaraboError):
            a = A(rehash(node=Hash("bvalue", 3)))
        with self.assertRaises(KaraboError):
            a = A(rehash(value=7))

        a = A(rehash(value=7, node=Hash("bvalue", 3)))
        self.assertEqual(a.value, 7 * unit.meter)
        self.assertEqual(a.node.bvalue, 3 * unit.meter)
        self.assertEqual(a.values_set,
                         [(A.value, 7 * unit.meter), (A.node, a.node)])
        a.values_set = []
        self.assertEqual(a.children_set,
                         [("value", 7 * unit.meter, A.value),
                          ("node", a.node, A.node)])
        a.children_set = []
        a.value = 9 * unit.meter
        self.assertEqual(a.value, 9 * unit.meter)
        self.assertEqual(a.values_set, [(A.value, 9 * unit.meter)])
        a.values_set = []
        self.assertEqual(a.children_set, [("value", 9 * unit.meter, A.value)])
        a.children_set = []
        a.node.bvalue = 4 * unit.meter
        self.assertEqual(a.node.bvalue, 4 * unit.meter)
        self.assertEqual(a.values_set, [])
        self.assertEqual(a.children_set,
                         [("node.bvalue", 4 * unit.meter, B.bvalue)])
        a.children_set = []
        with self.assertRaises(KaraboError):
            run_coro(a.slotReconfigure(rehash(value=10)))
        with self.assertRaises(KaraboError):
            run_coro(a.slotReconfigure(rehash(node=Hash("bvalue", 5))))

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

        class A(StoreChanges):
            nested = Node(B)

        a = A()
        self.assertEqual(a.values_set, [(A.nested, a.nested)])
        a.values_set = []
        self.assertEqual(a.children_set, [("nested", a.nested, A.nested)])
        a.children_set = []
        self.assertEqual(a.nested.nested.value, 3)
        a.nested.nested.value = 5
        self.assertEqual(a.values_set, [])
        self.assertEqual(a.children_set, [("nested.nested.value", 5, C.value)])
        a.children_set = []
        self.assertEqual(a.nested.nested.value, 5)
        run_coro(a.slotReconfigure(rehash(
            nested=Hash("nested", Hash("value", 7)))))
        self.assertEqual(a.nested.nested.value, 7)
        self.assertEqual(a.values_set, [])
        self.assertEqual(a.children_set, [("nested.nested.value", 7, C.value)])

    def test_table(self):
        class Row(Configurable):
            name = String()
            number = Int32()

        class A(Configurable):
            table = VectorHash(rows=Row, defaultValue=[])

        a = A()
        self.assertEqual(a.table.shape, (0,))
        a.table = [("asf", 3), ("fw", 2)]
        self.assertEqual(a.table.shape, (2,))
        self.assertEqual(a.table["name"][1], "fw")

        a = A(Hash("table", [Hash("name", "bla", "number", 5)]))
        self.assertEqual(a.table.shape, (1,))
        self.assertEqual(a.table["name"][0], "bla")


if __name__ == "__main__":
    main()
