import karabo
karabo.api_version = 2

from asyncio import coroutine, sleep
import gc
from unittest import TestCase, main
import sys
import time
import weakref

from karabo.macro import Macro
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
        del remote
        gc.collect()
        self.assertIsNone(r())

    def test_main(self):
        save = sys.argv
        try:
            sys.argv = ["", "count", "counter=7"]
            Remote.main()
        finally:
            sys.argv = save

    code = """if True:
        from time import sleep
        from karabo.macro import Macro
        from karabo.hashtypes import Slot, String

        class TestMacro(Macro):
            s = String()

            @Slot()
            def do(self):
                self.s = "done"
                sleep(1)
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
            yield from sleep(0.1)
        return proxy


    def test_macroserver(self):
        loop = setEventLoop()
        server = DeviceServer(dict(serverId="macroServer"))
        task = loop.create_task(self.init_macroserver(server), server)
        proxy = loop.run_until_complete(task)
        self.assertEqual(proxy.s, "done")


if __name__ == "__main__":
    main()
