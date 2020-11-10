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
from contextlib import contextmanager
from copy import copy
from decimal import Decimal
from weakref import ref

import dateutil.parser
import dateutil.tz

from karabo.common.services import KARABO_CONFIG_MANAGER
from karabo.native.data.basetypes import KaraboValue
from karabo.native.exceptions import KaraboError
from karabo.native.data.hash import Hash, Schema, Type
from karabo.native.timestamp import Timestamp

from .device import Device
from .configuration import config_changes, sanitize_init_configuration
from .eventloop import EventLoop, synchronize
from .proxy import (AutoDisconnectProxyFactory, DeviceClientProxyFactory,
                    ProxyBase, ProxyNodeBase)
from .signalslot import coslot, slot
from .synchronization import firstCompleted


class DeviceClientBase(Device):
    """Keep track of other devices

    A :class:`~karabo.middlelayer.Device` which also inherits from this class
    keeps track of all the other devices in this Karabo installation. Without
    inheriting from this class, listing other devices is impossible."""
    abstract = True

    def __init__(self, configuration):
        # "unknown" is default type for bare C++ SignalSlotable
        self.systemTopology = Hash("device", Hash(), "server", Hash(),
                                   "macro", Hash(), "client", Hash(),
                                   "unknown", Hash())
        self.loggerMap = Hash()
        super().__init__(configuration)

    @asyncio.coroutine
    def _run(self, **kwargs):
        yield from super()._run(**kwargs)
        self._ss.emit("call", {"*": ["slotPing"]}, self.deviceId, 0, False)

    @coslot
    def slotInstanceNew(self, instanceId, info):
        self.removeServerChildren(instanceId, info)
        self.updateSystemTopology(instanceId, info, "instanceNew")
        yield from super().slotInstanceNew(instanceId, info)

    @slot
    def slotInstanceUpdated(self, instanceId, info):
        self.updateSystemTopology(instanceId, info, "instanceUpdated")
        super().slotInstanceUpdated(instanceId, info)

    @slot
    def slotInstanceGone(self, instanceId, info):
        self.removeServerChildren(instanceId, info)
        self.systemTopology[info["type"]].pop(instanceId, None)
        return super().slotInstanceGone(instanceId, info)

    @slot
    def slotPingAnswer(self, deviceId, info):
        self.updateSystemTopology(deviceId, info, None)

    def removeServerChildren(self, instanceId, info):
        """Cleanup the device children from the server
        """
        if info["type"] == "server":
            devices = [k for k, v, a in self.systemTopology["device"].iterall()
                       if a["serverId"] == instanceId]
            for deviceId in devices:
                self.systemTopology["device"].pop(deviceId, None)

    def updateSystemTopology(self, instanceId, info, task):
        type = info["type"]
        ret = Hash(type, Hash())
        ret[type][instanceId] = Hash()
        ret[type][instanceId, ...] = dict(info.items())
        self.systemTopology.merge(ret)
        return ret


class OneShotQueue(asyncio.Future):
    """ This is a future that looks like a queue

    It may be registered in the list of queues for a property, and is removed
    from that property once fired """

    def put_nowait(self, value):
        if not self.done():
            self.set_result(value)


@synchronize
def waitUntilNew(*props):
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
    yield from firstCompleted(*futures)


def _parse_date(date):
    if isinstance(date, Timestamp):
        date = date.toLocal()
    d = dateutil.parser.parse(date)
    if d.tzinfo is None:
        d = d.replace(tzinfo=dateutil.tz.tzlocal())
    return d.astimezone(dateutil.tz.tzutc()).replace(tzinfo=None).isoformat()


@synchronize
def call(device, target_slot, *args):
    """Call a target slot from a device

    :param device: deviceId or proxy
    :param target_slot: slot to be called
    """
    if isinstance(device, ProxyBase):
        device = device._deviceId
    return (yield from get_instance().call(device, target_slot, *args))


def callNoWait(device, target_slot, *args):
    """Call a target slot from a device without waiting

    :param device: deviceId or proxy
    :param target_slot: slot to be called
    """
    if isinstance(device, ProxyBase):
        device = device._deviceId
    get_instance().callNoWait(device, target_slot, *args)


@synchronize
def getSchema(device):
    """Get a schema from a target device

    :param device: deviceId or proxy

    :returns: Full Schema object
    """
    if isinstance(device, ProxyBase):
        return Schema(name=device.classId, hash=device._schema_hash)

    schema, _ = yield from get_instance().call(device, "slotGetSchema",
                                               False)
    return schema


@synchronize
def getConfiguration(device):
    """Get a configuration from a target device

    :param device: deviceId or proxy

    NOTE: When passing a proxy, the proxy is updated and the configuration
    if extracted from the proxy. In this case, `ChoiceOfNodes` and
    `ListOfNodes` (mostly) properties will be omitted in the configuration.

    :returns: Configuration Hash
    """
    if isinstance(device, ProxyBase):
        yield from device._update()
        return device.configurationAsHash()

    hsh, _ = yield from get_instance().call(device, "slotGetConfiguration")
    return hsh


@synchronize
def compareDeviceConfiguration(device_a, device_b):
    """Compare device configurations (key, values) of two devices

    The changes are provided in a list for comparison::

    -> h = compareDeviceConfiguration(device_a, device_b)
    -> h
    -> <disableEpsilonFeedback{}: [True, False]>

    :returns: changes Hash
    """
    a_schema, b_schema = yield from gather(
        getSchema(device_a), getSchema(device_b))
    a_conf, b_conf = yield from gather(
        getConfiguration(device_a), getConfiguration(device_b))

    # Take into account only reconfigurable and init parameters!
    a_san = sanitize_init_configuration(a_schema, a_conf)
    b_san = sanitize_init_configuration(b_schema, b_conf)
    changes = config_changes(a_san, b_san)

    return changes


@synchronize
def compareDeviceWithPast(device, timepoint):
    """Compare device configuration (key, values) between present and past

    The changes are provided in a list for comparison::

    changes = [PRESENT | PAST]

    -> h = compareDeviceWithPast(device, minutesAgo(2))
    -> h
    -> <disableEpsilonFeedback{}: [True, False]>

    :param timepoint: The timepoint to compare

    :returns: changes Hash
    """
    # Get the config and schema first!
    a_conf, a_schema = yield from gather(
        getConfiguration(device), getSchema(device))
    b_conf, b_schema = yield from _get_configuration_from_past(
        device, timepoint)
    # Take into account only reconfigurable and init parameters!
    a_san = sanitize_init_configuration(a_schema, a_conf)
    b_san = sanitize_init_configuration(b_schema, b_conf)
    changes = config_changes(a_san, b_san)

    return changes


@synchronize
def _getLogReaderId(deviceId):
    instance = get_instance()
    did = "DataLogger-{}".format(deviceId)
    if did not in instance.loggerMap:
        instance.loggerMap = yield from instance.call(
            "Karabo_DataLoggerManager_0", "slotGetLoggerMap")
        if did not in instance.loggerMap:
            raise KaraboError('no logger for device "{}"'.
                              format(deviceId))
    return "DataLogReader0-{}".format(instance.loggerMap[did])


class _getHistory_old:
    def __init__(self, proxy, begin, end, maxNumData, timeout):
        self.proxy = proxy
        self.begin = _parse_date(begin)
        self.end = _parse_date(end)
        self.maxNumData = maxNumData
        self.timeout = timeout

    def __dir__(self):
        return dir(self.proxy)

    def __getattr__(self, attr):
        return self._synchronized_getattr(attr, timeout=self.timeout)

    @synchronize
    def _synchronized_getattr(self, attr):
        # this method contains a lot of hard-coded strings. It follows
        # GuiServerDevice::onGetPropertyHistory. One day we should
        # de-hard-code both.
        if isinstance(self.proxy, ProxyBase):
            # does the attribute actually exist?
            assert isinstance(getattr(type(self.proxy), attr), Type)
            deviceId = self.proxy._deviceId
        else:
            deviceId = str(self.proxy)

        reader = yield from _getLogReaderId(deviceId)
        r_deviceId, r_attr, data = yield from get_instance().call(
            reader, "slotGetPropertyHistory", deviceId, attr,
            Hash("from", self.begin, "to", self.end,
                 "maxNumData", self.maxNumData))
        assert r_deviceId == deviceId and r_attr == attr
        return [(Decimal(int(d["v", "frac"])) / 10 ** 18 +
                 Decimal(int(d["v", "sec"])), d["v", "tid"],
                 "isLast" in d["v", ...], d["v"]) for d in data]


@synchronize
def _getHistory_new(prop, begin, end, maxNumData):
    try:
        attr = prop.descriptor.longkey
        deviceId = prop._parent._deviceId
    except AttributeError:
        deviceId, attr = str(prop).split(".", 1)

    begin = _parse_date(begin)
    end = _parse_date(end)
    reader = yield from _getLogReaderId(deviceId)
    r_deviceId, r_attr, data = yield from get_instance().call(
        reader, "slotGetPropertyHistory", deviceId, attr,
        Hash("from", begin, "to", end, "maxNumData", maxNumData))
    assert r_deviceId == deviceId and r_attr == attr
    return [(float(d["v", "frac"]) / 10 ** 18 + float(d["v", "sec"]),
             d["v", "tid"], "isLast" in d["v", ...], d["v"]) for d in data]


def getHistory(prop, begin, end, *, maxNumData=10000, timeout=-1, wait=True):
    """get the history of device properties

    with this function one can get all values of a property in a given
    timespan::

        getHistory(device.someProperty, "2009-09-01", "2009-09-02")

    returns a list of tuples, which contain all changes of *someProperty*
    between the two given dates. The tuple contains four fields, the
    seconds since 1970-01-01 UTC, the train ID, a flag whether this is
    the last row in a set (typically, the device has been switched off
    afterwards), and the value of the property at that time.

    Given that a device may not be on line by the time one is trying to
    get the history, the property name may also be given as a string::

        getHistory("device.someProperty", "2009-09-01", "2009-09-02")

    The dates of the timespan are parsed using
    :func:`dateutil.parser.parse`, allowing many ways to write the date.
    The most precise way is to write "2009-09-01T15:32:12 UTC", but you may
    omit any part, like "10:32", only giving the time, where we assume
    the current day.  Unless specified otherwise, your local timezone is
    assumed.

    Another parameter, *maxNumData*, may be given, which gives the maximum
    number of data points to be returned. It defaults to 10000. The returned
    data will be reduced appropriately to still span the full timespan."""
    # this method contains a lot of hard-coded strings. It follows
    # GuiServerDevice::onGetPropertyHistory. One day we should
    # de-hard-code both.

    if (isinstance(prop, ProxyBase) or isinstance(prop, str) and
            "." not in prop):
        assert wait
        return _getHistory_old(prop, begin, end, maxNumData, timeout)
    else:
        return _getHistory_new(prop, begin, end, maxNumData,
                               timeout=timeout, wait=wait)


@synchronize
def _get_configuration_from_past(device, timepoint):
    """The configuration from past implementation to retrieve a configuration
    and schema from the archiving system

    :param device: deviceId or Karabo proxy
    :param timepoint: time point parsed using :func:`dateutil.parser.parse`,
    """
    if isinstance(device, ProxyBase):
        device = device._deviceId
    timepoint = _parse_date(timepoint)
    instance = get_instance()
    reader = yield from _getLogReaderId(device)
    slot = "slotGetConfigurationFromPast"
    conf, schema, *_ = yield from instance.call(reader, slot,
                                                device, timepoint)
    return conf, schema


@synchronize
def getConfigurationFromPast(device, timepoint):
    """Get the configuration of a deviceId or proxy at a given time::

        getConfigurationFromPast(device, "12:30")

    Returns a karabo configuration hash of the device at the specified time.

    The date of the time point is parsed using :func:`dateutil.parser.parse`,
    allowing many ways to write the date.
    """
    conf, _ = yield from _get_configuration_from_past(device, timepoint)
    return conf


@synchronize
def getSchemaFromPast(device, timepoint):
    """Get the schema of a deviceId or proxy at a given time::

        getSchemaFromPast(deviceId, "12:30")

    Returns a karabo schema object of the device at the specified time.

    The date of the time point is parsed using :func:`dateutil.parser.parse`,
    allowing many ways to write the date.
    """
    _, schema = yield from _get_configuration_from_past(device, timepoint)
    return schema


@synchronize
def getConfigurationFromName(device, name):
    """Get the configuration of a deviceId or proxy with a given `name`::

        getConfigurationFromName(device, "run2012")

    Returns a karabo configuration hash of the device saved under the `name`.
    """
    if isinstance(device, ProxyBase):
        device = device._deviceId
    instance = get_instance()
    slot = "slotGetConfigurationFromName"
    h = Hash("deviceId", device, "name", name)
    reply = yield from instance.call(KARABO_CONFIG_MANAGER, slot, h)
    config = reply["item.config"]

    return config


@synchronize
def getLastConfiguration(device, priority=3):
    """Get the last configuration of a deviceId with given `priority`::

        getLastConfiguration(device, 3)

    Returns a karabo configuration hash of the device.
    """
    if isinstance(device, ProxyBase):
        device = device._deviceId
    instance = get_instance()
    slot = "slotGetLastConfiguration"
    h = Hash("deviceId", device, "priority", priority)
    reply = yield from instance.call(KARABO_CONFIG_MANAGER, slot, h)
    item = reply["item"]

    return item


@synchronize
def listConfigurationFromName(device, name_part=''):
    """List the list of configurations of a deviceId with given `name_part`::

        listConfigurationFromName(device, '')

    Returns a list of configuration items of the device. Optionally, a `name
    part` can be provided to filter the configurations on manager side.

    Each configuration item is a Hash containing:

        - name: the configuration name
        - timepoint: the timepoint the device was taken
        - description: the description of the configuration
        - priority: the priority of the configuration
        - user: the user belonging to the configuration
    """
    if isinstance(device, ProxyBase):
        device = device._deviceId
    instance = get_instance()
    slot = "slotListConfigurationFromName"
    h = Hash("deviceId", device, "name", name_part)
    reply = yield from instance.call(KARABO_CONFIG_MANAGER, slot, h)
    configs = reply["items"]

    return configs


@synchronize
def listDevicesWithConfiguration(priority=3):
    """Return the list of devices which have a configuration of `priority`::

        listDevicesWithConfiguration(priority=3)

    Returns a list of deviceIds.
    """
    instance = get_instance()
    slot = "slotListDevices"
    h = Hash("priority", priority)
    reply = yield from instance.call(KARABO_CONFIG_MANAGER, slot, h)
    deviceIds = reply["item"]

    return deviceIds


@synchronize
def instantiateFromName(device, name=None, classId=None, serverId=None):
    """Instantiate a device from `name` via the ConfigurationManager::

        instantiateFromName(device, name='run2015')

    - device: The mandatory parameter, either deviceId or proxy
    - name: Optional parameter. If no `name` is provided, the latest
            configuration is retrieved with priority 3 (INIT).
    - classId: Optional parameter for validation of classId
    - serverId: Optional parameter
    """
    if isinstance(device, ProxyBase):
        device = device._deviceId
    h = Hash("deviceId", device)
    if name is not None:
        h["name"] = name
    if classId is not None:
        h["classId"] = classId
    if serverId is not None:
        h["serverId"] = serverId

    instance = get_instance()
    slot = "slotInstantiateDevice"
    reply = yield from instance.call(KARABO_CONFIG_MANAGER, slot, h)
    success = reply["success"]

    return success


@synchronize
def saveConfigurationFromName(devices, name, description='', priority=1):
    """Save configuration(s) in the KaraboConfigurationManager::

        - The parameter `devices` can be a Karabo `proxy`, a list of deviceIds,
          a list of proxies or a mixture of them. It can be as well a single
          deviceId string.

        - The description is by default empty.

        - The priority default is the lowest -> 1.

        saveConfigurationFromName(devices, name="proposal2020",
                                  description="working at end", priority=3)
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
    h = Hash("deviceIds", devices, "priority", priority,
             "name", name, "description", description)

    yield from instance.call(KARABO_CONFIG_MANAGER, slot, h)


class Queue(object):
    """A queue of property changes

    This allows you to track all the changes of a property on a remote device.
    An example of usage::

        q = Queue(motor.position)
        new_position = yield from q.get()
    """

    def __init__(self, prop):
        self.queue = asyncio.Queue(loop=EventLoop.global_loop)
        prop._parent._queues[prop.descriptor.longkey].add(self.queue)

    @synchronize
    def get(self):
        return (yield from self.queue.get())


def get_instance():
    return get_event_loop().instance()


@synchronize
def waitUntil(condition):
    """Wait until the condition is True

    The condition is typically a lambda function, as in::

        waitUntil(lambda: device.speed > 3)

    The condition will be evaluated each time something changes. Note
    that for this to work, it is necessary that all the devices used in the
    condition are connected while we are waiting (so typically they appear
    in a with statement)"""
    loop = get_event_loop()
    # suspend once to assure the event loop gets a chance to run
    yield from sleep(0)
    while not condition():
        yield from loop.waitForChanges()


@synchronize
def waitWhile(condition):
    """Wait while the condition is True

    The condition is typically a lambda function, as in::

        waitWhile(lambda: state == State.MOVING)

    The condition will be evaluated each time something changes. Note
    that for this to work, it is necessary that all the devices used in the
    condition are connected while we are waiting (so typically they appear
    in a with statement)"""
    loop = get_event_loop()
    # suspend once to assure the event loop gets a chance to run
    yield from sleep(0)
    while condition():
        yield from loop.waitForChanges()


@synchronize
def _getDevice(deviceId, sync, factory=DeviceClientProxyFactory):
    instance = get_instance()
    proxy = instance._proxies.get(deviceId)
    if proxy is not None:
        if not isinstance(proxy, factory.Proxy):
            raise KaraboError(
                "do not mix getDevice with connectDevice!\n"
                '(deleting the old proxy with "del proxy" may help)')
        yield from proxy
        return proxy

    futures = instance._proxy_futures
    future = futures.get(deviceId)
    if future is not None:
        return (yield from asyncio.shield(future))

    @asyncio.coroutine
    def create():
        try:
            schema, _ = yield from instance._call_once_alive(
                deviceId, "slotGetSchema", False)

            cls = factory.createProxy(schema)
            proxy = cls(instance, deviceId, sync)
            proxy._schema_hash = schema.hash
            instance._proxies[deviceId] = proxy
            weakproxy = ref(proxy)

            @contextmanager
            def killer():
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
                        proxy.__del__()

            instance._ss.enter_context(killer())
        finally:
            del futures[deviceId]

        # the following is pure black magic. Originally, (git blame should
        # show you where exactly) the following context manager was simply
        # a method of Proxy. Unfortunately, context managers (IMHO
        # illegally) hold a hidden reference to their call attributes,
        # including self, meaning that they cannot be collected. This
        # is why we sneak the proxy into method using a closure.
        closure_proxy = proxy

        @contextmanager
        def connectSchemaUpdated():
            nonlocal closure_proxy
            closure_proxy._device._ss.connect(
                closure_proxy._deviceId, "signalSchemaUpdated",
                closure_proxy._device.slotSchemaUpdated)
            closure_proxy._schemaUpdateConnected = True
            closure_proxy = ref(closure_proxy)
            try:
                yield
            finally:
                closure_proxy = closure_proxy()
                if closure_proxy is not None:
                    closure_proxy._disconnectSchemaUpdated()

        instance._ss.enter_context(connectSchemaUpdated())
        yield from proxy
        return proxy

    future = asyncio.ensure_future(create())
    futures[deviceId] = future
    return (yield from asyncio.shield(future))


def getDevice(deviceId, *, sync=None, timeout=5):
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
    return _getDevice(deviceId, sync=sync, timeout=timeout)


@synchronize
def connectDevice(device, *, autodisconnect=None, timeout=5):
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
        device = yield from _getDevice(device, sync=get_event_loop().sync_set,
                                       timeout=timeout, factory=factory)
    if autodisconnect is None:
        ret = device.__enter__()
    else:
        device._interval = autodisconnect
        ret = device
    yield from ret
    return ret


@synchronize
def disconnectDevice(device):
    """disconnect the device proxy *device*

    Once you call this function, the device proxy's properties won't
    be updated anymore, reducing the network load generated by this device"""
    device.__exit__(None, None, None)


@synchronize
def lock(proxy, wait_for_release=None):
    """return a context manager to lock a device

    This allows to lock another devices for exclusive use::

        with (yield from lock(proxy)):
            #do stuff

    In a synchronous context, this function waits at the end of the block
    until the lock is released, unless *wait_for_release* is *False*. In an
    asynchronous context, we cannot wait for release, so we don't.
    """

    myId = get_instance().deviceId
    if proxy._lock_count == 0:
        if proxy.lockedBy == myId:
            # we just unlocked the device but didn't get a response yet
            yield from proxy._update()
        while proxy.lockedBy != myId:
            if proxy.lockedBy == "":
                proxy.lockedBy = myId
            yield from waitUntilNew(proxy.lockedBy)

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


def findDevices(matchPattern, visibility=3):
    """Return a list of found deviceId's.

    This function is a shortcut to find deviceIds with `getDevices`

    :param matchPattern: String pattern, to find the deviceId's containing
                         the matchPattern.
    :param visibility: Integer specifying the visibility of desired devices.
                       Default visibility level is 3.
    """
    return getDevices(matchPattern=matchPattern, visibility=visibility)


def getDevices(serverId=None, visibility=3, matchPattern=None):
    """Return a list of currently running devices

    :param serverId: Optional serverId, so that only devices are returned
                     running on device server.
    :param visibility: Integer specifying the visibility of desired devices.
                       Default visibility level is 3.
    :param matchPattern: Optional string pattern, to find deviceId's containing
                         the matchPattern.
    """
    topology = getTopology()
    if serverId is None:
        ret = [k for k, v, a in topology["device"].iterall()
               if a["visibility"] <= visibility]
    else:
        ret = [k for k, v, a in topology["device"].iterall()
               if a["serverId"] == serverId and a["visibility"] <= visibility]
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
    return copy(get_instance().systemTopology)


def findServers(matchPattern, visibility=3):
    """Return a list of found serverId's

    This function is a shortcut to find serverId's with `getServers`

    :param matchPattern: String pattern, to find the serverId's containing
                         the matchPattern.
    :param visibility: Integer specifying the visibility of desired servers.
                       Default visibility level is 3.
    """
    ret = getServers(matchPattern=matchPattern, visibility=visibility)

    return ret


def getServers(visibility=3, matchPattern=None):
    """Return a list of currently running servers

    :param visibility: Integer specifying the visiblity of desired servers.
                       Default visibility level is 3.
    :param matchPattern: Optional string pattern, to find serverId's containing
                         the matchPattern.
    """
    topology = getTopology()
    ret = [k for k, v, a in topology["server"].iterall()
           if a["visibility"] <= visibility]
    if matchPattern is not None:
        ret = [serv for serv in ret if matchPattern.lower() in serv.lower()]

    return ret


def getClasses(serverId):
    """Return a list of device classes (plugins) available on a server"""
    servers = getTopology()["server"]
    return servers.getAttributes(serverId)["deviceClasses"]


@synchronize
def instantiate(serverId, classId, deviceId="", configuration=None, **kwargs):
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
    ok, msg = yield from get_instance().call(serverId, "slotStartDevice", h)
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
def shutdown(device):
    """shut down the given device

    :param deviceId: may be a device proxy, or just the id of a device"""
    if isinstance(device, ProxyBase):
        device = device._deviceId
    ok = yield from get_instance().call(device, "slotKillDevice")
    return ok


def shutdownNoWait(device):
    """shut down the given device

    not waiting version of :func:`shutdown`"""
    if isinstance(device, ProxyBase):
        device = device._deviceId
    get_instance()._ss.emit("call", {device: ["slotKillDevice"]})


@synchronize
def setWait(device, *args, **kwargs):
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
        yield from device._update()
        device = device._deviceId

    kwargs.update(zip(args[::2], args[1::2]))
    h = Hash()
    for k, v in kwargs.items():
        if isinstance(v, KaraboValue):
            h[k] = v.value
        else:
            h[k] = v

    yield from get_instance().call(device, "slotReconfigure", h)


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
def execute(device, slot):
    """execute a slot and wait until it finishes"""
    if isinstance(device, ProxyBase):
        device = device._deviceId
    assert isinstance(slot, str)
    return (yield from get_instance().call(device, slot))


@synchronize
def updateDevice(device):
    """wait for an update of a device

    request new properties and wait until they arrive. This is the only
    way to receive changes on a device while the device is not connected."""
    yield from device
    return device


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
