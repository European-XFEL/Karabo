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
"""This is the machinery to communicate with remote devices.

Usually, one communicated with a remote device by creating a device
proxy :class:`Proxy` using :func:`getDevice`. This is a convenient way to
access a remote device often.

A more rare usecase is that many remote devices should be accessed, but
only very simple. This can be achieved by functions which operate directly
on the deviceId, without going through the hazzle of creating a device
proxy. """
import asyncio
from asyncio import gather, get_event_loop, sleep
from contextlib import asynccontextmanager, contextmanager
from copy import deepcopy
from time import time
from weakref import ref

import dateutil.parser
import dateutil.tz

from karabo.common.services import KARABO_CONFIG_MANAGER
from karabo.native import (
    Hash, KaraboError, KaraboValue, Schema, Timestamp, isStringSet)

from .configuration import config_changes, sanitize_init_configuration
from .eventloop import synchronize
from .proxy import (
    AutoDisconnectProxyFactory, DeviceClientProxyFactory, ProxyBase,
    ProxyNodeBase)
from .synchronization import firstCompleted
from .utils import get_property_hash


class OneShotQueue(asyncio.Future):
    """ This is a future that looks like a queue

    It may be registered in the list of queues for a property, and is removed
    from that property once fired """

    def put_nowait(self, value):
        if not self.done():
            self.set_result(value)


@synchronize
async def getInstanceInfo(device):
    """ Get the `instanceInfo` Hash of a device"""
    if isinstance(device, ProxyBase):
        device = device._deviceId
    rand_ping = 1
    track = True  # Not used, kept for backward compatibility
    return await get_instance().call(device, "slotPing", device,
                                     rand_ping, track)


@synchronize
async def waitUntilNew(*props):
    """Wait until new futures provide a result

    The futures can be provided as in::

        waitUntilNew(device.speed, device.targetPosition)

    where device is a proxy. The first completed future will return the
    function. This function can be used to wait for a global property event
    on a device as well::

        waitUntilNew(device)

    Note that for this to work, it is necessary that all the devices used in
    the future parameters are connected while we are waiting.

    For the asynchronous case a :func:`asyncio.wait_for` should be combined
    if a `timeout` is desired.
    """
    futures = []
    for prop in props:
        if isinstance(prop, ProxyBase):
            future = OneShotQueue(loop=prop._device._ss.loop)
            prop._queues[None].add(future)
        else:
            proxy = prop._parent
            future = OneShotQueue(loop=proxy._device._ss.loop)
            proxy._queues[prop.descriptor.longkey].add(future)
        futures.append(future)
    await firstCompleted(*futures)


def get_utc_string(date: None | str | Timestamp) -> str:
    """Parse date via dateutil.parser.parse and return UTC iso string

    :param date: None means now and Timestamp objects are understood
    """
    if date is None:
        date = Timestamp()
    if isinstance(date, Timestamp):
        date = date.toLocal()
    d = dateutil.parser.parse(date)
    if d.tzinfo is None:
        d = d.replace(tzinfo=dateutil.tz.tzlocal())
    return d.astimezone(dateutil.tz.tzutc()).replace(tzinfo=None).isoformat()


@synchronize
async def call(device, target_slot, *args):
    """Call a target slot from a device

    :param device: deviceId or proxy
    :param target_slot: slot to be called
    """
    if isinstance(device, ProxyBase):
        device = device._deviceId
    return (await get_instance().call(device, target_slot, *args))


def callNoWait(device, target_slot, *args):
    """Call a target slot from a device without waiting

    :param device: deviceId or proxy
    :param target_slot: slot to be called
    """
    if isinstance(device, ProxyBase):
        device = device._deviceId
    get_instance().callNoWait(device, target_slot, *args)


@synchronize
async def getSchema(device, onlyCurrentState=False):
    """Get a schema from a target device

    :param device: deviceId or proxy
    :param onlyCurrentState: Boolean for the state dependent schema
                             The default is `False`.

    :returns: Full Schema object
    """
    if isinstance(device, ProxyBase):
        if not onlyCurrentState:
            return Schema(name=device.classId, hash=device._schema_hash)
        else:
            device = device._deviceId

    schema, _ = await get_instance().call(device, "slotGetSchema",
                                          onlyCurrentState)
    return schema


@synchronize
async def getClassSchema(serverId, classId):
    """Get the class schema for `classId` from server with `serverId`

    :param serverId: the serverId of the server
    :param classId: the classId of the device class

    :returns: Full Schema object
    """
    schema, _, _ = await get_instance().call(serverId, "slotGetClassSchema",
                                             classId)
    return schema


@synchronize
async def getConfiguration(device):
    """Get a configuration from a target device

    :param device: deviceId or proxy

    NOTE: When passing a proxy, the proxy is updated and the configuration
    if extracted from the proxy. In this case, `ChoiceOfNodes` and
    `ListOfNodes` (mostly) properties will be omitted in the configuration.

    :returns: Configuration Hash
    """
    if isinstance(device, ProxyBase):
        await device._update()
        return device.configurationAsHash()

    hsh, _ = await get_instance().call(device, "slotGetConfiguration")
    return hsh


@synchronize
async def getProperties(device, properties):
    """Get the property configuration from a target device

    :param device: deviceId or proxy
    :param properties: list of keys

    Note: When passing a proxy, the proxy is updated and the
    properties are derived from the proxy.

    :returns: Configuration Hash
    """
    if isinstance(properties, str):
        properties = [properties]

    if isinstance(device, ProxyBase):
        await device._update()
        return get_property_hash(device, properties)
    slot = "slotGetConfigurationSlice"
    return await get_instance().call(device, slot, Hash("paths", properties))


@synchronize
async def compareDeviceConfiguration(device_a, device_b):
    """Compare device configurations (key, values) of two devices

    The changes are provided in a list for comparison::

        >>> h = compareDeviceConfiguration(device_a, device_b)
        >>> h
        <disableEpsilonFeedback{}: [True, False]>

    :returns: changes Hash
    """
    a_schema, b_schema = await gather(
        getSchema(device_a), getSchema(device_b))
    a_conf, b_conf = await gather(
        getConfiguration(device_a), getConfiguration(device_b))

    # Take into account only reconfigurable and init parameters!
    a_san = sanitize_init_configuration(a_schema, a_conf)
    b_san = sanitize_init_configuration(b_schema, b_conf)
    changes = config_changes(a_san, b_san)

    return changes


@synchronize
async def compareDeviceWithPast(device, timepoint):
    """Compare device configuration (key, values) between present and past

    The changes are provided in a list for comparison::

        changes = [PRESENT | PAST]

        >>> h = compareDeviceWithPast(device, minutesAgo(2))
        >>> h
        <disableEpsilonFeedback{}: [True, False]>

    :param timepoint: The timepoint to compare

    :returns: changes Hash
    """
    # Get the config and schema first!
    a_conf, a_schema = await gather(
        getConfiguration(device), getSchema(device))
    b_conf, b_schema = await _get_configuration_from_past(
        device, timepoint)
    # Take into account only reconfigurable and init parameters!
    a_san = sanitize_init_configuration(a_schema, a_conf)
    b_san = sanitize_init_configuration(b_schema, b_conf)
    changes = config_changes(a_san, b_san)

    return changes


@synchronize
async def compareConfigurationsFromPast(device, first_timepoint,
                                        second_timepoint):
    """Compare device configuration (key, values) between two timepoints

    The changes are provided in a list for comparison::

        changes = [FIRST | SECOND]

        >>> h = compareConfigurationsFromPast(device, daysAgo(7), daysAgo(14))
        >>> h
        <disableEpsilonFeedback{}: [True, False]>

    :param first_timepoint: The first timepoint to compare
    :param second_timepoint: The second timepoint to compare

    :returns: changes Hash
    """
    # Get the config and schema first!
    a_conf, a_schema = await _get_configuration_from_past(
        device, first_timepoint)
    b_conf, b_schema = await _get_configuration_from_past(
        device, second_timepoint)
    # Take into account only reconfigurable and init parameters!
    a_san = sanitize_init_configuration(a_schema, a_conf)
    b_san = sanitize_init_configuration(b_schema, b_conf)
    changes = config_changes(a_san, b_san)

    return changes


@synchronize
async def _getLogReaderId(deviceId):
    # this method contains a lot of hard-coded strings. It follows
    # GuiServerDevice::onGetPropertyHistory. One day we should
    # de-hard-code both.
    instance = get_instance()
    loggerId = f"DataLogger-{deviceId}"
    loggerMap = getattr(instance, "loggerMap", None)
    if loggerMap is None or loggerId not in loggerMap:
        instance.loggerMap = await instance.call(
            "Karabo_DataLoggerManager_0", "slotGetLoggerMap")
        if loggerId not in instance.loggerMap:
            raise KaraboError('no logger for device "{}"'.
                              format(deviceId))
    return f"DataLogReader-{instance.loggerMap[loggerId]}"


@synchronize
async def _getHistory(prop, begin, end, maxNumData, verbose=False):
    try:
        attr = prop.descriptor.longkey
        deviceId = prop._parent._deviceId
    except AttributeError:
        deviceId, attr = str(prop).split(".", 1)

    begin = get_utc_string(begin)
    end = get_utc_string(end)
    reader = await _getLogReaderId(deviceId)
    if verbose:
        message = f"""Requesting history of property '{attr}'
        of device '{deviceId}'
        between {begin} UTC and {end} UTC
        from the DataLogReader device '{reader}'
        """
        print(message)
    r_deviceId, r_attr, data = await get_instance().call(
        reader, "slotGetPropertyHistory", deviceId, attr,
        Hash("from", begin, "to", end, "maxNumData", maxNumData))
    assert r_deviceId == deviceId and r_attr == attr
    return [(float(d["v", "frac"]) / 10 ** 18 + float(d["v", "sec"]),
             "isLast" in d["v", ...], d["v"]) for d in data]


def getHistory(prop, begin,
               end=None, maxNumData=10000, timeout=-1, verbose=False):
    """get the history of device properties

    with this function one can get all values of a property in a given
    timespan::

        getHistory(device.someProperty, "2009-09-01", "2009-09-02")

    returns a list of tuples, which contain all changes of *someProperty*
    between the two given dates. The tuple contains four fields, the
    seconds since 1970-01-01 UTC, the train ID, a flag whether this is
    the last row in a set (typically, the device has been switched off
    afterwards), and the value of the property at that time.

    If you want to directly see the result in a more readable format,
    try the 'printHistory' method.

    Given that a device may not be online by the time one is trying to
    get the history, the property name may also be given as a string::

        getHistory("device.someProperty", "2009-09-01", "2009-09-02")

    The dates of the timespan are parsed using
    :func:`dateutil.parser.parse`, allowing many ways to write the date.
    The most precise way is to write "2009-09-01T15:32:12 UTC", but you may
    omit any part, like "10:32", only giving the time, where we assume
    the current day.  Unless specified otherwise, your local timezone is
    assumed.

    Another parameter, *maxNumData*, may be given, which gives the maximum
    number of data points to be returned. Acceptable values for *maxNumData*
    are in the range [1, 10_000], the default being 10_000. The returned
    data will be reduced appropriately to still span the full timespan.

    An shorthand syntax can be used with the `minutesAgo` and `hourseAgo`
    helper functions:

        getHistory(device.someProperty, hoursAgo(10), hoursAgo(5))
        getHistory(device.someProperty, minutesAgo(10))

    will return the history of `device.someProperty` between 10 and 5 hours ago
    and of the last 10 minutes respectively. The shorthand can be with string
    property parameters as well.

    :param prop: device property
    :param begin: begin of the timespan
    :param end: end of the timespan, None means now
    :param maxNumData: max. number of points to return
                       (downsampling depends on storage backend)
    :param verbose: if True, print request details
    """
    if (isinstance(prop, ProxyBase) or isinstance(prop, str) and
            "." not in prop):
        raise RuntimeError("Parameter Input malformatted")

    return _getHistory(prop, begin, end, maxNumData,
                       timeout=timeout, verbose=verbose)


@synchronize
async def printHistory(prop, begin, end=None, maxNumData=100):
    """Print the history of a device property.

    See getHistory about arguments and their meaning and format
    """
    ret = await getHistory(prop, begin, end, maxNumData=maxNumData,
                           verbose=True)

    def create_lines():
        for item in ret:
            stamp = Timestamp(item[0]).toLocal(sep=' ')
            value = str(item[2])
            # Newline for all stuff with new line (e.g. table) else space
            sep = "\n" if "\n" in value else " "
            yield f"{stamp}{sep}{value}"

    print("\n".join(line for line in create_lines()))


@synchronize
async def _get_configuration_from_past(device, timepoint):
    """The configuration from past implementation to retrieve a configuration
    and schema from the archiving system

    :param device: deviceId or Karabo proxy
    :param timepoint: time point parsed using :func:`dateutil.parser.parse`,
    """
    if isinstance(device, ProxyBase):
        device = device._deviceId
    timepoint = get_utc_string(timepoint)
    instance = get_instance()
    reader = await _getLogReaderId(device)
    slot = "slotGetConfigurationFromPast"
    conf, schema, *_ = await instance.call(reader, slot,
                                           device, timepoint)
    return conf, schema


@synchronize
async def getConfigurationFromPast(device, timepoint):
    """Get the configuration of a deviceId or proxy at a given time::

        getConfigurationFromPast(device, "12:30")

    :returns: A karabo configuration hash of the device at the specified time.

    The date of the time point is parsed using :func:`dateutil.parser.parse`,
    allowing many ways to write the date.
    """
    conf, _ = await _get_configuration_from_past(device, timepoint)
    return conf


@synchronize
async def getSchemaFromPast(device, timepoint):
    """Get the schema of a deviceId or proxy at a given time::

        getSchemaFromPast(deviceId, "12:30")

    Returns a karabo schema object of the device at the specified time.

    The date of the time point is parsed using :func:`dateutil.parser.parse`,
    allowing many ways to write the date.
    """
    _, schema = await _get_configuration_from_past(device, timepoint)
    return schema


@synchronize
async def getConfigurationFromName(device, name):
    """Get the configuration of a deviceId or proxy with a given `name`::

        getConfigurationFromName(device, "run2012")

    :returns: A karabo configuration hash of the device saved under the `name`.
    """
    if isinstance(device, ProxyBase):
        device = device._deviceId
    instance = get_instance()
    slot = "slotGetConfigurationFromName"
    h = Hash("deviceId", device, "name", name)
    reply = await instance.call(KARABO_CONFIG_MANAGER, slot, h)
    config = reply["item.config"]

    return config


@synchronize
async def listConfigurationFromName(device, name_part=''):
    """List the list of configurations of a deviceId with given `name_part`::

        listConfigurationFromName(device, '')

    Returns a list of configuration items of the device. Optionally, a `name
    part` can be provided to filter the configurations on manager side.

    Each configuration item is a Hash containing:

        - name: the configuration name
        - timepoint: the timepoint the configuration of the device was taken
    """
    if isinstance(device, ProxyBase):
        device = device._deviceId
    instance = get_instance()
    slot = "slotListConfigurationFromName"
    h = Hash("deviceId", device, "name", name_part)
    reply = await instance.call(KARABO_CONFIG_MANAGER, slot, h)
    configs = reply["items"]

    return configs


@synchronize
async def listDevicesWithConfiguration():
    """Return the list of devices which have a configuration::

        listDevicesWithConfiguration()

    :returns: List of deviceIds, e.g. ["deviceA", "deviceB"].
    """
    instance = get_instance()
    slot = "slotListDevices"
    reply = await instance.call(KARABO_CONFIG_MANAGER, slot, Hash())
    deviceIds = reply["item"]
    return deviceIds


@synchronize
async def instantiateFromName(
    device: str | ProxyBase, name: str,
        classId: str | None = None, serverId: str | None = None):
    """Instantiate a device from `name` via the ConfigurationManager::

        instantiateFromName(device, name='run2015')

    - device: Mandatory parameter, either deviceId or proxy
    - name: Mandatory parameter, name of configuration
    - classId: Optional parameter for validation of classId
    - serverId: Optional parameter
    """
    if isinstance(device, ProxyBase):
        device = device._deviceId
    h = Hash("deviceId", device, "name", name)
    if classId is not None:
        h["classId"] = classId
    if serverId is not None:
        h["serverId"] = serverId

    instance = get_instance()
    slot = "slotInstantiateDevice"
    reply = await instance.call(KARABO_CONFIG_MANAGER, slot, h)
    success = reply["success"]

    return success


@synchronize
async def saveConfigurationFromName(devices, name):
    """Save configuration(s) in the KaraboConfigurationManager::

        - The parameter `devices` can be a Karabo `proxy`, a list of deviceIds,
          a list of proxies or a mixture of them. It can be as well a single
          deviceId string.

        saveConfigurationFromName(devices, name="proposal2020")
    """
    if isinstance(devices, list):
        devices = [dev._deviceId if isinstance(dev, ProxyBase)
                   else dev for dev in devices]
        # Prepare uniqueness!
        devices = list(set(devices))
    elif isinstance(devices, ProxyBase):
        devices = [devices._deviceId]
    else:
        # A single deviceId as string
        devices = [devices]
    instance = get_instance()
    slot = "slotSaveConfigurationFromName"
    h = Hash("deviceIds", devices, "name", name)
    await instance.call(KARABO_CONFIG_MANAGER, slot, h)


class Queue:
    """A queue of property changes

    This allows you to track all the changes of a property on a remote device.
    An example of usage::

        q = Queue(motor.position)
        new_position = await q.get()
    """

    def __init__(self, prop):
        self.queue = asyncio.Queue()
        prop._parent._queues[prop.descriptor.longkey].add(self.queue)

    @synchronize
    async def get(self):
        return (await self.queue.get())


def get_instance():
    return get_event_loop().instance()


@synchronize
async def waitUntil(condition):
    """Wait until the condition is True

    The condition is typically a lambda function, as in::

        waitUntil(lambda: device.speed > 3)

    The condition will be evaluated each time something changes. Note
    that for this to work, it is necessary that all the devices used in the
    condition are connected while we are waiting (so typically they appear
    in a with statement)"""
    loop = get_event_loop()
    # suspend once to assure the event loop gets a chance to run
    await sleep(0)
    while not condition():
        await loop.waitForChanges()


@synchronize
async def waitWhile(condition):
    """Wait while the condition is True

    The condition is typically a lambda function, as in::

        waitWhile(lambda: state == State.MOVING)

    The condition will be evaluated each time something changes. Note
    that for this to work, it is necessary that all the devices used in the
    condition are connected while we are waiting (so typically they appear
    in a with statement)"""
    loop = get_event_loop()
    # suspend once to assure the event loop gets a chance to run
    await sleep(0)
    while condition():
        await loop.waitForChanges()


class DeviceFuture:

    def __init__(self, base):
        self.base = base
        self.proxy = None

    def __await__(self):
        return self.base.__await__()

    async def __aenter__(self):
        self.proxy = await self
        return (await self.proxy.__aenter__())

    async def __aexit__(self, etype, value, tb):
        if self.proxy is not None:
            return (await self.proxy.__aexit__(etype, value, tb))

    __iter__ = __await__


@synchronize
async def _getDevice(deviceId, sync, initialize,
                     factory=DeviceClientProxyFactory):
    if not isStringSet(deviceId):
        raise KaraboError(
            f"Need a proper 'deviceId' for a proxy, got {str(deviceId)}.")

    instance = get_instance()
    proxy = instance._proxies.get(deviceId)
    if proxy is not None:
        if not isinstance(proxy, factory.Proxy):
            raise KaraboError(
                "do not mix getDevice with connectDevice!\n"
                '(deleting the old proxy with "del proxy" may help)')
        if initialize:
            await proxy.initialize_proxy()
        await proxy.update_proxy()
        return proxy

    futures = instance._proxy_futures
    future = futures.get(deviceId)
    if future is not None:
        return (await asyncio.shield(future))

    async def create():
        try:
            schema, _ = await instance._call_once_alive(
                deviceId, "slotGetSchema", False)

            cls = factory.createProxy(schema)
            proxy = cls(instance, deviceId, sync)
            instance._proxies[deviceId] = proxy
            weakproxy = ref(proxy)

            @asynccontextmanager
            async def killer():
                """kill the proxy once the owning device dies

                In an ideal world, we would let proxies only be used in with
                statements, so that we can control their lifetime.
                But our users will kill us if they have to do that on the
                command line. So we trust the garbage collector to kill
                superfluous proxies, but will finish them off once the
                owning device goes down.
                """
                try:
                    yield
                finally:
                    proxy = weakproxy()
                    if proxy is not None:
                        await proxy.delete_proxy()

            await instance._ss.enter_async_context(killer())
        finally:
            del futures[deviceId]

        # the following is pure black magic. Originally, (git blame should
        # show you where exactly) the following context manager was simply
        # a method of Proxy. Unfortunately, context managers (IMHO
        # illegally) hold a hidden reference to their call attributes,
        # including self, meaning that they cannot be collected. This
        # is why we sneak the proxy into method using a closure.
        closure_proxy = proxy

        @asynccontextmanager
        async def connectSchemaUpdated():
            nonlocal closure_proxy
            await closure_proxy._device._ss.async_connect(
                closure_proxy._deviceId, "signalSchemaUpdated",
                closure_proxy._device.slotSchemaUpdated)
            closure_proxy._schemaUpdateConnected = True
            closure_proxy = ref(closure_proxy)
            try:
                yield
            finally:
                closure_proxy = closure_proxy()
                if closure_proxy is not None:
                    await closure_proxy._async_disconnectSchemaUpdated()

        await instance._ss.enter_async_context(connectSchemaUpdated())
        if initialize:
            await proxy.initialize_proxy()
        await proxy.update_proxy()
        return proxy

    future = asyncio.ensure_future(create())
    futures[deviceId] = future
    return (await asyncio.shield(future))


def getDevice(deviceId, *, sync=None, initialize=True, timeout=5):
    """get a device proxy for the device deviceId

    Request a schema of a remote device and create a proxy object which
    acts as if one would act on the device itself.

    Note that the created device will not be connected to changes on the
    actual device. In order to achieve this, put the returned device in a
    with statement as such::

        with getDevice("someDevice") as device:
            # do something with device

    On the command line, you might prefer using :func:`connectDevice`.

    This is a synchronized coroutine.
    """
    if sync is None:
        sync = get_event_loop().sync_set

    ret = _getDevice(deviceId, sync=sync, initialize=initialize,
                     timeout=timeout)
    if asyncio.iscoroutine(ret):
        return DeviceFuture(ret)
    else:
        return ret


@synchronize
async def connectDevice(device, *, autodisconnect=None, timeout=5):
    """get and connect a device proxy for the device *deviceId*

    This connects a given device proxy to the real device such that the
    proxy's parameters are properly updated. If the *deviceId* is just
    a string, :func:`getDevice` is called on that name, as a shortcut
    especially on the command line.

    If the device is needed only within a specific block, it is nicer
    tu use instead a with statement as described in :func:`getDevice`."""
    if isinstance(device, ProxyBase):
        if autodisconnect is not None:
            raise RuntimeError(
                "autodisconnect can only be set at proxy creation time")
    else:
        if autodisconnect is None:
            factory = DeviceClientProxyFactory
        else:
            factory = AutoDisconnectProxyFactory
        device = await _getDevice(device, sync=get_event_loop().sync_set,
                                  initialize=True,
                                  timeout=timeout, factory=factory)
    if autodisconnect is None:
        ret = device.__enter__()
    else:
        device._interval = autodisconnect
        ret = device
    return ret


@synchronize
async def disconnectDevice(device):
    """disconnect the device proxy *device*

    Once you call this function, the device proxy's properties won't
    be updated anymore, reducing the network load generated by this device"""
    await device.__aexit__(None, None, None)


@synchronize
async def lock(proxy, wait_for_release=None):
    """return a context manager to lock a device

    This allows to lock another devices for exclusive use::

        with (await lock(proxy)):
            #do stuff

    In a synchronous context, this function waits at the end of the block
    until the lock is released, unless *wait_for_release* is *False*. In an
    asynchronous context, we cannot wait for release, so we don't.
    """

    myId = get_instance().deviceId
    if proxy._lock_count == 0:
        if proxy.lockedBy == myId:
            # we just unlocked the device but didn't get a response yet
            await proxy._update()
        while proxy.lockedBy != myId:
            if proxy.lockedBy == "":
                proxy.lockedBy = myId
            await waitUntilNew(proxy.lockedBy)

    @contextmanager
    def context():
        proxy._lock_count += 1
        try:
            yield
        finally:
            proxy._lock_count -= 1
            if proxy._lock_count == 0:
                if wait_for_release is False:
                    setNoWait(proxy, lockedBy="")
                else:
                    proxy.lockedBy = ""

    return context()


def findDevices(matchPattern):
    """Return a list of found deviceId's.

    This function is a shortcut to find deviceIds with `getDevices`

    :param matchPattern: String pattern, to find the deviceId's containing
                         the matchPattern.
    """
    return getDevices(matchPattern=matchPattern)


def getDevices(serverId=None, matchPattern=None):
    """Return a list of currently running devices

    :param serverId: Optional serverId, so that only devices are returned
                     running on device server.
    :param matchPattern: Optional string pattern, to find deviceId's containing
                         the matchPattern.
    """
    topology = getTopology()
    if serverId is None:
        ret = list(topology["device"])
    else:
        ret = [k for k, v, a in topology["device"].iterall()
               if a["serverId"] == serverId]
    if matchPattern is not None:
        ret = [dev for dev in ret if matchPattern.lower() in dev.lower()]

    return ret


def getClients():
    """Return a list of currently running external clients
    """
    instance = get_instance()
    return list(instance.systemTopology["client"])


def getTopology():
    """Return the full topology Hash of the DeviceClient

    NOTE: We provide a copy here as the topology might change while working
    with it.
    """
    return deepcopy(get_instance().systemTopology)


def findServers(matchPattern):
    """Return a list of found serverId's

    This function is a shortcut to find serverId's with `getServers`

    :param matchPattern: String pattern, to find the serverId's containing
                         the matchPattern.
    """
    return getServers(matchPattern=matchPattern)


def getServers(matchPattern=None):
    """Return a list of currently running servers

    :param matchPattern: Optional string pattern, to find serverId's containing
                         the matchPattern.
    """
    topology = getTopology()
    ret = list(topology["server"])
    if matchPattern is not None:
        ret = [serv for serv in ret if matchPattern.lower() in serv.lower()]

    return ret


def getClasses(serverId):
    """Return a list of device classes (plugins) available on a server"""
    topology = getTopology()
    servers = topology["server"]
    return servers.getAttributes(serverId)["deviceClasses"]


@synchronize
async def instantiate(serverId, classId, deviceId="", configuration=None,
                      **kwargs):
    """Instantiate and configure a device on a running server

    :param serverId: The serverId of the server on which the device should be
        started.
    :param classId: The classId of the device (the corresponding plugin must
        already be loaded on the server)
    :param deviceId: The future name of the device in the Karabo installation
        (will fail if not unique)
    :param configuration: the configuration of the device (optional)

    The keyword arguments are used to further configure the device. """
    if configuration is None:
        configuration = Hash()
    configuration.update(kwargs)
    h = Hash("classId", classId, "deviceId", deviceId,
             "configuration", configuration)
    ok, msg = await get_instance().call(serverId, "slotStartDevice", h)
    if not ok:
        raise KaraboError(msg)
    return msg


def instantiateNoWait(serverId, classId, deviceId="", configuration=None,
                      **kwargs):
    """Instantiate and configure a device on a running server

    Non-waiting version of :func:`instantiate`"""
    if configuration is None:
        configuration = Hash()
    configuration.update(kwargs)
    h = Hash("classId", classId, "deviceId", deviceId,
             "configuration", configuration)
    get_instance()._ss.emit("call", {serverId: ["slotStartDevice"]}, h)


@synchronize
async def shutdown(device):
    """shut down the given device

    :param deviceId: may be a device proxy, or just the id of a device"""
    if isinstance(device, ProxyBase):
        device = device._deviceId
    ok = await get_instance().call(device, "slotKillDevice")
    return ok


def shutdownNoWait(device):
    """shut down the given device

    not waiting version of :func:`shutdown`"""
    if isinstance(device, ProxyBase):
        device = device._deviceId
    get_instance()._ss.emit("call", {device: ["slotKillDevice"]})


@synchronize
async def setWait(device, *args, **kwargs):
    """Set properties of a device

    device may either be a device proxy as returned by :func:`getDevice`, or
    simply the deviceId as a string. Add the properties to be set as keyword
    arguments, as in::

        setWait("someDevice", speed=3, position=5)
        setWait("someDevice", "speed", 3, "position", 5)

    Note that for proxy devices this method is normally not necessary,
    you may just write::

        someDevice.speed = 3
        someDevice.position = 5

    the function waits until the device has acknowledged that the values
    have been set."""
    if len(args) % 2 > 0:
        raise RuntimeError("Arguments passed need to be pairs!")

    if isinstance(device, ProxyBase):
        await device._update()
        device = device._deviceId

    kwargs.update(zip(args[::2], args[1::2]))
    h = Hash()
    for k, v in kwargs.items():
        if isinstance(v, KaraboValue):
            h[k] = v.value
        else:
            h[k] = v

    await get_instance().call(device, "slotReconfigure", h)


def setNoWait(device, *args, **kwargs):
    """Same as :func:`setWait`, but don't wait for acknowledgement"""
    if len(args) % 2 > 0:
        raise RuntimeError("Arguments passed need to be pairs!")

    if isinstance(device, ProxyBase):
        device = device._deviceId

    kwargs.update(zip(args[::2], args[1::2]))
    h = Hash()
    for k, v in kwargs.items():
        if isinstance(v, KaraboValue):
            h[k] = v.value
        else:
            h[k] = v

    get_instance()._ss.emit("call", {device: ["slotReconfigure"]}, h)


def executeNoWait(device, slot):
    """execute a slot without waiting for it to be finished

    device may be a device proxy returned by getDevice, or simply a string
    with the device id, as in::

        executeNoWait("someDevice", "start")

    Normally you would call a slot simply using its proxy devive, as in::

        someDevice.start()

    but then we wait until the device has finished with the operation. If
    this is not desired, use executeNoWait."""
    if isinstance(device, ProxyBase):
        device = device._deviceId
    get_instance()._ss.emit("call", {device: [slot]})


@synchronize
async def execute(device, slot):
    """execute a slot and wait until it finishes"""
    if isinstance(device, ProxyBase):
        device = device._deviceId
    assert isinstance(slot, str)
    return (await get_instance().call(device, slot))


@synchronize
async def updateDevice(device):
    """wait for an update of a device

    request new properties and wait until they arrive. This is the only
    way to receive changes on a device while the device is not connected."""
    await device.update_proxy()
    return device


@synchronize
async def getTimeInfo(device):
    """Retrieve the time information of a device `device`

    :param device: deviceId or proxy
    :returns: Hash with timing information

    Note: This also adds the `latency` (response time in seconds) to the
          return Hash
    """
    if isinstance(device, ProxyBase):
        device = device._deviceId
    before = time()
    ret = await get_instance().call(device, "slotGetTime", Hash())
    after = time()
    latency = float(after - before)
    ret["latency"] = latency

    return ret


@synchronize
async def getSystemInfo(device):
    """Retrieve the system information of a device `device`

    :param device: deviceId or proxy
    :returns: Hash with system information
    """
    if isinstance(device, ProxyBase):
        device = device._deviceId
    ret = await get_instance().call(device, "slotGetSystemInfo", Hash())
    return ret


def isAlive(proxy):
    """Check whether a device represented by a proxy is still running"""
    assert isinstance(proxy, ProxyBase)
    return proxy._alive


def filterByTags(proxy, *tags):
    """Device proxy is filtered by provided tags

    This method returns a list of all descriptors which contain the tags::

        proxy = connectDevice("someDevice")

        for desc in filterByTags(proxy, "mytag"):
            ... do something
    """
    assert isinstance(proxy, ProxyBase)
    tags = frozenset(tags)
    filtered = [desc for desc in getDescriptors(proxy)
                if desc.tags is not None and
                not tags.isdisjoint(desc.tags)]

    return filtered


def getDescriptors(proxy):
    """Device proxy is searched for descriptors

    This generator returns all descriptors in the proxy::

        proxy = connectDevice("someDevice")

        for descr in getDescriptors(proxy):
            ... do something
    """
    assert isinstance(proxy, ProxyBase)

    def recurse(proxy):
        klass = proxy.__class__
        for key in proxy._allattrs:
            descr = getattr(klass, key, None)
            if descr is None:
                # NOTE: This protection is solely for the unsupported
                # NodeTypes, e.g. ListOfNodes
                continue
            if isinstance(descr, ProxyNodeBase):  # recurse Nodes
                yield from recurse(getattr(proxy, descr.key))
            else:
                yield descr

    yield from recurse(proxy)


def getAliasFromKey(proxy, key):
    """Retrieve the property alias for a given key from a proxy

    This method returns the related alias in the proxy::

        proxy = connectDevice("someDevice")

        alias = getAliasFromKey(proxy, "encoderPosition")

    """
    assert isinstance(proxy, ProxyBase)

    for desc in getDescriptors(proxy):
        if desc.longkey == key:
            return desc.alias
    else:
        raise KeyError("Key not found in device schema.")


def getKeyFromAlias(proxy, alias):
    """Retrieve the property key for a given alias from a proxy

    This method returns the related key in the proxy::

        proxy = connectDevice("someDevice")

        key = getKeyFromAlias(proxy, "AEncoderPosition")

    """
    assert isinstance(proxy, ProxyBase)

    for desc in getDescriptors(proxy):
        if desc.alias == alias:
            return desc.longkey
    else:
        raise KeyError("Alias not found in device schema.")
