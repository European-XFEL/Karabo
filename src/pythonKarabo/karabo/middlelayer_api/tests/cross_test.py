"""This tests the communication between bound API and middlelayer API"""

from asyncio import async, coroutine, create_subprocess_exec, get_event_loop
from contextlib import contextmanager
from datetime import datetime
import os
import os.path
from subprocess import PIPE
import sys
from unittest import main

from karabo.middlelayer import (
    AccessLevel, AlarmCondition, Assignment, background, Configurable, Device,
    DeviceClientBase, getDevice, getHistory, Int32, MetricPrefix, Node,
    shutdown, sleep, Slot, State, unit, Unit, waitUntil, waitUntilNew)

from .eventloop import DeviceTest, async_tst


class Child(Configurable):
    number = Int32()


class MiddlelayerDevice(DeviceClientBase):
    value = Int32()

    child = Node(Child)

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

        for fn in ("karabo.log", "serverId.xml", "loggermap.xml"):
            try:
                os.remove(fn)
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
        self.assertEqual(proxy.table[0]["d"], 5 * unit.meter)

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

            def setter():
                proxy.a = 22.3 * unit.milliampere
                self.assertEqual(proxy.a, 22.3 * unit.milliampere)
            yield from background(setter)

            proxy.a = 0.0228 * unit.ampere
            self.assertNotEqual(proxy.a, 22.8 * unit.milliampere,
                                "proxy should set device's, not own value")
            yield from waitUntil(lambda: proxy.a == 22.8 * unit.milliampere)
            self.assertEqual(proxy.a, 22.8 * unit.milliampere,
                             "didn't receive change from bound device")

            proxy.table = [(3, "african"), (7, "european")]
            self.assertEqual(len(proxy.table), 1)
            yield from waitUntilNew(proxy.table)
            self.assertEqual(len(proxy.table), 2)
            self.assertEqual(proxy.table[1]["d"], 7 * unit.meter)
            self.assertEqual(proxy.table[0]["s"], "african")

            yield from proxy.injectSchema()
            yield from sleep(0.1)
            self.assertEqual(proxy.word, "Hello")
            self.assertEqual(proxy.word.descriptor.description, "The word")

        yield from proxy.backfire()
        self.assertEqual(self.device.value, 99)
        self.assertTrue(self.device.marker)
        yield from shutdown(proxy)
        # it takes up to 5 s for the bound device to actually shut down
        yield from self.process.wait()

    @async_tst
    def test_history(self):
        before = datetime.now()

        karabo = os.environ["KARABO"]
        self.process = yield from create_subprocess_exec(
            os.path.join(karabo, "bin", "karabo-cppserver"),
            "historytest.xml", stderr=PIPE, stdout=PIPE)

        @coroutine
        def print_stdout():
            while not self.process.stdout.at_eof():
                line = yield from self.process.stdout.readline()
                print(line.decode("ascii"), end="")
        async(print_stdout())

        with (yield from getDevice("DataLogger-middlelayerDevice")) as logger:
            yield from waitUntil(lambda: logger.state == State.NORMAL)

        for i in range(4):
            self.device.value = i
            self.device.child.number = -i
            self.device.update()

        after = datetime.now()

        # This is the first history request ever, so it returns an empty
        # list (see https://in.xfel.eu/redmine/issues/9414).
        yield from getHistory(
            "middlelayerDevice.value", before.isoformat(), after.isoformat())
        yield from getHistory(
            "middlelayerDevice.child.number", before.isoformat(),
            after.isoformat())

        # We have to write another value to close the first archive file :-(...
        self.device.value = 4
        self.device.child.number = -4
        self.device.update()

        # ... and finally need to wait until the new archive and index files
        # are flushed (see flushInterval in history.xml).
        yield from sleep(1.1)

        after = datetime.now()

        old_history = yield from getHistory(
            "middlelayerDevice", before.isoformat(), after.isoformat()).value
        str_history = yield from getHistory(
            "middlelayerDevice.value", before.isoformat(), after.isoformat())
        device = yield from getDevice("middlelayerDevice")
        proxy_history = yield from getHistory(
            device.value, before.isoformat(), after.isoformat())

        for hist in old_history, str_history, proxy_history:
            self.assertEqual([h for _, _, _, h in hist[-5:]], list(range(5)))

        node_history = yield from getHistory(
            "middlelayerDevice.child.number", before.isoformat(),
            after.isoformat())
        node_proxy_history = yield from getHistory(
            device.child.number, before.isoformat(), after.isoformat())

        for hist in node_history, node_proxy_history:
            self.assertEqual([-h for _, _, _, h in hist[-5:]], list(range(5)))

        yield from get_event_loop().instance()._ss.request(
            "Karabo_DLManagerServer", "slotKillServer")
        yield from self.process.wait()
    test_history.slow = True


if __name__ == "__main__":
    main()
