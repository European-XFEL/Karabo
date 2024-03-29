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
from threading import Thread

from karabo.bound import (
    DeviceClient, EventLoop, Hash, Logger, startDeviceServer, stopDeviceServer)


def test_device_client_sync_api():
    # Run CPP event loop in background ...
    loopThread = Thread(target=EventLoop.work)
    loopThread.start()

    # Configure and start device server ...
    # ... and make sure that plugins contains DataGenerator
    serverId = f'cppServer/{uuid.uuid4()}'
    config = Hash("serverId", serverId, "scanPlugins", False)
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

    last_conf = c.getLastConfiguration(deviceId)
    assert last_conf['success'] is False
    assert 'config' in last_conf

    comms = c.getCurrentlyExecutableCommands(deviceId)
    assert 'writeOutput' in comms
    assert 'eosOutput' in comms

    props = c.getCurrentlySettableProperties(deviceId)
    assert 'int32Property' in props
    assert 'boolProperty' in props

    names = c.getOutputChannelNames(deviceId)
    if not isinstance(names, list):
        names = list(names)
    assert names == ['output']

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
    config = Hash("serverId", serverId, "scanPlugins", False)
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
