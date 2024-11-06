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
from asyncio import get_running_loop

import pytest
import pytest_asyncio

from karabo.middlelayer import (
    DeviceClientBase, VectorString, listCameras, listDeviceInstantiators,
    listMotors, listProcessors, listTriggers)
from karabo.middlelayer.testing import AsyncDeviceContext, run_test


class MyDevice(DeviceClientBase):
    interfaces = VectorString(
        defaultValue=["Motor", "Camera", "Processor", "DeviceInstantiator"])


@pytest_asyncio.fixture(scope="module")
async def deviceTest():
    myDevice = MyDevice(dict(_deviceId_="MyDevice"))
    get_running_loop().lead = myDevice
    async with AsyncDeviceContext(myDevice=myDevice) as ctx:
        yield ctx


@pytest.mark.timeout(30)
@run_test
async def test_find_interfaces(deviceTest):
    must_have = ["Motor", "Camera", "Processor", "DeviceInstantiator"]
    assert deviceTest["myDevice"].interfaces == must_have
    motors = listMotors()
    assert "MyDevice" in motors
    cameras = listCameras()
    assert "MyDevice" in cameras
    cameraName = listCameras("MYDEVICE")
    assert "MyDevice" in cameraName
    cameraName = listCameras("mydevice")
    assert "MyDevice" in cameraName
    triggers = listTriggers()
    assert "MyDevice" not in triggers
    processors = listProcessors()
    assert "MyDevice" in processors
    # Find rubbish
    motors = listMotors("deep")
    assert "MyDevice" not in motors
    instantiator = listDeviceInstantiators("mydevice")
    assert "MyDevice" in instantiator
