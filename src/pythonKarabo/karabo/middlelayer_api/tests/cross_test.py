"""This tests the communication between bound API and middlelayer API"""

from asyncio import coroutine, create_subprocess_exec, wait_for
import os
import os.path
from subprocess import PIPE
import sys
from unittest import TestCase, main

from karabo.middlelayer import (
    Device, getDevice, Int32, shutdown, Slot, waitUntilNew)


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
        self.assertEqual(proxy.a, 55,
                         "didn't receive initial value from bound device")
        with proxy:
            yield from proxy.setA()
            self.assertEqual(proxy.a, 33,
                             "didn't receive change from bound device")
            proxy.a = 77
            self.assertEqual(proxy.a, 33,
                             "proxy should set value on device, not own value")
            yield from waitUntilNew(proxy).a
            self.assertEqual(proxy.a, 77,
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
        os.chdir(self.__starting_dir)
        if self.bound.returncode is None:
            self.bound.kill()
            self.loop.run_until_complete(self.bound.wait())
        self.loop.close()

    def test_cross(self):
        md = MiddlelayerDevice(dict(_deviceId_="middlelayerDevice"))
        md.startInstance()
        self.loop.run_until_complete(wait_for(
            self.loop.create_task(self.connect(md), md),
            15))


if __name__ == "__main__":
    main()
