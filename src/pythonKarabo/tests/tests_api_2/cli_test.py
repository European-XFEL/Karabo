import karabo
from asyncio import async, coroutine, sleep
import gc
from unittest import TestCase, main
import sys
import time
import weakref

from karabo.macro import Macro, EventThread
from karabo import Integer, Slot
from karabo.eventloop import EventLoop
from karabo.hash import Hash
from karabo.python_server import DeviceServer
from karabo.device_client import getDevice

from .eventloop import setEventLoop


class Remote(Macro):
    counter = Integer(defaultValue=-1)

    @Slot()
    def count(self):
        for i in range(30):
            self.counter = i
            time.sleep(0.1)


class Tests(TestCase):
    def test_delete(self):
        remote = Remote()
        remote.count()
        self.assertEqual(remote.counter, 29)
        r = weakref.ref(remote)
        thread = EventThread.instance().thread
        del remote
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
        from karabo.macro import Macro
        from karabo.hashtypes import Slot, String

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
        hash = Hash("classId", "MetaMacro", "configuration", config,
                    "deviceId", "bla")
        yield from server.call("macroServer", "slotStartDevice", hash)
        yield from sleep(4)
        proxy = yield from getDevice("bla-TestMacro")
        with proxy:
            yield from proxy.do()
        return proxy


    def test_macroserver(self):
        loop = setEventLoop()
        server = DeviceServer(dict(serverId="macroServer"))
        task = loop.create_task(self.init_macroserver(server), server)
        proxy = loop.run_until_complete(task)
        self.assertEqual(proxy.s, "done")
        async(server.slotKillServer())
        loop.run_forever()
        loop.close()


if __name__ == "__main__":
    main()
