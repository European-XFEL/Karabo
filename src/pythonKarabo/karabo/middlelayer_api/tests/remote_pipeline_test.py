from asyncio import sleep
from contextlib import contextmanager
from unittest import main

from karabo.middlelayer import (
    Bool, call, Configurable, Device, getDevice, Hash, isAlive, InputChannel,
    Int32, Overwrite, OutputChannel, setWait, coslot, Slot, State, Timestamp,
    UInt32, waitUntil)
from .eventloop import DeviceTest, async_tst

FIXED_TIMESTAMP = Timestamp("2009-04-20T10:32:22 UTC")


class ChannelNode(Configurable):
    data = Int32(defaultValue=0)


class Alice(Device):
    # The state is explicitly overwritten, State.UNKNOWN is always possible by
    # default! We test that a proxy can reach State.UNKNOWN even if it is
    # removed from the allowed options.

    state = Overwrite(options=[State.ON])
    useTimestamp = Bool(
        defaultValue=False)

    output = OutputChannel(ChannelNode)

    @Slot()
    async def sendData(self):
        self.output.schema.data = self.output.schema.data.value + 1
        timestamp = FIXED_TIMESTAMP if self.useTimestamp else Timestamp()
        await self.output.writeData(timestamp=timestamp)

    @Slot()
    async def sendEndOfStream(self):
        await self.output.writeEndOfStream()

    def __init__(self, configuration):
        super().__init__(configuration)


class Bob(Device):

    received = UInt32(
        defaultValue=0,
        displayedName="Received Packets")

    closed = Bool(
        defaultValue=False)

    eosReceived = Bool(
        defaultValue=False)

    @coslot
    async def connectInputChannel(self):
        await self.input.connectChannel("alice:output")
        return True

    # close the input channel `name`
    @InputChannel(raw=True)
    async def input(self, data, meta):
        self.received = self.received.value + 1

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
        cls.alice = Alice({"_deviceId_": "alice"})
        cls.bob = Bob({"_deviceId_": "bob"})
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
        ret = await call("bob", "connectInputChannel")
        self.assertTrue(ret)
        await sleep(1)
        channels = self.bob.input.connectedOutputChannels.value
        self.assertIn("alice:output", channels)
        proxy = await getDevice("alice")
        with proxy:
            self.assertEqual(self.bob.received, 0)
            await proxy.sendData()
            await waitUntil(lambda: self.bob.received == 1)
            self.assertEqual(self.bob.received, 1)
            await proxy.sendEndOfStream()
            await waitUntil(lambda: self.bob.eosReceived.value  == True)
            self.assertEqual(self.bob.eosReceived.value, True)
            await proxy.sendData()
            await waitUntil(lambda: self.bob.received == 2)
            self.assertEqual(self.bob.received, 2)

    @async_tst
    async def test_output_reconnect(self):
        NUM_DATA = 5
        output_device = Alice({"_deviceId_": "outputdevice"})
        await output_device.startInstance()

        with (await getDevice("outputdevice")) as proxy:
            self.assertTrue(isAlive(proxy))
            received = False

            def handler(data, meta):
                """Output handler to see if we received data
                """
                nonlocal received
                received = True

            self.assertEqual(received, False)
            # Patch the handler to see if our boolean triggers
            proxy.output.setDataHandler(handler)
            proxy.output.connect()
            # Send more often as our proxy has a drop setting and we are busy
            # in tests.
            for data in range(NUM_DATA):
                await proxy.sendData()
            self.assertEqual(received, True)
            # We received data and now kill the device
            await output_device.slotKillDevice()
            await waitUntil(lambda: not isAlive(proxy))
            self.assertFalse(isAlive(proxy))
            # The device is gone, now we instantiate the device with same
            # deviceId to see if the output automatically reconnects
            output_device = Alice({"_deviceId_": "outputdevice"})
            await output_device.startInstance()
            await waitUntil(lambda: isAlive(proxy))
            self.assertEqual(received, True)
            received = False
            self.assertEqual(received, False)
            for data in range(NUM_DATA):
                await proxy.sendData()
            # Our reconnect was successful, we are receiving data via the
            # output channel
            self.assertEqual(received, True)
            received = False

        self.assertEqual(received, False)
        # Delete our proxy and see if we still receive data!
        del proxy
        await output_device.slotKillDevice()
        await sleep(1)

        # Bring up our device with same deviceId, we should not have a
        # channel active with the handler
        output_device = Alice({"_deviceId_": "outputdevice"})
        await output_device.startInstance()
        with (await getDevice("outputdevice")) as proxy:
            self.assertTrue(isAlive(proxy))
            self.assertEqual(received, False)
            for data in range(NUM_DATA):
                await proxy.sendData()
            self.assertEqual(received, False)

        await output_device.slotKillDevice()

    @async_tst
    async def test_output_timestamp(self):
        NUM_DATA = 5
        output_device = Alice({"_deviceId_": "outputdevice"})
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


if __name__ == "__main__":
    main()
