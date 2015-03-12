import karabo
karabo.api_version = 2

from asyncio import (async, coroutine, gather, set_event_loop, sleep,
                     TimeoutError)
from unittest import TestCase, main
from functools import wraps
import time

from karabo.eventloop import EventLoop
from karabo.macro import Macro, Monitor, RemoteDevice
from karabo.python_device import Device
from karabo import Slot, Integer

from .eventloop import startDevices, stopDevices, sync_tst

class Remote(Device):
    counter = Integer(defaultValue=-1)

    @Slot()
    def count(self):
        for i in range(30):
            self.counter = i
            yield from sleep(0.1)


class Local(Macro):
    remoteA = RemoteDevice("remA")
    remoteB = RemoteDevice("remB")

    @Monitor()
    @Integer()
    def sum(self):
        return self.remoteA.counter + self.remoteB.counter

    @Slot()
    def startA(self):
        self.remoteA.count()

    @Slot()
    def startB(self):
        self.remoteB.count()


class Tests(TestCase):
    @sync_tst
    def test_count(self):
        local.startA()
        local.startB()
        time.sleep(0.2)
        for i in range(30):
            self.assertEqual(local.sum, remA.counter + remB.counter)
            time.sleep(0.1)


def setUpModule():
    global loop, remA, remB, local
    local = Local(_deviceId_="local", project="test", module="test",
                  may_start_thread=False)
    remA = Remote(dict(_deviceId_="remA"))
    remB = Remote(dict(_deviceId_="remB"))
    loop = startDevices(local, remA, remB)
    Tests.instance = local


def tearDownModule():
    stopDevices(local, remA, remB)


if __name__ == "__main__":
    main()
