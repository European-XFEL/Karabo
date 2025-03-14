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
import os
import socket
import time
import uuid
from threading import Thread

import pytest

from karabo.bound import (
    DeviceClient, EventLoop, Hash, Logger, Schema, startDeviceServer,
    stopDeviceServer)

timeoutSec = 15  # test_zcore_device_client2.py::test_device_client_sync_api
Logger.configure(Hash())


def test_device_client_sync_api():
    # Run CPP event loop in background ...
    loopThread = Thread(target=EventLoop.work)
    loopThread.start()

    # Create DeviceClient ... generated clientId ...
    c = DeviceClient()
    # test getInstanceId
    clientId = c.getInstanceId()
    (host, klass, pid) = clientId.split('_', 3)
    assert host == socket.gethostname().split(".", 1)[0]  # split domain away
    assert klass == 'DeviceClient'
    assert pid == str(os.getpid())

    # Test getPropertyHistory
    # Expect TymeoutError since DataLogReader is not running.
    # To reduce waiting time, set new internal timeout
    cachedTimeout = c.getInternalTimeout()
    c.setInternalTimeout(50)

    with pytest.raises(RuntimeError):
        c.getPropertyHistory("data/gen/22", "int32Property", "2023-10-25",
                             "2023-11-01")

    # Test getConfigurationFromPast...
    with pytest.raises(RuntimeError):
        c.getConfigurationFromPast("data/gen/22", "2023-11-01")

    # Test listConfigurationFromName...
    h = c.listConfigurationFromName("data/gen/22", "config1")
    assert h['success'] is False
    assert 'timed out' in h['reason']

    # Test 'saveConfigurationFromName'...
    (success, errmsg) = c.saveConfigurationFromName(
        "config1", ["data/gen/22", "data/gen/33"])
    assert success is False
    assert 'timed out' in errmsg

    # Restore default ...
    c.setInternalTimeout(cachedTimeout)

    # avoid id clash between test runs
    serverId = f'cppServer/{Hash.__module__}'
    deviceId = f'data/prop/{Hash.__module__}'

    # test registerInstance{New,Update}Monitor...
    instanceNewArg = instanceUpdatedArg = None

    def onInstanceNew(topologyEntry):
        nonlocal instanceNewArg
        instanceNewArg = topologyEntry

    def onInstanceUpdated(topologyEntry):
        nonlocal instanceUpdatedArg
        instanceUpdatedArg = topologyEntry

    c.registerInstanceNewMonitor(onInstanceNew)
    c.registerInstanceUpdatedMonitor(onInstanceUpdated)
    c.enableInstanceTracking()

    # Configure and start device server ...
    logLevel = "FATAL"
    config = Hash("serverId", serverId, "Logger.priority", logLevel)

    # Test getServers...
    startDeviceServer(config)

    max_tries = 10
    n = 0
    server_not_present = True

    while n < max_tries and server_not_present:
        time.sleep(0.1)
        server_not_present = serverId not in c.getServers()
        n += 0.1

    assert not server_not_present

    # Check that instanceNew handler was called properly
    # (If 'is Hash' fails sometimes, better check first in a loop with timeout.
    # But as the client knows the server, instanceNew should have been called.)
    assert type(instanceNewArg) is Hash
    assert instanceNewArg.has("server." + serverId)
    serverInfo = instanceNewArg["server"]
    assert serverInfo.getAttribute(serverId, "type") == "server"
    assert serverInfo.getAttribute(serverId, "lang") == "cpp"
    assert serverInfo.getAttribute(serverId, "host") == host
    assert serverInfo.getAttribute(serverId, "serverId") == serverId
    assert serverInfo.getAttribute(serverId, "log") == logLevel

    # This test fails because instanceUpdate is not sent anymore by the C++
    # server. Removed for now, eventually, it will have to be revived; see
    # https://git.xfel.eu/Karabo/Framework/-/issues/983
    #
    # # Check that instanceUpdated handler was called properly. Note that
    # # here we rely on the fact that C++ server sends instanceUpdate soon
    # # after instanceNew, extending the known deviceClasses. (Instead of
    # # sending  instanceNew after plugins are known. If that changes, find
    # # something else for testing instanceUpdateHandler...)
    # timeout = 10
    # while timeout > 0:
    #     if type(instanceUpdatedArg) is Hash:
    #         break
    #     timeout -= 0.1
    #     time.sleep(0.1)

    # assert type(instanceUpdatedArg) is Hash
    # assert instanceUpdatedArg.has("server." + serverId)
    # serverUpdate = instanceUpdatedArg["server"]
    # assert serverUpdate.getAttribute(serverId, "type") == "server"
    # klasses = serverUpdate.getAttribute(serverId, "deviceClasses")
    # assert "PropertyTest" in klasses

    # Test getSystemTopology ...
    topology = c.getSystemTopology()
    assert isinstance(topology, Hash) is True
    assert 'client' in topology
    assert 'server' in topology
    assert serverId in topology["server"]
    # Test getSystemInformation
    system_information = c.getSystemInformation()
    assert isinstance(system_information, Hash)
    assert 'client' in system_information
    assert 'server' in system_information

    # Test exists...
    a, b = c.exists(serverId)
    assert a is True
    # Test getClasses... (can test directly since waited already for update)
    classes = c.getClasses(serverId)
    assert isinstance(classes, list)
    assert "PropertyTest" in classes

    # Test getClassProperties...
    props = c.getClassProperties(serverId, "PropertyTest")
    assert isinstance(props, list) is True
    assert 'classId' in props
    assert 'deviceId' in props
    assert 'serverId' in props
    assert 'karaboVersion' in props

    # Test instantiate...
    instanceNewArg = None
    #     synopsis: instantiate(serverId, classId, config, timeout)
    c.instantiate(serverId, Hash('PropertyTest.deviceId', deviceId),
                  timeoutSec)
    # Test getDevices...
    assert deviceId in c.getDevices(serverId)
    assert type(instanceNewArg) is Hash
    assert instanceNewArg.has("device." + deviceId)
    deviceInfo = instanceNewArg["device"]
    assert deviceInfo.getAttribute(deviceId, "type") == "device"
    assert deviceInfo.getAttribute(deviceId, "host") == host
    assert deviceInfo.getAttribute(deviceId, "serverId") == serverId

    # Test get/set ...
    assert c.get(deviceId, 'node.counter') == 0
    c.set(deviceId, 'node.counter', 10)
    assert c.get(deviceId, 'node.counter') == 10
    assert c.get(deviceId, 'node.counterReadOnly') == 10

    # Schema update monitor
    onSchemaUpdatedArg1 = onSchemaUpdatedArg2 = None

    def onSchemaUpdated(devId, schema):
        nonlocal onSchemaUpdatedArg1, onSchemaUpdatedArg2
        onSchemaUpdatedArg1 = devId
        onSchemaUpdatedArg2 = schema

    c.registerSchemaUpdatedMonitor(onSchemaUpdated)
    c.execute(deviceId, "slotUpdateSchema")
    assert type(onSchemaUpdatedArg1) is str
    assert onSchemaUpdatedArg1 == deviceId
    assert type(onSchemaUpdatedArg2) is Schema

    onPropertyChangeArgs = None

    # Switch on monitor for device property
    def onPropertyChange(devId, key, value, ts):
        nonlocal onPropertyChangeArgs
        onPropertyChangeArgs = (devId, key, value, ts)

    c.registerPropertyMonitor(deviceId, 'int32Property', onPropertyChange)

    # test 'set' ...
    c.set(deviceId, 'int32Property', 48000000)
    assert type(onPropertyChangeArgs) is tuple
    assert onPropertyChangeArgs[0] == deviceId
    assert onPropertyChangeArgs[1] == "int32Property"
    assert onPropertyChangeArgs[2] == 48000000
    assert onPropertyChangeArgs[3].toIso8601Ext().endswith("Z")

    c.set(deviceId, 'int32Property', 777)

    c.unregisterPropertyMonitor(deviceId, 'int32Property')

    # Test execute...
    # increment node.counter ...
    for i in range(3):
        c.execute(deviceId, 'node.increment', 5)

    assert c.get(deviceId, 'node.counter') == 13
    assert c.get(deviceId, 'node.counterReadOnly') == 10

    # Test getDeviceSchema...
    device_schema = c.getDeviceSchema(deviceId)
    assert 'outputFrequency' in device_schema.getParameterHash()
    assert 'output.connections' in device_schema.getParameterHash()
    # Test getActiveSchema...
    active_schema = c.getActiveSchema(deviceId)
    assert 'int16Property' in active_schema.getParameterHash()
    assert 'table' in active_schema.getParameterHash()
    # Test getClassSchema...
    class_schema = c.getClassSchema(serverId, 'PropertyTest')
    assert 'karaboVersion' in class_schema.getParameterHash()
    assert 'boolProperty' in class_schema.getParameterHash()
    # Test getProperties...
    props = c.getProperties(deviceId)
    assert 'status' in props
    assert 'vectors.boolProperty' in props
    # Test getCurrentlyExecutableCommands...
    comms = c.getCurrentlyExecutableCommands(deviceId)
    assert 'writeOutput' in comms
    assert 'eosOutput' in comms
    # Test getCurrentlySettableProperties...
    props = c.getCurrentlySettableProperties(deviceId)
    assert 'int32Property' in props
    assert 'boolProperty' in props
    # Test getOutputChannelNames...
    names = c.getOutputChannelNames(deviceId)
    if not isinstance(names, list):
        names = list(names)
    assert names == ['output']
    # Test getOutputChannelSchema...
    schema = c.getOutputChannelSchema(deviceId, 'output')
    assert 'node.ndarray.shape' in schema
    assert 'node.image.pixels.data' in schema
    assert 'node.image.pixels.shape' in schema

    # c.setDeviceMonitorInterval(5)

    # Switch on the monitoring for device properties...
    onDeviceParametersChangeArg1 = onDeviceParametersChangeArg2 = None

    def onDeviceParametersChange(devId, configChanged):
        nonlocal onDeviceParametersChangeArg1, onDeviceParametersChangeArg2
        onDeviceParametersChangeArg1 = devId
        onDeviceParametersChangeArg2 = configChanged

    c.registerDeviceMonitor(deviceId, onDeviceParametersChange)

    c.execute(deviceId, 'node.increment', 5)
    assert onDeviceParametersChangeArg1 == deviceId
    assert type(onDeviceParametersChangeArg2) is Hash
    assert onDeviceParametersChangeArg2.has("node.counter")

    # Switch off monitoring for device properties...
    c.unregisterDeviceMonitor(deviceId)

    for i in range(6, 8):
        c.execute(deviceId, 'node.increment', 5)

    # XXX: TODO this times out
    # lck = c.lock(deviceId, recursive=False, timeout=timeoutSec)
    # assert lck.valid()
    # lck.unlock()

    # Test instanceGoneMonitor
    instanceGoneArg1 = instanceGoneArg2 = None

    def onInstanceGone(instId, info):
        nonlocal instanceGoneArg1, instanceGoneArg2
        instanceGoneArg1 = instId
        instanceGoneArg2 = info

    c.registerInstanceGoneMonitor(onInstanceGone)

    # shutdown device ...
    # Test killDevice and gone monitor...
    c.killDevice(deviceId, 10)
    assert type(instanceGoneArg1) is str
    assert instanceGoneArg1 == deviceId
    assert type(instanceGoneArg2) is Hash
    assert instanceGoneArg2["type"] == "device"
    assert instanceGoneArg2["classId"] == "PropertyTest"
    assert instanceGoneArg2["host"] == host
    assert instanceGoneArg2["status"] == "ok"

    # de-register device server...
    stopDeviceServer(serverId)
    Logger.reset()
    EventLoop.stop()
    loopThread.join()


def test_device_client_async_api():
    # Run CPP event loop in background ...
    loopThread = Thread(target=EventLoop.work)
    loopThread.start()

    # Start device server (suppress plugins scan) ...
    serverId = f'cppServer/{uuid.uuid4()}'
    config = Hash("serverId", serverId, "scanPlugins", False)
    startDeviceServer(config)
    # Create DeviceClient ...
    clientId = str(uuid.uuid4())
    c = DeviceClient(clientId)
    assert c.getInstanceId() == clientId

    # Test setInternalTimeout/getInternalTimeout
    assert c.getInternalTimeout() == 3000
    c.setInternalTimeout(1000)
    to = c.getInternalTimeout()
    assert to == 1000

    # Test instantiateNoWait ...
    #     synopsis: instantiateNoWait(serverId, config)
    deviceId = f'data/prop/{uuid.uuid4()}'
    a, b = c.exists(deviceId)
    assert a is False
    c.instantiateNoWait(serverId, Hash('PropertyTest.deviceId', deviceId))

    trials = 1000
    while trials > 0 and deviceId not in c.getDevices(serverId):
        time.sleep(0.002)
        trials -= 1
    a, b = c.exists(deviceId)
    assert a is True
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

    # getDeviceSchemaNoWait (pre-cache) ...
    trials = 1000
    while trials > 0 and c.getDeviceSchemaNoWait(deviceId).empty():
        time.sleep(0.02)
        trials -= 1
    schema = c.getDeviceSchemaNoWait(deviceId)
    assert schema.empty() is False

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
