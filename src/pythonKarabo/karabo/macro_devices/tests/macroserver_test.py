from asyncio import coroutine
from contextlib import closing
from unittest import TestCase, main
import uuid

from karabo.middlelayer import getDevice
from karabo.middlelayer_api.tests.eventloop import setEventLoop
from karabo.native import Hash

from ..macro_server import MacroServer


class Tests(TestCase):
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
            self.assertEqual(proxy.s.value, 'done')
        return proxy

    def test_macroserver(self):
        loop = setEventLoop()
        with closing(loop):
            server = MacroServer(dict(serverId="Karabo_MacroServer"))
            loop.run_until_complete(server.startInstance())
            task = loop.create_task(self.init_macroserver(server), server)
            proxy = loop.run_until_complete(task)
            self.assertEqual(proxy.s, "done")
            loop.create_task(server.slotKillServer(), server)
            loop.run_forever()


if __name__ == "__main__":
    main()
