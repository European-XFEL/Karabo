"""This is the machinery to communicate with remote devices.

Usually, one communicated with a remote device by creating a device
proxy :class:`Proxy` using :func:`getDevice`. This is a convenient way to access
a remote device often.

A more rare usecase is that many remote devices should be accessed, but
only very simple. This can be achieved by functions which operate directly
on the deviceId, without going through the hazzle of creating a device
proxy. """
import asyncio
from asyncio import get_event_loop
from collections import defaultdict
from decimal import Decimal
from functools import wraps
from weakref import WeakSet

from karabo import KaraboError
from karabo.hash import Hash
from karabo.hashtypes import Slot, Type, Descriptor
from karabo.signalslot import slot
from karabo.python_device import Device

class DeviceClientBase(Device):
    def __init__(self, configuration):
        self.systemTopology = Hash("device", Hash(), "server", Hash(),
                                   "macro", Hash())
        self.loggerMap = Hash()
        super().__init__(configuration)

    def run(self):
        super().run()
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


def synchronize(func):
    coro = asyncio.coroutine(func)
    @wraps(coro)
    def wrapper(*args, timeout=-1, **kwargs):
        return get_event_loop().sync(coro(*args, **kwargs), timeout)
    return wrapper


class ProxySlot(Slot):
    def __get__(self, instance, owner):
        if instance is None:
            return self
        key = self.longkey

        @synchronize
        def method(self):
            self._update()
            return (yield from self._device.call(self._deviceId, key))
        method.__doc__ = self.description
        return method.__get__(instance, owner)


class Proxy(object):
    """A proxy for a remote device

    This proxy represents a real device somewhere in the karabo system.
    It is typically created by :func:`getDevice`, do not create it yourself.

    properties and slots may be accessed as if one would access the actual
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

    def _onChanged(self, hash):
        for k, v, a in hash.iterall():
            d = getattr(type(self), k, None)
            if d is not None:
                if isinstance(d, ProxyNode):
                    d.setValue(getattr(self, d.key), v, self)
                else:
                    self.__dict__[d.key] = v
                    for q in self._queues[k]:
                        q.put_nowait(v)
        for q in self._queues[None]:
            q.put_nowait(hash)

    def setValue(self, attr, value):
        loop = get_event_loop()
        if loop.sync_set:
            ok, msg = loop.sync(self._device.call(
                self.deviceId, "slotReconfigure", Hash(attr.longkey, value)),
                -1)
            if not ok:
                raise KaraboError(msg)
        else:
            update = not self._sethash
            self._sethash[attr.longkey] = value
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
        return self

    def __exit__(self, a, b, c):
        self._used -= 1
        if self._used == 0:
            self._device._ss.disconnect(self._deviceId, "signalChanged",
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


class ProxyNode(Descriptor):
    def __init__(self, cls):
        self.cls = cls

    def setValue(self, instance, hash, parent):
        for k, v, a in hash.iterall():
            d = getattr(self.cls, k, None)
            if d is not None:
                if isinstance(d, ProxyNode):
                    d.setValue(getattr(instance, d.key), v, parent)
                else:
                    instance.__dict__[d.key] = v
                    for q in parent._queues[self.longkey]:
                        q.put_nowait(v)

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
    def setValue(self, desc, value):
        self.__dict__[desc.key] = value
        self._parent.setValue(desc, value)


class OneShotQueue(asyncio.Future):
    """ This is a future that looks like a queue

    It may be registered in the list of queues for a property, and is removed
    from that property once fired """
    def put_nowait(self, value):
        if not self.done():
            self.set_result(value)


class waitUntilNew:
    # this looks like a function to the user, although it is a class
    """wait until a new value for a property is available

    this function waits until a specific property of a device changes.
    It has an unusual syntax::

        waitUntilNew(someDevice).someProperty

    this function is used rather rarely. If you want to wait for something
    to reach a certain value, use :func:`waitUntil`. If you want to get all
    updates of a property, use :func:`iterate`."""
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

    Another parameter, *maxNumData*, may be given, which gives the maximum
    number of data points to be returned. The returned data will be
    reduced appropriately to still span the full timespan."""
    def __init__(self, proxy, begin, end, maxNumData=0):
        self.proxy = proxy
        self.begin = begin
        self.end = end
        self.maxNumData = maxNumData

    def __dir__(self):
        return dir(self.proxy)

    @synchronize
    def __getattr__(self, attr):
        assert isinstance(getattr(type(self.proxy), attr), Type)
        instance = get_instance()
        id = "DataLogger-{}".format(self.proxy._deviceId)
        if id not in instance.loggerMap:
            instance.loggerMap = yield from instance.call(
                "Karabo_DataLoggerManager_0", "slotGetLoggerMap")
            if id not in instance.loggerMap:
                raise KaraboError('no logger for device "{}"'.
                                  format(self.proxy._deviceId))
        reader = "DataLogReader-{}".format(instance.loggerMap[id])
        deviceId, property, data = yield from get_instance().call(
            reader, "slotGetPropertyHistory", self.proxy._deviceId, attr,
            Hash("from", self.begin, "to", self.end,
                 "maxNumData", self.maxNumData))
        assert deviceId == self.proxy._deviceId
        assert property == attr
        return [(Decimal(int(d["v", "frac"])) / 10 ** 18 +
                 Decimal(int(d["v", "sec"])), d["v", "tid"],
                 "isLast" in d["v", ...], d["v"]) for d in data]


class Queue:
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

    the condition is typically a lambda function, as in::

        waitUntil(lambda: device.speed > 3)

    The condition will be evaluated each time something changes. Note
    that for this to work, it is necessary that all the devices used in the
    condition are connected while we are waiting (so typically they appear
    in a with statement)"""
    loop = get_event_loop()
    while not condition():
        yield from loop.changedFuture


def _createProxyDict(hash, prefix):
    dict = {}
    for k, v, a in hash.iterall():
        if a["nodeType"] == 0:
            d = Type.fromname[a["valueType"]]()
            d.key = k
            d.longkey = prefix + k
            dict[k] = d
        elif a["nodeType"] == 1:
            if a.get("displayType") == "Slot":
                del a["nodeType"]
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
def _getDevice(deviceId, sync, timeout=None):
    instance = get_instance()
    ret = instance._devices.get(deviceId)
    if ret is not None:
        yield from ret
        return ret

    schema, _ = yield from instance.call(deviceId, "slotGetSchema", False)

    dict = _createProxyDict(schema.hash, "")
    Cls = type(schema.name, (Proxy,), dict)

    ret = Cls(instance, deviceId, sync)
    instance._devices[deviceId] = ret
    yield from ret
    return ret


def getDevice(deviceId, *, sync=None, timeout=-1):
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


def connectDevice(deviceId):
    """get and connect a device proxy for the device *deviceId*

    This is a shortcut of :func:`getDevice` for the command line, which
    also connects the proxy for updates of its parameters.

    As with statements on the command line are cumbersome, this function
    gets a device an connects it, until it is not used anymore."""
    return getDevice(deviceId).__enter__()


def getDevices(serverId=None):
    """Return a list of currently running devices

    Optionally, it may only return the devices running on device server
    `serverId`."""
    instance = get_instance()
    if serverId is None:
        return list(instance.systemTopology["device"])
    else:
        return [k for k, v in instance.systemTopology["device"]
                if v["serverId"] == serverId]

def getServers():
    """Return a list of currently running servers"""
    return list(get_instance().systemTopology["server"])


def getClasses(serverId):
    """Return a list of device classes (plugins) available on a server"""
    instance = get_instance()
    return instance.systemTopology["server"]["serverId"]["deviceClasses"]


@synchronize
def instantiate(serverId, classId, deviceId="", configuration=Hash(),
                **kwargs):
    """Instantiate and configure a device on a running server

    Arguments:
      * *serverId*: The serverId of the server on which the device should be started.
      * *classId*: The classId of the device (corresponding plugin must already be loaded on the server)
      * *deviceId*: The future name of the device in the Karabo installation (will fail if not unique)
      * *configuration*: the configuration of the device (optional)

    The keyword arguments are used to further configure the device. """
    configuration.update(kwargs)
    h = Hash("classId", classId, "deviceId", deviceId,
             "configuration", configuration)
    ok, msg = yield from get_instance().call(serverId, "slotStartDevice", h)
    if not ok:
        raise KaraboError(msg)
    return msg


@synchronize
def instantiateNoWait(serverId, classId, deviceId="", configuration=Hash(),
                **kwargs):
    """Instantiate and configure a device on a running server

    Non-waiting version of :func:`instantiate`"""
    configuration.update(kwargs)
    h = Hash("classId", classId, "deviceId", deviceId,
             "configuration", configuration)
    get_instance()._ss.emit("call", {serverId: ["slotStartDevice"]}, h)


@synchronize
def shutdown(device):
    """shut down the given device

    *deviceId* may be a device proxy, or just the id of a device"""
    if isinstance(device, Proxy):
        device = device._deviceId
    ok = yield from get_instance().call(device, "slotKillDevice")
    return ok


@synchronize
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
    """same as :func:`set`, but don't wait for acknowledgement"""
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
    return (yield from asyncio.sleep(delay, result))
