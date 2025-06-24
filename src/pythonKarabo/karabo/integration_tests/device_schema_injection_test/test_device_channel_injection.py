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
import threading
import time

from karabo.bound import (
    INPUT_CHANNEL, INT32_ELEMENT, NODE_ELEMENT, OUTPUT_CHANNEL,
    OVERWRITE_ELEMENT, Hash, Schema, SignalSlotable)
from karabo.bound.testing import BoundDeviceTestCase

max_timeout = 20    # in seconds
max_timeout_ms = max_timeout * 1000


class Channel_Injection_TestCase(BoundDeviceTestCase):

    sigSlot = None
    server_id = None
    devClass = None

    def test_channel_injection(self):
        # test that injected input/output channels automatically get connected

        self.devClass = "DeviceChannelInjection"
        self.server_id = "server/channelInjection"
        self.start_server("bound", self.server_id, [self.devClass],
                          # , logLevel="INFO"
                          namespace="karabo.bound_device_test")

        dev_id = "device/channelInjection"
        cfg = Hash("deviceId", dev_id)

        ok, msg = self.dc.instantiate(self.server_id, self.devClass, cfg,
                                      max_timeout)
        self.assertTrue(ok,
                        "Could not start device '{}' on server '{}': '{}'."
                        .format(self.devClass, self.server_id, msg))

        if self.sigSlot is None:
            self.sigSlot = SignalSlotable("sigSlot")
            self.sigSlot.start()

        with self.subTest(msg="channel injection - slotUpdateSchema"):
            self._test_channel_injection(dev_id, "slotUpdateSchema")

        with self.subTest(msg="channel injection - slotAppendSchema"):
            self._test_channel_injection(dev_id, "slotAppendSchema")

        with self.subTest(msg="change output schema  - slotUpdateSchema"):
            self._test_change_output_schema(dev_id, "slotUpdateSchema")

        with self.subTest(msg="change output schema  - slotAppendSchema"):
            self._test_change_output_schema(dev_id, "slotAppendSchema")

    def _test_channel_injection(self, dev_id, updateSlot):
        # Test that input and output channels are created if
        # part of injected schema
        req = self.sigSlot.request(dev_id, "slotGetOutputChannelNames")
        (outnames,) = req.waitForReply(max_timeout_ms)
        self.assertEqual(1, len(outnames))
        self.assertEqual("output", outnames[0])

        # Checks that injected input and output channels are created
        dataSchema = Schema()
        INT32_ELEMENT(dataSchema).key("int32").readOnly().commit()

        schema = Schema()
        ele = OUTPUT_CHANNEL(schema).key("injectedOutput")
        ele.dataSchema(dataSchema).commit()
        INPUT_CHANNEL(schema).key("injectedInput").commit()
        ele = OVERWRITE_ELEMENT(schema)
        ele.key("injectedInput.connectedOutputChannels")
        ele.setNewDefaultValue([dev_id + ":injectedOutput",
                                dev_id + ":output"]).commit()
        NODE_ELEMENT(schema).key("node").commit()
        ele = INT32_ELEMENT(schema).key("node.anInt32").readOnly()
        ele.initialValue(42).commit()

        req = self.sigSlot.request(dev_id, updateSlot, schema)
        req.waitForReply(max_timeout_ms)

        # Now, also the injectedOutput is there:
        req = self.sigSlot.request(dev_id, "slotGetOutputChannelNames")
        (outputChannels,) = req.waitForReply(max_timeout_ms)

        self.assertEqual(len(outputChannels), 2)
        self.assertIn("output", outputChannels)
        self.assertIn("injectedOutput", outputChannels)

        # Check that, after some time, the injected input is connected to both,
        # the injected and the static output
        def condition():
            cfg = self.dc.get(dev_id)
            # Check both output channel connection tables for injected input
            if (cfg.has("output.connections")
                    and cfg.has("injectedOutput.connections")):

                tableSt = cfg.get("output.connections")
                tableIn = cfg.get("injectedOutput.connections")
                expected_id = f"{dev_id}:injectedInput"
                if (len(tableSt) == 1 and len(tableIn) == 1
                        and tableSt[0].get("remoteId") == expected_id
                        and tableIn[0].get("remoteId") == expected_id
                        and cfg.has("node.anInt32")):
                    return True
            return False

        res = self.waitUntilTrue(condition, max_timeout, 100)
        cfg = self.dc.get(dev_id)
        self.assertTrue(res, str(cfg))
        # Not channel related - injected element under existing node is there:
        self.assertTrue(cfg.has("node"))
        self.assertTrue(cfg.has("node.anInt32"), str(cfg))

        # Now START test that re-injecting an input channel keeps handlers
        # registered with KARABO_ON_DATA/KARABO_ON_INPUT/KARABO_ON_EOS.
        # Register data handler for "injectedInput" channel
        req = self.sigSlot.request(dev_id, "slotRegisterOnDataInputEos",
                                   "injectedInput")
        req.waitForReply(max_timeout_ms)
        # Check that initially "intInOnData" is not one, i.e. ensure that
        # following actions will make it one. (It is either zero [initial
        # value] or -3 [from previous run of this test with other updateSlot].)
        self.assertTrue(1 != self.dc.get(dev_id, "intInOnData"))
        countEos = self.dc.get(dev_id, "numCallsOnInput")

        # Request data to be sent from "output" to "injectedInput" channel
        req = self.sigSlot.request(dev_id, "slotWriteOutput", Hash("int", 1))
        req.waitForReply(max_timeout_ms)

        # Check that data arrived and onData/onInput handlers called
        res = self.waitUntilTrue(
            lambda: (self.dc.get(dev_id, "intInOnData") == 1 and
                     self.dc.get(dev_id, "numCallsOnInput") == countEos + 1),
            max_timeout, 100)
        self.assertEqual(self.dc.get(dev_id, "intInOnData"), 1)
        self.assertEqual(self.dc.get(dev_id, "numCallsOnInput"), countEos + 1)

        # Request EOS to be sent to "injectedInput" channel.
        # All outputs that an input is connected to have to send EOS to get
        # the eos handler called...
        req = self.sigSlot.request(dev_id, "slotSendEos",
                                   ["output", "injectedOutput"])
        req.waitForReply(max_timeout_ms)

        # Check that EOS arrived and flipped sign
        res = self.waitUntilTrue(
            lambda: self.dc.get(dev_id, "intInOnData") == -1,
            max_timeout, 100)
        self.assertEqual(self.dc.get(dev_id, "intInOnData"), -1)

        # Re-inject input - channel will be recreated and onData handler should
        # be passed to new incarnation
        schema = Schema()
        INPUT_CHANNEL(schema).key("injectedInput").commit()
        # Note that here we need to use "slotAppendSchema" and not updateSlot
        # since "slotUpdateSchema" would erase "injectedOutput", and then
        # slotSendEos would need a different list of output channels.
        req = self.sigSlot.request(dev_id, "slotAppendSchema", schema)
        req.waitForReply(max_timeout_ms)
        # Wait for connection being re-established
        # HACK: Without sleep might be fooled, i.e. traces of connection of
        #       previous input channel not yet erased...
        time.sleep(1)
        self.waitUntilTrue(condition, max_timeout, 100)
        self.assertTrue(res, str(self.dc.get(dev_id)))
        # Request again data sending from "output" to "injectedInput" channel
        req = self.sigSlot.request(dev_id, "slotWriteOutput", Hash("int", 2))
        req.waitForReply(max_timeout_ms)

        # Check that new data arrived
        self.waitUntilTrue(
            lambda: (self.dc.get(dev_id, "intInOnData") == 2 and
                     self.dc.get(dev_id, "numCallsOnInput") == countEos + 2),
            max_timeout, 100)
        self.assertEqual(self.dc.get(dev_id, "intInOnData"), 2)
        self.assertEqual(self.dc.get(dev_id, "numCallsOnInput"),
                         countEos + 2)

        # Request EOS to be sent again
        req = self.sigSlot.request(dev_id, "slotSendEos",
                                   ["output", "injectedOutput"])
        req.waitForReply(max_timeout_ms)
        # Check that EOS arrived and flipped sign again
        res = self.waitUntilTrue(
            lambda: self.dc.get(dev_id, "intInOnData") == -2,
            max_timeout, 100)
        self.assertEqual(self.dc.get(dev_id, "intInOnData"), -2)
        # END test that re-injecting input channels keeps handlers registered
        #     with KARABO_ON_DATA/KARABO_ON_INPUT/KARABO_ON_EOS!

        # Test that re-injection of input channel works even during input
        # channel data handling (crashed in past due to ref-counting bugs):
        sch = Schema()
        INPUT_CHANNEL(sch).key("injectedInput").commit()
        new = "new_" + updateSlot  # work around: dc cash keeps removed props
        INT32_ELEMENT(sch).key(new).readOnly().initialValue(77).commit()
        data = Hash("schema", sch, "int", 3)
        req = self.sigSlot.request(dev_id, "slotWriteOutput", data)
        req.waitForReply(max_timeout_ms)
        # See that injection happened...
        self.waitUntilTrue(
            lambda: (new in self.dc.get(dev_id)
                     and self.dc.get(dev_id, new) == 77
                     and self.dc.get(dev_id, "intInOnData") == 3),
            max_timeout, 100)
        self.assertEqual(self.dc.get(dev_id, new), 77)
        self.assertEqual(self.dc.get(dev_id, "intInOnData"), 3)
        # ...and that channels are connected again...
        res = self.waitUntilTrue(condition, max_timeout, 100)
        self.assertTrue(res, str(self.dc.get(dev_id)))
        # ... and that data can go through it, i.e. handlers are re-assigned
        req = self.sigSlot.request(dev_id, "slotSendEos",  # check EOS only...
                                   ["output", "injectedOutput"])
        req.waitForReply(max_timeout_ms)
        # Check that EOS arrived and flipped sign again
        res = self.waitUntilTrue(
            lambda: self.dc.get(dev_id, "intInOnData") == -3,
            max_timeout, 100)
        self.assertEqual(self.dc.get(dev_id, "intInOnData"), -3)

        # Remove the channels again:
        req = self.sigSlot.request(dev_id, "slotUpdateSchema", Schema())
        req.waitForReply(max_timeout_ms)

        # Now only the static OutputChannel is kept
        req = self.sigSlot.request(dev_id, "slotGetOutputChannelNames")
        (outnames,) = req.waitForReply(max_timeout_ms)
        self.assertEqual(1, len(outnames))
        self.assertEqual("output", outnames[0])

        # Verify also that the injected channels are gone from config
        # TODO: We directly call slotGetConfiguration instead of using self.dc
        #       Looks like the client cache will not erase removed properties.
        #    cfg = self.dc.get(dev_id)
        req = self.sigSlot.request(dev_id, "slotGetConfiguration")
        (cfg, _) = req.waitForReply(max_timeout_ms)
        self.assertFalse(cfg.has("injectedOutput"), str(cfg))
        self.assertFalse(cfg.has("injectedInput"), str(cfg))
        # Not channel related - empty 'node' kept, but injected anInt32 not:
        self.assertFalse(cfg.has("node.anInt32"), str(cfg))
        self.assertTrue(cfg.has("node"), str(cfg))

    def _test_change_output_schema(self, dev_id, updateSlot):
        # test that touching the output channel schema forces the channel to
        # be recreated, i.e. reconnect
        # Use unique id in case other case fails and thus does not kill device:
        receiver_id = "receiver_" + updateSlot
        cfg = Hash("deviceId", receiver_id,
                   "input.connectedOutputChannels", dev_id + ":output")
        ok, msg = self.dc.instantiate(self.server_id, self.devClass, cfg,
                                      max_timeout)
        self.assertTrue(ok,
                        "Could not start device '{}' on server '{}': '{}'."
                        .format(self.devClass, self.server_id, msg))
        # Ensure that input channel is connected before 'slotConnectionChanged'
        # below tracks further connection changes
        ok = self.waitUntilTrue(
            lambda: len(self.dc.get(dev_id, "input.missingConnections")) == 0,
            max_timeout, 100)
        self.assertTrue(ok,
                        str(self.dc.get(dev_id, "input.missingConnections")))

        connectionChangesLock = threading.Lock()
        connectionChanges = []

        def slotConnectionChanged(h, inst_id):
            if inst_id == receiver_id and h.has("input.missingConnections"):
                with connectionChangesLock:
                    connectionChanges.append(h.get("input.missingConnections"))

        # Note: Better create independent slots for each test run to avoid that
        #       in second run same slot has two functions registered
        slotConnectionChanged.__name__ += updateSlot  # used by registerSlot
        self.sigSlot.registerSlot(slotConnectionChanged)
        connected = self.sigSlot.connect(receiver_id, "signalChanged",
                                         slotConnectionChanged.__name__)
        self.assertTrue(connected)

        # Create several schema injections that should trigger output channel
        # reconnection (or not).
        # The Boolean tells whether "output" channel is recreated (and thus
        # reconnection happens) when injected and when injection is removed by
        # updating with an empty Schema.
        schemasToInject = []
        # Schema where OUTPUT_CHANNEL is explicitly changed
        schema1 = Schema()
        dataSchema = Schema()
        INT32_ELEMENT(dataSchema).key("injectedInt32").readOnly().commit()
        OUTPUT_CHANNEL(schema1).key("output").dataSchema(dataSchema).commit()
        schemasToInject.append((schema1, True))

        # Schema where output schema is changed silently, i.e. w/o mentioning
        # OUTPUT_CHANNEL
        schema2 = Schema()
        NODE_ELEMENT(schema2).key("output").commit()
        NODE_ELEMENT(schema2).key("output.schema").commit()
        ele = INT32_ELEMENT(schema2).key("output.schema.injectedInt32")
        ele.readOnly().commit()
        schemasToInject.append((schema2, True))

        # Schema where something else changed - channel is untouched
        schema3 = Schema()
        ele = INT32_ELEMENT(schema3).key("injectedUnrelated")
        ele.assignmentOptional().defaultValue(1).reconfigurable().commit()
        schemasToInject.append((schema3, False))

        for i, (schema, triggerReconnect) in enumerate(schemasToInject):

            req = self.sigSlot.request(dev_id, updateSlot, schema)
            req.waitForReply(max_timeout_ms)

            # If output channel schema changed, we expect that the channel was
            # recreated and thus the InputChannel of the receiver was
            # disconnected and reconnected. Both should trigger a change of the
            # input channel's missingConnections property which should trigger
            # a call to our "injected" slot that is connected to
            # 'signalChanged'.
            # If triggerReconnect is false, nothing such happens and we run
            # into the timeout :-(.
            def condition():
                with connectionChangesLock:
                    return len(connectionChanges) >= 2

            res = self.waitUntilTrue(condition, max_timeout, 100)
            with connectionChangesLock:
                debugMsg = f"Scenario {i}: {str(connectionChanges)}"
                self.assertEqual(res, triggerReconnect, debugMsg)

                if triggerReconnect:
                    self.assertEqual(2, len(connectionChanges), debugMsg)
                    self.assertEqual(connectionChanges[0],
                                     [dev_id + ":output"], debugMsg)
                    self.assertEqual(connectionChanges[1], [], debugMsg)

            req = self.sigSlot.request(dev_id, "slotUpdateSchema", Schema())
            req.waitForReply(max_timeout_ms)

            if triggerReconnect:
                # If schema changed in the first place, it changes back now and
                # thus has to reconnect
                def condition2():
                    with connectionChangesLock:
                        return len(connectionChanges) >= 4  # two more now

                res = self.waitUntilTrue(condition2, max_timeout, 100)
                with connectionChangesLock:
                    debugMsg = f"Scenario {i}: {str(connectionChanges)}"
                    self.assertTrue(res, debugMsg)
                    self.assertEqual(len(connectionChanges), 4, debugMsg)
                    self.assertEqual(connectionChanges[2],
                                     [dev_id + ":output"], debugMsg)
                    self.assertEqual(connectionChanges[3], [], debugMsg)

            with connectionChangesLock:
                connectionChanges.clear()

        # Clean up
        self.sigSlot.disconnect(receiver_id, "signalChanged",
                                slotConnectionChanged.__name__)
        # Cannot remove slotConnectionChanged...
        ok, msg = self.dc.killDevice(receiver_id, max_timeout)
        self.assertTrue(ok, msg)
