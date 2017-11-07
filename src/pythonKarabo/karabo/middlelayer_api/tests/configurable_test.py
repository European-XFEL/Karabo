import asyncio
from unittest import TestCase, main
from zlib import adler32

from karabo.middlelayer import (
    AccessMode, Assignment, Configurable, DaqDataType, decodeBinary,
    encodeBinary, Hash, Int32, isSet, KaraboError, MetricPrefix, Node,
    Overwrite, State, String, Unit, unit, VectorHash)
from karabo.middlelayer_api.injectable import Injectable


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
    return decodeBinary(encodeBinary(h))


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
        self.assertEqual(a.children_set, [("node", a.node, A.node)])
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

        a.table.append(("lll", 55))
        self.assertEqual(len(a.table), 3)
        self.assertEqual(a.table[2]["name"], "lll")
        self.assertEqual(a.table["number"][2], 55)

        a.table.extend([(str(i), i) for i in range(10)])
        self.assertEqual(a.table[10]["number"], 7)

        del a.table[10]
        self.assertEqual(a.table[10]["number"], 8)
        del a.table[10:12]
        self.assertEqual(len(a.table), 10)

        a.table[1] = ("k", 111)
        self.assertEqual(a.table[1]["name"], "k")
        self.assertEqual(len(a.table), 10)

        a.table[5:7] = [("A", 2), ("B", 3)]
        self.assertEqual(a.table[5]["name"], "A")
        self.assertEqual(a.table["number"][6], 3)
        self.assertEqual(a.table["number"][7], 4)

        a.table.insert(2, ("C", 11))
        self.assertEqual(a.table[2]["name"], "C")
        self.assertEqual(a.table[3]["name"], "lll")
        self.assertEqual(len(a.table), 11)

        self.assertEqual(adler32(str(a.table).encode("ascii")), 1313745943)

        a = A(Hash("table", [Hash("name", "bla", "number", 5)]))
        self.assertEqual(a.table.shape, (1,))
        self.assertEqual(a.table["name"][0], "bla")
        b = a.table.pop()
        self.assertEqual(a.table.shape, (0,))
        self.assertEqual(b["name"], "bla")
        self.assertEqual(b["number"], 5)

    def test_overwrite(self):
        class Mandy(Configurable):
            number = Int32(displayedName="whatever", minExc=7,
                           accessMode=AccessMode.READONLY,
                           allowedStates={State.ON}, tags=set(),
                           unitSymbol=Unit.METER,
                           metricPrefixSymbol=MetricPrefix.MILLI,
                           options=[8, 9, 10])
            cookies = String(allowedStates={State.HOMING},
                             displayedName="else",
                             unitSymbol=Unit.METER,
                             metricPrefixSymbol=MetricPrefix.MILLI,
                             options=["a", "b"])
            state = String(enum=State)

        class Brian(Mandy):
            number = Overwrite(minExc=3, allowedStates={State.OFF},
                               accessMode=AccessMode.INITONLY,
                               unitSymbol=Unit.SECOND,
                               metricPrefixSymbol=MetricPrefix.MEGA,
                               tags={"naughty"},
                               options=[6, 4])
            cookies = Overwrite()
            state = Overwrite(options=[State.ON, State.OFF])

        self.assertEqual(Brian.number.key, "number")
        self.assertEqual(Mandy.number.minExc, 7)
        self.assertEqual(Brian.number.minExc, 3)
        self.assertEqual(Mandy.number.displayedName, "whatever")
        self.assertEqual(Brian.number.displayedName, "whatever")
        self.assertEqual(Mandy.number.allowedStates, {State.ON})
        self.assertEqual(Brian.number.allowedStates, {State.OFF})
        self.assertEqual(Mandy.number.tags, set())
        self.assertEqual(Brian.number.tags, {"naughty"})
        self.assertIs(Mandy.number.accessMode, AccessMode.READONLY)
        self.assertIs(Brian.number.accessMode, AccessMode.INITONLY)
        self.assertIs(Mandy.number.unitSymbol, Unit.METER)
        self.assertIs(Brian.number.unitSymbol, Unit.SECOND)
        self.assertEqual(Mandy.number.units, unit.millimeter)
        self.assertEqual(Brian.number.units, unit.megasecond)
        self.assertEqual(Mandy.number.options, [8, 9, 10])
        self.assertEqual(Brian.number.options, [6, 4])

        self.assertEqual(Mandy.cookies.allowedStates, {State.HOMING})
        self.assertEqual(Brian.cookies.allowedStates, {State.HOMING})
        self.assertEqual(Mandy.cookies.displayedName, "else")
        self.assertEqual(Brian.cookies.displayedName, "else")
        self.assertIs(Mandy.cookies.unitSymbol, Unit.METER)
        self.assertIs(Brian.cookies.unitSymbol, Unit.METER)
        self.assertEqual(Mandy.cookies.units, unit.millimeter)
        self.assertEqual(Brian.cookies.units, unit.millimeter)
        self.assertEqual(Mandy.cookies.options, ["a", "b"])
        self.assertEqual(Brian.cookies.options, ["a", "b"])

        self.assertIsNone(Mandy.state.options)
        self.assertEqual(Brian.state.options, [State.ON, State.OFF])

    def test_overwrite_inject(self):
        class Mandy(Injectable):
            number = Int32(displayedName="whatever", minExc=7,
                           accessMode=AccessMode.READONLY,
                           allowedStates={State.ON}, tags=set(),
                           unitSymbol=Unit.METER,
                           metricPrefixSymbol=MetricPrefix.MILLI,
                           options=[8, 9, 10])

            deviceId = None

            def _register_slots(self):
                pass

            def _notifyNewSchema(self):
                pass

            def signalChanged(self, deviceId, hsh):
                pass

        mandy = Mandy()
        mandy.__class__.number = Overwrite(
            minExc=3, allowedStates={State.OFF},
            accessMode=AccessMode.INITONLY,
            unitSymbol=Unit.SECOND, metricPrefixSymbol=MetricPrefix.MEGA,
            tags={"naughty"}, options=[6, 4])
        run_coro(mandy.publishInjectedParameters())

        self.assertEqual(mandy.number.descriptor.key, "number")
        self.assertEqual(Mandy.number.minExc, 7)
        self.assertEqual(mandy.number.descriptor.minExc, 3)
        self.assertEqual(Mandy.number.displayedName, "whatever")
        self.assertEqual(mandy.number.descriptor.displayedName, "whatever")
        self.assertEqual(Mandy.number.allowedStates, {State.ON})
        self.assertEqual(mandy.number.descriptor.allowedStates, {State.OFF})
        self.assertEqual(Mandy.number.tags, set())
        self.assertEqual(mandy.number.descriptor.tags, {"naughty"})
        self.assertIs(Mandy.number.accessMode, AccessMode.READONLY)
        self.assertIs(mandy.number.descriptor.accessMode, AccessMode.INITONLY)
        self.assertIs(Mandy.number.unitSymbol, Unit.METER)
        self.assertIs(mandy.number.descriptor.unitSymbol, Unit.SECOND)
        self.assertEqual(Mandy.number.units, unit.millimeter)
        self.assertEqual(mandy.number.descriptor.units, unit.megasecond)
        self.assertEqual(Mandy.number.options, [8, 9, 10])
        self.assertEqual(mandy.number.descriptor.options, [6, 4])

    def test_daqDataType(self):
        class B(Configurable):
            daqDataType = DaqDataType.TRAIN

        class A(StoreChanges):
            node = Node(B)

        a = A()
        self.assertEqual(a.node.daqDataType, DaqDataType.TRAIN)


if __name__ == "__main__":
    main()
