from asyncio import Future, sleep, wait_for
from contextlib import contextmanager
from unittest import main

from karabo.middlelayer import (
    AccessMode, AccessLevel, Assignment,  Bool, call, Configurable, coslot,
    Device, getDevice, Hash, isAlive, InputChannel, Int32, Overwrite,
    OutputChannel, setWait, Slot, State, Timestamp, UInt32,
    waitUntil)
from .eventloop import DeviceTest, async_tst

FIXED_TIMESTAMP = Timestamp("2009-04-20T10:32:22 UTC")


def get_channel_node(displayed_name=""):
    class ChannelNode(Configurable):
        data = Int32(
            displayedName=displayed_name,
            defaultValue=0)

    return ChannelNode


class Sender(Device):
    # The state is explicitly overwritten, State.UNKNOWN is always possible by
    # default! We test that a proxy can reach State.UNKNOWN even if it is
    # removed from the allowed options.

    state = Overwrite(options=[State.ON])
    useTimestamp = Bool(
        defaultValue=False)

    output = OutputChannel(
        get_channel_node(),
        assignment=Assignment.OPTIONAL,
        requiredAccessLevel=AccessLevel.OPERATOR,
        accessMode=AccessMode.READONLY)

    outputCounter = UInt32(
        defaultValue=0,
        displayedName="Output Counter")

    @Slot()
    async def sendData(self):
        self.output.schema.data = self.output.schema.data.value + 1
        timestamp = FIXED_TIMESTAMP if self.useTimestamp else Timestamp()
        self.outputCounter = self.outputCounter.value + 1
        await self.output.writeData(timestamp=timestamp)

    @Slot()
    async def sendEndOfStream(self):
        await self.output.writeEndOfStream()

    @Slot()
    async def resetCounter(self):
        self.outputCounter = 0

    async def changeSchema(self, displayed):
        schema = get_channel_node(displayed)
        await self.setOutputSchema("output", schema)

    def __init__(self, configuration):
        super().__init__(configuration)


class Receiver(Device):
    received = UInt32(
        defaultValue=0,
        displayedName="Received Packets")

    connected = Bool(
        defaultValue=False)

    closed = Bool(
        defaultValue=False)

    eosReceived = Bool(
        defaultValue=False)

    def __init__(self, configuration):
        super().__init__(configuration)

    @coslot
    async def connectInputChannel(self, output="alice"):
        await self.input.connectChannel(f"{output}:output")
        return True

    @Slot()
    async def resetCounter(self):
        self.received = 0

    # close the input channel `name`
    @InputChannel(raw=True)
    async def input(self, data, meta):
        self.received = self.received.value + 1

    @input.connect
    async def input(self, name):
        self.connected = True

    @input.close
    async def input(self, name):
        self.closed = True

    @input.endOfStream
    async def input(self, name):
        self.eosReceived = True

    async def onInitialization(self):
        self.state = State.ON


class RemotePipelineTest(DeviceTest):

    @classmethod
    @contextmanager
    def lifetimeManager(cls):
        cls.alice = Sender({"_deviceId_": "alice"})
        cls.bob = Receiver({"_deviceId_": "bob",
                            "input": {"dataDistribution": "copy",
                                      "onSlowness": "drop"}})
        with cls.deviceManager(cls.bob, lead=cls.alice):
            yield

    @async_tst
    def test_configurationAsHash(self):
        """Test the empty Hash of the output schema"""
        conf = self.alice.configurationAsHash()
        self.assertEqual(conf['output.schema'], Hash())

    @async_tst
    async def test_input_output(self):
        """Test the input and output channel connection"""
        charlie = Receiver({"_deviceId_": "charlie",
                            "input": {"dataDistribution": "shared"}})

        await charlie.startInstance()
        ret = await call("bob", "connectInputChannel")
        self.assertTrue(ret)
        ret = await call("charlie", "connectInputChannel")
        self.assertTrue(ret)
        await sleep(1)
        channels = self.bob.input.connectedOutputChannels.value
        self.assertIn("alice:output", channels)
        channels = charlie.input.connectedOutputChannels.value
        self.assertIn("alice:output", channels)
        # Charlie is new and we check the handler
        self.assertEqual(charlie.connected.value, True)

        self.assertEqual(self.bob.input.onSlowness, "drop")
        self.assertEqual(self.bob.input.dataDistribution, "copy")
        self.assertEqual(charlie.input.dataDistribution, "shared")

        proxy = await getDevice("alice")
        with proxy:
            self.assertEqual(self.bob.received, 0)
            self.assertEqual(charlie.received, 0)
            await proxy.sendData()
            await waitUntil(lambda: self.bob.received == 1)
            await waitUntil(lambda: charlie.received == 1)
            self.assertEqual(charlie.received, 1)
            self.assertEqual(self.bob.received, 1)
            await proxy.sendEndOfStream()
            await waitUntil(lambda: self.bob.eosReceived.value is True)
            await waitUntil(lambda: charlie.eosReceived.value is True)
            self.assertEqual(self.bob.eosReceived.value, True)
            self.assertEqual(charlie.eosReceived.value, True)
            await proxy.sendData()
            await waitUntil(lambda: self.bob.received == 2)
            await waitUntil(lambda: charlie.received == 2)
            self.assertEqual(self.bob.received, 2)
            self.assertEqual(charlie.received, 2)
            # Shutdown the shared channel. The queue gets removed and
            # we test that we are not blocked.
            await charlie.slotKillDevice()

            await proxy.sendData()
            await waitUntil(lambda: self.bob.received == 3)
            self.assertEqual(self.bob.received, 3)

    @async_tst
    async def test_output_reconnect(self):
        """Test the output reconnect of a proxy"""
        NUM_DATA = 5
        output_device = Sender({"_deviceId_": "outputdevice"})
        await output_device.startInstance()

        with (await getDevice("outputdevice")) as proxy:
            self.assertTrue(isAlive(proxy))
            received = False
            connected = False

            def connect_handler(channel_name):
                nonlocal connected
                connected = True

            def handler(data, meta):
                """Output handler to see if we received data
                """
                nonlocal received
                received = True

            self.assertEqual(received, False)
            # Patch the handler to see if our boolean triggers
            proxy.output.setConnectHandler(connect_handler)
            proxy.output.setDataHandler(handler)
            proxy.output.connect()
            # Send more often as our proxy has a drop setting and we are busy
            # in tests.
            for data in range(NUM_DATA):
                await proxy.sendData()
            self.assertEqual(received, True)
            self.assertEqual(connected, True)
            # We received data and now kill the device
            await output_device.slotKillDevice()
            await waitUntil(lambda: not isAlive(proxy))
            self.assertFalse(isAlive(proxy))
            # The device is gone, now we instantiate the device with same
            # deviceId to see if the output automatically reconnects
            output_device = Sender({"_deviceId_": "outputdevice"})
            await output_device.startInstance()
            await waitUntil(lambda: isAlive(proxy))
            self.assertEqual(received, True)
            # Set the received and connected to false!
            received = False
            connected = False
            self.assertEqual(connected, False)
            self.assertEqual(received, False)
            # Wait a few seconds because the reconnect will wait seconds
            # as well before attempt
            await sleep(2)
            for data in range(NUM_DATA):
                await proxy.sendData()
            # Our reconnect was successful, we are receiving data via the
            # output channel
            self.assertEqual(received, True)
            self.assertEqual(connected, True)

            received = False

        self.assertEqual(received, False)
        # Delete our proxy and see if we still receive data!
        del proxy
        await output_device.slotKillDevice()
        await sleep(1)

        # Bring up our device with same deviceId, we should not have a
        # channel active with the handler
        output_device = Sender({"_deviceId_": "outputdevice"})
        await output_device.startInstance()
        with (await getDevice("outputdevice")) as proxy:
            self.assertTrue(isAlive(proxy))
            self.assertEqual(received, False)
            for data in range(NUM_DATA):
                await proxy.sendData()
            self.assertEqual(received, False)

        await output_device.slotKillDevice()

    @async_tst
    async def test_output_reconnect_device(self):
        """Test the output reconnect of a device"""
        output_device = Sender({"_deviceId_": "outputdevice"})
        await output_device.startInstance()
        receiver = Receiver({"_deviceId_": "receiverdevice"})
        await receiver.startInstance()
        await receiver.connectInputChannel("outputdevice")

        NUM_DATA = 5
        with (await getDevice("outputdevice")) as proxy:
            self.assertTrue(isAlive(proxy))
            self.assertEqual(receiver.received, 0)
            for data in range(NUM_DATA):
                await proxy.sendData()
            self.assertGreater(receiver.received, 0)
            # We received data and now kill the device
            await output_device.slotKillDevice()
            await waitUntil(lambda: not isAlive(proxy))  # noqa
            self.assertFalse(isAlive(proxy))
            # The device is gone, now we instantiate the device with same
            # deviceId to see if the output automatically reconnects
            output_device = Sender({"_deviceId_": "outputdevice"})
            await output_device.startInstance()
            await waitUntil(lambda: isAlive(proxy))  # noqa

            # We set the counter to zero!
            await receiver.resetCounter()
            self.assertEqual(receiver.received, 0)
            # Wait a few seconds because the reconnect will wait seconds
            # as well before attempt
            await sleep(2)
            for data in range(NUM_DATA):
                await proxy.sendData()
            # Our reconnect was successful, we are receiving data via the
            # output channel
            self.assertGreater(receiver.received, 0)

        del proxy
        await output_device.slotKillDevice()
        await receiver.slotKillDevice()

    @async_tst
    async def test_output_change_schema(self):
        """Test the output schema change of a device"""
        output_device = Sender({"_deviceId_": "outputdevice"})
        await output_device.startInstance()
        receiver = Receiver({"_deviceId_": "receiverdevice"})
        await receiver.startInstance()
        await receiver.connectInputChannel("outputdevice")

        NUM_DATA = 5
        with (await getDevice("outputdevice")) as proxy:
            self.assertTrue(isAlive(proxy))
            self.assertEqual(receiver.received, 0)
            for data in range(NUM_DATA):
                await proxy.sendData()
            self.assertGreater(receiver.received, 0)
            # We received data and now change the schema
            data_desc = output_device.output.schema.data.descriptor
            self.assertEqual(data_desc.displayedName, "")
            await output_device.changeSchema("TestDisplayed")
            # We set the counter to zero!
            await receiver.resetCounter()
            # Changing schema closes output channel, hence we wait
            # a few seconds because the reconnect will wait seconds
            # as well before attempt
            await sleep(2)
            data_desc = output_device.output.schema.data.descriptor
            self.assertEqual(data_desc.displayedName, "TestDisplayed")
            self.assertEqual(receiver.received, 0)
            for data in range(NUM_DATA):
                await proxy.sendData()
            # Our reconnect was successful, we are receiving data via the
            # output channel
            self.assertGreater(receiver.received, 0)

        del proxy
        await output_device.slotKillDevice()
        await receiver.slotKillDevice()

    @async_tst
    async def test_output_timestamp(self):
        """Test the meta data timestamp of a proxy"""
        NUM_DATA = 5
        output_device = Sender({"_deviceId_": "outputdevice"})
        await output_device.startInstance()

        with (await getDevice("outputdevice")) as proxy:
            self.assertTrue(isAlive(proxy))
            proxy.output.connect()
            # Send more often as our proxy has a drop setting and we are busy
            # in tests.
            for data in range(NUM_DATA):
                await proxy.sendData()
            self.assertNotEqual(proxy.output.schema.data.timestamp,
                                FIXED_TIMESTAMP)
            await setWait(proxy, 'useTimestamp', True)
            for data in range(NUM_DATA):
                await proxy.sendData()
            self.assertEqual(proxy.output.schema.data.timestamp,
                             FIXED_TIMESTAMP)

            proxy.output.meta = False
            for data in range(NUM_DATA):
                await proxy.sendData()
            self.assertNotEqual(proxy.output.schema.data.timestamp,
                                FIXED_TIMESTAMP)
        del proxy
        await output_device.slotKillDevice()

    @async_tst
    async def test_output_proxy_connected_close_handler(self):
        """Test the output connected and close handler of a proxy"""
        NUM_DATA = 5
        output_device = Sender({"_deviceId_": "outputdevice"})
        await output_device.startInstance()

        closed = False
        closed_name = ""

        connected_name = ""
        connected = Future()

        def close_handler(channel_name):
            nonlocal closed, closed_name
            closed = True
            closed_name = channel_name

        def connect_handler(channel_name):
            nonlocal connected, connected_name
            connected.set_result(True)
            connected_name = channel_name

        with (await getDevice("outputdevice")) as proxy:
            self.assertTrue(isAlive(proxy))
            proxy.output.setConnectHandler(connect_handler)
            proxy.output.setCloseHandler(close_handler)
            proxy.output.connect()
            await connected
            self.assertEqual(connected.result(), True)
            self.assertEqual(connected_name, "outputdevice:output")
            # Check that we are sending data with timestamps
            for data in range(NUM_DATA):
                await proxy.sendData()
                ts1 = proxy.output.schema.data.timestamp
            for data in range(NUM_DATA):
                await proxy.sendData()
                ts2 = proxy.output.schema.data.timestamp

            self.assertGreater(ts2, ts1)
            # No we kill the sender and verify our closed handler is called
            await output_device.slotKillDevice()
            self.assertEqual(closed, True)
            self.assertEqual(closed_name, "outputdevice:output")

        del proxy

    @async_tst
    async def test_multi_shared_pipelines(self):
        """Test the shared queue for multiple shared consumers"""
        # Check that we are connected
        await self.bob.connectInputChannel()
        charlie = Receiver({"_deviceId_": "charlie",
                            "input": {"dataDistribution": "shared",
                                      "delayOnInput": 0}})
        delta = Receiver({"_deviceId_": "delta",
                          "input": {"dataDistribution": "shared",
                                    "delayOnInput": 0}})
        await charlie.startInstance()
        await charlie.connectInputChannel()
        await delta.startInstance()
        await delta.connectInputChannel()

        # Check that we are connected
        channels = self.bob.input.connectedOutputChannels.value
        self.assertIn("alice:output", channels)
        channels = charlie.input.connectedOutputChannels.value
        self.assertIn("alice:output", channels)
        channels = delta.input.connectedOutputChannels.value
        self.assertIn("alice:output", channels)

        # Reset and check counters
        await self.alice.resetCounter()
        await self.bob.resetCounter()
        await charlie.resetCounter()
        await delta.resetCounter()
        self.assertEqual(self.alice.outputCounter, 0)
        # Bob is drop
        self.assertEqual(self.bob.received, 0)
        # Charlie is shared
        self.assertEqual(charlie.received, 0)
        # Delta is shared
        self.assertEqual(delta.received, 0)
        proxy = await getDevice("alice")
        with proxy:
            await proxy.sendData()
            await waitUntil(lambda: self.bob.received == 1)
            await proxy.sendData()
            await waitUntil(lambda: self.bob.received == 2)
            await proxy.sendData()
            await waitUntil(lambda: self.bob.received == 3)
            await proxy.sendData()
            await waitUntil(lambda: self.bob.received == 4)

            pot_charlie = charlie.received
            pot_delta = delta.received
            self.assertGreater(pot_charlie, 0)
            self.assertGreater(pot_delta, 0)
            self.assertEqual(pot_charlie + pot_delta, 4)
            await delta.slotKillDevice()

            await proxy.sendData()
            await waitUntil(lambda: self.bob.received == 5)
            await proxy.sendData()
            await waitUntil(lambda: self.bob.received == 6)
            self.assertEqual(pot_charlie + pot_delta + 2, 6)
            # Delta got killed, only charly has 2 more
            new_pot_charlie = charlie.received
            self.assertEqual(pot_charlie + 2, new_pot_charlie)

        await charlie.slotKillDevice()

    @async_tst
    async def test_zero_sockets_output_close(self):
        """This test must be executed last"""
        self.assertIsNotNone(self.alice.output.server.sockets)
        self.assertGreater(len(self.alice.output.active_channels), 0)
        await self.alice.output.close()
        self.assertEqual(self.alice.output.server.sockets, ())
        self.assertEqual(len(self.alice.output.active_channels), 0)


if __name__ == "__main__":
    main()
