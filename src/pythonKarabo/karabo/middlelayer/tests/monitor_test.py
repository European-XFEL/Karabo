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
import time
from asyncio import Future, ensure_future, sleep

import pytest
import pytest_asyncio

from karabo.middlelayer.device import Device
from karabo.middlelayer.macro import Macro, Monitor, RemoteDevice
from karabo.middlelayer.testing import AsyncDeviceContext, run_test, sleepUntil
from karabo.native import Int32 as Int, Slot


class Remote(Device):
    counter = Int(defaultValue=-1)

    @Slot()
    async def count(self):
        ensure_future(self.do_count())

    async def do_count(self):
        for i in range(1, 30):
            self.counter = i
            await sleep(0.1)


class LocalRemoteDevice(Macro):
    # The timeout has to be large for the test, because
    # we want to cancel the connection
    notthere = RemoteDevice("notmonitorthere", timeout=30)

    def __init__(self, config):
        super().__init__(config)
        self.run_future = Future()

    async def _run(self, **kwargs):
        self.run_future.set_result(None)
        await super()._run(**kwargs)


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


@pytest_asyncio.fixture(scope="module")
@pytest.mark.asyncio
async def deviceTest():
    local = Local(_deviceId_="local", project="test", module="test",
                  may_start_thread=False)
    remA = Remote(dict(_deviceId_="remA"))
    remB = Remote(dict(_deviceId_="remB"))
    async with AsyncDeviceContext(local=local, remA=remA, remB=remB) as ctx:
        yield ctx


@pytest.mark.timeout(30)
@run_test
def test_count(deviceTest):
    local = deviceTest["local"]
    local.startA()
    local.startB()
    remA = deviceTest["remA"]
    remB = deviceTest["remB"]

    time.sleep(0.3)
    for _ in range(30):
        sleepUntil(lambda: local.division == remA.counter // remB.counter,
                   timeout=0.5)
        time.sleep(0.1)


@pytest.mark.timeout(30)
@run_test
def test_error(deviceTest, caplog):
    local = deviceTest["local"]
    with caplog.at_level("ERROR", logger="local"):
        local.error()
    assert len(caplog.records)


@pytest.mark.timeout(30)
@run_test
async def test_quick_shutdown():
    """Test that a quick instantiation and shutdown works"""
    device = LocalRemoteDevice(dict(_deviceId_="localWithRemote"))
    ensure_future(device.startInstance())
    await device.run_future
    # Use sleep to make sure we are trying to connect to remote
    # but only sleep a bit
    await sleep(0.5)
    assert not device.is_initialized
    await device.slotKillDevice()
    assert not device.is_initialized
