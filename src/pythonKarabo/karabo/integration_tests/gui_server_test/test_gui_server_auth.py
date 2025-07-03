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
import json

import pytest
import pytest_asyncio

from karabo.middlelayer import AccessLevel, Hash, State
from karabo.middlelayer.testing import (
    AsyncServerContext, assert_wait_property, sleepUntil)

from .adapter import GuiAdapter

VALID_TEST_TOKEN = "01234567-89ab-cdef-0123-456789abcdef"
VALID_ACCESS_LEVEL = AccessLevel.OPERATOR
INVALID_TOKEN_MSG = "Invalid one-time token!"
VALID_USER_ID = "Bob"

TEST_GUI_SERVER_ID = "guiServerAuthTest_auth"
TEST_SERVER_ID = "karabo_guiServerAuth_test_auth"


@pytest_asyncio.fixture(loop_scope="module", scope="module")
async def guiServerAuthAuth():
    serverId = TEST_SERVER_ID
    config = {
        TEST_GUI_SERVER_ID: {
            "classId": "guiServerAuthDevice", "port": 44450, "timeout": 1,
        }
    }
    init = json.dumps(config)
    server = AsyncServerContext(serverId, [f"init={init}"], api="cpp")
    async with server:
        await assert_wait_property(TEST_GUI_SERVER_ID, "state",
                                   State.ON, timeout=10)
        adapter = GuiAdapter(host="localhost", port=44450)
        await adapter.connect()
        yield adapter
        await adapter.disconnect()


@pytest.mark.skip
@pytest.mark.timeout(30)
@pytest.mark.asyncio(loop_scope="module")
async def test_missing_token_on_login(guiServerAuth):
    login_info = Hash("type", "login",
                      "clientId", "bobHost(pid 10264)",
                      "version", "2.20.0")

    await guiServerAuth.login()
    await guiServerAuth.reset()

    # Expect loginInformation message
    msg = await guiServerAuth.get_next("loginInformation")
    guiServerAuth.tcpWriteHash(login_info)

    login_response = await msg
    assert login_response["readOnly"] is True
    assert login_response["accessLevel"] == 1  # OBSERVER

    # Try to start temporary session → should fail
    begin_temp_session = Hash(
        "type", "beginTemporarySession",
        "temporarySessionToken", VALID_TEST_TOKEN,
        "version", "2.20.0",
        "levelBeforeTemporarySession", 2)  # OPERATOR

    notification = await guiServerAuth.get_next("notification")
    guiServerAuth.tcpWriteHash(begin_temp_session)

    msg = await notification
    assert "cannot be started from a readOnly" in msg["message"]

    # Wait for disconnect
    await sleepUntil(lambda: guiServerAuth.connected is False)
