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

import time
import uuid
from collections.abc import Iterable
from threading import Thread

from karabind import (
    DeviceClient, EventLoop, Hash, Logger, SignalSlotable, startDeviceServer,
    stopDeviceServer)


def test_device_client_sync_api():
    # Run CPP event loop in background ...
    loopThread = Thread(target=EventLoop.work)
    loopThread.start()

    # Configure and start device server ...
    # ... and make sure that plugins contains DataGenerator
    serverId = f'cppServer/{uuid.uuid4()}'
    config = Hash("serverId", serverId)
    startDeviceServer(config)
    # Create DeviceClient ...
    clientId = str(uuid.uuid4())
    c = DeviceClient(clientId)
    assert c.getInstanceId() == clientId
    assert serverId in c.getServers()
    # instantiate 'data/prop/1' device ... timeout = 5secs
    # synopsis: instantiate(serverId, classId, config, timeout)
    deviceId = f'data/prop/{uuid.uuid4()}'
    c.instantiate(
        serverId, 'PropertyTest', Hash('deviceId', deviceId), 5)
    # Check that device was created ...
    assert deviceId in c.getDevices(serverId)

    assert c.get(deviceId, 'node.counter') == 0
    c.set(deviceId, 'node.counter', 10)
    assert c.get(deviceId, 'node.counter') == 10
    assert c.get(deviceId, 'node.counterReadOnly') == 10
    # increment node.counter ...
    for i in range(3):
        c.execute(deviceId, 'node.increment', 5)

    assert c.get(deviceId, 'node.counter') == 13
    assert c.get(deviceId, 'node.counterReadOnly') == 10

    device_schema = c.getDeviceSchema(deviceId)
    assert 'outputFrequency' in device_schema.getParameterHash()
    assert 'output.connections' in device_schema.getParameterHash()

    active_schema = c.getActiveSchema(deviceId)
    assert 'int16Property' in active_schema.getParameterHash()
    assert 'table' in active_schema.getParameterHash()

    class_schema = c.getClassSchema(serverId, 'PropertyTest')
    assert 'karaboVersion' in class_schema.getParameterHash()
    assert 'boolProperty' in class_schema.getParameterHash()

    props = c.getProperties(deviceId)
    assert 'status' in props
    assert 'vectors.boolProperty' in props

    comms = c.getCurrentlyExecutableCommands(deviceId)
    assert 'writeOutput' in comms
    assert 'eosOutput' in comms

    props = c.getCurrentlySettableProperties(deviceId)
    assert 'int32Property' in props
    assert 'boolProperty' in props

    names = c.getOutputChannelNames(deviceId)
    assert isinstance(names, Iterable)  # it's a 'list', but let's not assert
    assert 'output' in names

    schema = c.getOutputChannelSchema(deviceId, 'output')
    assert 'node.ndarray.shape' in schema
    assert 'node.image.pixels.data' in schema
    assert 'node.image.pixels.shape' in schema

    # shutdown device ...
    c.killDevice(deviceId, 5)
    assert deviceId not in c.getDevices(serverId)
    # stop and de-register device server...
    stopDeviceServer(serverId)
    Logger.reset()
    EventLoop.stop()
    loopThread.join()


def test_device_client_async_api():
    # Run CPP event loop in background ...
    loopThread = Thread(target=EventLoop.work)
    loopThread.start()

    # Configure and start device server ...
    serverId = f'cppServer/{uuid.uuid4()}'
    config = Hash("serverId", serverId)
    startDeviceServer(config)
    # Create DeviceClient ...
    clientId = str(uuid.uuid4())
    c = DeviceClient(clientId)
    assert c.getInstanceId() == clientId
    assert serverId in c.getServers()

    # instantiate (no wait!) 'data/prop/<uuid>' device ... timeout = 5secs
    # synopsis: instantiateNoWait(serverId, classId, config)
    deviceId = f'data/prop/{uuid.uuid4()}'
    c.instantiateNoWait(serverId, 'PropertyTest', Hash('deviceId', deviceId))
    trials = 1000
    while trials > 0 and deviceId not in c.getDevices(serverId):
        time.sleep(0.002)
        trials -= 1
    assert deviceId in c.getDevices(serverId)

    assert c.get(deviceId, 'node.counter') == 0

    c.setNoWait(deviceId, 'node.counter', 10)
    trials = 1000
    while trials > 0 and c.get(deviceId, 'node.counter') != 10:
        time.sleep(0.02)
        trials -= 1
    assert c.get(deviceId, 'node.counter') == 10
    assert c.get(deviceId, 'node.counterReadOnly') == 10

    # increment counter
    for i in range(3):
        c.executeNoWait(deviceId, 'node.increment')
    trials = 1000
    while trials > 0 and c.get(deviceId, 'node.counter') != 13:
        time.sleep(0.02)
        trials -= 1

    assert c.get(deviceId, 'node.counter') == 13
    assert c.get(deviceId, 'node.counterReadOnly') == 10

    # shutdown device ...
    c.killDeviceNoWait(deviceId)
    trials = 1000
    while trials > 0 and deviceId in c.getDevices(serverId):
        time.sleep(0.002)
        trials -= 1
    assert deviceId not in c.getDevices(serverId)

    # stop and de-register device server...
    stopDeviceServer(serverId)
    Logger.reset()
    EventLoop.stop()
    loopThread.join()


def test_slots_with_args():

    # Run CPP event loop in background ...
    loopThread = Thread(target=EventLoop.work)
    loopThread.start()

    deviceId = "slotWithArgsTester"
    sigSlot = SignalSlotable(deviceId, Hash(), 60, Hash())

    product = 0  # Updated by argSlotMultiplyInternal

    def argSlotMultiplyInternal(num1, num2):
        """A slot with args that returns no value"""
        nonlocal product
        product = num1 * num2

    def argSlotMultiply(num1, num2):
        """A slot with args that returns a single value"""
        nonlocal sigSlot
        sigSlot.reply(num1 * num2)

    def argSlotDivide(dividend, divisor):
        """A slot with args that returns two values"""
        nonlocal sigSlot
        sigSlot.reply(dividend // divisor, dividend % divisor)

    def argSlotThreeCases(input_str):
        """A slot with arg that returns three values"""
        nonlocal sigSlot
        sigSlot.reply(input_str.upper(), input_str.lower(), input_str)

    def argSlotZahlen(digit):
        """A slot with arg that returns four values"""
        nonlocal sigSlot
        digit_dict = {
            "ptBR": ["Zero", "Um", "Dois", "Três", "Quatro", "Cinco", "Seis",
                     "Sete", "Oito", "Nove"],
            "enUS": ["Zero", "One", "Two", "Three", "Four", "Five", "Six",
                     "Seven", "Eight", "Nine"],
            "deDE": ["Null", "Eins", "Zwei", "Drei", "Vier", "Fünf", "Sechs",
                     "Sieben", "Acht", "Neun"],
            "roman": ["? (Romans didn't have zero)", "I", "II", "III", "IV",
                      "V", "VI", "VII", "VIII", "IX"]
        }
        idx = digit % 10
        sigSlot.reply(digit_dict["ptBR"][idx], digit_dict["enUS"][idx],
                      digit_dict["deDE"][idx], digit_dict["roman"][idx])

    sigSlot.start()
    cli = DeviceClient("DeviceClientUnderTest")

    # Slot with two args and no return value (actually returns an empty tuple)
    sigSlot.registerSlot(argSlotMultiplyInternal)
    emptyTuple = cli.executeN(deviceId, "argSlotMultiplyInternal", 12, 4)
    assert product == 48
    assert len(emptyTuple) == 0

    # Slot with two args and one return value
    sigSlot.registerSlot(argSlotMultiply)
    multResult, = cli.executeN(deviceId, "argSlotMultiply", 12, 4)
    assert multResult == 48

    # Slot with two args and two return values
    sigSlot.registerSlot(argSlotDivide)
    (quotient, remainder) = cli.executeN(deviceId, "argSlotDivide", 12, 4)
    assert quotient == 3
    assert remainder == 0

    # Slot with one arg and three return values
    sigSlot.registerSlot(argSlotThreeCases)
    (allUp, allLow, echoed) = cli.executeN(deviceId,
                                           "argSlotThreeCases", "Input_Str",
                                           timeoutInSeconds=20)
    assert allUp == "INPUT_STR"
    assert allLow == "input_str"
    assert echoed == "Input_Str"

    # Slot with one arg and four return values
    sigSlot.registerSlot(argSlotZahlen)
    (ptBr, enUS, deDE, roman) = cli.executeN(
        deviceId, "argSlotZahlen", 5, timeoutInSeconds=15)
    assert ptBr == "Cinco"
    assert enUS == "Five"
    assert deDE == "Fünf"
    assert roman == "V"

    EventLoop.stop()
    loopThread.join()
