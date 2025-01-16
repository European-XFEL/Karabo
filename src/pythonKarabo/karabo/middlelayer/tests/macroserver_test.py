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

import pytest

from karabo.middlelayer import call, getDevice, updateDevice
from karabo.middlelayer.testing import AsyncDeviceContext
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


async def init_macro(code, expected):
    uuid_ = str(uuid.uuid4())
    config = Hash("uuid", uuid_, "module", "test",
                  "code", code)
    h = Hash("classId", "MetaMacro", "configuration", config,
             "deviceId", f"bla-{uuid_}")
    assert h["configuration"] == config
    await call(TEST_MACROSERVER, "slotStartDevice", h)
    proxy = await getDevice(f"bla-{uuid_}")
    with proxy:
        await updateDevice(proxy)
        await proxy.do()
        assert proxy.s.value == expected
    await call(proxy.deviceId, "slotKillDevice")


@pytest.mark.timeout(30)
@pytest.mark.asyncio
async def test_macroserver():
    server = MacroServer(dict(serverId=TEST_MACROSERVER))
    async with AsyncDeviceContext(server=server):
        await init_macro(SYNC_CODE, "sync")
        await init_macro(ASYNC_CODE, "async")
