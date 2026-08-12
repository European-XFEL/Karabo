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
from asyncio import sleep, wait_for

import pytest
import pytest_asyncio

from karabo.middlelayer import (
    Hash, State, call, getProperties, instantiate, setWait, shutdown)
from karabo.middlelayer.testing import (
    AsyncServerContext, assert_wait_property, sleepUntil)

from .adapter import GuiAdapter
from .websockets_adapter import WebsocketsAdapter

# Parametrize all tests in a module by setting 'pytestmark' global variable
# Setting 'indirect=True' option is critical.
pytestmark = pytest.mark.parametrize(
      "guiServer", [
                (WebsocketsAdapter, "ws"),
                (GuiAdapter, "tcp"),
           ],
      indirect=True)

TEST_GUI_SERVER_ID = "guiServerTest"
TEST_SERVER_ID = "karabo_guiserver_test"
TEST_PYTHON_SERVER_ID = "karabo_guiserver_test_scene_python"


@pytest_asyncio.fixture(loop_scope="module", scope="module")
async def guiServer(request, unused_tcp_port_factory):
    TCPPORT, WEBPORT = unused_tcp_port_factory(), unused_tcp_port_factory()
    Adapter, adapterType = request.param
    if adapterType == "tcp":
        PORT = TCPPORT
    else:
        PORT = WEBPORT
    serverId = TEST_SERVER_ID
    config = {
        TEST_GUI_SERVER_ID: {
            "classId": "GuiServerDevice", "port": TCPPORT, "webport": WEBPORT,
            "activePorts": "Tcp and Websocket", "timeout": 1,
        }
    }
    init = json.dumps(config)
    server = AsyncServerContext(serverId, [f"init={init}"], api="cpp")
    async with server:
        await assert_wait_property(TEST_GUI_SERVER_ID, "state",
                                   State.ON, timeout=20)
        adapter = Adapter(host="localhost", port=PORT)
        await adapter.connect()
        yield adapter, Adapter, PORT
        await adapter.disconnect()


@pytest.mark.timeout(60)
@pytest.mark.asyncio
async def test_gui_server_execute_before_login(guiServer):
    blockedTypes = {"execute", "reconfigure", "getDeviceConfiguration",
                    "getDeviceSchema", "getClassSchema", "initDevice",
                    "killServer", "killDevice", "startMonitoringDevice",
                    "stopMonitoringDevice", "getPropertyHistory",
                    "getConfigurationFromPast", "subscribeNetwork",
                    "requestNetwork", "error", "requestGeneric",
                    "getClientDebugInfo"}
    adapter, _, _ = guiServer
    for msg_type in blockedTypes:
        h = Hash("type", msg_type)
        await adapter.send(h)
        msg = await adapter.get_next("notification")
        message = msg["message"]
        assert message == f"Action '{msg_type}' refused before log in"


@pytest.mark.timeout(60)
@pytest.mark.asyncio
async def test_gui_server_execute(guiServer):
    adapter, _, _ = guiServer

    await adapter.login()
    await sleep(0.2)  # Fix GUI server to register above login
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
    await adapter.send(h)
    msg = await adapter.get_next("executeReply")
    assert msg["type"] == "executeReply"
    input_hash = msg["input"]
    assert input_hash.fullyEqual(h)
    assert not msg["success"]
    assert msg["reason"] == "Request not answered within 1 seconds."

    # Request execution of non-existing slot of existing device
    # (the GuiServerDevice itself...)
    h = Hash("type", "execute",
             "deviceId", TEST_GUI_SERVER_ID,
             "command", "not.existing",
             "reply", True,
             "timeout", 1)
    await adapter.send(h)
    msg = await adapter.get_next("executeReply")
    assert msg["type"] == "executeReply"
    assert msg["input"].fullyEqual(h)
    assert not msg["success"]

    reason = msg["reason"]
    prefix = (f"'{TEST_GUI_SERVER_ID}' has no slot 'not.existing'")
    assert reason.startswith(prefix)

    # Request execution of existing slot of existing device
    # (the GuiServerDevice itself...)
    h = Hash("type", "execute",
             "deviceId", TEST_GUI_SERVER_ID,
             "command", "slotGetConfiguration",
             "reply", True,
             "timeout", 1)
    await adapter.send(h)
    msg = await adapter.get_next("executeReply")
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
    await adapter.send(h)
    await assert_wait_property(TEST_GUI_SERVER_ID, "lockedBy", "")


@pytest.mark.timeout(60)
@pytest.mark.asyncio
async def test_request_fail_protocol(guiServer):
    adapter, _, _ = guiServer
    await adapter.login()
    await adapter.reset()
    msg_type = "GuiServerDoesNotHaveThisType"
    h = Hash("type", msg_type)
    conf = await getProperties(TEST_GUI_SERVER_ID, "classVersion")
    class_version = conf["classVersion"]

    await adapter.send(h)
    msg = await adapter.get_next("notification")

    expected_message = (
        "The gui server with version " + class_version +
        " does not support the client application request of " + msg_type
    )
    assert msg["message"] == expected_message


@pytest.mark.timeout(60)
@pytest.mark.asyncio
async def test_request_generic(guiServer):
    adapter, _, _ = guiServer
    info = Hash("type", "login",
                "clientId", "mrusp",
                "password", "12345",
                "version", "42.0.0")
    await adapter.login(info)

    # Case 1: Request to offline device, with slot
    h = Hash("type", "requestGeneric",
             "instanceId", "isnotonline",
             "timeout", 1,
             "slot", "requestScene")
    h.set("args", Hash("name", "scene"))

    await adapter.send(h)
    msg = await adapter.get_next("requestGeneric")

    assert not msg["success"]
    assert msg["type"] == "requestGeneric"
    assert msg["request"]["args"]["name"] == "scene"

    # Case 2: Ill-formed (missing 'slot')
    h = Hash("type", "requestGeneric",
             "instanceId", "isnotonline",
             "timeout", 1)
    h.set("args", Hash("name", "scene"))

    await adapter.send(h)
    msg = await adapter.get_next("requestGeneric")

    assert not msg["success"]
    assert msg["type"] == "requestGeneric"
    reason = msg["reason"]
    assert "Key 'slot' does not exist" in reason

    # Case 3: Valid request with different replyType
    h = Hash("type", "requestGeneric",
             "instanceId", "isnotonline",
             "timeout", 1,
             "replyType", "requestSuperScene",
             "slot", "slotDumpDebugInfo")
    h.set("args", Hash("name", "noname"))

    await adapter.send(h)
    msg = await adapter.get_next("requestSuperScene")

    assert not msg["success"]
    assert msg["request"].fullyEqual(h)
    assert msg["reason"] == "Request not answered within 1 seconds."
    assert msg["type"] == "requestSuperScene"
    assert msg["request"]["args"]["name"] == "noname"

    # Case 4: Online device, empty request
    h = Hash("type", "requestGeneric",
             "instanceId", TEST_GUI_SERVER_ID,
             "timeout", 1,
             "replyType", "debug",
             "empty", True,
             "slot", "slotDumpDebugInfo")
    h.set("args", Hash("clients", True))

    await adapter.send(h)
    msg = await adapter.get_next("debug")

    assert msg["success"]
    assert msg["type"] == "debug"
    assert msg["request"] == Hash()
    assert len(msg["reply"]) >= 1

    # Case 5: Online device, with token in request
    h = Hash("type", "requestGeneric",
             "instanceId", TEST_GUI_SERVER_ID,
             "timeout", 1,
             "replyType", "debug",
             "empty", True,
             "token", "here is a token of my appreciation",
             "slot", "slotDumpDebugInfo")
    h.set("args", Hash("clients", True))

    await adapter.send(h)
    msg = await adapter.get_next("debug")

    assert msg["success"]
    assert msg["type"] == "debug"
    request = msg["request"]
    assert len(request) == 1
    assert request["token"] == "here is a token of my appreciation"
    assert len(msg["reply"]) >= 1


@pytest.mark.timeout(60)
@pytest.mark.asyncio
async def test_get_client_debug_info(guiServer):
    adapter, Adapter, port = guiServer
    first_version = "42.1.0"
    second_version = "42.2.0"
    await adapter.login(Hash("type", "login", "clientId", "first-client",
                             "version", first_version))

    second_adapter = Adapter(host="localhost", port=port)
    try:
        await second_adapter.login(
            Hash("type", "login", "clientId", "second-client",
                 "version", second_version)
        )
        await sleep(0.2)

        request = Hash("type", "getClientDebugInfo")
        await adapter.send(request)
        first_reply = await adapter.get_next("clientDebugInfo")
        assert set(first_reply.keys()) == {"type", "debugInfo"}
        debug_info = first_reply["debugInfo"]
        assert len(debug_info) == 2
        topology = debug_info["systemTopology"]
        assert isinstance(topology, Hash)
        assert TEST_GUI_SERVER_ID in topology["device"]
        client_address = next(key for key in debug_info.keys()
                              if key != "systemTopology")
        assert debug_info[client_address]["clientVersion"] == first_version

        await second_adapter.send(request)
        second_reply = await second_adapter.get_next("clientDebugInfo")
        assert set(second_reply.keys()) == {"type", "debugInfo"}
        debug_info = second_reply["debugInfo"]
        assert len(debug_info) == 2
        assert isinstance(debug_info["systemTopology"], Hash)
        client_address = next(key for key in debug_info.keys()
                              if key != "systemTopology")
        assert debug_info[client_address]["clientVersion"] == second_version
    finally:
        await second_adapter.disconnect()


@pytest.mark.timeout(60)
@pytest.mark.asyncio
async def test_get_device_schema(guiServer):
    adapter, _, _ = guiServer
    info = Hash("type", "login",
                "clientId", "mrusp",
                "password", "12345",
                "version", "42.0.0")
    await adapter.login(info)

    h = Hash("type", "getDeviceSchema",
             "deviceId", TEST_GUI_SERVER_ID)

    # First request
    await adapter.send(h)
    h1 = await adapter.get_next("deviceSchema")

    assert h1["type"] == "deviceSchema"
    assert h1["deviceId"] == TEST_GUI_SERVER_ID
    assert h1["schema"]  # Not empty

    # Second request (from cache)
    await adapter.send(h)
    h2 = await adapter.get_next("deviceSchema")

    assert h1["deviceId"] == h2["deviceId"]
    assert h1["schema"] == h2["schema"]


@pytest.mark.timeout(60)
@pytest.mark.asyncio
async def test_slow_slots(guiServer):
    adapter, _, _ = guiServer
    await adapter.login()

    # Instantiate test device
    await instantiate(
        TEST_SERVER_ID, "PropertyTest",  "testGuiServerDevicePropertyTest")

    # Case 1: slowSlot should timeout
    h = Hash("type", "execute",
             "deviceId", "testGuiServerDevicePropertyTest",
             "command", "slowSlot",
             "reply", True,
             "timeout", 1)

    await adapter.send(h)
    msg = await adapter.get_next("executeReply")

    assert msg["type"] == "executeReply"
    assert not msg["success"]
    assert "reason" in msg
    assert "Request not answered within 1 seconds" in msg["reason"]

    # Case 2: ignoreTimeoutClasses allows slot to succeed
    await setWait(TEST_GUI_SERVER_ID,
                  ignoreTimeoutClasses=["PropertyTest"])

    await adapter.send(h)
    msg = await adapter.get_next("executeReply")

    assert msg["type"] == "executeReply"
    assert msg["success"]
    assert "reason" not in msg

    # Case 3: Remove ignoreTimeoutClasses, should timeout again
    await setWait(TEST_GUI_SERVER_ID, ignoreTimeoutClasses=[])

    await adapter.send(h)
    msg = await adapter.get_next("executeReply")
    assert msg["type"] == "executeReply"
    assert not msg["success"]
    assert "reason" in msg
    assert "Request not answered within 1 seconds" in msg["reason"]

    # Case 4: Set server timeout high to allow slow slot to succeed
    r = await getProperties(TEST_GUI_SERVER_ID, "timeout")
    previous_timeout = r["timeout"]
    await setWait(TEST_GUI_SERVER_ID, "timeout", 30)

    await adapter.send(h)
    msg = await adapter.get_next("executeReply")

    assert msg["type"] == "executeReply"
    assert msg["success"]
    assert "reason" not in msg
    # Case 5: Restore timeout, should timeout again
    await setWait(TEST_GUI_SERVER_ID, "timeout", previous_timeout)

    await adapter.send(h)
    msg = await adapter.get_next("executeReply")

    assert msg["type"] == "executeReply"
    assert not msg["success"]
    assert "reason" in msg
    assert "Request not answered within 1 seconds" in msg["reason"]

    # Cleanup: kill device
    await shutdown("testGuiServerDevicePropertyTest")


@pytest.mark.timeout(60)
@pytest.mark.asyncio
async def test_get_class_schema(guiServer):
    adapter, _, _ = guiServer
    await adapter.login()

    h = Hash("type", "getClassSchema",
             "serverId", TEST_SERVER_ID,
             "classId", "PropertyTest")

    # First request
    await adapter.send(h)
    msg1 = await adapter.get_next("classSchema")

    assert msg1["type"] == "classSchema"
    assert msg1["serverId"] == TEST_SERVER_ID
    assert msg1["classId"] == "PropertyTest"
    assert msg1["schema"]  # Not empty

    # Second request (should come from cache)
    await adapter.send(h)
    msg2 = await adapter.get_next("classSchema")

    assert msg2 == msg1

    # Request schema for non-existent class
    h2 = Hash("type", "getClassSchema",
              "serverId", TEST_SERVER_ID,
              "classId", "NonExistingDeviceClass")

    await adapter.send(h2)
    msg3 = await adapter.get_next("classSchema")

    assert msg3["type"] == "classSchema"
    assert msg3["serverId"] == TEST_SERVER_ID
    assert msg3["classId"] == "NonExistingDeviceClass"
    # C++ can just do not schema
    assert msg3["schema"].hash.empty()


@pytest.mark.timeout(60)
@pytest.mark.asyncio
async def test_reconfigure(guiServer):
    adapter, _, _ = guiServer
    await adapter.login()

    # Case 1: Reconfigure non-existent device
    h = Hash("type", "reconfigure",
             "deviceId", "not_there",
             "configuration", Hash("whatever", 1),
             "reply", True,
             "timeout", 1)

    await adapter.send(h)
    msg = await adapter.get_next("reconfigureReply")

    assert msg["type"] == "reconfigureReply"
    assert msg["input"].fullyEqual(h)
    assert not msg["success"]
    assert msg["reason"] == "Request not answered within 1 seconds."

    # Case 2: Invalid reconfigure of existing device
    h = Hash("type", "reconfigure",
             "deviceId", TEST_GUI_SERVER_ID,
             "configuration", Hash("whatever", 1),
             "reply", True,
             "timeout", 1)

    await adapter.send(h)
    msg = await adapter.get_next("reconfigureReply")

    assert msg["type"] == "reconfigureReply"
    assert msg["input"].fullyEqual(h)
    assert not msg["success"]

    reason = msg["reason"]
    expected_start = (
        f"Error in slot 'slotReconfigure' from {TEST_GUI_SERVER_ID}\n"
        "  because: Encountered unexpected configuration parameter: "
        "\"whatever\"\nDetails:\n"
    )
    assert reason.startswith(expected_start)
    assert "1. Exception =====>" in reason[len(expected_start):]

    # Case 3: Valid reconfigure with reply
    current = await getProperties(TEST_GUI_SERVER_ID,
                                  "networkPerformance.sampleInterval")
    current = current["networkPerformance.sampleInterval"]
    new_target = current * 2

    h = Hash("type", "reconfigure",
             "deviceId", TEST_GUI_SERVER_ID,
             "configuration",
             Hash("networkPerformance.sampleInterval", new_target),
             "reply", True,
             "timeout", 1)

    await adapter.send(h)
    msg = await adapter.get_next("reconfigureReply")

    assert msg["type"] == "reconfigureReply"
    assert msg["input"].fullyEqual(h)
    assert msg["success"]
    assert "reason" not in msg

    updated = await getProperties(TEST_GUI_SERVER_ID,
                                  "networkPerformance.sampleInterval")
    updated = updated["networkPerformance.sampleInterval"]
    assert updated == new_target

    # Case 4: Valid reconfigure without reply
    new_target = updated + 2
    h = Hash("type", "reconfigure",
             "deviceId", TEST_GUI_SERVER_ID,
             "configuration",
             Hash("networkPerformance.sampleInterval", new_target))
    await adapter.send(h)

    await assert_wait_property(
        TEST_GUI_SERVER_ID, "networkPerformance.sampleInterval",
        new_target, timeout=10)


@pytest.mark.timeout(60)
@pytest.mark.asyncio
async def test_device_config_updates(guiServer):
    adapter, Adapter, PORT = guiServer
    await adapter.login()

    # Setup second client
    anotherClient = Adapter("localhost", port=PORT)
    await anotherClient.connect()
    await anotherClient.login()

    # Set property update interval
    await setWait(TEST_GUI_SERVER_ID, propertyUpdateInterval=1500)
    interval = 1.5
    timeout = interval + 0.5

    # Instantiate two PropertyTest devices
    await instantiate(TEST_SERVER_ID, "PropertyTest", "PropTest_1")

    await instantiate(TEST_SERVER_ID, "PropertyTest", "PropTest_2")

    # Change a property without monitoring → expect no config update
    h = Hash("type", "reconfigure", "deviceId", "PropTest_1",
             "configuration", Hash("int32Property", 10))
    await adapter.send(h)
    with pytest.raises(TimeoutError):
        await wait_for(adapter.get_next("deviceConfigurations"),
                       timeout=timeout)

    await assert_wait_property("PropTest_1", "int32Property", 10)

    # First client subscribes to PropTest_1
    h = Hash("type", "startMonitoringDevice", "deviceId", "PropTest_1")
    await adapter.send(h)
    msg = await wait_for(adapter.get_next("deviceConfigurations"),
                         timeout=timeout)
    assert "configurations.PropTest_1" in msg
    assert "configurations.PropTest_1.deviceId" in msg
    assert not anotherClient.get_all("deviceConfigurations")

    # Second client subscribes (should work from cache)
    await anotherClient.send(h)
    msg = await wait_for(anotherClient.get_next("deviceConfigurations"),
                         timeout=timeout)
    assert "configurations.PropTest_1" in msg
    assert "configurations.PropTest_1.deviceId" in msg

    # anotherClient unsubscribes twice
    h = Hash("type", "stopMonitoringDevice", "deviceId", "PropTest_1")
    await anotherClient.send(h)
    await anotherClient.send(h)
    await anotherClient.reset()

    # The pre-2.15.X problem of a connection miscount by this duplicated
    # "stopMonitoringDevice" is
    # unfortunately only seen after the device has "aged to death"
    # inside the DeviceClient.
    # That requires this very long sleep to be sure to test that
    # the issue is fixed - without it, the
    # next adapter.get_next("deviceConfigurations", ...)
    # does NOT timeout despite of the bug.
    # XXX: What sleep
    await sleep(4)

    # Change properties on both, only PropTest_1 should be seen
    h1 = Hash("type", "reconfigure", "deviceId", "PropTest_1",
              "configuration", Hash("int32Property", 12))
    h2 = Hash("type", "reconfigure", "deviceId", "PropTest_2",
              "configuration", Hash("int32Property", 22))
    await adapter.send(h2)
    await adapter.send(h1)

    msg = await wait_for(adapter.get_next("deviceConfigurations"),
                         timeout=timeout)
    configs = msg["configurations"]
    assert "PropTest_1" in configs
    assert configs["PropTest_1"]["int32Property"] == 12
    assert len(configs) == 1
    assert not anotherClient.get_all("deviceConfigurations")

    # Re-subscribe anotherClient, should work again
    h = Hash("type", "startMonitoringDevice", "deviceId", "PropTest_1")
    await anotherClient.send(h)
    msg = await wait_for(anotherClient.get_next("deviceConfigurations"),
                         timeout=timeout)
    assert "configurations.PropTest_1" in msg

    # Subscribe to PropTest_2
    h = Hash("type", "startMonitoringDevice", "deviceId", "PropTest_2",
             "reply", True, "timeout", 1)
    await adapter.send(h)
    msg = await wait_for(adapter.get_next("deviceConfigurations"),
                         timeout=timeout)
    assert "configurations.PropTest_2" in msg
    assert msg["configurations"]["PropTest_2"]["int32Property"] == 22

    # Change properties on both, both configs should appear
    h1 = Hash("type", "reconfigure", "deviceId", "PropTest_1",
              "configuration", Hash("int32Property", 14))
    h2 = Hash("type", "reconfigure", "deviceId", "PropTest_2",
              "configuration", Hash("int32Property", 24))

    await adapter.send(h2)
    await adapter.send(h1)

    msg = await wait_for(adapter.get_next("deviceConfigurations"),
                         timeout=timeout)
    configs = msg["configurations"]
    assert configs["PropTest_1"]["int32Property"] == 14
    assert configs["PropTest_2"]["int32Property"] == 24
    assert len(configs) == 2

    # Unsubscribe from both
    await adapter.send(
        Hash("type", "stopMonitoringDevice", "deviceId", "PropTest_1"))
    await adapter.send(
        Hash("type", "stopMonitoringDevice", "deviceId", "PropTest_2"))

    # Reconfigure again → expect no update
    h1 = Hash("type", "reconfigure", "deviceId", "PropTest_1",
              "configuration", Hash("int32Property", 16))
    h2 = Hash("type", "reconfigure", "deviceId", "PropTest_2",
              "configuration", Hash("int32Property", 26))
    await adapter.send(h2)
    await adapter.send(h1)

    with pytest.raises(TimeoutError):
        await wait_for(adapter.get_next("deviceConfigurations"),
                       timeout=timeout)

    await assert_wait_property("PropTest_1", "int32Property", 16)
    await assert_wait_property("PropTest_2", "int32Property", 26)

    await anotherClient.disconnect()
    # Shutdown devices
    await shutdown("PropTest_1")
    await shutdown("PropTest_2")


@pytest.mark.timeout(60)
@pytest.mark.asyncio
async def test_disconnect(guiServer):
    adapter, _, _ = guiServer
    await adapter.login()
    assert adapter.connected

    # Test: disconnect non-existent client
    disconnected = await call(
        TEST_GUI_SERVER_ID, "slotDisconnectClient", "BLAnoPORT")
    assert not disconnected
    assert adapter.connected

    # Test: disconnect valid client
    result = await call(
        TEST_GUI_SERVER_ID, "slotDumpDebugInfo", Hash("clients", 0))
    assert len(result) == 1
    client_id = next(iter(result.keys()))

    disconnected = await call(
        TEST_GUI_SERVER_ID, "slotDisconnectClient", client_id)
    assert disconnected
    await sleepUntil(lambda: adapter.connected is False)


@pytest.mark.timeout(60)
@pytest.mark.asyncio
async def test_slot_notify(guiServer):
    adapter, Adapter, PORT = guiServer
    await adapter.login()
    await adapter.reset()

    message_to_send = "Banner for everyone!"
    arg = Hash("message", message_to_send,
               "contentType", "banner",
               "foreground", "red")
    expected_data = [message_to_send, "", "red"]

    # Send banner via slotNotify and wait for incoming notification
    notify_future = adapter.get_next("notification")
    await call(TEST_GUI_SERVER_ID, "slotNotify", arg)
    msg = await notify_future

    assert msg["message"] == message_to_send
    assert msg["contentType"] == "banner"
    assert msg["foreground"] == "red"
    assert "background" not in msg

    # Wait until bannerData is updated on the device
    async def banner_data_ready():
        data = await getProperties(TEST_GUI_SERVER_ID, "bannerData")
        data = data["bannerData"]
        return isinstance(data, list) and len(data) == 3

    success = await wait_for(banner_data_ready(), timeout=5)
    assert success
    data = await getProperties(TEST_GUI_SERVER_ID, "bannerData")
    data = data["bannerData"]
    assert data == expected_data

    # Connect second client and verify it receives stored banner
    adapter2 = Adapter("localhost", PORT)
    await adapter2.login()

    banner = await wait_for(adapter2.get_next("notification"), timeout=1)
    assert banner["message"] == message_to_send
    assert banner["contentType"] == "banner"
    assert banner["foreground"] == "red"
    assert "background" not in banner

    await adapter2.disconnect()

    # Clear banner using slotNotify
    clear_arg = Hash("message", "",
                     "contentType", "banner")
    clear_future = adapter.get_next("notification")
    await call(TEST_GUI_SERVER_ID, "slotNotify", clear_arg)
    msg = await clear_future

    assert msg["message"] == ""
    assert msg["contentType"] == "banner"
    assert "foreground" not in msg
    assert "background" not in msg

    # Banner data should now be empty
    data = await getProperties(TEST_GUI_SERVER_ID, "bannerData")
    data = data["bannerData"]
    assert data == []

    # Connect third client and ensure no banner is received
    adapter3 = Adapter("localhost", PORT)
    await adapter3.login()

    with pytest.raises(TimeoutError):
        await wait_for(adapter3.get_next("notification"), timeout=1)

    await adapter2.disconnect()
    await adapter3.disconnect()


@pytest.mark.timeout(60)
@pytest.mark.asyncio
async def test_slot_broadcast(guiServer):
    adapter, _, _ = guiServer
    await adapter.login()
    timeout = 5

    # 1. Broadcast to all clients
    message = Hash("isSkookum", True,
                   "type", "unimplementedDangerousCall")
    arg = Hash("message", message,
               "clientAddress", "")

    receive_future = wait_for(
        adapter.get_next("unimplementedDangerousCall"), timeout=timeout)
    reply = await call(TEST_GUI_SERVER_ID, "slotBroadcast", arg)
    assert reply["success"]
    assert len(reply) == 1

    received = await receive_future
    assert message.fullyEqual(received)

    # 2. Broadcast without message type → should raise
    bad_arg = Hash("isSkookum", False)
    with pytest.raises(Exception):
        await call(TEST_GUI_SERVER_ID, "slotBroadcast", bad_arg)

    # 3. Broadcast to unknown client → should fail but not raise
    bad_msg = Hash("isSkookum", False,
                   "type", "unimplementedDangerousCall")
    bad_client_arg = Hash("message", bad_msg,
                          "clientAddress", "pinneberg")

    reply = await call(TEST_GUI_SERVER_ID, "slotBroadcast", bad_client_arg)
    assert reply["success"] is False
    assert len(reply) == 1

    # 4. Send message to specific connected client
    debug_info = await call(
        TEST_GUI_SERVER_ID, "slotDumpDebugInfo", Hash("clients", True)
    )
    assert len(debug_info) == 1
    client_id = next(iter(debug_info.keys()))
    client_msg = Hash("skookumFactor", 42,
                      "type", "unimplementedDangerousCall")
    client_arg = Hash("clientAddress", client_id, "message", client_msg)

    receive_future = wait_for(
        adapter.get_next("unimplementedDangerousCall"), timeout=timeout)
    reply = await call(TEST_GUI_SERVER_ID, "slotBroadcast", client_arg)

    assert reply["success"]
    assert len(reply) == 1

    received = await receive_future
    assert client_msg.fullyEqual(received)


@pytest.mark.timeout(60)
@pytest.mark.asyncio
async def test_version_control(guiServer):
    adapter, _, _ = guiServer
    await adapter.disconnect()
    login_info = Hash("type", "login",
                      "username", "mrusp",
                      "password", "12345",
                      "version", "100.1.0")

    test_cases = [
        ("version control supported", "100.1.0", True),
        ("version control unsupported", "0.1.0", False),
        ("version control disabled", "0.1.0", False),
    ]

    for test_name, client_version, should_connect in test_cases:
        login_info.set("version", client_version)
        await adapter.login(login_info)

        if should_connect:
            await adapter.send(login_info)
            msg = await adapter.get_next("systemTopology")
            assert "systemTopology" in msg
        else:
            await adapter.send(login_info)
            msg = await adapter.get_next("notification")
            expected_prefix = (
                f"Your GUI client has version '{client_version}', "
                "but the minimum required is:")
            assert msg["message"].startswith(expected_prefix)
        try:
            await sleepUntil(
                lambda: adapter.connected is should_connect)
        except TimeoutError:
            assert False, test_name


@pytest.mark.timeout(60)
@pytest.mark.asyncio
async def test_gui_server_scene_retrieval(guiServer):
    adapter, _, _ = guiServer
    await adapter.login()
    config = {
        "testNoSceneProvider": {
            "classId": "NonSceneProvidingDevice"},
        "testSceneProvider": {
            "classId": "SceneProvidingDevice"}
    }
    init = json.dumps(config)
    python_server = AsyncServerContext(
        TEST_PYTHON_SERVER_ID,
        [f"init={init}", "pluginNamespace=karabo.bound_device_test"],
        api="python")
    async with python_server:
        await assert_wait_property("testNoSceneProvider", "state",
                                   State.NORMAL, timeout=10)
        await assert_wait_property("testSceneProvider", "state",
                                   State.NORMAL, timeout=10)
        args = Hash("name", "scene")
        message = Hash(
            "type", "requestGeneric",
            "instanceId", "testSceneProvider",
            "slot", "requestScene",
            "args", args,
            "token", "notAVeryUniqueToken"
        )
        await adapter.send(message)
        msg = await adapter.get_next("requestGeneric")

        assert msg["type"] == "requestGeneric"
        assert msg["reply.payload.name"] == "scene"
        assert msg["request.token"] == "notAVeryUniqueToken"
        assert msg["success"]

        args = Hash("name", "scene")
        message = Hash(
            "type", "requestGeneric",
            "instanceId", "testNoSceneProvider",
            "slot", "requestScene",
            "args", args,
            "token", "notAVeryUniqueToken",
            "timeout", 2)

        await adapter.send(message)
        msg = await adapter.get_next("requestGeneric")
        assert msg["request.type"] == "requestGeneric"
        assert msg["request.token"] == "notAVeryUniqueToken"
        assert not msg["success"]
