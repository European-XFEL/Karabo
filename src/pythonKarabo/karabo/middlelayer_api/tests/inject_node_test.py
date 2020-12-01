import asyncio
from unittest import TestCase, main
import string

import numpy

from karabo.middlelayer import (
    AccessLevel, AccessMode, Assignment, Configurable, DeviceNode, Int32,
    MetricPrefix, Overwrite, Slot, State, unit, Unit)
from ..injectable import InjectMixin


def run_coro(coro):
    loop = asyncio.get_event_loop()
    return loop.run_until_complete(coro)


class Tests(TestCase):
    @classmethod
    def setUpClass(cls):
        asyncio.set_event_loop(asyncio.new_event_loop())

    def test_deviceNode(self):
        class A(Configurable):
            node = DeviceNode()

        a = A({"node": "remote"})
        schema = a.getClassSchema()
        self.assertEqual(schema.hash['node', 'accessMode'],
                         AccessMode.INITONLY.value)
        self.assertEqual(schema.hash['node', 'assignment'],
                         Assignment.MANDATORY.value)

    def test_deviceNode_default(self):
        class A(Configurable):
            node = DeviceNode(defaultValue="remote")

        a = A()
        schema = a.getClassSchema()
        self.assertEqual(schema.hash['node', 'accessMode'],
                         AccessMode.INITONLY.value)
        self.assertEqual(schema.hash['node', 'assignment'],
                         Assignment.MANDATORY.value)
        self.assertEqual(schema.hash['node', 'defaultValue'],
                         "remote")
        # Becomes a node!
        self.assertEqual(a.node, None)

    def test_overwrite_inject(self):
        class Mandy(InjectMixin):
            number = Int32(displayedName="whatever", minExc=7,
                           accessMode=AccessMode.READONLY,
                           allowedStates={State.ON}, tags=set(),
                           unitSymbol=Unit.METER,
                           metricPrefixSymbol=MetricPrefix.MILLI,
                           options=[8, 9, 10])

            numberEnum = Int32(displayedName="EnumAccess",
                               defaultValue=AccessLevel.OPERATOR,
                               enum=AccessLevel)

            @Slot(displayedName="MandyRandy", allowedStates=[State.INIT])
            def randyMandy(self):
                pass

            deviceId = None

            def _register_slots(self):
                pass

            def _notifyNewSchema(self):
                pass

            def signalChanged(self, deviceId, hsh):
                pass

        mandy = Mandy()
        setter_before_inject = mandy.__class__.number.setter
        mandy.__class__.number = Overwrite(
            minExc=3, allowedStates={State.OFF},
            accessMode=AccessMode.INITONLY,
            unitSymbol=Unit.SECOND, metricPrefixSymbol=MetricPrefix.MEGA,
            tags={"naughty"}, options=[6, 4])

        mandy.__class__.numberEnum = Overwrite(defaultValue=AccessLevel.ADMIN,
                                               options=[AccessLevel.ADMIN])

        mandy.__class__.randyMandy = Overwrite(
            displayedName="NoMandy", allowedStates=[State.ON]
        )
        run_coro(mandy.publishInjectedParameters())
        setter_after_inject = mandy.__class__.number.setter
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
        self.assertEqual(mandy.numberEnum.descriptor.options,
                         [AccessLevel.ADMIN])
        self.assertEqual(mandy.numberEnum.descriptor.defaultValue,
                         AccessLevel.ADMIN)
        self.assertIs(mandy.randyMandy.descriptor.displayedName, "NoMandy")
        self.assertEqual(mandy.randyMandy.descriptor.allowedStates,
                         {State.ON})
        self.assertEqual(setter_before_inject, setter_after_inject)


if __name__ == "__main__":
    main()
