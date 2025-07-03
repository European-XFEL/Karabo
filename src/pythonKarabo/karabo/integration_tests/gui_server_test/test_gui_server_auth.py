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
from asyncio import start_server

import pytest
import pytest_asyncio

from karabo.middlelayer import (
    AccessLevel, Hash, State, Timestamp, call, instantiate, shutdown)
from karabo.middlelayer.testing import (
    AsyncServerContext, assert_wait_property, sleepUntil)

from .adapter import GuiAdapter
from .authserver import (
    INVALID_TOKEN_MSG, VALID_ACCESS_LEVEL, VALID_TOKEN, VALID_USER_ID,
    handle_client)

TEST_GUI_SERVER_ID = "guiServerAuthTest_auth"
TEST_SERVER_ID = "karabo_guiServerAuth_test_auth"
AUTH_PORT = 8052
AUTH_ADDRESS = "127.0.0.1"

CHECK_SESSION_EXPIRATION_INTERVAL_SECS = 5

MAX_TEMPORARY_SESSION_TIME = CHECK_SESSION_EXPIRATION_INTERVAL_SECS * 2
END_TEMPORARY_SESSION_NOTICE_TIME = CHECK_SESSION_EXPIRATION_INTERVAL_SECS - 1

MAX_SESSION_TIME = CHECK_SESSION_EXPIRATION_INTERVAL_SECS * 3
END_SESSION_NOTICE_TIME = CHECK_SESSION_EXPIRATION_INTERVAL_SECS * 2

CLIENT_SESSIONS_OPTION_KEY = "onlyTempSessions"


@pytest_asyncio.fixture(loop_scope="module", scope="module")
async def authServer():
    server = await start_server(handle_client, AUTH_ADDRESS, AUTH_PORT)
    async with server:
        await server.start_serving()
        yield server


@pytest_asyncio.fixture(loop_scope="module", scope="module")
async def guiServerAuth(authServer):
    serverId = TEST_SERVER_ID
    config = {
        TEST_GUI_SERVER_ID: {
            "classId": "GuiServerDevice", "port": 44450, "timeout": 1,
            "authServer": f"http://{AUTH_ADDRESS}:{AUTH_PORT}",
            "minClientVersion": "2.20.0",
            "maxSessionDuration": MAX_SESSION_TIME,
            "endSessionNoticeTime": END_SESSION_NOTICE_TIME,
            "maxTemporarySessionTime": MAX_TEMPORARY_SESSION_TIME,
            "endTemporarySessionNoticeTime": END_TEMPORARY_SESSION_NOTICE_TIME
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
async def test_missing_token_on_login(guiServerAuth):
    login_info = Hash("type", "login",
                      "clientId", "bobHost(pid 10264)",
                      "version", "2.20.0")
    await guiServerAuth.reset()
    await guiServerAuth.login(login_info)

    # Expect loginInformation message
    msg = await guiServerAuth.get_next("loginInformation")

    assert msg["readOnly"] is True
    assert msg["accessLevel"] == AccessLevel.OBSERVER

    # Try to start temporary session → should fail
    begin_temp_session = Hash(
        "type", "beginTemporarySession",
        "temporarySessionToken", VALID_TOKEN,
        "version", "2.20.0",
        "levelBeforeTemporarySession", AccessLevel.OPERATOR)
    guiServerAuth.tcpWriteHash(begin_temp_session)
    msg = await guiServerAuth.get_next("notification")

    assert "cannot be started from a readOnly" in msg["message"]
    await guiServerAuth.disconnect()
    await sleepUntil(lambda: guiServerAuth.connected is False)


@pytest.mark.timeout(30)
@pytest.mark.asyncio(loop_scope="module")
async def test_invalid_token_on_login(guiServerAuth):
    login_info = Hash("type", "login",
                      "username", "bob",
                      "oneTimeToken", "abcd",
                      "version", "2.20.0")
    await guiServerAuth.reset()
    await guiServerAuth.login(login_info)
    msg = await guiServerAuth.get_next("notification")

    expected_msg = (
        "Error validating token: " + INVALID_TOKEN_MSG)
    assert msg["message"] == expected_msg

    await sleepUntil(lambda: guiServerAuth.connected is False)


@pytest.mark.timeout(30)
@pytest.mark.asyncio(loop_scope="module")
async def test_valid_token_on_login(guiServerAuth):
    login_info = Hash("type", "login",
                      "username", VALID_USER_ID,
                      "oneTimeToken", VALID_TOKEN,
                      "version", "2.20.0")

    await guiServerAuth.reset()
    await guiServerAuth.login(login_info)

    msg = await guiServerAuth.get_next("loginInformation")
    assert msg["accessLevel"] == VALID_ACCESS_LEVEL
    assert msg["username"] == VALID_USER_ID

    guiServerAuth.tcpWriteHash(Hash("type", "getGuiSessionInfo"))
    msg = await guiServerAuth.get_next("getGuiSessionInfo")

    assert msg["sessionStartTime"] != ""
    assert msg["sessionDuration"] == MAX_SESSION_TIME
    assert msg["tempSessionStartTime"] == ""
    assert msg["tempSessionDuration"] == MAX_TEMPORARY_SESSION_TIME

    await guiServerAuth.disconnect()
    await sleepUntil(lambda: guiServerAuth.connected is False)


@pytest.mark.timeout(30)
@pytest.mark.asyncio(loop_scope="module")
async def test_valid_token_on_in_session_login(guiServerAuth):
    login_info = Hash("type", "login",
                      "username", VALID_USER_ID,
                      "oneTimeToken", VALID_TOKEN,
                      "version", "2.20.0")
    await guiServerAuth.reset()
    await guiServerAuth.login(login_info)

    msg = await guiServerAuth.get_next("loginInformation")
    assert msg["accessLevel"] == VALID_ACCESS_LEVEL
    assert msg["username"] == VALID_USER_ID

    # Re-login with same valid token — should succeed and refresh session
    guiServerAuth.tcpWriteHash(login_info)
    msg = await guiServerAuth.get_next("loginInformation")
    assert msg["accessLevel"] == VALID_ACCESS_LEVEL
    assert msg["username"] == VALID_USER_ID


@pytest.mark.timeout(30)
@pytest.mark.asyncio(loop_scope="module")
async def test_invalid_token_on_in_session_login(guiServerAuth):
    login_info = Hash("type", "login",
                      "username", VALID_USER_ID,
                      "oneTimeToken", VALID_TOKEN,
                      "version", "2.20.0")

    invalid_login_info = Hash("type", "login",
                              "username", VALID_USER_ID,
                              "oneTimeToken", "abcd",
                              "version", "2.20.0")

    await guiServerAuth.reset()
    await guiServerAuth.login(login_info)

    msg = await guiServerAuth.get_next("loginInformation")
    assert msg["accessLevel"] == VALID_ACCESS_LEVEL
    assert msg["username"] == VALID_USER_ID

    # Now try logging in again with an invalid token
    guiServerAuth.tcpWriteHash(invalid_login_info)
    msg = await guiServerAuth.get_next("notification")
    expected_msg = "Error validating token: " + INVALID_TOKEN_MSG
    assert msg["message"] == expected_msg

    # Verify that the original session remains active
    session_info = await call(TEST_GUI_SERVER_ID, "slotGetClientSessions",
                              Hash(CLIENT_SESSIONS_OPTION_KEY, False))
    sessions = session_info["clientSessions"]
    assert len(sessions) == 1
    assert sessions[0]["sessionToken"] == VALID_TOKEN


@pytest.mark.timeout(30)
@pytest.mark.asyncio(loop_scope="module")
async def test_missing_token_on_begin_temporary_session(guiServerAuth):
    login_info = Hash(
        "type", "login",
        "clientId", "desk010",
        "oneTimeToken", VALID_TOKEN,
        "version", "2.20.0",
        "levelBeforeTemporarySession", AccessLevel.OPERATOR)
    begin_temp_session_info = Hash(
        "type", "beginTemporarySession",
        "version", "2.20.0")

    await guiServerAuth.reset()
    await guiServerAuth.login(login_info)

    msg = await guiServerAuth.get_next("loginInformation")
    assert "accessLevel" in msg

    guiServerAuth.tcpWriteHash(begin_temp_session_info)
    msg = await guiServerAuth.get_next("onBeginTemporarySession")

    assert not msg["success"]
    assert "Required \"temporarySessionToken\" field missing" in msg["reason"]
    await guiServerAuth.disconnect()
    await sleepUntil(lambda: guiServerAuth.connected is False)


@pytest.mark.timeout(30)
@pytest.mark.asyncio(loop_scope="module")
async def test_invalid_token_on_begin_temporary_session(guiServerAuth):
    login_info = Hash(
        "type", "login",
        "clientId", "desk010",
        "oneTimeToken", VALID_TOKEN,
        "version", "2.20.0")

    begin_temp_session_info = Hash(
        "type", "beginTemporarySession",
        "temporarySessionToken", "abcd",
        "version", "2.20.0",
        "levelBeforeTemporarySession", int(AccessLevel.OPERATOR))

    await guiServerAuth.reset()
    await guiServerAuth.login(login_info)

    msg = await guiServerAuth.get_next("loginInformation")
    assert "accessLevel" in msg

    guiServerAuth.tcpWriteHash(begin_temp_session_info)
    msg = await guiServerAuth.get_next("onBeginTemporarySession")

    assert not msg["success"]
    assert msg["reason"] == INVALID_TOKEN_MSG
    await guiServerAuth.disconnect()
    await sleepUntil(lambda: guiServerAuth.connected is False)


@pytest.mark.timeout(30)
@pytest.mark.asyncio(loop_scope="module")
async def test_begin_end_temporary_session(guiServerAuth):
    login_info = Hash(
        "type", "login",
        "clientId", "desk010",
        "oneTimeToken", VALID_TOKEN,
        "version", "2.20.0"
    )
    begin_temp_session_info = Hash(
        "type", "beginTemporarySession",
        "temporarySessionToken", VALID_TOKEN,
        "version", "2.20.0",
        "levelBeforeTemporarySession", AccessLevel.OPERATOR
    )
    end_temp_session_info = Hash(
        "type", "endTemporarySession",
        "temporarySessionToken", VALID_TOKEN,
        "version", "2.20.0"
    )

    await guiServerAuth.reset()

    # Step 1: no sessions before login
    sessions = await call(TEST_GUI_SERVER_ID, "slotGetClientSessions",
                          Hash("dummy", ""))
    assert sessions["clientSessions"] == []

    # Step 2: login
    await guiServerAuth.login(login_info)
    msg = await guiServerAuth.get_next("loginInformation")
    assert "accessLevel" in msg

    # Step 3: verify session state after login
    all_sessions = await call(TEST_GUI_SERVER_ID, "slotGetClientSessions",
                              Hash(CLIENT_SESSIONS_OPTION_KEY, False))
    assert len(all_sessions["clientSessions"]) == 1
    session = all_sessions["clientSessions"][0]
    assert session["clientVersion"] == "2.20.0"
    assert session["sessionToken"] != ""
    assert session["sessionStartTime"] != ""
    assert session["temporarySessionToken"] == ""
    assert session["temporarySessionStartTime"] == ""
    session_token = session["sessionToken"]
    session_start_time = session["sessionStartTime"]

    # Step 4: verify onlyTempSessions gives empty before temp session starts
    only_temp = await call(TEST_GUI_SERVER_ID, "slotGetClientSessions",
                           Hash(CLIENT_SESSIONS_OPTION_KEY, True))
    assert only_temp["clientSessions"] == []

    # Step 5: begin temporary session
    guiServerAuth.tcpWriteHash(begin_temp_session_info)
    msg = await guiServerAuth.get_next("onBeginTemporarySession")
    assert msg["success"] is True
    assert msg["reason"] == ""
    assert "accessLevel" in msg
    assert "temporarySessionDurationSecs" in msg
    assert "username" in msg
    assert msg["username"] == VALID_USER_ID

    # Step 6: session reflects temp session
    temp_sessions = await call(
        TEST_GUI_SERVER_ID, "slotGetClientSessions",
        Hash(CLIENT_SESSIONS_OPTION_KEY, True))
    assert len(temp_sessions["clientSessions"]) == 1
    temp_sess = temp_sessions["clientSessions"][0]
    assert temp_sess["sessionToken"] == session_token
    assert temp_sess["sessionStartTime"] == session_start_time
    assert temp_sess["temporarySessionToken"] != ""
    assert temp_sess["temporarySessionStartTime"] != ""
    temp_token = temp_sess["temporarySessionToken"]

    # Step 7: same session also returned from all sessions
    full_sessions = await call(
        TEST_GUI_SERVER_ID, "slotGetClientSessions",
        Hash(CLIENT_SESSIONS_OPTION_KEY, False))

    session = full_sessions["clientSessions"][0]
    assert session["temporarySessionToken"] == temp_token

    # Step 8: verify getGuiSessionInfo response
    guiServerAuth.tcpWriteHash(Hash("type", "getGuiSessionInfo"))
    msg = await guiServerAuth.get_next("getGuiSessionInfo")
    assert msg["sessionStartTime"] == session_start_time
    assert msg["sessionDuration"] == MAX_SESSION_TIME
    assert msg["tempSessionStartTime"] != ""
    assert msg["tempSessionDuration"] == MAX_TEMPORARY_SESSION_TIME

    # Step 9: try begin again while already in temp session — must fail
    guiServerAuth.tcpWriteHash(begin_temp_session_info)
    msg = await guiServerAuth.get_next("onBeginTemporarySession")
    assert msg["success"] is False
    assert msg["reason"] == "There's already an active temporary session."

    # Step 10: try login while in temp session — must fail
    guiServerAuth.tcpWriteHash(login_info)
    msg = await guiServerAuth.get_next("notification")
    assert msg["message"] == (
        "There's an active temporary session. Please terminate "
        "it before trying to login again.")

    # Step 11: end temporary session
    guiServerAuth.tcpWriteHash(end_temp_session_info)
    msg = await guiServerAuth.get_next("onEndTemporarySession")
    assert msg["success"] is True
    assert msg["levelBeforeTemporarySession"] == AccessLevel.OPERATOR
    assert msg["loggedUserId"] == VALID_USER_ID

    # Step 12: try to end again — must fail
    guiServerAuth.tcpWriteHash(end_temp_session_info)
    msg = await guiServerAuth.get_next("onEndTemporarySession")
    assert msg["success"] is False
    assert msg["reason"] == (
        "There's no active temporary session associated "
        "with the requesting client.")

    # Step 13: wait for logout
    await guiServerAuth.disconnect()
    await sleepUntil(lambda: guiServerAuth.connected is False)


@pytest.mark.timeout(60)
@pytest.mark.asyncio(loop_scope="module")
async def test_session_expiration(guiServerAuth):
    login_info = Hash(
        "type", "login",
        "clientId", "desk010",
        "oneTimeToken", VALID_TOKEN,
        "version", "2.20.0")

    begin_temp_session_info = Hash(
        "type", "beginTemporarySession",
        "temporarySessionToken", VALID_TOKEN,
        "version", "2.20.0",
        "levelBeforeTemporarySession", int(AccessLevel.OPERATOR))

    await guiServerAuth.reset()
    await guiServerAuth.login(login_info)

    msg = await guiServerAuth.get_next("loginInformation")
    assert "accessLevel" in msg

    # Wait for expiration notice
    notice = await guiServerAuth.get_next("onEndSessionNotice")
    assert "aboutToExpireToken" in notice
    assert notice["aboutToExpireToken"] == VALID_TOKEN
    assert "secondsToExpiration" in notice
    assert int(notice["secondsToExpiration"]) <= END_SESSION_NOTICE_TIME

    # Try to start a temporary session after notice (should fail)
    guiServerAuth.tcpWriteHash(begin_temp_session_info)
    reply = await guiServerAuth.get_next("onBeginTemporarySession")
    assert not reply["success"]
    assert reply["reason"] == (
        "Refusing to put a temporary session on top of a login "
        "that expires soon. First re-login.")

    # Wait for actual session expiration
    expired = await guiServerAuth.get_next("onSessionExpired")
    assert "expiredToken" in expired
    assert expired["expiredToken"] == VALID_TOKEN
    assert "expirationTime" in expired

    expired_at = Timestamp(
        expired["expirationTime"]).toTimestamp()
    now = Timestamp().toTimestamp()
    assert expired_at < now
    await guiServerAuth.disconnect()


@pytest.mark.timeout(60)
@pytest.mark.asyncio(loop_scope="module")
async def test_temporary_session_expiration(guiServerAuth):
    login_info = Hash(
        "type", "login",
        "clientId", "desk010",
        "oneTimeToken", VALID_TOKEN,
        "version", "2.20.0")

    begin_temp_session_info = Hash(
        "type", "beginTemporarySession",
        "temporarySessionToken", VALID_TOKEN,
        "version", "2.20.0",
        "levelBeforeTemporarySession", AccessLevel.OPERATOR)

    await guiServerAuth.reset()
    await guiServerAuth.login(login_info)
    msg = await guiServerAuth.get_next("loginInformation")
    assert "accessLevel" in msg

    # Begin temporary session
    guiServerAuth.tcpWriteHash(begin_temp_session_info)
    msg = await guiServerAuth.get_next("onBeginTemporarySession")
    assert msg["success"] is True
    assert msg["reason"] == ""
    assert "accessLevel" in msg
    assert msg["temporarySessionDurationSecs"] == MAX_TEMPORARY_SESSION_TIME

    # Wait for expiration notice
    notice = await guiServerAuth.get_next("onEndTemporarySessionNotice")
    assert "aboutToExpireToken" in notice
    assert notice["aboutToExpireToken"] == VALID_TOKEN
    assert "secondsToExpiration" in notice
    assert notice["secondsToExpiration"] <= END_TEMPORARY_SESSION_NOTICE_TIME

    # Wait for actual expiration
    expired = await guiServerAuth.get_next(
        "onTemporarySessionExpired")
    assert "expiredToken" in expired
    assert expired["expiredToken"] == VALID_TOKEN
    assert "expirationTime" in expired
    assert "levelBeforeTemporarySession" in expired
    assert expired["levelBeforeTemporarySession"] == AccessLevel.OPERATOR
    assert "loggedUserId" in expired
    assert expired["loggedUserId"] == VALID_USER_ID

    # Validate expiration time is in the future
    expired_at = Timestamp(expired["expirationTime"]).toTimestamp()
    now = Timestamp().toTimestamp()
    assert expired_at < now

    await guiServerAuth.disconnect()
    # Wait for disconnection
    await sleepUntil(lambda: guiServerAuth.connected is False)


@pytest.mark.timeout(30)
@pytest.mark.asyncio(scope="module")
async def test_only_app_mode_clients(guiServerAuth):
    APP_ONLY_DEVICE_ID = "testGuiServerApp"

    PORT = 44454
    await instantiate(
         TEST_SERVER_ID, "GuiServerDevice", APP_ONLY_DEVICE_ID,
         Hash("port", PORT, "minClientVersion", "2.20.0",
              "onlyAppModeClients", True))
    login_info = Hash(
        "type", "login",
        "username", "bob",
        "applicationMode", False,
        "version", "2.20.0")
    await assert_wait_property(APP_ONLY_DEVICE_ID, "state", State.ON)

    adapter = GuiAdapter("localhost", PORT)
    await adapter.reset()
    await adapter.login(login_info)

    msg = await adapter.get_next("notification")
    expected_msg_part = ("configured to refuse connections from"
                         " the standard Karabo GUI Client")
    assert expected_msg_part in msg["message"]

    # Wait for the server to disconnect us
    await sleepUntil(lambda: adapter.connected is False)

    # Now try with applicationMode = True → should succeed
    login_info_app_mode = Hash(
        "type", "login",
        "clientId", "alice",
        "applicationMode", True,
        "version", "2.20.0")
    await adapter.reset()
    await adapter.login(login_info_app_mode)

    msg = await adapter.get_next("systemTopology")
    assert "systemTopology" in msg
    await adapter.disconnect()
    await shutdown(APP_ONLY_DEVICE_ID)
