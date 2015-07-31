import karabo

from asyncio import async, coroutine, sleep
import gc
from unittest import TestCase, main
import sys
import time
import weakref

from karabo.api import Int, Slot
from karabo.macro import Macro, EventThread
from karabo.hash import Hash
from karabo.python_server import DeviceServer
from karabo.device_client import (
    getDevice, instantiate, DeviceClientBase, getDevices)
from karabo.python_device import Device

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


class Other(Device):
    something = Int(defaultValue=333)


class DeviceClient(Macro, DeviceClientBase):
    pass


class Tests(TestCase):
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
