from karabo.integration_tests.utils import BoundDeviceTestCase


from karabo.bound import (
    Hash, Schema,
    INT32_ELEMENT,
    INPUT_CHANNEL,
    NODE_ELEMENT,
    OUTPUT_CHANNEL,
    OVERWRITE_ELEMENT,
    SignalSlotable
)

import os.path as op
import threading

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
        own_dir = op.dirname(op.abspath(__file__))
        self.start_server("bound", self.server_id, [self.devClass],
                          plugin_dir=own_dir)  # , logLevel="INFO")

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
        ele = INPUT_CHANNEL(schema).key("injectedInput").dataSchema(dataSchema)
        ele.commit()
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
                if (len(tableSt) == 1 and len(tableIn) == 1
                    and tableSt[0].get("remoteId") == f"{dev_id}:injectedInput"
                    and tableIn[0].get("remoteId") == f"{dev_id}:injectedInput"
                    and cfg.has("node.anInt32")):
                        return True
            return False

        res = self.waitUntilTrue(condition, max_timeout, 100)
        cfg = self.dc.get(dev_id)
        self.assertTrue(res, str(cfg))
        # Not channel related - injected element under existing note is there:
        self.assertTrue(cfg.has("node"))
        self.assertTrue(cfg.has("node.anInt32"), str(cfg))

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
        receiver_id = "receiver"
        cfg = Hash("deviceId", receiver_id,
                   "input.connectedOutputChannels", dev_id + ":output")
        ok, msg = self.dc.instantiate(self.server_id, self.devClass, cfg,
                                      max_timeout)
        self.assertTrue(ok,
                        "Could not start device '{}' on server '{}': '{}'."
                        .format(self.devClass, self.server_id, msg))

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
                                         "", slotConnectionChanged.__name__)
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

        for schema, triggerReconnect in schemasToInject:

            req = self.sigSlot.request(dev_id, updateSlot, schema)
            req.waitForReply(max_timeout_ms)

            # If output channel schema changed, we expect that the channel was
            # recreated and and thus the InputChannel of the receiver was
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
                self.assertEqual(res, triggerReconnect, str(connectionChanges))

                if triggerReconnect:
                    self.assertEqual(2, len(connectionChanges),
                                     str(connectionChanges))
                    self.assertEqual(connectionChanges[0],
                                     [dev_id + ":output"])
                    self.assertEqual(connectionChanges[1], [])

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
                    self.assertTrue(res, str(connectionChanges))
                    self.assertEqual(len(connectionChanges), 4,
                                     str(connectionChanges))
                    self.assertEqual(connectionChanges[2],
                                     [dev_id + ":output"])
                    self.assertEqual(connectionChanges[3], [])

            with connectionChangesLock:
                connectionChanges.clear()

        # Clean up
        self.sigSlot.disconnect(receiver_id, "signalChanged",
                                "", slotConnectionChanged.__name__)
        # Cannot remove slotConnectionChanged...
        ok, msg = self.dc.killDevice(receiver_id, max_timeout)
        self.assertTrue(ok, msg)
