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

from asyncio import (async, coroutine, Future, get_event_loop, iscoroutine,
                     sleep, TimeoutError, wait, wait_for)
from functools import wraps
import random
import sys
import time
import weakref


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
        async(inner(device, message, args))

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
        self._sethash = Hash()
        for k in dir(type(self)):
            if isinstance(getattr(self, k, None), Signal):
                setattr(self, k, BoundSignal(self, k, getattr(self, k)))
        super().__init__(configuration)
        self.deviceId = self._deviceId_
        self.info = Hash("heartbeatInterval", self.heartbeatInterval)
        self._devices = {}
        self.__randPing = random.randint(2, 0x7fffffff)
        self._tasks = set()

    def startInstance(self, loop=None):
        if loop is None:
            loop = get_event_loop()
        self._ss = loop.getBroker(self.deviceId, type(self).__name__)
        self._sethash = None
        return loop.create_task(self.run_async(), self)

    def slotPing(self, instanceId, rand, track=None):
        """return our info to show that we are here"""
        # during startup, we ping possible other instances with our name,
        # no response means that we are alone. To avoid that we respond
        # ourselves, we set self.__randPing to a random value and pass it
        # as the parameter rand, so that we now we pinged ourselves.
        # Once we know we are alone, self.__randPing is set to 0 meaning
        # that we start responding to other pings.
        if rand:
            if instanceId == self.deviceId and self.__randPing != rand:
                return self.info
        elif self.__randPing == 0:
            self._ss.emit("call", {instanceId: ["slotPingAnswer"]},
                          self.deviceId, self.info)
        if track and instanceId != self.deviceId:
            self.signalHeartbeat.connect(instanceId, "slotHeartbeat", None)

    def inner(self, message, args):
        ret = self.slotPing(*args)
        if ret is not None:
            self._ss.reply(message, ret)
    slotPing.slot = inner
    del inner

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
        while self is not None:
            self.signalHeartbeat(self.deviceId, self.heartbeatInterval,
                                 self.info)
            interval = self.heartbeatInterval
            self = weakref.ref(self)
            yield from sleep(interval)
            self = self()

    @coroutine
    def run_async(self):
        async(self._ss.consume(self))
        try:
            yield from wait_for(
                self.call("*", "slotPing", self.deviceId,
                          self.__randPing, False), timeout=3)
            yield from self.slotKillDevice()
            raise RuntimeError('deviceId "{}" already in use'.
                               format(self.deviceId))
        except TimeoutError:
            pass
        self.run()
        self.__randPing = 0
        self._ss.emit('call', {'*': ['slotInstanceNew']},
                      self.deviceId, self.info)
        async(self.heartbeats())

    def executeSlot(self, slot, message):
        coro = slot(self)
        if iscoroutine(coro):
            def inner():
                self._ss.reply(message, (yield from coro))
            return async(inner())
        ret = Future()
        ret.set_result(coro)
        self._ss.reply(message, coro)
        return ret

    @coslot
    def slotKillDevice(self):
        for t in self._tasks:
            t.cancel()
        yield from wait(self._tasks)
        return True

    @coroutine
    def call(self, device, target, *args):
        reply = "{}-{}".format(self.deviceId, time.monotonic().hex())
        self._ss.call("call", {device: [target]}, reply, args)
        future = Future(loop=self._ss.loop)
        self._ss.repliers[reply] = future
        return (yield from future)

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
        if self._sethash is None:
            self._sethash = Hash()
            self._ss.loop.call_soon_threadsafe(self.update)
        self._sethash[attr.key] = value

    def update(self):
        if self._sethash is not None:
            self.signalChanged(self._sethash, self.deviceId)
            self._sethash = None

    def setChildValue(self, key, value):
        if self._sethash is None:
            self._sethash = Hash()
            self._ss.loop.call_soon_threadsafe(self.update)
        self._sethash[key] = value

    @slot
    def slotSchemaUpdated(self, schema, deviceId):
        print("slot schema updated called:", deviceId)

    @slot
    def slotChanged(self, configuration, deviceId):
        d = self._devices.get(deviceId)
        if d is not None:
            d._onChanged(configuration)
        loop = get_event_loop()
        if not loop.changedFuture.done():
            loop.changedFuture.set_result(None)
        loop.changedFuture = Future(loop=loop)
