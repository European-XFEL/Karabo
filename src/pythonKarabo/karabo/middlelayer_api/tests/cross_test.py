"""This tests the communication between bound API and middlelayer API"""

from asyncio import coroutine, sleep, wait_for
import os
import os.path
from subprocess import Popen
import sys
from unittest import TestCase, main

from karabo.middlelayer import getDevice, shutdown
from karabo.middlelayer_api.cli import DeviceClient

from .eventloop import setEventLoop


class Tests(TestCase):
    @coroutine
    def connect(self):
        # it takes typically 2 s for the bound device to start
        yield from sleep(3)
        proxy = yield from getDevice("testDevice")
        self.assertEqual(proxy.a, 55,
                         "didn't receive initial value from bound device")
        with proxy:
            yield from proxy.setA()
            self.assertEqual(proxy.a, 33,
                             "didn't receive change from bound device")
            proxy.a = 77
            self.assertEqual(proxy.a, 33,
                             "proxy should set value on device, not own value")
            yield from sleep(0.1)
            self.assertEqual(proxy.a, 77,
                             "didn't receive change from bound device")
        yield from shutdown(proxy)
        # it takes up to 5 s for the bound device to actually shut down
        self.bound.wait(10)

    def setUp(self):
        self.__starting_dir = os.curdir
        os.chdir(os.path.dirname(os.path.abspath(__file__)))
        self.bound = Popen([sys.executable, "bounddevice.py"])

    def tearDown(self):
        os.chdir(self.__starting_dir)
        if self.bound.returncode is None:
            self.bound.kill()
            self.bound.wait()

    def test_cross(self):
        loop = setEventLoop()
        dc = DeviceClient(dict(_deviceId_="dc"))
        dc.startInstance()
        loop.run_until_complete(wait_for(
            loop.create_task(self.connect(), dc),
            15))
        loop.close()


if __name__ == "__main__":
    main()
