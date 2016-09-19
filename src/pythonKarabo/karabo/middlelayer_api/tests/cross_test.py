"""This tests the communication between bound API and middlelayer API"""

from asyncio import coroutine, create_subprocess_exec, get_event_loop
from contextlib import contextmanager
import os
import os.path
from subprocess import PIPE
import sys
from unittest import main

from karabo.middlelayer import (
    AccessLevel, AlarmCondition, Assignment, Device, getDevice, Int32,
    MetricPrefix, shutdown, Slot, State, unit, Unit, waitUntil)

from .eventloop import DeviceTest, async_tst


class MiddlelayerDevice(Device):
    value = Int32()

    @Slot()
    def slot(self):
        self.marker = True


class Tests(DeviceTest):
    @classmethod
    @contextmanager
    def lifetimeManager(cls):
        cls.device = MiddlelayerDevice(dict(_deviceId_="middlelayerDevice"))
        with cls.deviceManager(lead=cls.device):
            yield

    def setUp(self):
        self.__starting_dir = os.curdir
        os.chdir(os.path.dirname(os.path.abspath(__file__)))
        self.process = None

    def tearDown(self):
        had_to_kill = False
        if self.process is not None and self.process.returncode is None:
            self.process.kill()
            self.loop.run_until_complete(self.process.wait())
            had_to_kill = True
        try:
            os.remove("karabo.log")
        except FileNotFoundError:
            pass  # never mind
        os.chdir(self.__starting_dir)
        if had_to_kill:
            self.fail("process didn't properly go down")

    @coroutine
    def wait_for_stderr(self, wait):
        line = ""
        while wait not in line:
            line = (yield from self.process.stderr.readline()).decode("ascii")

    @async_tst
    def test_cross(self):
        # it takes typically 2 s for the bound device to start
        self.process = yield from create_subprocess_exec(
             sys.executable, "bounddevice.py", stderr=PIPE)
        yield from self.wait_for_stderr("got started")
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
            self.assertEqual(proxy.state, State.UNKNOWN)
            self.assertEqual(proxy.alarmCondition, AlarmCondition.NONE)

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
        self.assertEqual(self.device.value, 99)
        self.assertTrue(self.device.marker)
        yield from shutdown(proxy)
        # it takes up to 5 s for the bound device to actually shut down
        yield from self.process.wait()

if __name__ == "__main__":
    main()
