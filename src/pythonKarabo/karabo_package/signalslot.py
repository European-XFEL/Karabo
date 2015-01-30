from __future__ import unicode_literals
from karabo.decorators import KARABO_CLASSINFO, KARABO_CONFIGURATION_BASE_CLASS
from karabo import hashtypes
from karabo.hash import BinaryParser, Hash
from karabo.hashtypes import String, Int32, Type, Slot
from karabo.enums import AccessLevel, Assignment, AccessMode
from karabo.schema import Schema, Configurable
from karabo.p2p import NetworkOutput
from karabo.timestamp import Timestamp
from karabo.registry import Registry

from asyncio import (async, coroutine, Future, get_event_loop, sleep,
                     TimeoutError, wait, wait_for)
import random
import sys
import time


class Signal(object):
    def __init__(self, *args):
        self.args = args


class Device(Registry):
    @classmethod
    def schematize(cls):
        schema = Schema(cls.__name__, hash)
        for k, v in cls.__dict__.items():
            if isinstance(v, Type):
                h = Hash()
                hash[k] = h
                hash[k, 'nodeType'] = 0
                hash[k, 'valueType'] = v.hashname()
        return schema

    @classmethod
    def register(cls, name, dict):
        super(Device, cls).register(name, dict)
        #globalDevice.slotNewClassAvailable('bla', name, cls.schematize())


def slot(f):
    def inner(device, message, args):
        device._ss.reply(message, f(device, *args))
    f.slot = inner
    return f


def coslot(f):
    f = coroutine(f)

    @coroutine
    def inner(device, message, args):
        device._ss.reply(message, (yield from f(device, *args)))

    def outer(device, message, args):
        device.async(inner(device, message, args))

    f.slot = outer
    return f


def replySlot(name):
    def outer(f):
        def inner(device, message, args):
            ret = f(device, *args)
            device._ss.reply(message, ret)
            device._ss.emit("call", {
                message.properties["signalInstanceId"].decode("utf8"): [name]},
                *ret)
        f.slot = inner
        return f
    return outer


class DeviceServer(Device):
    def __init__(self, deviceId):
        Device.__init__(self, deviceId, type="server", serverId=deviceId,
                        version=23, host="exflpcx18981", visibility=0)


class ConnectionType(object):
    NO_TRACK = 0


class BoundSignal(object):
    def __init__(self, device, name, signal):
        self.device = device
        self.name = name
        self.connected = {}
        self.args = signal.args

    def connect(self, target, slot, dunno):
        self.connected.setdefault(target, set()).add(slot)

    def disconnect(self, target, slot):
        s = self.connected.get(target, None)
        if s is not None:
            s.discard(slot)
            if not s:
                del self.connected[target]

    def __call__(self, *args):
        args = [d.cast(v) for d, v in zip(self.args, args)]
        self.device._ss.emit(self.name, self.connected, *args)


class Proxy(object):
    def __init__(self, device, deviceId):
        self._device = device
        self._futures = {}
        self._deviceId = deviceId
        self._used = 0

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
                    f.set_result((k, v, Timestamp.fromHashAttributes(a)))
        f = self._futures.pop(None, None)
        if f is not None:
            f.set_result(hash)

    def setValue(self, attr, value):
        self._device._ss.emit("call", {self.deviceId: ["slotReconfigure"]},
                              Hash(attr.key, value))

    @coroutine
    def set(self, **kwargs):
        h = Hash()
        for k, v in kwargs.items():
            h[k] = v
        yield from self._device.call(self.deviceId, "slotReconfigure", h)

    def setNoWait(self, **kwargs):
        h = Hash()
        for k, v in kwargs.items():
            h[k] = v
        self._device._ss.emit("call", {self.deviceId: ["slotReconfigure"]}, h)

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
        conf, _ = yield from self._device.call(self._deviceId,
                                               "slotGetConfiguration")
        self._onChanged(conf)
        return self


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO('SignalSlotable', '1.0')
class SignalSlotable(Configurable):
    """The base class for all objects connected to the broker

    This contains everything to talk to the internet, especially
    the handling of signals and slots, but also heartbeats.

    Every coroutine is automatically a slot and can be called from the
    outside. Signals must be declared as follows:

    ::

        from karabo import String, Integer
        from karabo.signalslotable import SignalSlotable, Signal

        class Spam(SignalSlotable):
            signalHam = Signal(String(), Integer())"""
    signalHeartbeat = Signal(hashtypes.String(), hashtypes.Int32(),
                             hashtypes.Hash())
    signalChanged = Signal(hashtypes.Hash(), hashtypes.String())

    _deviceId_ = String(
        displayedName="_DeviceID_",
        description="Do not set this property, it will be set by the "
                    "device-server",
        requiredAccessLevel=AccessLevel.EXPERT,
        assignment=Assignment.INTERNAL, accessMode=AccessMode.INITONLY,
        defaultValue="__none__")

    deviceId = String(
        displayedName="DeviceID",
        description="The device instance ID uniquely identifies a device "
                    "instance in the distributed system",
        accessMode=AccessMode.READONLY)

    heartbeatInterval = Int32(
        displayedName="Heartbeat interval",
        description="The heartbeat interval",
        assignment=Assignment.OPTIONAL, defaultValue=20,
        requiredAccessLevel=AccessLevel.ADMIN)

    def __init__(self, configuration):
        self.notify_changes = False
        for k in dir(type(self)):
            if isinstance(getattr(self, k, None), Signal):
                setattr(self, k, BoundSignal(self, k, getattr(self, k)))
        super().__init__(configuration)
        self.deviceId = self._deviceId_
        self._ss = get_event_loop().getBroker(self.deviceId,
                                              type(self).__name__)
        self.notify_changes = True
        self.info = Hash("heartbeatInterval", self.heartbeatInterval)

        self.__devices = {}
        self.__randPing = 0
        self._tasks = set()

    @slot
    def slotPing(self, instanceId, rand, track=None):
        if rand:
            if instanceId == self.deviceId and self.__randPing != rand:
                return self.info
        else:
            self._ss.emit("call", {instanceId: ["slotPingAnswer"]},
                          self.deviceId, self.info)
        if track and instanceId != self.deviceId:
            self.signalHeartbeat.connect(instanceId, "slotHeartbeat", None)

    @slot
    def slotHeartbeat(self, networkId, heartbeatInterval, info):
        pass

    @slot
    def slotStopTrackingExistenceOfConnection(self, *args):
        print('receivet stopTracking...', args)

    @slot
    def slotDisconnectFromSlot(self, *args):
        print("SDFS", args)

    @slot
    def slotGetOutputChannelInformation(self, ioChannelId, processId):
        ch = getattr(self, ioChannelId, None)
        if isinstance(ch, NetworkOutput):
            ret = ch.getInformation()
            ret["memoryLocation"] = "remote"
            return True, ret
        else:
            return False, Hash()

    @coroutine
    def heartbeats(self):
        while True:
            self.signalHeartbeat(self.deviceId, self.heartbeatInterval,
                                 self.info)
            yield from sleep(self.heartbeatInterval)

    @coroutine
    def run_async(self):
        self.async(self._ss.consume(self))
        try:
            self.__randPing = random.randint(1, 0x7fffffff)
            yield from wait_for(
                self.call("*", "slotPing", self.deviceId,
                          self.__randPing, False), timeout=3)
            yield from self.slotKillDevice()
            return
        except TimeoutError:
            pass
        self.run()
        self._ss.emit('call', {'*': ['slotInstanceNew']},
                      self.deviceId, self.info)
        self.async(self.heartbeats())

    def async(self, coro):
        """start a coroutine asynchronously

        Use this to execute a coroutine in the background. The coroutine
        is registered with this object so it will be stopped once this
        object is stopped. """
        task = async(coro, loop=self._ss.loop)
        self._tasks.add(task)
        task.add_done_callback(self._tasks.remove)
        return task

    @coslot
    def slotKillDevice(self):
        self.log.INFO("Device is going down as instructed")
        for t in self._tasks:
            t.cancel()
        yield from wait(self._tasks)
        return True

    def call(self, device, target, *args):
        reply = "{}-{}".format(self.deviceId, time.monotonic().hex())
        self._ss.call("call", {device: [target]}, reply, args)
        future = Future(loop=self._ss.loop)
        self._ss.repliers[reply] = future
        return future

    def stopEventLoop(self):
        get_event_loop().stop()

    @slot
    def slotConnectToSignal(self, signal, target, slot, dunno):
        getattr(self, signal).connect(target, slot, dunno)
        return True

    @slot
    def slotDisconnectFromSignal(self, signal, target, slot):
        getattr(self, signal).disconnect(target, slot)
        return True

    @staticmethod
    def create(serverid):
        return SignalSlotable(serverid)

    def updateInstanceInfo(self, info):
        self.info.merge(info)
        self._ss.emit('call', {'*': ['slotInstanceUpdated']},
                      self.deviceId, self.info)

    def setValue(self, attr, value):
        self.__dict__[attr] = value
        if self.notify_changes:
            self.signalChanged(Hash(attr.key, value), self.deviceId)

    def setChildValue(self, key, value):
        if self.notify_changes:
            self.signalChanged(Hash(key, value), self.deviceId)

    @slot
    def slotSchemaUpdated(self, schema, deviceId):
        print("slot schema updated called:", deviceId)

    @slot
    def slotChanged(self, configuration, deviceId):
        d = self.__devices.get(deviceId)
        if d is not None:
            d._onChanged(configuration)
        loop = get_event_loop()
        loop.changedFuture.set_result(None)
        loop.changedFuture = Future()

    @coroutine
    def waitUntil(self, condition):
        loop = get_event_loop()
        while not condition():
            yield from loop.changedFuture

    @coroutine
    def getDevice(self, deviceId):
        ret = self.__devices.get(deviceId)
        if ret is not None:
            yield from ret
            return ret

        schema, _ = yield from self.call(deviceId, "slotGetSchema", False)

        dict = {}
        for k, v, a in schema.hash.iterall():
            if a["nodeType"] == 0:
                d = Type.fromname[a["valueType"]]()
                d.key = k
                dict[k] = d
            elif a["nodeType"] == 1 and a.get("displayType") == "Slot":
                s = Slot()
                s.method = lambda self, name=k: self._device._ss.emit(
                    "call", {self.deviceId: [name]})
                dict[k] = s
        Cls = type(schema.name, (Proxy,), dict)

        ret = Cls(self, deviceId)
        self.__devices[deviceId] = ret
        yield from ret
        return ret
