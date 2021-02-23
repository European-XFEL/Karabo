from contextlib import closing
from unittest import TestCase, main
import uuid

from karabo.middlelayer import call, getDevice, sleep
from karabo.middlelayer_api.tests.eventloop import setEventLoop
from karabo.native import Hash

from ..macro_server import MacroServer


TEST_MACROSERVER="Karabo_TestMacroServer"

SYNC_CODE = """from karabo.middlelayer import *

class TestMacro(Macro):
    s = String()

    @Slot()
    def do(self):
        self.s = "sync"
"""

ASYNC_CODE = """from karabo.middlelayer import *

class TestMacro(Macro):
    s = String()

    @Slot()
    async def do(self):
        self.s = "async"
"""

CORO_CODE = """from asyncio import coroutine
from karabo.middlelayer import *

class TestMacro(Macro):
    s = String()

    @Slot()
    @coroutine
    def do(self):
        self.s = "coro"
"""

class Tests(TestCase):

    async def init_macro(self, code, expected, server):
        # allow the server to start
        uuid_ = str(uuid.uuid4())
        config = Hash("uuid", uuid_, "module", "test",
                      "code", code)
        h = Hash("classId", "MetaMacro", "configuration", config,
                 "deviceId", f"bla-{uuid_}")
        await server.call(TEST_MACROSERVER, "slotStartDevice", h)
        proxy = await getDevice(f"bla-{uuid_}-TestMacro")
        with proxy:
            await proxy.do()
            self.assertEqual(proxy.s.value, expected)
        await server.call(proxy.deviceId, "slotKillDevice")

    def test_macroserver(self):
        loop = setEventLoop()
        with closing(loop):
            server = MacroServer(dict(serverId=TEST_MACROSERVER))
            loop.run_until_complete(server.startInstance())

            task = loop.create_task(
                self.init_macro(SYNC_CODE, "sync", server), server)
            loop.run_until_complete(task)

            task = loop.create_task(
                self.init_macro(ASYNC_CODE, "async", server), server)
            loop.run_until_complete(task)

            task = loop.create_task(
                self.init_macro(CORO_CODE, "coro", server), server)
            loop.run_until_complete(task)


if __name__ == "__main__":
    main()
