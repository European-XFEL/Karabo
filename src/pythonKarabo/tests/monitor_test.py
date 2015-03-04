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

def async_tst(f):
    @wraps(f)
    def wrapper(self):
        loop.run_until_complete(loop.run_in_executor(None, f, self))
    return wrapper


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
    @async_tst
    def test_count(self):
        local.startA()
        local.startB()
        time.sleep(0.2)
        for i in range(30):
            self.assertEqual(local.sum, remA.counter + remB.counter)
            time.sleep(0.1)


def setUpModule():
    global loop, remA, remB, local
    loop = EventLoop()
    set_event_loop(loop)

    local = Local(dict(_deviceId_="local", project="test", module="test"))
    remA = Remote(dict(_deviceId_="remA"))
    remB = Remote(dict(_deviceId_="remB"))
    loop.run_until_complete(gather(local.run_async(), remA.run_async(),
                                   remB.run_async()))


def tearDownModule():
    loop.run_until_complete(gather(
        local.slotKillDevice(), remA.slotKillDevice(), remB.slotKillDevice()))
    loop.close()


if __name__ == "__main__":
    main()
