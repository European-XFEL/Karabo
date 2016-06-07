"""This tests the communication between bound API and middlelayer API"""

from asyncio import coroutine, create_subprocess_exec, wait_for
import os
import os.path
from subprocess import PIPE
import sys
from unittest import TestCase, main

from karabo.middlelayer import (
    AccessLevel, Assignment, Device, getDevice, Int32,
    MetricPrefix, shutdown, Slot, Unit, waitUntilNew)

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
        self.assertEqual(proxy.a, 22.5,
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
        self.assertEqual(a_desc.allowedStates, ["some", "thing"])

        with proxy:
            yield from proxy.setA()
            self.assertEqual(proxy.a, 22.7,
                             "didn't receive change from bound device")
            proxy.a = 22.8
            self.assertEqual(proxy.a, 22.7,
                             "proxy should set value on device, not own value")
            yield from waitUntilNew(proxy).a
            self.assertEqual(proxy.a, 22.8,
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
