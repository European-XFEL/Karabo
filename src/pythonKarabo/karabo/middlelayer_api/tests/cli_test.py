from asyncio import (
    async, coroutine, get_event_loop, set_event_loop, sleep, wait_for)
from contextlib import closing
import gc
from itertools import count
import os
import socket
import time
from unittest import TestCase, main, skip
import uuid
import weakref

from karabo.middlelayer_api.device import Device
from karabo.middlelayer_api.device_client import (
    getDevice, findDevices, shutdown, getClients, getDevices)
from karabo.middlelayer_api.device_server import DeviceServer
from karabo.middlelayer_api.eventloop import NoEventLoop
from karabo.middlelayer_api.exceptions import KaraboError
from karabo.middlelayer_api.hash import Hash, Int32 as Int, Slot
from karabo.middlelayer_api.ikarabo import (
    connectDevice, DeviceClient, start_device_client)
from karabo.middlelayer_api.macro import Macro, EventThread, RemoteDevice

from .eventloop import setEventLoop


class Remote(Macro):
    counter = Int(defaultValue=-1)

    # avoid that test_main gets stuck. Raise a timeout error instead.
    @coroutine
    def _run(self, *args, **kwargs):
        return (yield from wait_for(super()._run(*args, **kwargs), timeout=5))

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

    @coroutine
    def do_count(self):
        for i in count():
            self.counter = i
            yield from sleep(0.1)

    @Slot()
    @coroutine
    def count(self):
        async(self.do_count())

    @coroutine
    def onInitialization(self):
        self.myself = yield from getDevice("other")
        self.myself.something = 222

    @coroutine
    def onDestruction(self):
        with self.myself:
            self.myself.something = 111
            yield from sleep(0.02)


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
        try:
            os.remove("serverId.xml")
        except FileNotFoundError:
            pass
        os.chdir(self.__starting_dir)

    def test_delete(self):
        thread = EventThread()
        thread.start()
        try:
            remote = Remote()
            remote.count()
            self.assertEqual(remote.counter, 29)
            r = weakref.ref(remote)
            Remote.destructed = False
            del remote
            time.sleep(0.02)
            gc.collect()
            time.sleep(0.02)
            self.assertIsNone(r())
            self.assertTrue(Remote.destructed)
        finally:
            thread.stop()
            thread.join(0.1)
            self.assertFalse(thread.is_alive())

    @skip
    def test_remote_timeout(self):
        with self.assertLogs("NoRemote"):
            NoRemote(_deviceId_="NoRemote")

    def test_main(self):
        Remote.main(["", "count", "counter=7",
                     "device=Remote_{}_{}".format(
                        socket.gethostname().split(".")[0], os.getpid())])

    code = """if True:
        from karabo.middlelayer import *

        class TestMacro(Macro):
            s = String()

            @Slot()
            def do(self):
                self.s = "done"
    """

    @coroutine
    def init_macroserver(self, server):
        config = Hash("uuid", str(uuid.uuid4()), "module", "test",
                      "code", self.code)
        h = Hash("classId", "MetaMacro", "configuration", config,
                 "deviceId", "bla")
        yield from server.call("Karabo_MacroServer", "slotStartDevice", h)
        proxy = yield from getDevice("bla-TestMacro")
        with proxy:
            yield from proxy.do()
        return proxy

    def test_macroserver(self):
        loop = setEventLoop()
        with closing(loop):
            server = DeviceServer(dict(serverId="Karabo_MacroServer"))
            loop.run_until_complete(server.startInstance())
            task = loop.create_task(self.init_macroserver(server), server)
            proxy = loop.run_until_complete(task)
            self.assertEqual(proxy.s, "done")
            loop.create_task(server.slotKillServer(), server)
            loop.run_forever()

    @coroutine
    def init_other(self):
        self.other = Other(dict(_deviceId_="other", _serverId_="tserver"))
        yield from self.other.startInstance()

    def test_autodisconnect(self):
        """test the automatic disconnect after 15 s ATTENTION! LONG TEST!

        this tests that a device connected to by connectDevice automatically
        disconnects again after 15 s. Unfortunately this means the test
        needs to run very long..."""
        thread = EventThread()
        thread.start()
        try:
            devices = DeviceClient(_deviceId_="ikarabo-test")
            oel = get_event_loop()
            set_event_loop(NoEventLoop(devices))
            time.sleep(0.1)
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
        finally:
            thread.stop()
            thread.join(0.1)
            self.assertFalse(thread.is_alive())
    test_autodisconnect.slow = 1

    @coroutine
    def init_topo(self, dc):
        other = Other(dict(_deviceId_="other", _serverId_="tserver"))
        self.assertNotIn("other", getDevices())
        self.assertNotIn("other", getClients())
        self.assertNotIn("other", getDevices("tserver"))
        yield from other.startInstance()
        yield from sleep(0.1)
        self.assertIn("other", getDevices())
        self.assertIn("other", getDevices("tserver"))
        self.assertNotIn("other", findDevices("beep"))
        self.assertIn("other", findDevices("other"))
        self.assertNotIn("other", getDevices("bserver"))

        double = Other(dict(_deviceId_="other", _serverId_="bserver"))
        with self.assertRaises(KaraboError):
            yield from double.startInstance()
        self.assertIn("other", getDevices())
        self.assertIn("other", getDevices("tserver"))
        self.assertNotIn("other", getDevices("bserver"))

        yield from shutdown("other")
        self.assertNotIn("other", getDevices())

    def test_topology(self):
        loop = setEventLoop()
        with closing(loop):
            dc = DeviceClient(dict(_deviceId_="dc"))
            dc.startInstance()
            task = loop.create_task(self.init_topo(dc), dc)
            loop.run_until_complete(task)
            loop.run_until_complete(dc.slotKillDevice())

    def test_ikarabo(self):
        thread = None
        try:
            with self.assertLogs(level="WARNING"):
                thread = start_device_client()
        except AssertionError:
            pass
        else:
            self.fail("no log should be generated!")
        finally:
            if thread is not None:
                thread.stop()
                thread.join(0.1)
                self.assertFalse(thread.is_alive())


if __name__ == "__main__":
    main()
