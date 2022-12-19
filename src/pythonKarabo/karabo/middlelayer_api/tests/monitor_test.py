import time
from asyncio import ensure_future, sleep

import pytest

from karabo.middlelayer.testing import assertLogs, run_test
from karabo.middlelayer_api.broker import amqp, redis
from karabo.middlelayer_api.device import Device
from karabo.middlelayer_api.macro import Macro, Monitor, RemoteDevice
from karabo.middlelayer_api.tests.eventloop import (
    AsyncDeviceContext, event_loop)
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
    notthere = RemoteDevice("notthere", timeout=5)


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


@pytest.fixture(scope="module")
@pytest.mark.asyncio
async def deviceTest(event_loop: event_loop):
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

    if amqp or redis:
        time.sleep(1.0)
    else:
        time.sleep(0.3)
    for _ in range(30):
        assert local.division == remA.counter // remB.counter
        time.sleep(0.1)


@pytest.mark.timeout(30)
@run_test
def test_error(deviceTest):
    local = deviceTest["local"]
    with assertLogs("local", "ERROR"):
        local.error()


@pytest.mark.timeout(30)
@run_test
async def test_quick_shutdown(deviceTest):
    """Test that a quick instantiation and shutdown works"""
    device = LocalRemoteDevice(dict(_deviceId_="localWithRemote"))
    ensure_future(device.startInstance())
    # Use sleep to make sure we are trying to connect to remote
    await sleep(1)
    assert not device.is_initialized
    await device.slotKillDevice()
    assert not device.is_initialized
