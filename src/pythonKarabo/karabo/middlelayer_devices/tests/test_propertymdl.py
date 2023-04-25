# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from contextlib import contextmanager

from karabo.middlelayer import (
    KaraboError, get_property, getDevice, setWait, updateDevice)
from karabo.middlelayer_api.broker import jms
from karabo.middlelayer_api.tests.eventloop import DeviceTest, async_tst

from ..property_test import PropertyTestMDL

conf = {
    "classId": "PropertyTestMDL",
    "_deviceId_": "Test_PropertyTestMDL"
}


class Tests(DeviceTest):
    @classmethod
    @contextmanager
    def lifetimeManager(cls):
        cls.dev = PropertyTestMDL(conf)
        with cls.deviceManager(lead=cls.dev):
            yield

    @async_tst
    async def test_property_test(self):
        # tests are run in sequence as sub tests
        # device server thus is only instantiated once

        with self.subTest(msg="Test setting read-only"):
            props = ["boolProperty",
                     "int32Property", "uint32Property",
                     "int64Property", "uint64Property",
                     "floatProperty", "doubleProperty",
                     "node.counter"]

            with (await getDevice("Test_PropertyTestMDL")) as d:
                if not jms:
                    await updateDevice(d)
                for i, prop in enumerate(props, 10):
                    self.assertNotEqual(get_property(d, prop), i)
                    await setWait(d, prop, i)
                    self.assertEqual(get_property(d, prop), i)
                    self.assertEqual(get_property(d, prop + "ReadOnly"), i)

        with self.subTest(msg="Test alarm slots"):
            with (await getDevice("Test_PropertyTestMDL")) as d:
                if not jms:
                    await updateDevice(d)
                # invalid alarm condition in stringProperty
                await setWait(d, "stringProperty", "")
                with self.assertRaises(KaraboError):
                    await d.setAlarm()

                # valid alarm condition in stringProperty
                await setWait(d, "stringProperty", "warn")
                await d.setAlarm()
                self.assertEqual(d.alarmCondition.value, "warn")

                # slot that raises NotImplementedError
                with self.assertRaises(KaraboError):
                    await d.setNoAckAlarm()

        with self.subTest(msg="Test noded slots"):
            with (await getDevice("Test_PropertyTestMDL")) as d:
                if not jms:
                    await updateDevice(d)
                await d.node.reset()
                self.assertEqual(d.node.counterReadOnly, 0)
                await d.node.increment()
                self.assertEqual(d.node.counterReadOnly, 1)
                await d.node.reset()
                self.assertEqual(d.node.counterReadOnly, 0)
