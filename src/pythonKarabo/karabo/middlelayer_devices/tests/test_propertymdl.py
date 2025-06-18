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
from contextlib import contextmanager

from karabo.middlelayer import (
    KaraboError, get_property, getDevice, setWait, updateDevice)
from karabo.middlelayer.tests.eventloop import DeviceTest, async_tst

from ..property_test import PropertyTest

conf = {
    "classId": "PropertyTest",
    "_deviceId_": "Test_PropertyTest"
}


class Tests(DeviceTest):
    @classmethod
    @contextmanager
    def lifetimeManager(cls):
        cls.dev = PropertyTest(conf)
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

            with (await getDevice("Test_PropertyTest")) as d:
                await updateDevice(d)
                for i, prop in enumerate(props, 10):
                    self.assertNotEqual(get_property(d, prop), i)
                    await setWait(d, prop, i)
                    self.assertEqual(get_property(d, prop), i)
                    self.assertEqual(get_property(d, prop + "ReadOnly"), i)

        with self.subTest(msg="Test alarm slots"):
            with (await getDevice("Test_PropertyTest")) as d:
                await updateDevice(d)
                # invalid alarm condition in stringProperty
                await setWait(d, "stringProperty", "")
                with self.assertRaises(KaraboError):
                    await d.setAlarm()

                # valid alarm condition in stringProperty
                await setWait(d, "stringProperty", "warn")
                await d.setAlarm()
                self.assertEqual(d.alarmCondition.value, "warn")

        with self.subTest(msg="Test noded slots"):
            with (await getDevice("Test_PropertyTest")) as d:
                await updateDevice(d)
                await d.node.reset()
                self.assertEqual(d.node.counterReadOnly, 0)
                await d.node.increment()
                self.assertEqual(d.node.counterReadOnly, 1)
                await d.node.reset()
                self.assertEqual(d.node.counterReadOnly, 0)
