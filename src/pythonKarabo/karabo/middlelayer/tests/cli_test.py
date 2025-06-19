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
import gc
import time
import weakref
from asyncio import (
    ensure_future, get_event_loop, new_event_loop, set_event_loop, sleep,
    wait_for)
from contextlib import closing
from itertools import count

import pytest

from karabo.common.states import State
from karabo.middlelayer.device import Device
from karabo.middlelayer.device_client import (
    callNoWait, findDevices, getClients, getDevice, getDevices, shutdown)
from karabo.middlelayer.eventloop import NoEventLoop, global_sync
from karabo.middlelayer.ikarabo import (
    DeviceClient, connectDevice, start_device_client)
from karabo.middlelayer.macro import EventThread, Macro, RemoteDevice
from karabo.middlelayer.testing import setEventLoop, sleepUntil
from karabo.native import Int32 as Int, KaraboError, Slot


class Remote(Macro):
    counter = Int(defaultValue=-1)

    # avoid that test_main gets stuck. Raise a timeout error instead.
    async def _run(self, *args, **kwargs):
        return (await wait_for(super()._run(*args, **kwargs), timeout=5))

    @Slot()
    def count(self):
        for i in range(30):
            self.counter = i
            time.sleep(0.1)

    def onDestruction(self):
        Remote.destructed = True


class NoRemote(Macro):
    rd = RemoteDevice("DoesNotExist")


class Other(Device):
    something = Int(defaultValue=333)
    counter = Int(defaultValue=-1)
    task = None

    async def do_count(self):
        try:
            for i in count():
                self.counter = i
                await sleep(0.1)
        finally:
            self.state = State.ON

    @Slot()
    async def count(self):
        self.state = State.PROCESSING
        self.task = ensure_future(self.do_count())

    @Slot()
    async def stopCount(self):
        if self.task is not None:
            self.task.cancel()

    async def onInitialization(self):
        self.myself = await getDevice("other")
        self.myself.something = 222
        self.state = State.ON

    async def onDestruction(self):
        with self.myself:
            self.myself.something = 111
            await sleep(0.02)


def sleepSync(condition, timeout=1):
    """A synchronously sleeping block"""
    total = timeout
    interval = 0.05
    while not condition():
        time.sleep(interval)
        total -= interval
        if total < 0:
            break


@pytest.mark.timeout(30)
def test_delete():
    # Set a loop in the main thread for testing
    loop = new_event_loop()
    set_event_loop(loop)

    thread = EventThread()
    thread.start()

    gc_time = 0.05
    try:
        remote = Remote()
        remote.count()
        assert remote.counter == 29
        r = weakref.ref(remote)
        Remote.destructed = False
        del remote
        time.sleep(gc_time)
        gc.collect()
        time.sleep(gc_time)
        assert r() is None
        sleepSync(lambda: Remote.destructed)
        assert Remote.destructed
    finally:
        thread.stop()
        thread.join(1.0)
        assert not thread.is_alive()


@pytest.mark.timeout(30)
def test_main():
    Remote.main(["", "count", "counter=7"])


@pytest.mark.timeout(30)
def test_remote_timeout(caplog):
    """Test that remote devices can timeout and send logs"""
    thread = None
    try:
        # start a thread with event loop and then try
        # to run the device
        assert not global_sync()
        thread = start_device_client()
        assert global_sync()
        with caplog.at_level("INFO", logger="NoRemote"):
            NoRemote(_deviceId_="NoRemote")
            assert len(caplog.records)
    finally:
        if thread is not None:
            thread.stop()
            thread.join(0.5)
            assert not thread.is_alive()


@pytest.mark.timeout(60)
def test_autodisconnect():
    """test the automatic disconnect after 15 s ATTENTION! LONG TEST!

    this tests that a device connected to by connectDevice automatically
    disconnects again after 15 s. Unfortunately this means the test
    needs to run very long..."""
    thread = EventThread()
    thread.start()
    try:
        devices = DeviceClient(_deviceId_="ikarabo-test")
        assert devices.deviceId == "ikarabo-test"
        oel = get_event_loop()
        set_event_loop(NoEventLoop(devices))
        time.sleep(0.1)

        other = None

        async def init_other():
            nonlocal other
            other = Other(dict(_deviceId_="other", serverId="tserver"))
            assert other.deviceId == "other"
            await other.startInstance()

        thread.loop.call_soon_threadsafe(ensure_future, init_other())
        time.sleep(4)

        # test whether proxy stays alive while used
        other = connectDevice("other")
        assert other.state == State.ON
        other.count()
        lastcount = -2
        for i in range(20):
            assert other.state == State.PROCESSING
            assert lastcount < other.counter
            lastcount = other.counter
            time.sleep(1)

        # test proxy disconnects when not used
        time.sleep(16)
        # but call to stop counting, we don't receive the state
        # update as well
        callNoWait("other", "stopCount")
        lastcount = other.__dict__["counter"]
        laststate = other.__dict__["state"]
        time.sleep(3)
        assert other.__dict__["counter"] == lastcount
        assert laststate == State.PROCESSING
        # reconnect with fresh state and counter
        assert other.counter != lastcount
        assert other.state == State.ON

        set_event_loop(oel)
        del other
    finally:
        thread.stop()
        thread.join(5)
        assert not thread.is_alive()


@pytest.mark.timeout(30)
def test_topology():
    loop = setEventLoop()
    with closing(loop):
        dc = DeviceClient(dict(_deviceId_="ikarabo-test2"))
        loop.run_until_complete(dc.startInstance())

        async def init_topo(dc):
            other = Other(dict(_deviceId_="other", serverId="tserver"))
            assert "other" not in getDevices()
            assert "other" not in getClients()
            assert "other" not in getDevices("tserver")
            await other.startInstance()

            def other_online():
                return "other" in getDevices()

            await sleepUntil(other_online)
            assert "other" in getDevices()
            assert "other" in getDevices("tserver")
            assert "other" not in findDevices("beep")
            assert "other" in findDevices("other")
            assert "other" in findDevices("OTHER")
            assert "other" in findDevices("OT")
            assert "other" not in getDevices("bserver")

            double = Other(dict(_deviceId_="other", serverId="bserver"))
            with pytest.raises(KaraboError):
                await double.startInstance()
            assert "other" in getDevices()
            assert "other" in getDevices("tserver")
            assert "other" not in getDevices("bserver")
            await shutdown("other")
            assert "other" not in getDevices()

        task = loop.create_task(init_topo(dc), dc)
        loop.run_until_complete(task)
        loop.run_until_complete(dc.slotKillDevice())


@pytest.mark.timeout(30)
def test_ikarabo(caplog):
    thread = None
    try:
        with caplog.at_level("WARNING"):
            thread = start_device_client()
            assert caplog.text == "", "no log should be generated!"
    finally:
        if thread is not None:
            thread.stop()
            thread.join(0.5)
            assert not thread.is_alive()
