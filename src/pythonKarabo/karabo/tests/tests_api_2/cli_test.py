from asyncio import async, coroutine, get_event_loop, set_event_loop, sleep
import gc
from itertools import count
import os
import sys
import time
from unittest import TestCase, main, skip
import weakref

from karabo.api import Int, Slot
from karabo.cli import connectDevice, DeviceClient
from karabo.device_client import (
    getDevice, instantiate, DeviceClientBase, getDevices)
from karabo.eventloop import NoEventLoop
from karabo.hash import Hash
from karabo.macro import Macro, EventThread, RemoteDevice
from karabo.python_device import Device
from karabo.python_server import DeviceServer

from .eventloop import setEventLoop


class Remote(Macro):
    counter = Int(defaultValue=-1)

    @Slot()
    def count(self):
        for i in range(30):
            self.counter = i
            time.sleep(0.1)

    @Slot()
    def instantiate(self):
        instantiate("testServer", "Other", "other")


class NoRemote(Macro):
    rd = RemoteDevice("DoesNotExist")


class Other(Device):
    something = Int(defaultValue=333)
    counter = Int(defaultValue=-1)

    @coroutine
    def do_count(self):
        for i in count():
            self.counter = i
            yield from sleep(0.1)

    @Slot()
    @coroutine
    def count(self):
        async(self.do_count())


class Tests(TestCase):

    def setUp(self):
        # Change the current directory to the directory containing this test.
        # DeviceServer looks for a file named serverId.xml in the current
        # directory, so we should use the one that comes with the unit tests
        # to avoid creating a new one in a random part of the user's filesystem
        self.__starting_dir = os.curdir
        this_dir = os.path.dirname(os.path.abspath(__file__))
        os.chdir(this_dir)

    def tearDown(self):
        os.chdir(self.__starting_dir)

    def test_delete(self):
        remote = Remote()
        remote.count()
        self.assertEqual(remote.counter, 29)
        r = weakref.ref(remote)
        thread = EventThread.instance().thread
        del remote
        time.sleep(0.1)
        gc.collect()
        self.assertIsNone(r())
        self.assertIsNone(EventThread.instance())
        time.sleep(0.1)
        self.assertFalse(thread.is_alive())

    @skip
    def test_remote_timeout(self):
        with self.assertLogs("NoRemote"):
            remote = NoRemote(_deviceId_="NoRemote")

    def test_main(self):
        save = sys.argv
        try:
            sys.argv = ["", "count", "counter=7"]
            Remote.main()
        finally:
            sys.argv = save

    code = """if True:
        from karabo.api import *

        class TestMacro(Macro):
            s = String()

            @Slot()
            def do(self):
                self.s = "done"
    """

    @coroutine
    def init_macroserver(self, server):
        yield from server.startInstance()
        config = Hash("project", "test", "module", "test", "code", self.code)
        h = Hash("classId", "MetaMacro", "configuration", config,
                 "deviceId", "bla")
        yield from server.call("Karabo_MacroServer", "slotStartDevice", h)
        yield from sleep(4)
        proxy = yield from getDevice("bla-TestMacro")
        with proxy:
            yield from proxy.do()
        return proxy


    def test_macroserver(self):
        loop = setEventLoop()
        server = DeviceServer(dict(serverId="Karabo_MacroServer"))
        task = loop.create_task(self.init_macroserver(server), server)
        proxy = loop.run_until_complete(task)
        self.assertEqual(proxy.s, "done")
        async(server.slotKillServer())
        loop.run_forever()
        loop.close()

    @coroutine
    def init_server(self, server):
        yield from server.startInstance()
        remote = Remote(_deviceId_="remote")
        yield from remote.startInstance()
        proxy = yield from getDevice("remote")
        yield from proxy.instantiate()
        yield from sleep(4)
        return (yield from getDevice("other"))

    def test_server(self):
        loop = setEventLoop()
        server = DeviceServer(dict(serverId="testServer"))
        task = loop.create_task(self.init_server(server), server)
        proxy = loop.run_until_complete(task)
        self.assertEqual(proxy.something, 333)
        async(server.slotKillServer())
        loop.run_forever()
        loop.close()

    @coroutine
    def init_other(self):
        self.other = Other(dict(_deviceId_="other", _serverId_="tserver"))
        yield from self.other.startInstance()

    def test_autodisconnect(self):
        devices = DeviceClient(_deviceId_="ikarabo-test")
        oel = get_event_loop()
        set_event_loop(NoEventLoop(devices))
        time.sleep(0.1)
        thread = EventThread.instance().thread
        thread.loop.call_soon_threadsafe(async, self.init_other())
        time.sleep(4)

        # test whether proxy stays alive while used
        other = connectDevice("other")
        other.count()
        lastcount = -2
        for i in range(20):
            self.assertLess(lastcount, other.counter)
            lastcount = other.counter
            time.sleep(1)

        # test proxy disconnects when not used
        time.sleep(16)
        lastcount = other.__dict__["counter"]
        time.sleep(3)
        self.assertEqual(other.__dict__["counter"], lastcount)
        self.assertNotEqual(other.counter, lastcount)

        set_event_loop(oel)
        del self.other
        thread.stop()
        EventThread.instance = None

    @coroutine
    def init_topo(self, dc):
        yield from dc.startInstance()
        other = Other(dict(_deviceId_="other", _serverId_="tserver"))
        self.assertNotIn("other", getDevices())
        self.assertNotIn("other", getDevices("tserver"))
        yield from other.startInstance()
        yield from sleep(1)
        self.assertIn("other", getDevices())
        self.assertIn("other", getDevices("tserver"))
        self.assertNotIn("other", getDevices("bserver"))

    def test_topology(self):
        loop = setEventLoop()
        dc = DeviceClient(dict(_deviceId_="dc"))
        task = loop.create_task(self.init_topo(dc), dc)
        loop.run_until_complete(task)
        loop.close()

if __name__ == "__main__":
    main()
