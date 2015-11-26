from asyncio import (async, coroutine, gather, set_event_loop, sleep,
                     TimeoutError)
import time
from unittest import TestCase, main

from karabo.api import Slot, Int, waitUntil
from karabo._api2.device import Device
from karabo._api2.macro import Macro, Monitor, RemoteDevice

from .eventloop import startDevices, stopDevices, sync_tst


class Remote(Device):
    counter = Int(defaultValue=-1)

    @Slot()
    @coroutine
    def count(self):
        async(self.do_count())

    @coroutine
    def do_count(self):
        for i in range(1, 30):
            self.counter = i
            yield from sleep(0.1)


class Local(Macro):
    remoteA = RemoteDevice("remA")
    remoteB = RemoteDevice("remB")

    @Monitor()
    @Int()
    def division(self):
        if self.remoteB.counter == 0:
            raise RuntimeError
        return self.remoteA.counter / self.remoteB.counter

    @Slot()
    def startA(self):
        self.remoteA.count()

    @Slot()
    def startB(self):
        self.remoteB.count()

    @Slot()
    def error(self):
        self.remoteA.counter = 1
        self.remoteB.counter = 0


class Tests(TestCase):
    @sync_tst
    def test_count(self):
        local.startA()
        local.startB()
        time.sleep(0.2)
        for i in range(30):
            self.assertEqual(local.division, remA.counter // remB.counter)
            time.sleep(0.1)

    @sync_tst
    def test_error(self):
        with self.assertLogs("local", "ERROR"):
            local.error()


def setUpModule():
    global loop, remA, remB, local
    local = Local(_deviceId_="local", project="test", module="test",
                  may_start_thread=False)
    remA = Remote(dict(_deviceId_="remA"))
    remB = Remote(dict(_deviceId_="remB"))
    loop = startDevices(local, remA, remB)
    Tests.instance = local


def tearDownModule():
    global remA, remB, local
    stopDevices(local, remA, remB)
    Tests.instance = local = remA = remB = None


if __name__ == "__main__":
    main()
