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

# Here we test C++ features for SignalSlotable connection management between
# Signal and Slot for the case of both instances on different processes:
# connect(..), disconnect(..), asyncConnect(..) and asyncDisconnect(..)

import json
from time import sleep

import pytest

from karabo.bound import Hash, SignalSlotable
from karabo.bound.testing import ServerContext, sleepUntil

timeout = 5
timeoutMs = timeout * 1000

SERVER_ID = "sigSlotConnectServer"
SYNC_SLOTTESTER = "sigSlotConnectTester"
ASYNC_SLOTTESTER = "sigSlotAsyncConnectTester"


@pytest.fixture(scope="module")
def connectTest(eventLoop):
    config = {
        SYNC_SLOTTESTER: {
            "classId": "SignalDevice",
        },
        ASYNC_SLOTTESTER: {
            "classId": "SignalDevice",
        }
    }
    init = json.dumps(config)
    server = ServerContext(
        SERVER_ID, [f"init={init}",
                    "pluginNamespace=karabo.bound_device_test"],
        api="python")
    with server:
        remote = server.remote()
        sleepUntil(lambda: (SYNC_SLOTTESTER in remote.getDevices()
                            and ASYNC_SLOTTESTER in remote.getDevices()),
                   timeout=10)
        yield server


def test_connect(connectTest):
    """
    Test SignalSlotable.connect and disconnect between instances
    in different processes.
    (Even tests this for C++ where all tests run in a single process!)
    """
    # Create an instance with a slot that it can connect to the signal
    slotter = SignalSlotable("slotInstance_sync")
    slotCalled = False
    inSlot = None

    def slotFunc(i):
        nonlocal slotCalled, inSlot
        inSlot = i
        slotCalled = True

    slotter.registerSlot(slotFunc, "slot")
    slotter.start()

    # First test successful connect
    connected = slotter.connect(SYNC_SLOTTESTER, "signal", "slot")
    assert connected

    slotter.request(
        SYNC_SLOTTESTER, "slotEmitSignal", 42).waitForReply(timeoutMs)

    sleepUntil(lambda: slotCalled, timeout=timeout)

    assert slotCalled
    assert 42 == inSlot

    ##################################################
    # Test handling of failures: non-existing slot
    connected = slotter.connect(
        SYNC_SLOTTESTER, "signal", "NOT_A_slot")
    assert not connected

    #################################################################
    # Now disconnect
    disconnected = slotter.disconnect(
        SYNC_SLOTTESTER, "signal", "slot")
    assert disconnected

    slotCalled = False
    slotter.request(SYNC_SLOTTESTER,
                    "slotEmitSignal", 77).waitForReply(timeoutMs)

    # wait only a bit (0.1 seconds) for something not coming
    sleep(0.1)
    assert not slotCalled

    #################################################################
    # Finally a failing disconnect
    disconnected = slotter.disconnect(
        SYNC_SLOTTESTER, "signal", "not_a_slot")
    assert not disconnected


def test_asyncConnect(connectTest):
    """
    Test SignalSlotable.asyncConnect and asyncDisconnect
    between instances in different processes.
    (Even tests this for C++ where all tests run in a single process!)
    Very similar to 'test_sigslot_asyncConnect' from
    karabo/bound/tests/binding/test_sigslot.py,
    but that tests in-process short-cut.
    """
    # Create an instance with a slot that it can connect to the signal
    slotter = SignalSlotable("slotInstance_async")

    slotCalled = False
    inSlot = None

    def slotFunc(i):
        nonlocal slotCalled, inSlot
        inSlot = i
        slotCalled = True

    slotter.registerSlot(slotFunc, "slot")
    slotter.start()

    # First test successful connectAsync
    failureMsg = None
    failureDetails = None

    def connectCallback(failMsg, failDetails):
        nonlocal failureMsg, failureDetails
        failureMsg = failMsg
        failureDetails = failDetails

    slotter.asyncConnect(
        ASYNC_SLOTTESTER, "signal", "slot", connectCallback)
    sleepUntil(lambda: failureDetails is not None, timeout)

    assert failureMsg is not None  # callback called
    assert failureMsg == ""  # connect succeeded

    slotter.request(
        ASYNC_SLOTTESTER, "slotEmitSignal", 42).waitForReply(timeoutMs)

    sleepUntil(lambda: slotCalled, timeout)

    assert slotCalled
    assert 42 == inSlot

    ##################################################
    # Test handling of failures

    # Non-existing slot gives failure
    failureMsg = failureDetails = None

    slotter.asyncConnect(ASYNC_SLOTTESTER, "signal", "NOT_A_slot",
                         connectCallback)
    sleepUntil(lambda: failureDetails is not None, timeout)

    assert failureMsg is not None  # callback called
    assert "no slot 'NOT_A_slot'" in failureMsg
    # Details contain the same, but also C++ exception etails
    assert "no slot 'NOT_A_slot'" in failureDetails
    assert "Exception Type....:  SignalSlot Exception" in failureDetails

    #################################################################
    # Now asyncronously disconnect
    failureMsg = failureDetails = None

    slotter.asyncDisconnect(ASYNC_SLOTTESTER, "signal", "slot",
                            connectCallback)
    sleepUntil(lambda: failureDetails is not None, timeout)

    assert failureMsg is not None  # callback called
    assert failureMsg == ""  # successfully disconnected

    slotCalled = False
    slotter.request(
        ASYNC_SLOTTESTER, "slotEmitSignal", 77).waitForReply(timeoutMs)

    # wait only a bit (0.1 seconds) for something not coming
    sleep(0.1)
    assert not slotCalled

    #################################################################
    # Finally a failing asyncDisconnect
    failureMsg = failureDetails = None

    slotter.asyncDisconnect(ASYNC_SLOTTESTER, "signal", "not_a_slot",
                            connectCallback)
    sleepUntil(lambda: failureDetails is not None, timeout)

    assert failureMsg is not None  # callback called
    assert "no slot 'not_a_slot'" in failureMsg
    assert "no slot 'not_a_slot'" in failureDetails
    assert "Exception Type....:  SignalSlot Exception" in failureDetails


def test_autoConnect_sync(connectTest):
    """
    Test automatic (re-)connect (after synchronous connect)
    (Even tests this for C++ where all tests run in a single process!)
    """
    # Note: We do not need the devices that are already started by connectTest

    # Create an instance with a slot that it can connect to the signal
    slotter = SignalSlotable("slotInstance_syncReconnect")
    slotCalled = False
    inSlot = None

    def slotFunc(i):
        nonlocal slotCalled, inSlot
        inSlot = i
        slotCalled = True

    slotter.registerSlot(slotFunc, "slot")
    slotter.start()

    otherId = "sigSlotReconnectTester_sync"

    # Other still offline - but subscription on broker succeeds
    connected = slotter.connect(otherId, "signal", "slot")
    assert connected

    # Now start the other
    cfg = Hash("classId", "SignalDevice", "deviceId", otherId,
               "configuration", Hash())
    slotter.request(SERVER_ID, "slotStartDevice", cfg).waitForReply(timeoutMs)

    # Now test that we receive other's emitted signal
    slotter.request(
        otherId, "slotEmitSignal", 42).waitForReply(timeoutMs)

    sleepUntil(lambda: slotCalled, timeout=timeout)

    assert slotCalled
    assert 42 == inSlot

    slotter.request(otherId, "slotKillDevice").waitForReply(timeoutMs)


def test_autoConnect_async(connectTest):
    """
    Test automatic (re-)connect (after asynchronous connect)
    (Even tests this for C++ where all tests run in a single process!)
    """
    # Note: We do not need the devices that are already started by connectTest

    # Create an instance with a slot that it can connect to the signal
    slotter = SignalSlotable("slotInstance_asyncReconnect")
    slotCalled = False
    inSlot = None

    def slotFunc(i):
        nonlocal slotCalled, inSlot
        inSlot = i
        slotCalled = True

    slotter.registerSlot(slotFunc, "slot")
    slotter.start()

    otherId = "sigSlotReconnectTester_async"

    # Other still offline - but subscription on broker succeeds
    failureMsg = None

    def connectCallback(failMsg, failDetails):
        nonlocal failureMsg
        failureMsg = failMsg

    slotter.asyncConnect(
        otherId, "signal", "slot", connectCallback)
    sleepUntil(lambda: failureMsg is not None, timeout)

    assert failureMsg is not None  # callback called
    assert failureMsg == ""  # connect succeeded

    # Now start the other
    cfg = Hash("classId", "SignalDevice", "deviceId", otherId,
               "configuration", Hash())
    slotter.request(SERVER_ID, "slotStartDevice", cfg).waitForReply(timeoutMs)

    # Now test that we receive other's emitted signal
    slotter.request(
        otherId, "slotEmitSignal", 42).waitForReply(timeoutMs)

    sleepUntil(lambda: slotCalled, timeout=timeout)

    assert slotCalled
    assert 42 == inSlot

    slotter.request(otherId, "slotKillDevice").waitForReply(timeoutMs)
