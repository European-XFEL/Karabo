from asyncio import coroutine
from contextlib import contextmanager
from unittest import main

from karabo.common.states import State
from karabo.macro_api.device_interface import (
    listMotors, listCameras, listTriggers)
from karabo.middlelayer_api.device_client import getDevice, DeviceClientBase
from karabo.middlelayer_api.hash import Int32, VectorString
from karabo.middlelayer_api.tests.eventloop import (
    DeviceTest, async_tst)


class MyDevice(DeviceClientBase):
    __version__ = "2.3"

    value = Int32(
        defaultValue=0)

    interfaces = VectorString(
        defaultValue=["Motor", "Camera"])

    @coroutine
    def onInitialization(self):
        self.state = State.ON


class Tests(DeviceTest):
    @classmethod
    @contextmanager
    def lifetimeManager(cls):
        cls.myDevice = MyDevice(dict(_deviceId_='MyDevice'))
        with cls.deviceManager(lead=cls.myDevice):
            yield

    @async_tst
    def test_find_interfaces(self):
        with (yield from getDevice("MyDevice")) as proxy:
            self.assertEqual(proxy.interfaces, ["Motor", "Camera"])

        motors = listMotors()
        self.assertIn("MyDevice", motors)
        cameras = listCameras()
        self.assertIn("MyDevice", cameras)
        triggers = listTriggers()
        self.assertNotIn("MyDevice", triggers)

        # Find rubbish
        motors = listMotors("deep")
        self.assertNotIn("MyDevice", motors)


if __name__ == '__main__':
    main()
