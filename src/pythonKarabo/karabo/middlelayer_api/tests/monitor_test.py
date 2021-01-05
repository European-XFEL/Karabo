from asyncio import ensure_future, sleep
from contextlib import contextmanager
import time
from unittest import main

from karabo.middlelayer_api.device import Device
from karabo.native import Int32 as Int, Slot
from karabo.middlelayer_api.macro import Macro, Monitor, RemoteDevice

from .eventloop import DeviceTest, sync_tst


class Remote(Device):
    counter = Int(defaultValue=-1)

    @Slot()
    async def count(self):
        ensure_future(self.do_count())

    async def do_count(self):
        for i in range(1, 30):
            self.counter = i
            await sleep(0.1)


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


class Tests(DeviceTest):
    @classmethod
    @contextmanager
    def lifetimeManager(cls):
        cls.local = Local(_deviceId_="local", project="test", module="test",
                          may_start_thread=False)
        cls.remA = Remote(dict(_deviceId_="remA"))
        cls.remB = Remote(dict(_deviceId_="remB"))
        with cls.deviceManager(cls.remA, cls.remB, lead=cls.local):
            yield

    @sync_tst
    def test_count(self):
        self.local.startA()
        self.local.startB()
        time.sleep(0.2)
        for i in range(30):
            self.assertEqual(self.local.division,
                             self.remA.counter // self.remB.counter)
            time.sleep(0.1)

    @sync_tst
    def test_error(self):
        with self.assertLogs("local", "ERROR"):
            self.local.error()


if __name__ == "__main__":
    main()
