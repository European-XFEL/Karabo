from asyncio import coroutine, Future, get_event_loop
from functools import wraps

from karabo.hash import Hash
from karabo.hashtypes import Slot, Type
from karabo.signalslot import slot
from karabo.python_device import Device

class DeviceClientBase(Device):
    def __init__(self, configuration):
        self.systemTopology = Hash("device", Hash(), "server", Hash(),
                                   "macro", Hash())
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
        self.systemTopology[info[type]].pop(instanceId, None)
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
    coro = coroutine(func)
    @wraps(coro)
    def wrapper(*args, timeout=-1, **kwargs):
        return get_event_loop().sync(coro(*args, **kwargs), timeout)
    return wrapper


class ProxySlot(Slot):
    def __get__(self, instance, owner):
        if instance is None:
            return self
        key = self.key

        @synchronize
        def method(self):
            self._update()
            return (yield from self._device.call(self._deviceId, key))
        return method.__get__(instance, owner)


class Proxy(object):
    def __init__(self, device, deviceId, sync):
        self._device = device
        self._futures = {}
        self._deviceId = deviceId
        self._used = 0
        self._sethash = None
        self._sync = sync

    @classmethod
    def __dir__(cls):
        return dir(cls)

    def _onChanged(self, hash):
        for k, v, a in hash.iterall():
            d = getattr(type(self), k, None)
            if d is not None:
                self.__dict__[d] = v
                f = self._futures.pop(k, None)
                if f is not None:
                    f.set_result(v)
        f = self._futures.pop(None, None)
        if f is not None:
            f.set_result(hash)

    def setValue(self, attr, value):
        loop = get_event_loop()
        if loop.sync_set:
            ok, msg = loop.sync(self._device.call(
                self.deviceId, "slotReconfigure", Hash(attr.key, value)), -1)
            if not ok:
                raise KaraboError(msg)
        else:
            if self._sethash is None:
                self._device._ss.loop.call_soon_threadsafe(self._update)
                self._sethash = Hash()
            self._sethash[attr.key] = value

    def _update(self):
        if self._sethash is None:
            return
        self._device._ss.emit("call", {self._deviceId: ["slotReconfigure"]},
                              self._sethash)
        self._sethash = None

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


class waitUntilNew:
    # this looks like a function to the user, although it is a class
    def __init__(self, proxy):
        self.proxy = proxy

    @synchronize
    def __getattr__(self, attr):
        assert isinstance(getattr(type(self.proxy), attr), Type)
        return self.proxy._futures.setdefault(
            attr, Future(loop=self.proxy._device._ss.loop))

    def __iter__(self):
        return (yield from self.proxy._futures.setdefault(
            None, Future(loop=self.proxy._device._ss.loop)))


def get_instance():
    return get_event_loop().instance()


@synchronize
def waitUntil(condition):
    loop = get_event_loop()
    while not condition():
        yield from loop.changedFuture


@synchronize
def _getDevice(deviceId, sync, timeout=None):
    instance = get_instance()
    ret = instance._devices.get(deviceId)
    if ret is not None:
        yield from ret
        return ret

    schema, _ = yield from instance.call(deviceId, "slotGetSchema", False)

    dict = {}
    for k, v, a in schema.hash.iterall():
        if a["nodeType"] == 0:
            d = Type.fromname[a["valueType"]]()
            d.key = k
            dict[k] = d
        elif a["nodeType"] == 1 and a.get("displayType") == "Slot":
            del a["nodeType"]
            dict[k] = ProxySlot()
            dict[k].key = k
    Cls = type(schema.name, (Proxy,), dict)

    ret = Cls(instance, deviceId, sync)
    instance._devices[deviceId] = ret
    yield from ret
    return ret


def getDevice(deviceId, *, sync=None, timeout=-1):
    if sync is None:
        sync = get_event_loop().sync_set
    return _getDevice(deviceId, sync=sync, timeout=timeout)


@synchronize
def set(device, **kwargs):
    if isinstance(device, Proxy):
        device._update()
        device = device._deviceId
    h = Hash()
    for k, v in kwargs.items():
        h[k] = v
    yield from get_instance().call(device, "slotReconfigure", h)


def setNoWait(device, **kwargs):
    if isinstance(device, Proxy):
        device = device._deviceId
    h = Hash()
    for k, v in kwargs.items():
        h[k] = v
    get_instance()._ss.emit("call", {device: ["slotReconfigure"]}, h)


def executeNoWait(device, slot):
    if isinstance(device, Proxy):
        device = device._deviceId
    get_instance()._ss.emit("call", {device: [slot]})


@synchronize
def updateDevice(device):
    yield from device
    return device
