from asyncio import Future, TimeoutError, sleep, wait_for
from contextlib import contextmanager
from unittest import main

import pytest

from karabo.middlelayer import (
    AccessLevel, AccessMode, Assignment, Bool, Configurable, Device, Hash,
    InputChannel, Int32, Node, OutputChannel, Overwrite, PipelineContext,
    PipelineMetaData, Slot, State, Timestamp, UInt32, background, call, coslot,
    getDevice, isAlive, setWait, waitUntil, waitUntilNew)
from karabo.middlelayer_api.tests.eventloop import (
    DeviceTest, async_tst, sleepUntil)

FIXED_TIMESTAMP = Timestamp("2009-04-20T10:32:22 UTC")


def get_channel_node(displayed_name=""):
    class ChannelNode(Configurable):
        data = Int32(
            displayedName=displayed_name,
            defaultValue=0)

    return ChannelNode


class NodeOutput(Configurable):
    output = OutputChannel(
        get_channel_node(),
        assignment=Assignment.OPTIONAL,
        requiredAccessLevel=AccessLevel.OPERATOR,
        accessMode=AccessMode.READONLY)


class NodedSender(Device):
    state = Overwrite(options=[State.ON])

    node = Node(NodeOutput)

    outputCounter = UInt32(
        defaultValue=0,
        displayedName="Output Counter")

    @Slot()
    async def sendData(self):
        self.node.output.schema.data += 1
        self.outputCounter += 1
        await self.node.output.writeData()

    def __init__(self, configuration):
        super().__init__(configuration)


class InjectedSender(Device):
    """A device that does not have an Output Channel in the static schema
    """
    state = Overwrite(options=[State.ON])

    @Slot()
    async def injectOutput(self):
        self.__class__.output = OutputChannel(
            get_channel_node(),
            assignment=Assignment.OPTIONAL,
            requiredAccessLevel=AccessLevel.OPERATOR,
            accessMode=AccessMode.READONLY)
        await self.publishInjectedParameters()

    outputCounter = UInt32(
        defaultValue=0,
        displayedName="Output Counter")

    @Slot()
    async def sendData(self):
        self.output.schema.data = self.output.schema.data.value + 1
        self.outputCounter = self.outputCounter.value + 1
        await self.output.writeData()


class Sender(Device):
    # The state is explicitly overwritten, State.UNKNOWN is always possible by
    # default! We test that a proxy can reach State.UNKNOWN even if it is
    # removed from the allowed options.
    running = False

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
    async def startSending(self):
        background(self._keep_sending())

    async def _keep_sending(self):
        self.running = True
        while self.running:
            await self.sendData()
            await sleep(0.1)

    @Slot()
    async def stopSending(self):
        self.running = False

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


class InputSchema(Configurable):

    received = UInt32(
        defaultValue=0,
        displayedName="Received Packets")

    connected = Bool(
        defaultValue=False)

    closed = Bool(
        defaultValue=False)

    eosReceived = Bool(
        defaultValue=False)

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


class Receiver(InputSchema, Device):

    node = Node(InputSchema)

    def __init__(self, configuration):
        super().__init__(configuration)

    @coslot
    async def connectInputChannel(self, output="alice", node=False):
        await self.input.connectChannel(f"{output}:output")
        if node:
            await self.node.input.connectChannel(f"{output}:output")
        return True

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
        ret = await call("bob", "connectInputChannel", "alice", True)
        self.assertTrue(ret)
        ret = await call("charlie", "connectInputChannel", "alice", True)
        self.assertTrue(ret)
        await sleep(1)
        for channels in [self.bob.input.connectedOutputChannels.value,
                         self.bob.node.input.connectedOutputChannels.value,
                         charlie.input.connectedOutputChannels.value,
                         charlie.node.input.connectedOutputChannels.value]:
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
            self.assertEqual(self.bob.node.received, 0)
            self.assertEqual(charlie.node.received, 0)

            await proxy.sendData()
            await waitUntil(lambda: self.bob.received == 1)
            await waitUntil(lambda: charlie.received == 1)
            await waitUntil(lambda: self.bob.node.received == 1)
            await waitUntil(lambda: charlie.node.received == 1)
            self.assertEqual(charlie.received, 1)
            self.assertEqual(self.bob.received, 1)
            self.assertEqual(self.bob.node.received, 1)
            self.assertEqual(charlie.node.received, 1)
            await proxy.sendEndOfStream()
            await waitUntil(lambda: self.bob.eosReceived.value is True)
            await waitUntil(lambda: charlie.eosReceived.value is True)
            await waitUntil(lambda: self.bob.node.eosReceived.value is True)
            await waitUntil(lambda: charlie.node.eosReceived.value is True)
            self.assertEqual(self.bob.eosReceived.value, True)
            self.assertEqual(charlie.eosReceived.value, True)
            self.assertEqual(self.bob.node.eosReceived.value, True)
            self.assertEqual(charlie.node.eosReceived.value, True)
            await proxy.sendData()
            await waitUntil(lambda: self.bob.received == 2)
            await waitUntil(lambda: charlie.received == 2)
            await waitUntil(lambda: self.bob.node.received == 2)
            await waitUntil(lambda: charlie.node.received == 2)
            self.assertEqual(self.bob.received, 2)
            self.assertEqual(charlie.received, 2)
            self.assertEqual(self.bob.node.received, 2)
            self.assertEqual(charlie.node.received, 2)

            # Shutdown the shared channel. The queue gets removed and
            # we test that we are not blocked.
            await charlie.slotKillDevice()

            await proxy.sendData()
            await waitUntil(lambda: self.bob.received == 3)
            self.assertEqual(self.bob.received, 3)
            await waitUntil(lambda: self.bob.node.received == 3)
            self.assertEqual(self.bob.node.received, 3)

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
        await receiver.connectInputChannel("outputdevice", True)

        name = "outputdevice:output"

        NUM_DATA = 5
        with (await getDevice("outputdevice")) as proxy:
            self.assertTrue(isAlive(proxy))
            self.assertEqual(receiver.received, 0)
            self.assertEqual(receiver.node.received, 0)
            for data in range(NUM_DATA):
                await proxy.sendData()
            self.assertGreater(receiver.received, 0)
            self.assertGreater(receiver.node.received, 0)
            # We received data and now kill the device
            self.assertNotIn(name, receiver.input.missingConnections)
            await output_device.slotKillDevice()
            await waitUntil(lambda: not isAlive(proxy))  # noqa
            self.assertFalse(isAlive(proxy))
            # The device is gone, now we instantiate the device with same
            # deviceId to see if the output automatically reconnects
            self.assertIn(name, receiver.input.missingConnections)
            output_device = Sender({"_deviceId_": "outputdevice"})
            await output_device.startInstance()
            await waitUntil(lambda: isAlive(proxy))  # noqa

            # We set the counter to zero!
            await receiver.resetCounter()
            await receiver.node.resetCounter()

            await sleepUntil(
                lambda: name not in receiver.input.missingConnections,
                timeout=5)
            self.assertNotIn(name, receiver.input.missingConnections)
            self.assertEqual(receiver.received, 0)
            self.assertEqual(receiver.node.received, 0)
            # Wait a few seconds because the reconnect will wait seconds
            # as well before attempt
            await sleep(2)
            await proxy.sendData()
            # Our reconnect was successful, we are receiving data via the
            # output channel
            await sleepUntil(lambda: receiver.received > 0, timeout=5)
            self.assertGreater(receiver.received, 0)
            self.assertGreater(receiver.node.received, 0)
            connections = output_device.output.connections
            self.assertEqual(len(connections.value), 2)

            await receiver.slotKillDevice()
            connections = output_device.output.connections
            self.assertEqual(len(connections.value), 0)

        del proxy
        await output_device.slotKillDevice()

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
        await sleep(2)
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

        del proxy
        # Now we kill the sender and verify our closed handler is called
        await output_device.slotKillDevice()
        self.assertEqual(closed, True)
        self.assertEqual(closed_name, "outputdevice:output")

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
        self.assertTrue(self.alice.output.alive)
        success, h = self.alice.slotGetOutputChannelInformation(
            "output", None)
        self.assertTrue(success)
        self.assertFalse(h.empty())
        await self.alice.output.close()
        self.assertIsNone(self.alice.output.server)
        self.assertFalse(self.alice.output.alive)
        success, h = self.alice.slotGetOutputChannelInformation(
            "output", None)
        self.assertFalse(success)
        self.assertTrue(h.empty())
        self.assertEqual(len(self.alice.output.active_channels), 0)

        # Closing multiple times does not hurt
        await self.alice.output.close()

    @async_tst
    async def test_pipeline_context(self):
        """Test the operation of a pipeline context channel"""
        output_device = Sender({"_deviceId_": "ContextSender"})
        await output_device.startInstance()
        try:
            proxy = await getDevice("ContextSender")
            self.assertTrue(isAlive(proxy))
            await proxy.startSending()
            channel = PipelineContext("ContextSender:output")
            data = None
            async with channel:
                data = await wait_for(channel.get_data(), timeout=5)
                self.assertIsNotNone(data)
                self.assertTrue(channel.is_alive())
                await proxy.stopSending()

                # Start fresh to reconnect
                await output_device.slotKillDevice()
                # XXX: wait for instanceGone signal
                await sleepUntil(lambda: not channel.is_alive())
                self.assertFalse(channel.is_alive())

                output_device = Sender({"_deviceId_": "ContextSender"})
                await output_device.startInstance()
                # Wait for connection
                # NOTE: `proxy` is not being updated here. But it is fine,
                #       since we are awaiting the connection and the device
                #       will be implicitly online.
                await channel.wait_connected()
                self.assertTrue(channel.is_alive())

                # Get data again
                data = None
                await proxy.startSending()
                data = await wait_for(channel.get_data(), timeout=5)
                self.assertIsNotNone(data)

            # Leave context and get data again
            data = None
            async with channel:
                data = await wait_for(channel.get_data(), timeout=5)
                self.assertIsNotNone(data)
            await proxy.stopSending()
            self.assertFalse(channel.is_alive())

            # Make sure we can show a repr in karabo!
            data, meta = data
            self.assertIsNotNone(repr(data))
            self.assertIsNotNone(repr(meta))
            self.assertIsInstance(meta, PipelineMetaData)
        finally:
            await output_device.slotKillDevice()

    @async_tst
    async def test_pipeline_context_no_output(self):
        """Test that connecting to a channel is not blocking"""
        channel = PipelineContext("NoDeviceOnline:output")
        async with channel:
            try:
                await wait_for(channel.wait_connected(), timeout=2)
            except TimeoutError:
                pass
            else:
                self.fail("Channel did not timeout correctly on connection")

    @async_tst
    async def test_noded_output_channel(self):
        """Test the operation of a noded output channel sending"""
        NUM_DATA = 5
        output_device = NodedSender({"_deviceId_": "NodedSender"})
        await output_device.startInstance()
        try:
            with (await getDevice("NodedSender")) as proxy:
                self.assertTrue(isAlive(proxy))
                received = False

                def handler(data, meta):
                    """Output handler to see if we received data
                    """
                    nonlocal received
                    received = True

                self.assertEqual(received, False)
                proxy.node.output.setDataHandler(handler)
                proxy.node.output.connect()
                for _ in range(NUM_DATA):
                    await proxy.sendData()
                self.assertEqual(received, True)
        finally:
            await output_device.slotKillDevice()

    @async_tst
    async def test_injected_output_channel_connection(self):
        """Test the re/connect to an injected output channel"""
        output_device = InjectedSender({"_deviceId_": "InjectedSender"})
        receiver = Receiver({"_deviceId_": "ReceiverInjectedSender"})
        await output_device.startInstance()
        await receiver.startInstance()
        await receiver.connectInputChannel("InjectedSender")

        try:
            with (await getDevice("InjectedSender")) as sender_proxy, \
                    await getDevice("ReceiverInjectedSender") as input_proxy:
                self.assertTrue(isAlive(sender_proxy))
                self.assertTrue(isAlive(input_proxy))

                self.assertEqual(receiver.received, 0)
                self.assertEqual(input_proxy.received, 0)

                # Sender and receiver are online, but no output
                await sender_proxy.injectOutput()
                # output is injected, wait for connection
                # must wait because proxy needs a schema to connect

                await sleep(3)
                received = False
                connected = False

                def handler(data, meta):
                    """Output handler to see if we received data
                    """
                    nonlocal received
                    received = True

                def connect_handler(channel):
                    nonlocal connected
                    connected = True

                self.assertEqual(received, False)
                sender_proxy.output.setDataHandler(handler)
                sender_proxy.output.setConnectHandler(connect_handler)
                sender_proxy.output.connect()
                await sleepUntil(lambda: connected is True)
                await sender_proxy.sendData()
                await sleepUntil(lambda: received is True)
                self.assertEqual(received, True)

                with pytest.raises(RuntimeError):
                    sender_proxy.output.setDataHandler(handler)

                def new_handler(data, meta):
                    nonlocal received
                    received = True

                def new_connect_handler(channel):
                    nonlocal connected
                    connected = True

                received = False
                connected = False
                # Disconnect and reattach a handler
                sender_proxy.output.disconnect()
                assert sender_proxy.output.task is None
                sender_proxy.output.setDataHandler(new_handler)
                sender_proxy.output.setConnectHandler(new_connect_handler)
                sender_proxy.output.connect()

                # Receiver device
                await sleepUntil(lambda: connected is True)
                self.assertTrue(receiver.connected)
                await sender_proxy.sendData()
                await waitUntilNew(input_proxy.received)
                self.assertGreater(input_proxy.received, 0)
                await sleepUntil(lambda: receiver.received > 0)
                self.assertGreater(receiver.received, 0)
                self.assertTrue(received)

                # Kill the device and bring up again
                await output_device.slotKillDevice()

                # Have to start a fresh device object for inject
                connected = False
                output_device = InjectedSender(
                    {"_deviceId_": "InjectedSender"})
                await output_device.startInstance()
                await waitUntil(lambda: isAlive(sender_proxy) is True)
                # Sender and receiver are online, but no output
                await sender_proxy.injectOutput()
                # output is injected, wait for connection
                await sleepUntil(lambda: connected is True)

                # Send data again
                received = False
                await receiver.resetCounter()
                await sender_proxy.sendData()
                await waitUntilNew(input_proxy.received)
                await sleepUntil(lambda: receiver.received > 0)
                self.assertGreater(input_proxy.received, 0)
                self.assertGreater(receiver.received, 0)
                self.assertTrue(received)
        finally:
            await output_device.slotKillDevice()
            await receiver.slotKillDevice()


if __name__ == "__main__":
    main()
