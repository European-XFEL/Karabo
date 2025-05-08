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

from karabo.bound import Hash, SignalSlotable
from karabo.bound.testing import BoundDeviceTestCase

instTimeout = 30
instTimeoutMs = instTimeout * 1000

timeout = 5
timeoutMs = timeout * 1000


class TestSigSlotSignalConnect(BoundDeviceTestCase):

    def test_connect(self):
        """
        Test SignalSlotable.connect and disconnect between instances
        in different processes.
        (Even tests this for C++ where all tests run in a single process!)
        """
        # Create server to instantiate device that can emit a signal
        SERVER_ID = "sigSlotConnectServer"
        self.start_server("bound", SERVER_ID, ["SignalDevice"],
                          namespace="karabo.bound_device_test")
        # Instantiate device that can emit a signal
        deviceId = "sigSlotConnectTester"
        cfg = Hash("classId", "SignalDevice",
                   "deviceId", deviceId,
                   "configuration", Hash())
        ok, msg = self.dc.instantiate(SERVER_ID, cfg, instTimeout)
        self.assertTrue(ok, msg)

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
        connected = slotter.connect(deviceId, "signal", "slot")
        self.assertTrue(connected)

        slotter.request(deviceId, "slotEmitSignal", 42).waitForReply(timeoutMs)

        self.waitUntilTrue(lambda: slotCalled, timeout)

        self.assertTrue(slotCalled)
        self.assertEqual(42, inSlot)

        ##################################################
        # Test handling of failures: non-existing slot
        connected = slotter.connect(deviceId, "signal", "NOT_A_slot")
        self.assertFalse(connected)

        #################################################################
        # Now disconnect
        disconnected = slotter.disconnect(deviceId, "signal", "slot")
        self.assertTrue(disconnected)

        slotCalled = False
        slotter.request(deviceId, "slotEmitSignal", 77).waitForReply(timeoutMs)

        # wait only a bit (0.1 seconds) for something not coming
        self.waitUntilTrue(lambda: slotCalled, 0.1)
        self.assertFalse(slotCalled)

        #################################################################
        # Finally a failing disconnect
        disconnected = slotter.disconnect(deviceId, "signal", "not_a_slot")
        self.assertFalse(disconnected)

    def test_asyncConnect(self):
        """
        Test SignalSlotable.asyncConnect and asyncDisconnect between instances
        in different processes.
        (Even tests this for C++ where all tests run in a single process!)
        Very similar to 'test_sigslot_asyncConnect' from
        karabo/bound/tests/binding/test_sigslot.py,
        but that tests in-process short-cut.
        """
        # Create server and instantiate device that can emit a signal
        SERVER_ID = "sigSlotAsyncConnectServer"
        deviceId = "sigSlotAsyncConnectTester"
        self.start_server("bound", SERVER_ID, ["SignalDevice"],
                          namespace="karabo.bound_device_test")

        cfg = Hash("classId", "SignalDevice",
                   "deviceId", deviceId,
                   "configuration", Hash())
        ok, msg = self.dc.instantiate(SERVER_ID, cfg, instTimeout)
        self.assertTrue(ok, msg)

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

        slotter.asyncConnect(deviceId, "signal", "slot", connectCallback)
        self.waitUntilTrue(lambda: failureDetails is not None, timeout)

        self.assertIsNotNone(failureMsg)  # callback called
        self.assertEqual(failureMsg, "")  # connect succeeded

        slotter.request(deviceId, "slotEmitSignal", 42).waitForReply(timeoutMs)

        self.waitUntilTrue(lambda: slotCalled, timeout)

        self.assertTrue(slotCalled)
        self.assertEqual(42, inSlot)

        ##################################################
        # Test handling of failures

        # Non-existing slot gives failure
        failureMsg = failureDetails = None

        slotter.asyncConnect(deviceId, "signal", "NOT_A_slot",
                             connectCallback)
        self.waitUntilTrue(lambda: failureDetails is not None, timeout)

        self.assertIsNotNone(failureMsg)  # callback called
        self.assertIn("no slot 'NOT_A_slot'", failureMsg)
        # Details contain the same, but also C++ exception etails
        self.assertIn("no slot 'NOT_A_slot'", failureDetails)
        self.assertIn("Exception Type....:  SignalSlot Exception",
                      failureDetails)

        #################################################################
        # Now asyncronously disconnect
        failureMsg = failureDetails = None

        slotter.asyncDisconnect(deviceId, "signal", "slot",
                                connectCallback)
        self.waitUntilTrue(lambda: failureDetails is not None, timeout)

        self.assertIsNotNone(failureMsg)  # callback called
        self.assertEqual(failureMsg, "")  # successfully disconnected

        slotCalled = False
        slotter.request(deviceId, "slotEmitSignal", 77).waitForReply(timeoutMs)

        # wait only a bit (0.1 seconds) for something not coming
        self.waitUntilTrue(lambda: slotCalled, 0.1)
        self.assertFalse(slotCalled)

        #################################################################
        # Finally a failing asyncDisconnect
        failureMsg = failureDetails = None

        slotter.asyncDisconnect(deviceId, "signal", "not_a_slot",
                                connectCallback)
        self.waitUntilTrue(lambda: failureDetails is not None, timeout)

        self.assertIsNotNone(failureMsg)  # callback called
        self.assertIn("was not connected", failureMsg)
        self.assertIn("was not connected", failureDetails)
        self.assertIn("Exception Type....:  SignalSlot Exception",
                      failureDetails)
