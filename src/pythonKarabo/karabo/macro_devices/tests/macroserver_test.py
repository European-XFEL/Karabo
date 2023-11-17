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
import uuid
from asyncio import get_event_loop, set_event_loop
from contextlib import ExitStack, contextmanager
from unittest import TestCase, main

from karabo.middlelayer import getDevice, updateDevice
from karabo.middlelayer.broker import jms
from karabo.middlelayer.signalslot import SignalSlotable
from karabo.middlelayer.tests.eventloop import EventLoop
from karabo.native import Hash

from ..macro_server import MacroServer

TEST_MACROSERVER = "Karabo_TestMacroServer"

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


class Tests(TestCase):

    @classmethod
    @contextmanager
    def createComm(cls):

        loop = cls.loop
        sigslot = SignalSlotable(
            {"_deviceId_": f"test-mdlserver-{uuid.uuid4()}"})
        sigslot.startInstance(loop=loop)
        cls.lead = sigslot

        loop.instance = lambda: sigslot
        yield
        loop.run_until_complete(sigslot.slotKillDevice())

    @classmethod
    def setUpClass(cls):
        with ExitStack() as cls.exit_stack:
            cls.loop = EventLoop()
            cls.old_event_loop = get_event_loop()
            cls.exit_stack.enter_context(cls.createComm())
            set_event_loop(cls.loop)
            cls.exit_stack = cls.exit_stack.pop_all()

    @classmethod
    def tearDownClass(cls):
        try:
            with cls.exit_stack:
                pass
        finally:
            del cls.lead
            cls.loop.close()
            set_event_loop(cls.old_event_loop)

    async def init_macro(self, code, expected, server):
        # allow the server to start
        uuid_ = str(uuid.uuid4())
        config = Hash("uuid", uuid_, "module", "test",
                      "code", code)
        h = Hash("classId", "MetaMacro", "configuration", config,
                 "deviceId", f"bla-{uuid_}")
        await server.call(TEST_MACROSERVER, "slotStartDevice", h)
        proxy = await getDevice(f"bla-{uuid_}")
        with proxy:
            if not jms:
                await updateDevice(proxy)
            await proxy.do()
            self.assertEqual(proxy.s.value, expected)
        await server.call(proxy.deviceId, "slotKillDevice")

    def test_macroserver(self):
        loop = get_event_loop()
        server = MacroServer(dict(serverId=TEST_MACROSERVER))
        loop.run_until_complete(server.startInstance())

        task = loop.create_task(
            self.init_macro(SYNC_CODE, "sync", server), server)
        loop.run_until_complete(task)

        task = loop.create_task(
            self.init_macro(ASYNC_CODE, "async", server), server)
        loop.run_until_complete(task)


if __name__ == "__main__":
    main()
