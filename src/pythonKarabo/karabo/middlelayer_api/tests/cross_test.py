"""This tests the communication between bound API and middlelayer API"""

from asyncio import coroutine, create_subprocess_exec, wait_for
import os
import os.path
from subprocess import PIPE
import sys
from unittest import TestCase, main

from karabo.middlelayer import (
    AccessLevel, Assignment, Device, getDevice, Int32,
    MetricPrefix, shutdown, Slot, State, unit, Unit, waitUntil)

from .eventloop import setEventLoop


class MiddlelayerDevice(Device):
    value = Int32()

    @Slot()
    def slot(self):
        self.marker = True


class Tests(TestCase):
    @coroutine
    def connect(self, device):
        # it takes typically 2 s for the bound device to start
        line = ""
        while "got started" not in line:
            line = (yield from self.bound.stderr.readline()).decode("ascii")
        proxy = yield from getDevice("boundDevice")
        self.assertEqual(proxy.a, 22.5 * unit.milliampere,
                         "didn't receive initial value from bound device")

        a_desc = type(proxy).a
        self.assertIs(a_desc.unitSymbol, Unit.AMPERE)
        self.assertIs(a_desc.metricPrefixSymbol, MetricPrefix.MILLI)
        self.assertIs(a_desc.requiredAccessLevel, AccessLevel.EXPERT)
        self.assertIs(a_desc.assignment, Assignment.OPTIONAL)
        self.assertEqual(a_desc.displayedName, "parameter a")
        self.assertEqual(a_desc.description, "a's description")
        self.assertEqual(a_desc.defaultValue, 22.5)
        self.assertEqual(a_desc.minExc, 22)
        self.assertEqual(a_desc.maxExc, 33)
        self.assertEqual(a_desc.minInc, 11)
        self.assertEqual(a_desc.maxInc, 23)
        self.assertEqual(a_desc.allowedStates, {State.INIT, State.UNKNOWN})

        self.assertEqual(len(proxy.table), 1)
        self.assertEqual(proxy.table[0].d, 5)

        with proxy:
            yield from proxy.setA()
            self.assertEqual(proxy.a, 22.7 * unit.milliampere,
                             "didn't receive change from bound device")
            self.assertEqual(repr(proxy.a.timestamp),
                             "2009-09-01T13:55:22 UTC")
            self.assertEqual(proxy.node.b, 100 * unit.kilometer)
            self.assertEqual(repr(proxy.node.b.timestamp),
                             "2016-06-17T13:55:22 UTC")

            with self.assertRaises(ValueError):
                proxy.a = 77
            self.assertEqual(proxy.a, 22.7 * unit.milliampere)

            proxy.a = 0.0228 * unit.ampere
            self.assertEqual(proxy.a, 22.7 * unit.milliampere,
                             "proxy should set value on device, not own value")
            yield from waitUntil(lambda: proxy.a == 22.8 * unit.milliampere)
            self.assertEqual(proxy.a, 22.8 * unit.milliampere,
                             "didn't receive change from bound device")

        yield from proxy.backfire()
        self.assertEqual(device.value, 99)
        self.assertTrue(device.marker)
        yield from shutdown(proxy)
        # it takes up to 5 s for the bound device to actually shut down
        yield from self.bound.wait()

    def setUp(self):
        self.__starting_dir = os.curdir
        os.chdir(os.path.dirname(os.path.abspath(__file__)))
        self.loop = setEventLoop()
        self.bound = self.loop.run_until_complete(
            create_subprocess_exec(sys.executable, "bounddevice.py",
                                   stderr=PIPE))

    def tearDown(self):
        if self.bound.returncode is None:
            self.bound.kill()
            self.loop.run_until_complete(self.bound.wait())
        self.loop.close()
        try:
            os.remove("karabo.log")
        except FileNotFoundError:
            pass  # never mind
        os.chdir(self.__starting_dir)

    def test_cross(self):
        md = MiddlelayerDevice(dict(_deviceId_="middlelayerDevice"))
        md.startInstance()
        self.loop.run_until_complete(wait_for(
            self.loop.create_task(self.connect(md), md),
            15))


if __name__ == "__main__":
    main()
