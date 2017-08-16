"""This is the machinery to communicate with remote devices.

Usually, one communicated with a remote device by creating a device
proxy :class:`Proxy` using :func:`getDevice`. This is a convenient way to
access a remote device often.

A more rare usecase is that many remote devices should be accessed, but
only very simple. This can be achieved by functions which operate directly
on the deviceId, without going through the hazzle of creating a device
proxy. """
import asyncio
from asyncio import get_event_loop, sleep
from contextlib import contextmanager
from decimal import Decimal
from functools import partial
from weakref import ref

import dateutil.parser
import dateutil.tz

from .basetypes import KaraboValue
from .device import Device
from .eventloop import EventLoop, synchronize
from .exceptions import KaraboError
from .hash import Hash, Type
from .proxy import (ProxyBase, AutoDisconnectProxyFactory,
                    DeviceClientProxyFactory)
from .signalslot import coslot, slot
from .synchronization import firstCompleted


class DeviceClientBase(Device):
    """Keep track of other devices

    A :class:`~karabo.middlelayer.Device` which also inherits from this class
    keeps track of all the other devices in this Karabo installation. Without
    inheriting from this class, listing other devices is impossible."""
    abstract = True

    def __init__(self, configuration):
        self.systemTopology = Hash("device", Hash(), "server", Hash(),
                                   "macro", Hash())
        self.loggerMap = Hash()
        super().__init__(configuration)

    @asyncio.coroutine
    def _run(self, **kwargs):
        yield from super()._run(**kwargs)
        self._ss.emit("call", {"*": ["slotPing"]}, self.deviceId, 0, False)

    @coslot
    def slotInstanceNew(self, instanceId, info):
        self.updateSystemTopology(instanceId, info, "instanceNew")
        yield from super().slotInstanceNew(instanceId, info)

    @slot
    def slotInstanceUpdated(self, instanceId, info):
        self.updateSystemTopology(instanceId, info, "instanceUpdated")
        super().slotInstanceUpdated(instanceId, info)

    @slot
    def slotInstanceGone(self, instanceId, info):
        self.systemTopology[info["type"]].pop(instanceId, None)
        return super().slotInstanceGone(instanceId, info)

    @slot
    def slotPingAnswer(self, deviceId, info):
        self.updateSystemTopology(deviceId, info, None)

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
    d = dateutil.parser.parse(date)
    if d.tzinfo is None:
        d = d.replace(tzinfo=dateutil.tz.tzlocal())
    return d.astimezone(dateutil.tz.tzutc()).replace(tzinfo=None).isoformat()


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
        instance = get_instance()
        did = "DataLogger-{}".format(deviceId)
        if did not in instance.loggerMap:
            instance.loggerMap = yield from instance.call(
                "Karabo_DataLoggerManager_0", "slotGetLoggerMap")
            if did not in instance.loggerMap:
                raise KaraboError('no logger for device "{}"'.
                                  format(deviceId))
        reader = "DataLogReader0-{}".format(instance.loggerMap[did])
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

    instance = get_instance()
    did = "DataLogger-{}".format(deviceId)
    if did not in instance.loggerMap:
        instance.loggerMap = yield from instance.call(
            "Karabo_DataLoggerManager_0", "slotGetLoggerMap")
        if did not in instance.loggerMap:
            raise KaraboError('no logger for device "{}"'.
                              format(deviceId))
    reader = "DataLogReader0-{}".format(instance.loggerMap[did])
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

    if (isinstance(prop, ProxyBase) or isinstance(prop, str)
            and "." not in prop):
        assert wait
        return _getHistory_old(prop, begin, end, maxNumData, timeout)
    else:
        return _getHistory_new(prop, begin, end, maxNumData,
                               timeout=timeout, wait=wait)


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
    yield from waitUntil(partial(lambda f: not f(), condition))


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
    future = asyncio.async(create())
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


def getDevices(serverId=None):
    """Return a list of currently running devices

    Optionally, it may only return the devices running on device server
    `serverId`."""
    instance = get_instance()
    if serverId is None:
        return list(instance.systemTopology["device"])
    else:
        return [k for k, v, a in instance.systemTopology["device"].iterall()
                if a["serverId"] == serverId]


def getServers():
    """Return a list of currently running servers"""
    return list(get_instance().systemTopology["server"])


def getClasses(serverId):
    """Return a list of device classes (plugins) available on a server"""
    servers = get_instance().systemTopology["server"]
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
def setWait(device, **kwargs):
    """Set properties of a device

    device may either be a device proxy as returned by :func:`getDevice`, or
    simply the deviceId as a string. Add the properties to be set as keyword
    arguments, as in::

        setWait("someDevice", speed=3, position=5)

    Note that for proxy devices this method is normally not necessary,
    you may just write::

        someDevice.speed = 3
        someDevice.position = 5

    the function waits until the device has acknowledged that the values
    have been set."""
    if isinstance(device, ProxyBase):
        yield from device._update()
        device = device._deviceId
    h = Hash()
    for k, v in kwargs.items():
        if isinstance(v, KaraboValue):
            h[k] = v.value
        else:
            h[k] = v
    yield from get_instance().call(device, "slotReconfigure", h)


def setNoWait(device, **kwargs):
    """Same as :func:`setWait`, but don't wait for acknowledgement"""
    if isinstance(device, ProxyBase):
        device = device._deviceId
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
