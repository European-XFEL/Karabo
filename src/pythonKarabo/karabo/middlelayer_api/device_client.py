"""This is the machinery to communicate with remote devices.

Usually, one communicated with a remote device by creating a device
proxy :class:`Proxy` using :func:`getDevice`. This is a convenient way to
access a remote device often.

A more rare usecase is that many remote devices should be accessed, but
only very simple. This can be achieved by functions which operate directly
on the deviceId, without going through the hazzle of creating a device
proxy. """
import asyncio
from asyncio import get_event_loop
from collections import defaultdict
from decimal import Decimal
import time
from weakref import WeakSet

import dateutil.parser
import dateutil.tz

from .basetypes import KaraboValue
from .device import Device
from .enums import NodeType
from .eventloop import synchronize
from .exceptions import KaraboError
from .hash import Hash, Slot, Type, Descriptor
from .signalslot import slot
from .timestamp import Timestamp
from .weak import Weak


class DeviceClientBase(Device):
    abstract = True

    def __init__(self, configuration):
        self.systemTopology = Hash("device", Hash(), "server", Hash(),
                                   "macro", Hash())
        self.loggerMap = Hash()
        super().__init__(configuration)

    @asyncio.coroutine
    def _run(self):
        yield from super()._run()
        self._ss.emit("call", {"*": ["slotPing"]}, self.deviceId, 0, False)

    @slot
    def slotInstanceNew(self, instanceId, info):
        self.updateSystemTopology(instanceId, info, "instanceNew")
        super().slotInstanceNew(instanceId, info)

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


class ProxySlot(Slot):
    def __get__(self, instance, owner):
        if instance is None:
            return self
        key = self.longkey

        @synchronize
        def method(self):
            self._update()
            self._device._use()
            return (yield from self._device.call(self._deviceId, key))
        method.__doc__ = self.description
        return method.__get__(instance, owner)


class Proxy(object):
    """A proxy for a remote device

    This proxy represents a real device somewhere in the karabo system.
    It is typically created by :func:`getDevice`, do not create it yourself.

    Properties and slots may be accessed as if one would access the actual
    device object, as in::

        device = getDevice("someDevice")
        device.speed = 7
        device.start()

    Note that in order to reduce network traffic, the proxy device is initially
    not *connected*, meaning that it does not receive updates of properties
    from the device. There are two ways to change that: you can call
    :func:`updateDevice` to get the current configuration once, or you may use
    the proxy in a with statment, which will connect the device during the
    execution of that statement, as in::

        with getDevice("someDevice") as device:
            print(device.speed)"""
    def __init__(self, device, deviceId, sync):
        self._device = device
        self._queues = defaultdict(WeakSet)
        self._deviceId = deviceId
        self._used = 0
        self._sethash = {}
        self._sync = sync

    @classmethod
    def __dir__(cls):
        return dir(cls)

    def _use(self):
        pass

    def _onChanged_r(self, hash, instance, parent):
        """the recursive part of _onChanged"""
        for k, v, a in hash.iterall():
            d = getattr(type(instance), k, None)
            if d is not None:
                if isinstance(d, ProxyNode):
                    self._onChanged_r(v, getattr(instance, d.key), parent)
                elif not isinstance(d, ProxySlot):
                    converted = d.toKaraboValue(v, strict=False)
                    converted.timestamp = Timestamp.fromHashAttributes(a)
                    converted._parent = self
                    instance.__dict__[d.key] = converted
                    for q in parent._queues[d.longkey]:
                        q.put_nowait(converted)

    def _onChanged(self, hash):
        self._onChanged_r(hash, self, self)
        for q in self._queues[None]:
            q.put_nowait(hash)

    def setValue(self, desc, value):
        self._use()
        loop = get_event_loop()
        assert isinstance(value, KaraboValue)
        if loop.sync_set:
            h = Hash()
            h[desc.longkey], _ = desc.toDataAndAttrs(value)
            ok, msg = loop.sync(self._device.call(
                self.deviceId, "slotReconfigure", h), timeout=-1, wait=True)
            if not ok:
                raise KaraboError(msg)
        else:
            update = not self._sethash
            self._sethash[desc.longkey], _ = desc.toDataAndAttrs(value)
            if update:
                self._device._ss.loop.call_soon_threadsafe(self._update)

    def _update(self):
        hash = Hash()
        while self._sethash:
            k, v = self._sethash.popitem()
            hash[k] = v
        if hash:
            self._device._ss.emit("call",
                                  {self._deviceId: ["slotReconfigure"]}, hash)

    def __enter__(self):
        self._used += 1
        if self._used == 1:
            self._device._ss.connect(self._deviceId, "signalChanged",
                                     self._device.slotChanged)
            self._device._ss.connect(self._deviceId, "signalStateChanged",
                                     self._device.slotChanged)
        return self

    def __exit__(self, a, b, c):
        self._used -= 1
        if self._used == 0:
            self._device._ss.disconnect(self._deviceId, "signalChanged",
                                        self._device.slotChanged)
            self._device._ss.disconnect(self._deviceId, "signalStateChanged",
                                        self._device.slotChanged)

    def __del__(self):
        if self._used > 0:
            self._used = 1
            self.__exit__(None, None, None)

    def __iter__(self):
        self._update()
        conf, _ = yield from self._device.call(self._deviceId,
                                               "slotGetConfiguration")
        self._onChanged(conf)
        return self


class AutoDisconnectProxy(Proxy):
    def __init__(self, device, deviceId, sync):
        super().__init__(device, deviceId, sync)
        self._interval = 1
        self._lastused = time.time()
        self._task = asyncio.async(self._connector())

    def _use(self):
        self._lastused = time.time()
        if self._task.done():
            self._connect()

    @synchronize
    def _connect(self):
        if self._task.done():
            yield from self
            self._task = asyncio.async(self._connector())

    @asyncio.coroutine
    def _connector(self):
        with self:
            while True:
                delta = self._interval - time.time() + self._lastused
                if delta < 0:
                    return
                yield from asyncio.sleep(delta)


class ProxyNode(Descriptor):
    def __init__(self, cls):
        self.cls = cls

    def __get__(self, instance, owner):
        if instance is None:
            return self
        ret = instance.__dict__.get(self.key, None)
        if ret is not None:
            return ret
        sub = self.cls()
        if isinstance(instance, SubProxy):
            sub._parent = instance._parent
        else:
            sub._parent = instance
        instance.__dict__[self.key] = sub
        return sub


class SubProxy(object):
    _parent = Weak()

    def setValue(self, desc, value):
        self.__dict__[desc.key] = value
        self._parent.setValue(desc, value)

    def _use(self):
        self._parent._use()

    def _update(self):
        self._parent._update()

    @property
    def _deviceId(self):
        return self._parent._deviceId

    @property
    def _device(self):
        return self._parent._device


class OneShotQueue(asyncio.Future):
    """ This is a future that looks like a queue

    It may be registered in the list of queues for a property, and is removed
    from that property once fired """
    def put_nowait(self, value):
        if not self.done():
            self.set_result(value)


class _WaitUntilNew_old:
    # this was current before 1.5, when the syntax was
    # waitUntilNew(someDevice).someProperty.
    # In some future this should go
    def __init__(self, proxy):
        self.proxy = proxy

    @synchronize
    def __getattr__(self, attr):
        assert isinstance(getattr(type(self.proxy), attr), Type)
        future = OneShotQueue(loop=self.proxy._device._ss.loop)
        self.proxy._queues[attr].add(future)
        return (yield from future)

    def __iter__(self):
        future = OneShotQueue(loop=self.proxy._device._ss.loop)
        self.proxy._queues[None].add(future)
        return (yield from future)


@synchronize
def _waitUntilNew_new(prop):
    # _new means since 1.5
    proxy = prop._parent
    future = OneShotQueue(loop=proxy._device._ss.loop)
    proxy._queues[prop.descriptor.longkey].add(future)
    return (yield from future)


def waitUntilNew(prop):
    """wait until a new value for a property is available

    this function waits until a specific property of a device changes::

        waitUntilNew(someDevice.someProperty)

    If you want to wait for something to reach a certain value, use
    :func:`waitUntil`. If you want to get all updates of a property, use
    :class:`Queue`."""
    if isinstance(prop, Proxy):
        return _WaitUntilNew_old(prop)
    else:
        return _waitUntilNew_new(prop)


class getHistory:
    """get the history of device properties

    with this function one can get all values of a property in a given
    timespan::

        getHistory(device, "2009-09-01", "2009-09-02").someProperty

    returns a list of tuples, which contain all changes of *someProperty*
    between the two given dates. The tuple contains four fields, the
    seconds since 1970-01-01 UTC, the train ID, a flag whether this is
    the last row in a set (typically, the device has been switched off
    afterwards), and the value of the property at that time.

    The dates of the timespan are parsed using
    :func:`dateutil.parser.parse`, allowing many ways to write the date.
    The most precise way is to write "2009-09-01T15:32:12 UTC", but you may
    omit any part, like "10:32", only giving the time, where we assume
    the current day.  Unless specified otherwise, your local timezone is
    assumed.

    Another parameter, *maxNumData*, may be given, which gives the maximum
    number of data points to be returned. It defaults to 10000. The returned
    data will be reduced appropriately to still span the full timespan."""
    def __init__(self, proxy, begin, end, maxNumData=10000, *, timeout=5):
        self.proxy = proxy
        self.begin = self._parse(begin)
        self.end = self._parse(end)
        self.maxNumData = maxNumData
        self.timeout = timeout

    def _parse(self, date):
        d = dateutil.parser.parse(date)
        if d.tzinfo is None:
            d = d.replace(tzinfo=dateutil.tz.tzlocal())
        return d.astimezone(dateutil.tz.tzutc()).replace(
            tzinfo=None).isoformat()

    def __dir__(self):
        return dir(self.proxy)

    def __getattr__(self, attr):
        return self._synchronized_getattr(attr, timeout=self.timeout)

    @synchronize
    def _synchronized_getattr(self, attr):
        # this method contains a lot of hard-coded strings. It follows
        # GuiServerDevice::onGetPropertyHistory. One day we should
        # de-hard-code both.
        if isinstance(self.proxy, Proxy):
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


class Queue:
    """A queue of property changes

    This allows you to track all the changes of a property on a remote device.
    An example of usage::

        q = Queue(motor.position)
        new_position = q.get()
    """

    def __init__(self, proxy):
        self.proxy = proxy

    def __getattr__(self, attr):
        assert isinstance(getattr(type(self.proxy), attr), Type)
        queue = get_event_loop().Queue()
        self.proxy._queues[attr].add(queue)
        return queue


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
    while not condition():
        yield from loop.waitForChanges()


def _createProxyDict(hash, prefix):
    dict = {}
    for k, v, a in hash.iterall():
        nodeType = NodeType(a["nodeType"])
        if nodeType is NodeType.Leaf:
            d = Type.fromname[a["valueType"]](strict=False, key=k, **a)
            d.longkey = prefix + k
            dict[k] = d
        elif nodeType is NodeType.Node:
            if a.get("displayType") == "Slot":
                dict[k] = ProxySlot()
            else:
                sub = _createProxyDict(v, prefix + k + ".")
                Cls = type(k, (SubProxy,), sub)
                dict[k] = ProxyNode(Cls)
            dict[k].key = k
            dict[k].longkey = prefix + k
            dict[k].description = a.get("description")
    return dict


@synchronize
def _getDevice(deviceId, sync, Proxy=Proxy):
    instance = get_instance()
    ret = instance._devices.get(deviceId)
    if ret is not None:
        yield from ret
        return ret

    schema, _ = yield from instance.call(deviceId, "slotGetSchema", False)

    dict = _createProxyDict(schema.hash, "")
    Cls = type(schema.name, (Proxy,), dict)

    ret = Cls(instance, deviceId, sync)
    ret._schema_hash = schema.hash
    instance._devices[deviceId] = ret
    yield from ret
    return ret


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
    if isinstance(device, Proxy):
        if autodisconnect is not None:
            raise RuntimeError(
                "autodisconnect can only be set at proxy creation time")
    else:
        if autodisconnect is None:
            P = Proxy
        else:
            P = AutoDisconnectProxy
        device = yield from _getDevice(device, sync=get_event_loop().sync_set,
                                       timeout=timeout, Proxy=P)
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
    if isinstance(device, Proxy):
        device = device._deviceId
    ok = yield from get_instance().call(device, "slotKillDevice")
    return ok


def shutdownNoWait(device):
    """shut down the given device

    not waiting version of :func:`shutdown`"""
    if isinstance(device, Proxy):
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
    if isinstance(device, Proxy):
        device._update()
        device = device._deviceId
    h = Hash()
    for k, v in kwargs.items():
        h[k] = v
    yield from get_instance().call(device, "slotReconfigure", h)


def setNoWait(device, **kwargs):
    """Same as :func:`setWait`, but don't wait for acknowledgement"""
    if isinstance(device, Proxy):
        device = device._deviceId
    h = Hash()
    for k, v in kwargs.items():
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
    if isinstance(device, Proxy):
        device = device._deviceId
    get_instance()._ss.emit("call", {device: [slot]})


@synchronize
def execute(device, slot):
    """execute a slot and wait until it finishes"""
    if isinstance(device, Proxy):
        device = device._deviceId
    assert isinstance(slot, str)
    yield from get_instance().call("device", slot)


@synchronize
def updateDevice(device):
    """wait for an update of a device

    request new properties and wait until they arrive. This is the only
    way to receive changes on a device while the device is not connected."""
    yield from device
    return device


@synchronize
def sleep(delay, result=None):
    """do nothing for *delay* seconds

    This method should be preferred over :func:`time.sleep`, as it is
    interruptable."""
    return asyncio.sleep(delay, result)
