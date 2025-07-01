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

from karabo.middlelayer import Hash, State, call, getProperties, setWait
from karabo.middlelayer.testing import AsyncServerContext, assert_wait_property

from .adapter import GuiAdapter

TEST_GUI_SERVER_ID = "guiServerTest"


@pytest_asyncio.fixture(loop_scope="module", scope="module")
async def guiServer():
    serverId = "karabo_guiserver_test"
    config = {
        TEST_GUI_SERVER_ID: {
            "classId": "GuiServerDevice", "port": 44450
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


@pytest.mark.timeout(30)
@pytest.mark.asyncio(loop_scope="module")
async def test_gui_server_execute_before_login(guiServer):
    blockedTypes = {"execute", "reconfigure", "getDeviceConfiguration",
                    "getDeviceSchema", "getClassSchema", "initDevice",
                    "killServer", "killDevice", "startMonitoringDevice",
                    "stopMonitoringDevice", "getPropertyHistory",
                    "getConfigurationFromPast", "subscribeNetwork",
                    "requestNetwork", "error", "requestGeneric"}
    for msg_type in blockedTypes:
        h = Hash("type", msg_type)
        guiServer.tcpWriteHash(h)
        msg = await guiServer.get_next("notification")
        message = msg["message"]
        assert message == f"Action '{msg_type}' refused before log in"


@pytest.mark.timeout(30)
@pytest.mark.asyncio(loop_scope="module")
async def test_gui_server_execute(guiServer):
    await guiServer.login()
    h = await call(TEST_GUI_SERVER_ID, "slotGetClientSessions",
                   Hash("dummy", ""))
    sessions = h["clientSessions"]
    assert len(sessions) == 1, (
        "Number of client connections differs from expected")
    session = sessions[0]

    assert session["clientVersion"] != "", (
        "Client version should not be empty")
    assert session["sessionToken"] == "", (
        "A session token is not expected (empty sessionToken)")
    assert session["sessionStartTime"] != "", (
        "A session start time is expected (non-empty sessionStartTime)")
    assert session["temporarySessionToken"] == "", (
        "No temporary session is expected (empty temporarySessionToken)")
    assert session["temporarySessionStartTime"] == "", (
        "No temporary session is expected (empty temporarySessionStartTime)")

    # Request execution of non existing device
    h = Hash("type", "execute",
             "deviceId", "not_there",
             "command", "does.not.matter",
             "reply", True,
             "timeout", 1)
    guiServer.tcpWriteHash(h)
    msg = await guiServer.get_next("executeReply")
    assert msg["type"] == "executeReply"
    input_hash = msg["input"]
    assert input_hash.fullyEqual(h)
    assert not msg["success"]
    # XXX: The timeout was not considered
    assert msg["reason"] == "Request not answered within 5 seconds."

    # Request execution of non-existing slot of existing device
    # (the GuiServerDevice itself...)
    h = Hash("type", "execute",
             "deviceId", TEST_GUI_SERVER_ID,
             "command", "not.existing",
             "reply", True,
             "timeout", 1)
    guiServer.tcpWriteHash(h)
    msg = await guiServer.get_next("executeReply")
    assert msg["type"] == "executeReply"
    assert msg["input"].fullyEqual(h)
    assert not msg["success"]

    reason = msg["reason"]
    prefix = (f"'{TEST_GUI_SERVER_ID}' has no slot 'not.existing'\nDetails:\n")
    assert reason.startswith(prefix)
    assert "1. Exception =====>" in reason[len(prefix):]

    # Request execution of existing slot of existing device
    # (the GuiServerDevice itself...)
    h = Hash("type", "execute",
             "deviceId", TEST_GUI_SERVER_ID,
             "command", "slotGetConfiguration",
             "reply", True,
             "timeout", 1)
    guiServer.tcpWriteHash(h)
    msg = await guiServer.get_next("executeReply")
    assert msg["type"] == "executeReply"
    assert msg["input"].fullyEqual(h)
    assert msg["success"]
    assert "reason" not in msg

    # Request execution of existing slot of existing device
    # (the GuiServerDevice itself...),
    await setWait(TEST_GUI_SERVER_ID, "lockedBy", "someone")
    val = await getProperties(TEST_GUI_SERVER_ID, "lockedBy")
    assert val["lockedBy"] == "someone"

    h = Hash("type", "execute",
             "deviceId", TEST_GUI_SERVER_ID,
             "command", "slotClearLock")
    guiServer.tcpWriteHash(h)
    await assert_wait_property(TEST_GUI_SERVER_ID, "lockedBy", "")
