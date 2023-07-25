# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
from contextlib import contextmanager
from unittest import main

from karabo.common.states import State
from karabo.macro_api.device_interface import (
    listCameras, listDeviceInstantiators, listMotors, listProcessors,
    listTriggers)
from karabo.middlelayer.device import DeviceClientBase
from karabo.middlelayer.device_client import getDevice
from karabo.middlelayer.tests.eventloop import DeviceTest, async_tst
from karabo.native import Int32, VectorString


class MyDevice(DeviceClientBase):
    __version__ = "2.3"

    value = Int32(
        defaultValue=0)

    interfaces = VectorString(
        defaultValue=["Motor", "Camera", "Processor", "DeviceInstantiator"])

    async def onInitialization(self):
        self.state = State.ON


class Tests(DeviceTest):
    @classmethod
    @contextmanager
    def lifetimeManager(cls):
        cls.myDevice = MyDevice(dict(_deviceId_='MyDevice'))
        with cls.deviceManager(lead=cls.myDevice):
            yield

    @async_tst
    async def test_find_interfaces(self):
        must_have = ["Motor", "Camera", "Processor", "DeviceInstantiator"]
        with (await getDevice("MyDevice")) as proxy:

            self.assertEqual(proxy.interfaces, must_have)

        motors = listMotors()
        self.assertIn("MyDevice", motors)
        cameras = listCameras()
        self.assertIn("MyDevice", cameras)
        cameraName = listCameras("MYDEVICE")
        self.assertIn("MyDevice", cameraName)
        cameraName = listCameras("mydevice")
        self.assertIn("MyDevice", cameraName)
        triggers = listTriggers()
        self.assertNotIn("MyDevice", triggers)
        processors = listProcessors()
        self.assertIn("MyDevice", processors)
        # Find rubbish
        motors = listMotors("deep")
        self.assertNotIn("MyDevice", motors)
        instantiator = listDeviceInstantiators("mydevice")
        self.assertIn("MyDevice", instantiator)


if __name__ == '__main__':
    main()
