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
"""This tests the long pipeline tests in the middlelayer API"""

from random import randint

import pytest

from karabo.middlelayer import (
    AccessLevel, AccessMode, Assignment, Bool, Configurable, Device,
    InputChannel, Int32, OutputChannel, Slot, State, UInt32, background, sleep,
    slot)
from karabo.middlelayer.testing import AsyncDeviceContext, sleepUntil


def get_channel_node(displayed_name=""):
    class ChannelNode(Configurable):
        data = Int32(
            displayedName=displayed_name,
            defaultValue=0)

    return ChannelNode


class Receiver(Device):

    received = UInt32(
        defaultValue=0,
        displayedName="Received Packets")

    connected = Bool(
        defaultValue=False)

    eosReceived = Bool(
        defaultValue=False)

    @Slot()
    async def resetCounter(self):
        self.received = 0

    # close the input channel `name`
    @InputChannel(raw=True)
    async def input(self, data, meta):
        self.received += 1

    @input.connect
    async def input(self, name):
        self.connected = True

    @input.close
    async def input(self, name):
        self.connected = False

    @slot
    async def connectInputChannel(self, output=""):
        await self.input.connectChannel(f"{output}:output")

    async def onInitialization(self):
        self.state = State.ON


class CrazyInjector(Device):
    """A device that constantly updates output channels schema"""
    running = True

    output = OutputChannel(
        get_channel_node(),
        assignment=Assignment.OPTIONAL,
        requiredAccessLevel=AccessLevel.OPERATOR,
        accessMode=AccessMode.READONLY)

    async def onInitialization(self):
        background(self._inject_chooch())

    async def _inject_chooch(self):
        while self.running:
            await self.setOutputSchema("output", get_channel_node())
            delay = randint(0, 300)
            await sleep(delay / 1000)

    @Slot()
    async def sendData(self):
        await self.output.writeData()

    @Slot()
    async def stop(self):
        self.running = False


@pytest.mark.timeout(90)
@pytest.mark.asyncio(loop_scope="module")
async def test_crazy_injected_channel_connection():
    """Test the crazy injected output channel"""
    INJECTING_TIME = 20

    output_device = CrazyInjector({"_deviceId_": "CrazyInjectedSender"})
    receiver = Receiver({"_deviceId_": "ReceiverCrazyInjectedSender"})
    # Start the devices
    async with AsyncDeviceContext(output=output_device, input=receiver):
        await receiver.connectInputChannel("CrazyInjectedSender")
        await sleep(INJECTING_TIME)
        await output_device.stop()

        # The reconnect takes a bit
        await sleep(3)
        assert receiver.connected

        # Start send a data
        await receiver.resetCounter()
        await output_device.sendData()
        await sleepUntil(lambda: receiver.received > 0)
        assert receiver.received == 1
    assert not receiver.connected
