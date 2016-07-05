from asyncio import async, coroutine, get_event_loop, set_event_loop, sleep
from contextlib import closing
import gc
from itertools import count
import os
import sys
import time
from unittest import TestCase, main, skip
import weakref

from karabo.middlelayer_api.cli import (connectDevice, DeviceClient,
                                        start_device_client)
from karabo.middlelayer_api.device import Device
from karabo.middlelayer_api.device_client import (
    getDevice, instantiate, shutdown, getDevices, getServers)
from karabo.middlelayer_api.device_server import DeviceServer
from karabo.middlelayer_api.eventloop import NoEventLoop
from karabo.middlelayer_api.exceptions import KaraboError
from karabo.middlelayer_api.hash import Hash, Int32 as Int, Slot
from karabo.middlelayer_api.macro import Macro, EventThread, RemoteDevice

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
        conf = Hash(
            "count", Hash(),
            "visibility", 0,
            "log", Hash(),
            "Logger", Hash(
                "categories", [
                    Hash("Category", Hash(
                        "name", "karabo", "additivity", False,
                        "appenders", [
                            Hash("RollingFile", Hash(
                                "layout", Hash("Pattern", Hash(
                                    "format", "something")),
                                "filename", "some.log"))]))],
                "appenders", [
                    Hash("Ostream", Hash(
                        "layout", Hash(
                            "Pattern", Hash("format", "some format")))),
                    Hash("RollingFile", Hash(
                        "layout", Hash("Pattern", Hash(
                            "format", "other format")),
                        "filename", "other.log")),
                    Hash("Network", Hash())]))
        instantiate("testServer", "Other", "other",
                     configuration=conf, do_count=Hash())

    @Slot()
    def shutdown(self):
        shutdown("other")

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
        save = sys.argv
        try:
            sys.argv = ["", "count", "counter=7"]
            Remote.main()
        finally:
            sys.argv = save

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
        config = Hash("project", "test", "module", "test", "code", self.code)
        h = Hash("classId", "MetaMacro", "configuration", config,
                 "deviceId", "bla")
        yield from sleep(1.1)
        yield from server.call("Karabo_MacroServer", "slotStartDevice", h)
        yield from sleep(1.1)
        proxy = yield from getDevice("bla-TestMacro")
        with proxy:
            yield from proxy.do()
        return proxy

    def test_macroserver(self):
        loop = setEventLoop()
        with closing(loop):
            server = DeviceServer(dict(serverId="Karabo_MacroServer"))
            server.startInstance()
            task = loop.create_task(self.init_macroserver(server), server)
            proxy = loop.run_until_complete(task)
            self.assertEqual(proxy.s, "done")
            loop.create_task(server.slotKillServer(), server)
            loop.run_forever()

    @coroutine
    def init_server(self, server):
        """initialize the device server and a device on it

        the device on the device server is started from within a
        macro `remote`, such that we can test device instantiation
        from macros."""
        remote = Remote(_deviceId_="remote")
        yield from remote.startInstance()
        proxy = yield from getDevice("remote")
        yield from proxy.instantiate()
        yield from sleep(1.1)
        return (yield from getDevice("other"))

    @coroutine
    def shutdown_server(self):
        """shutdown the device started by `init_server`

        Again a macro is called to shut down the device on the server,
        to test device shutdown in macros"""
        proxy = yield from getDevice("remote")
        yield from proxy.shutdown()
        yield from sleep(0.02)

    @coroutine
    def check_server_topology(self):
        self.assertIn("testServer", getServers())
        self.assertIn("other", getDevices("testServer"))

    def test_server(self):
        """test the full lifetime of a python device server

        this test
          * starts a device client
          * starts a device server
          * starts a macro which tells said device server to start a device
          * checks that the other device shows up in the topology
          * let the macro tell to shut down that device
          * checks everything is cleaned up afterwards
          * kills the device server
        """
        loop = setEventLoop()

        with closing(loop):
            dc = DeviceClient(dict(_deviceId_="dc"))
            dc.startInstance()

            server = DeviceServer(dict(serverId="testServer"))
            server.startInstance()
            proxy = loop.run_until_complete(
                loop.create_task(self.init_server(server), server))
            # test that onInitialization was run properly
            self.assertEqual(proxy.something, 222)
            loop.run_until_complete(
                loop.create_task(self.check_server_topology(), dc))
            self.assertIn("other", server.deviceInstanceMap)
            r = weakref.ref(server.deviceInstanceMap["other"])
            with proxy:
                loop.run_until_complete(
                    loop.create_task(self.shutdown_server(), server))
                self.assertNotIn("other", server.deviceInstanceMap)
                gc.collect()
                self.assertIsNone(r())
                async(dc.slotKillDevice())
                loop.run_until_complete(server.slotKillServer())
                self.assertEqual(proxy.something, 111)
            loop.run_forever()

    @coroutine
    def init_other(self):
        self.other = Other(dict(_deviceId_="other", _serverId_="tserver"))
        yield from self.other.startInstance()

    def test_autodisconnect(self):
        """test the automatic disconnect after 15 s ATTENION! LONG TEST!

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
        self.assertNotIn("other", getDevices("tserver"))
        yield from other.startInstance()
        yield from sleep(0.1)
        self.assertIn("other", getDevices())
        self.assertIn("other", getDevices("tserver"))
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
        try:
            with self.assertLogs(level="WARNING"):
                thread = start_device_client()
        except AssertionError:
            pass
        else:
            self.fail("no log should be generated!")
        finally:
            thread.stop()
            thread.join(0.1)
            self.assertFalse(thread.is_alive())

if __name__ == "__main__":
    main()
