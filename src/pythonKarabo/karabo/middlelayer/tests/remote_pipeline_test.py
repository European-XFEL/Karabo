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
from asyncio import Future, TimeoutError, get_running_loop, sleep, wait_for

import pytest
import pytest_asyncio

from karabo.middlelayer import (
    AccessLevel, AccessMode, Assignment, Bool, Configurable, Device, Hash,
    InputChannel, Int32, NetworkOutput, Node, OutputChannel, Overwrite,
    PipelineContext, PipelineMetaData, Slot, State, Timestamp, UInt32,
    background, call, getDevice, getSchema, isAlive, setWait, slot, waitUntil)
from karabo.middlelayer.testing import AsyncDeviceContext, run_test, sleepUntil

FIXED_TIMESTAMP = Timestamp("2009-04-20T10:32:22 UTC")

timeout = 10


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

    @slot
    async def connectInputChannel(self, output="alice", node=False):
        await self.input.connectChannel(f"{output}:output")
        if node:
            await self.node.input.connectChannel(f"{output}:output")
        return True

    async def onInitialization(self):
        self.state = State.ON


@pytest_asyncio.fixture(scope="module")
@pytest.mark.asyncio
async def deviceTest():
    alice = Sender({"_deviceId_": "alice"})
    bob = Receiver({"_deviceId_": "bob",
                    "input": {"dataDistribution": "copy",
                              "onSlowness": "drop"}})
    get_running_loop().lead = alice
    ctx = AsyncDeviceContext(alice=alice, bob=bob)
    async with ctx:
        yield ctx


@pytest.mark.timeout(30)
@run_test
def test_configurationAsHash(deviceTest):
    """Test the empty Hash of the output schema"""
    alice = deviceTest["alice"]
    conf = alice.configurationAsHash()
    assert conf['output.schema'] == Hash()


@pytest.mark.timeout(30)
@run_test
async def test_input_output(deviceTest):
    """Test the input and output channel connection"""
    bob = deviceTest["bob"]
    charlie = Receiver({"_deviceId_": "charlie",
                        "input": {"dataDistribution": "shared"}})

    await charlie.startInstance()
    ret = await call("bob", "connectInputChannel", "alice", True)
    assert ret
    ret = await call("charlie", "connectInputChannel", "alice", True)
    assert ret
    await sleep(1)
    for channels in [bob.input.connectedOutputChannels.value,
                     bob.node.input.connectedOutputChannels.value,
                     charlie.input.connectedOutputChannels.value,
                     charlie.node.input.connectedOutputChannels.value]:
        assert "alice:output" in channels

    # Charlie is new and we check the handler
    assert charlie.connected.value

    assert bob.input.onSlowness == "drop"
    assert bob.input.dataDistribution == "copy"
    assert charlie.input.dataDistribution == "shared"

    proxy = await getDevice("alice")
    with proxy:
        assert bob.received == 0
        assert charlie.received == 0
        assert bob.node.received == 0
        assert charlie.node.received == 0

        await proxy.sendData()
        await waitUntil(lambda: bob.received == 1)
        await waitUntil(lambda: charlie.received == 1)
        await waitUntil(lambda: bob.node.received == 1)
        await waitUntil(lambda: charlie.node.received == 1)
        assert charlie.received == 1
        assert bob.received == 1
        assert bob.node.received == 1
        assert charlie.node.received == 1
        await proxy.sendEndOfStream()
        await waitUntil(lambda: bob.eosReceived.value is True)
        await waitUntil(lambda: charlie.eosReceived.value is True)
        await waitUntil(lambda: bob.node.eosReceived.value is True)
        await waitUntil(lambda: charlie.node.eosReceived.value is True)
        assert bob.eosReceived.value is True
        assert charlie.eosReceived.value is True
        assert bob.node.eosReceived.value is True
        assert charlie.node.eosReceived.value is True
        await proxy.sendData()
        await waitUntil(lambda: bob.received == 2)
        await waitUntil(lambda: charlie.received == 2)
        await waitUntil(lambda: bob.node.received == 2)
        await waitUntil(lambda: charlie.node.received == 2)
        assert bob.received == 2
        assert charlie.received == 2
        assert bob.node.received == 2
        assert charlie.node.received == 2

        # Shutdown the shared channel. The queue gets removed and
        # we test that we are not blocked.
        await charlie.slotKillDevice()

        await proxy.sendData()
        await waitUntil(lambda: bob.received == 3)
        assert bob.received == 3
        await waitUntil(lambda: bob.node.received == 3)
        assert bob.node.received == 3


@pytest.mark.timeout(30)
@run_test
async def test_output_reconnect(deviceTest):
    """Test the output reconnect of a proxy"""
    NUM_DATA = 5
    output_device = Sender({"_deviceId_": "outputdevice"})
    await output_device.startInstance()

    with (await getDevice("outputdevice")) as proxy:
        assert isAlive(proxy)
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

        assert not received
        # Patch the handler to see if our boolean triggers
        proxy.output.setConnectHandler(connect_handler)
        proxy.output.setDataHandler(handler)
        proxy.output.connect()
        # Send more often as our proxy has a drop setting and we are busy
        # in tests.
        await sleepUntil(lambda: connected is True)
        await proxy.sendData()
        await sleepUntil(lambda: received is True)
        assert received
        assert connected
        # We received data and now kill the device
        await output_device.slotKillDevice()
        await waitUntil(lambda: not isAlive(proxy))
        assert not isAlive(proxy)
        # Set the received and connected to false!
        received = False
        connected = False
        # The device is gone, now we instantiate the device with same
        # deviceId to see if the output automatically reconnects
        output_device = Sender({"_deviceId_": "outputdevice"})
        await output_device.startInstance()
        await waitUntil(lambda: isAlive(proxy))
        # Wait a few seconds because the reconnect will wait seconds
        # as well before attempt
        await sleepUntil(lambda: connected is True)
        await proxy.sendData()
        await sleepUntil(lambda: received is True)
        # Our reconnect was successful, we are receiving data via the
        # output channel
        assert received
        assert connected

    # Delete our proxy and see if we still receive data!
    del proxy
    await sleep(1)
    received = False
    assert not received
    # No new data is arriving ...
    await sleep(1)
    # after 1 second
    assert not received

    await output_device.slotKillDevice()

    # Bring up our device with same deviceId, we should not have a
    # channel active with the handler
    output_device = Sender({"_deviceId_": "outputdevice"})
    await output_device.startInstance()
    with (await getDevice("outputdevice")) as proxy:
        assert isAlive(proxy)
        assert not received
        for _ in range(NUM_DATA):
            await proxy.sendData()
        assert not received

    await output_device.slotKillDevice()


@pytest.mark.timeout(30)
@run_test
async def test_output_reconnect_device(deviceTest):
    """Test the output reconnect of a device"""
    output_device = Sender({"_deviceId_": "outputdevice"})
    await output_device.startInstance()
    receiver = Receiver({"_deviceId_": "receiverdevice"})
    await receiver.startInstance()
    await receiver.connectInputChannel("outputdevice", True)

    name = "outputdevice:output"

    NUM_DATA = 5
    with (await getDevice("outputdevice")) as proxy:
        assert isAlive(proxy)
        assert receiver.received == 0
        assert receiver.node.received == 0
        for _ in range(NUM_DATA):
            await proxy.sendData()
        assert receiver.received > 0
        assert receiver.node.received > 0
        # We received data and now kill the device
        assert name not in receiver.input.missingConnections
        await output_device.slotKillDevice()
        await waitUntil(lambda: not isAlive(proxy))  # noqa
        assert not isAlive(proxy)
        # The device is gone, now we instantiate the device with same
        # deviceId to see if the output automatically reconnects
        await sleepUntil(
            lambda: name in receiver.input.missingConnections, timeout=2)
        assert name in receiver.input.missingConnections
        output_device = Sender({"_deviceId_": "outputdevice"})
        await output_device.startInstance()
        await waitUntil(lambda: isAlive(proxy))  # noqa

        # We set the counter to zero!
        await receiver.resetCounter()
        await receiver.node.resetCounter()

        await sleepUntil(
            lambda: name not in receiver.input.missingConnections,
            timeout=timeout)
        assert name not in receiver.input.missingConnections
        assert receiver.received == 0
        assert receiver.node.received == 0
        # Wait a few seconds because the reconnect will wait seconds
        # as well before attempt
        await sleep(2)
        await proxy.sendData()
        # Our reconnect was successful, we are receiving data via the
        # output channel
        await sleepUntil(lambda: receiver.received > 0, timeout=timeout)
        assert receiver.received > 0
        assert receiver.node.received > 0
        connections = output_device.output.connections
        await sleepUntil(lambda: len(connections) == 2, timeout=timeout)
        assert len(connections.value) == 2
        await receiver.slotKillDevice()
        output = proxy.output

        await sleepUntil(
            lambda: len(output.connections.value) == 0, timeout=timeout)
        assert len(output.connections.value) == 0

    del proxy
    await output_device.slotKillDevice()


@pytest.mark.timeout(30)
@run_test
async def test_output_change_schema(deviceTest):
    """Test the output schema change of a device"""
    output_device = Sender({"_deviceId_": "outputdevice"})
    await output_device.startInstance()
    receiver = Receiver({"_deviceId_": "receiverdevice"})
    await receiver.startInstance()
    await receiver.connectInputChannel("outputdevice")

    NUM_DATA = 5
    with (await getDevice("outputdevice")) as proxy:
        assert isAlive(proxy)
        # Check for initial connection
        output = proxy.output
        await waitUntil(lambda: len(output.connections) > 0)
        assert receiver.received == 0
        for _ in range(NUM_DATA):
            await proxy.sendData()
        await waitUntil(lambda: receiver.received > 0)
        assert receiver.received > 0

        # We received data and now change the schema
        output_schema = proxy.output.schema
        assert output_schema.data.descriptor.displayedName == ""

        # Displaytype should be set for the output schema
        proxy_schema = await getSchema(proxy)
        displayType = proxy_schema.hash.getAttribute(
            "output.schema", "displayType")
        assert displayType == "OutputSchema"
        await output_device.changeSchema("TestDisplayed")

        # Wait for schema arrival
        await sleepUntil(
            lambda: output_schema.data.descriptor.
            displayedName == "TestDisplayed")
        assert output_schema.data.descriptor.displayedName == "TestDisplayed"

        # Check for displayType after injection
        proxy_schema = await getSchema(proxy)
        displayType = proxy_schema.hash.getAttribute(
            "output.schema", "displayType")
        assert displayType == "OutputSchema"

        # Changing schema closes output channel, hence we wait
        # for connection
        await waitUntil(lambda: len(output.connections) > 0)
        # We set the counter to zero!
        await receiver.resetCounter()
        assert receiver.received == 0
        for _ in range(NUM_DATA):
            await proxy.sendData()
        # Our reconnect was successful, we are receiving data via the
        # output channel
        await waitUntil(lambda: receiver.received > 0)
        assert receiver.received > 0

    del proxy
    await output_device.slotKillDevice()
    await receiver.slotKillDevice()


@pytest.mark.timeout(30)
@run_test
async def test_output_timestamp(deviceTest):
    """Test the meta data timestamp of a proxy"""
    NUM_DATA = 5
    output_device = Sender({"_deviceId_": "outputdevice"})
    await output_device.startInstance()

    with (await getDevice("outputdevice")) as proxy:
        assert isAlive(proxy)
        proxy.output.connect()
        # Send more often as our proxy has a drop setting and we are busy
        # in tests.
        for _ in range(NUM_DATA):
            await proxy.sendData()
        assert proxy.output.schema.data.timestamp != FIXED_TIMESTAMP
        await setWait(proxy, 'useTimestamp', True)
        for _ in range(NUM_DATA):
            await proxy.sendData()
        assert proxy.output.schema.data.timestamp == FIXED_TIMESTAMP

        proxy.output.meta = False
        for _ in range(NUM_DATA):
            await proxy.sendData()
        assert proxy.output.schema.data.timestamp != FIXED_TIMESTAMP
    del proxy
    await output_device.slotKillDevice()


@pytest.mark.timeout(30)
@run_test
async def test_output_proxy_connected_close_handler(deviceTest):
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
        assert isAlive(proxy)
        proxy.output.setConnectHandler(connect_handler)
        proxy.output.setCloseHandler(close_handler)
        proxy.output.connect()
        await connected
        assert connected.result() is True
        assert connected_name == "outputdevice:output"
        # Check that we are sending data with timestamps
        for _ in range(NUM_DATA):
            await proxy.sendData()
            ts1 = proxy.output.schema.data.timestamp
        for _ in range(NUM_DATA):
            await proxy.sendData()
            ts2 = proxy.output.schema.data.timestamp

        assert ts2 > ts1

    del proxy
    # Now we kill the sender and verify our closed handler is called
    await output_device.slotKillDevice()
    assert closed
    assert closed_name == "outputdevice:output"


@pytest.mark.timeout(30)
@run_test
async def test_multi_shared_pipelines(deviceTest):
    """Test the shared queue for multiple shared consumers"""
    # Check that we are connected
    alice = deviceTest["alice"]
    bob = deviceTest["bob"]
    await bob.connectInputChannel()
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
    channels = bob.input.connectedOutputChannels.value
    assert "alice:output" in channels
    channels = charlie.input.connectedOutputChannels.value
    assert "alice:output" in channels
    channels = delta.input.connectedOutputChannels.value
    assert "alice:output" in channels

    # Reset and check counters
    await alice.resetCounter()
    await bob.resetCounter()
    await charlie.resetCounter()
    await delta.resetCounter()
    assert alice.outputCounter == 0
    # Bob is drop
    assert bob.received == 0
    # Charlie is shared
    assert charlie.received == 0
    # Delta is shared
    assert delta.received == 0
    proxy = await getDevice("alice")
    with proxy:
        await proxy.sendData()
        await waitUntil(lambda: bob.received == 1)
        await proxy.sendData()
        await waitUntil(lambda: bob.received == 2)
        await proxy.sendData()
        await waitUntil(lambda: bob.received == 3)
        await proxy.sendData()
        await waitUntil(lambda: bob.received == 4)

        pot_charlie = charlie.received
        pot_delta = delta.received
        assert pot_charlie > 0
        assert pot_delta > 0
        assert pot_charlie + pot_delta == 4
        await delta.slotKillDevice()

        await proxy.sendData()
        await waitUntil(lambda: bob.received == 5)
        await proxy.sendData()
        await waitUntil(lambda: bob.received == 6)
        assert pot_charlie + pot_delta + 2 == 6
        # Delta got killed, only charly has 2 more
        new_pot_charlie = charlie.received
        assert pot_charlie + 2 == new_pot_charlie

    await charlie.slotKillDevice()


@pytest.mark.timeout(30)
@run_test
async def test_zero_sockets_output_close(deviceTest):
    """This test must be executed last"""
    alice = deviceTest["alice"]
    assert alice.output.server.sockets is not None
    assert len(alice.output.active_channels) > 0
    assert alice.output.alive
    h = alice.slotGetOutputChannelInformation(
        Hash("channelId", "output"))
    assert h["success"]
    assert not h["info"].empty()
    await alice.output.close()
    assert alice.output.server is None
    assert not alice.output.alive
    h = alice.slotGetOutputChannelInformation(
        Hash("channelId", "output"))
    assert not h["success"]
    assert h["info"].empty()
    assert len(alice.output.active_channels) == 0

    # Closing multiple times does not hurt
    await alice.output.close()


@pytest.mark.timeout(30)
@run_test
async def test_pipeline_context(deviceTest):
    """Test the operation of a pipeline context channel"""
    output_device = Sender({"_deviceId_": "ContextSender"})
    async with AsyncDeviceContext(output_device=output_device) as ctx:
        proxy = await getDevice("ContextSender")
        assert isAlive(proxy)
        await proxy.startSending()
        channel = PipelineContext("ContextSender:output")
        data = None
        async with channel:
            data = await wait_for(channel.get_data(), timeout=timeout)
            assert data is not None
            assert channel.is_alive()
            await proxy.stopSending()

            await ctx.shutdown()
            # Start fresh to reconnect
            # XXX: wait for instanceGone signal
            await sleepUntil(lambda: not channel.is_alive())
            assert not channel.is_alive()

            output_device = Sender({"_deviceId_": "ContextSender"})
            await ctx.device_context(output_device=output_device)
            # Wait for connection
            await channel.wait_connected()
            assert channel.is_alive()

            # Proxy alive depends on instanceNew
            await waitUntil(lambda: isAlive(proxy))

            # Get data again
            data = None
            await proxy.startSending()
            data = await wait_for(channel.get_data(), timeout=timeout)
            assert data is not None

        # Leave context and get data again
        data = None
        assert channel.size() == 0
        async with channel:
            data = await wait_for(channel.get_data(), timeout=timeout)
            assert data is not None
            await sleepUntil(lambda: channel.size() >= 4)
            assert channel.size() >= 4
        assert channel.size() == 0
        await proxy.stopSending()
        assert not channel.is_alive()

        # Make sure we can show a repr in karabo!
        data, meta = data
        assert repr(data) is not None
        assert repr(meta) is not None
        assert isinstance(meta, PipelineMetaData)


@pytest.mark.timeout(30)
@run_test
async def test_pipeline_context_no_output(deviceTest):
    """Test that connecting to a channel is not blocking"""
    channel = PipelineContext("NoDeviceOnline:output")
    async with channel:
        try:
            await wait_for(channel.wait_connected(), timeout=2)
        except TimeoutError:
            pass
        else:
            assert False, ("Channel did not timeout correctly on connection")


@pytest.mark.timeout(30)
@run_test
async def test_noded_output_channel(deviceTest):
    """Test the operation of a noded output channel sending"""
    NUM_DATA = 5
    output_device = NodedSender({"_deviceId_": "NodedSender"})
    await output_device.startInstance()
    try:
        with (await getDevice("NodedSender")) as proxy:
            assert isAlive(proxy)
            received = False

            def handler(data, meta):
                """Output handler to see if we received data
                """
                nonlocal received
                received = True

            assert not received
            proxy.node.output.setDataHandler(handler)
            proxy.node.output.connect()
            for _ in range(NUM_DATA):
                await proxy.sendData()
            assert received
    finally:
        await output_device.slotKillDevice()


@pytest.mark.timeout(60)
@run_test
async def test_injected_output_channel_connection():
    """Test the re/connect to an injected output channel"""
    output_device = InjectedSender({"_deviceId_": "InjectedSender"})
    receiver = Receiver(
        {"_deviceId_": "ReceiverInjectedSender",
         "input": {"connectedOutputChannels": ["InjectedSender:output"]}})
    async with AsyncDeviceContext(output_device=output_device,
                                  receiver=receiver):
        with (await getDevice("InjectedSender")) as sender_proxy, \
                await getDevice("ReceiverInjectedSender") as input_proxy:
            assert isAlive(sender_proxy)
            assert isAlive(input_proxy)

            assert receiver.received == 0
            assert input_proxy.received == 0

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

            assert not received
            sender_proxy.output.setDataHandler(handler)
            sender_proxy.output.setConnectHandler(connect_handler)
            sender_proxy.output.connect()
            await sleepUntil(lambda: connected is True, timeout=timeout)
            await sender_proxy.sendData()
            await sleepUntil(lambda: received is True, timeout=timeout)
            assert received

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
            await sleepUntil(lambda: connected is True, timeout=timeout)
            assert receiver.connected
            await sender_proxy.sendData()
            await sleepUntil(lambda: receiver.received > 0, timeout=timeout)
            assert receiver.received > 0
            await sleepUntil(lambda: received is True, timeout=timeout)
            assert received
            await wait_for(waitUntil(lambda: input_proxy.received > 0),
                           timeout=timeout)
            assert input_proxy.received > 0

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
            await sleepUntil(lambda: connected is True, timeout=timeout)

            # Send data again
            received = False
            await receiver.resetCounter()
            await sender_proxy.sendData()
            await sleepUntil(lambda: received is True, timeout=timeout)
            await sleepUntil(lambda: receiver.received > 0,
                             timeout=timeout)
            await wait_for(waitUntil(lambda: input_proxy.received > 0),
                           timeout=timeout)
            assert receiver.received > 0


@pytest.mark.timeout(30)
@run_test
async def test_output_fail_address():
    """Test the output with a wrong address"""

    class Output(NetworkOutput):
        hostname = Overwrite(defaultValue="192.168.1/22")

    class SimpleSender(Device):

        output = Node(Output)

    config = {"_deviceId_": "outputraisedevice"}
    output_device = SimpleSender(config)
    with pytest.raises(ValueError) as exc:
        await output_device.startInstance()
    assert "cannot be found in network configuration" in str(exc)

    # And another case, with an invalid IP regex
    class Output(NetworkOutput):
        hostname = Overwrite(defaultValue="192.168.1.0/225")

    class SimpleSender(Device):

        output = Node(Output)

    config = {"_deviceId_": "outputraisedevice"}
    output_device = SimpleSender(config)
    with pytest.raises(ValueError) as exc:
        await output_device.startInstance()
    assert "does not appear to be an IPv4 or IPv6 network" in str(exc)
