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

import karabind
import pytest

import karathon


@pytest.mark.parametrize(
    "EventLoop, DeviceClient, Hash, Schema, startDeviceServer,"
    "stopDeviceServer, Logger",
    [
     (karathon.EventLoop, karathon.DeviceClient, karathon.Hash,
      karathon.Schema, karathon.startDeviceServer, karathon.stopDeviceServer,
      karathon.Logger),
     (karabind.EventLoop, karabind.DeviceClient, karabind.Hash,
      karabind.Schema, karabind.startDeviceServer, karabind.stopDeviceServer,
      karabind.Logger)
     ])
def test_device_client_sync_api(EventLoop, DeviceClient, Hash, Schema,
                                startDeviceServer, stopDeviceServer, Logger):
    # Run CPP event loop in background ...
    loopThread = Thread(target=EventLoop.work)
    loopThread.start()

    # Create DeviceClient ... generated clientId ...
    c = DeviceClient()
    # test getInstanceId
    clientId = c.getInstanceId()
    (host, klass, pid) = clientId.split('_', 3)
    assert host == socket.gethostname()
    assert klass == 'DeviceClient'
    assert pid == str(os.getpid())

    # Test getPropertyHistory
    # Expect TymeoutError since DataLogReader is not running.
    # To reduce waiting time, set new internal timeout
    c.setInternalTimeout(50)

    with pytest.raises(RuntimeError):
        c.getPropertyHistory("data/gen/22", "int32Property", "2023-10-25",
                             "2023-11-01")

    # Test getConfigurationFromPast...
    with pytest.raises(RuntimeError):
        c.getConfigurationFromPast("data/gen/22", "2023-11-01")

    # Test listConfigurationFromName...
    # with pytest.raises(RuntimeError):
    h = c.listConfigurationFromName("data/gen/22", "config1")
    assert h['success'] is False
    assert 'timed out' in h['reason']

    # Test 'saveConfigurationFromName'...
    (success, errmsg) = c.saveConfigurationFromName(
            "config1", ["data/gen/22", "data/gen/33"],
            "Description for config1", 2, "johnsmith")
    assert success is False
    assert 'timed out' in errmsg

    # Restore default ...
    c.setInternalTimeout(3000)

    serverId = f'cppServer/{uuid.uuid4()}'
    deviceId = f'data/prop/{uuid.uuid4()}'

    # test registerInstance{New,Update,Gone}Monitor...

    def onInstanceNew(topologyEntry):
        # topologyEntry is Hash
        assert isinstance(topologyEntry, Hash)
        if topologyEntry.getKeys()[0] == 'client':
            cinfo = topologyEntry['client']
            assert clientId == cinfo.getKeys()[0]
            assert cinfo.getAttribute(clientId, 'type') == 'client'
            assert (cinfo.getAttribute(clientId, 'lang') == 'bound' or
                    cinfo.getAttribute(clientId, 'lang') == 'cpp')
            assert cinfo.getAttribute(clientId, 'host') == host
            assert cinfo.getAttribute(clientId, 'status') == "ok"
            print("\nclient entry new     -------------------------------- OK")
        elif topologyEntry.getKeys()[0] == 'server':
            sinfo = topologyEntry['server']
            assert sinfo.getAttribute(serverId, 'type') == 'server'
            assert sinfo.getAttribute(serverId, 'lang') == 'cpp'
            assert sinfo.getAttribute(serverId, 'host') == host
            assert sinfo.getAttribute(serverId, 'serverId') == serverId
            assert sinfo.getAttribute(serverId, 'log') == "FATAL"
            print("server entry new     -------------------------------- OK")
        elif topologyEntry.getKeys()[0] == 'device':
            dinfo = topologyEntry['device']
            assert dinfo.getAttribute(deviceId, 'type') == 'device'
            assert dinfo.getAttribute(deviceId, 'classId') == 'PropertyTest'
            assert dinfo.getAttribute(deviceId, 'host') == host
            assert dinfo.getAttribute(deviceId, 'status') == "ok"
            print("device entry new     -------------------------------- OK")

    def onInstanceUpdated(topologyEntry):
        # topologyEntry is Hash
        assert isinstance(topologyEntry, Hash)
        if topologyEntry.getKeys()[0] == 'client':
            cinfo = topologyEntry['client']
            assert clientId == cinfo.getKeys()[0]
            assert cinfo.getAttribute(clientId, 'type') == 'client'
            assert (cinfo.getAttribute(clientId, 'lang') == 'bound' or
                    cinfo.getAttribute(clientId, 'lang') == 'cpp')
            assert cinfo.getAttribute(clientId, 'host') == host
            assert cinfo.getAttribute(clientId, 'status') == "ok"
            print("client entry updated -------------------------------- OK")
        elif topologyEntry.getKeys()[0] == 'server':
            sinfo = topologyEntry['server']
            assert sinfo.getAttribute(serverId, 'type') == 'server'
            assert sinfo.getAttribute(serverId, 'lang') == 'cpp'
            assert sinfo.getAttribute(serverId, 'host') == host
            assert sinfo.getAttribute(serverId, 'serverId') == serverId
            assert sinfo.getAttribute(serverId, 'log') == "FATAL"
            print("server entry updated -------------------------------- OK")
        elif topologyEntry.getKeys()[0] == 'device':
            dinfo = topologyEntry['device']
            assert dinfo.getAttribute(deviceId, 'type') == 'device'
            assert dinfo.getAttribute(deviceId, 'classId') == 'PropertyTest'
            assert dinfo.getAttribute(deviceId, 'host') == host
            assert dinfo.getAttribute(deviceId, 'status') == "ok"
            print("device entry updated -------------------------------- OK")

    def onInstanceGone(instId, info):
        assert isinstance(instId, str)
        assert isinstance(info, Hash)
        if info['type'] == 'client':
            assert instId == clientId
            assert info['host'] == host
            assert info['status'] == "ok"
            print("client entry gone    -------------------------------- OK")
        elif info['type'] == 'server':
            assert instId == serverId
            assert info['serverId'] == serverId
            assert info['lang'] == 'cpp'
            assert info['host'] == host
            assert info['log'] == "FATAL"
            print("server entry gone    -------------------------------- OK")
        elif info['type'] == 'device':
            assert instId == deviceId
            assert info['classId'] == 'PropertyTest'
            assert info['host'] == host
            assert info['status'] == "ok"
            print("device entry gone    -------------------------------- OK")

    def onSchemaUpdated(devId, schema):
        assert isinstance(devId, str)
        assert isinstance(schema, Schema)
        print('device schema updated ------------------------------- OK')

    c.registerInstanceNewMonitor(onInstanceNew)
    c.registerInstanceUpdatedMonitor(onInstanceUpdated)
    c.registerInstanceGoneMonitor(onInstanceGone)
    c.registerSchemaUpdatedMonitor(onSchemaUpdated)
    c.enableInstanceTracking()

    # Configure and start device server ...
    # ... and make sure that plugins contains DataGenerator
    config = Hash("serverId", serverId, "scanPlugins", False)
    startDeviceServer(config)
    # Test getServers...
    assert serverId in c.getServers()

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
    assert serverId in c.getServers()
    # Test getClasses...
    for i in range(30):
        classes = c.getClasses(serverId)
        if classes:
            break
        time.sleep(1)

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
    #     synopsis: instantiate(serverId, classId, config, timeout)
    c.instantiate(serverId, Hash('PropertyTest.deviceId', deviceId), 5)
    # Test getDevices...
    assert deviceId in c.getDevices(serverId)

    # Test get/set ...
    assert c.get(deviceId, 'node.counter') == 0
    c.set(deviceId, 'node.counter', 10)
    assert c.get(deviceId, 'node.counter') == 10
    assert c.get(deviceId, 'node.counterReadOnly') == 10

    # Switch on monitor for device property
    def onPropertyChange(devId, key, value, ts):
        assert isinstance(devId, str)
        assert isinstance(key, str)
        assert isinstance(value, int)
        # assert isinstance(ts, Timestamp)
        print(f'\tprop changed: {devId} {key} -> {value} ({ts})')

    c.registerPropertyMonitor(deviceId, 'int32Property', onPropertyChange)

    # test 'set' ...
    c.set(deviceId, 'int32Property', 48000000)
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
    # Test getLastConfiguration...
    last_conf = c.getLastConfiguration(deviceId)
    assert last_conf['success'] is False
    assert 'config' in last_conf
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
    def onDeviceParametersChange(devId, configChanged):
        assert isinstance(devId, str)
        assert isinstance(configChanged, Hash)
        print('\tdevice parameters changed OK')

    c.registerDeviceMonitor(deviceId, onDeviceParametersChange)

    for i in range(3, 5):
        c.execute(deviceId, 'node.increment', 5)

    # Switch off monitoring for device properties...
    c.unregisterDeviceMonitor(deviceId)

    for i in range(6, 8):
        c.execute(deviceId, 'node.increment', 5)

    # test getDataSourceSchemaAsHash
    h = c.getDataSourceSchemaAsHash(deviceId)
    assert isinstance(h, Hash)
    assert h.getKeys()[0] == deviceId
    assert h.getAttribute(deviceId, "classId") == "PropertyTest"
    print("data source as hash  -------------------------------- OK")

    # test 'setAttribute'
    c.setAttribute(deviceId, 'int32Property', 'blabla', 12)

    lck = c.lock(deviceId, recursive=False, timeout=0)
    print("device locked        -------------------------------- "
          f"{lck.valid()}")
    lck.unlock()

    # shutdown device ...
    # Test killDevice...
    c.killDevice(deviceId, 10)
    # de-register device server...
    stopDeviceServer(serverId)
    Logger.reset()
    EventLoop.stop()
    loopThread.join()


@pytest.mark.parametrize(
    "EventLoop, DeviceClient, Hash, startDeviceServer, stopDeviceServer, "
    "Logger",
    [
     (karathon.EventLoop, karathon.DeviceClient, karathon.Hash,
      karathon.startDeviceServer, karathon.stopDeviceServer, karathon.Logger),
     (karabind.EventLoop, karabind.DeviceClient, karabind.Hash,
      karabind.startDeviceServer, karabind.stopDeviceServer, karabind.Logger)
     ])
def test_device_client_async_api(EventLoop, DeviceClient, Hash,
                                 startDeviceServer, stopDeviceServer, Logger):
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
    schema = c.getDeviceSchemaNoWait(deviceId)
    assert schema.empty() is True

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
