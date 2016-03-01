from __future__ import unicode_literals
from asyncio import (async, CancelledError, coroutine, Future, get_event_loop,
                     iscoroutinefunction, sleep, TimeoutError, wait, wait_for)
import logging
import random
import time
import weakref

from .exceptions import KaraboError
from .enums import AccessLevel, Assignment, AccessMode
from .hash import Hash, HashType, Int32, String
from .p2p import NetworkOutput
from .schema import Configurable


class Signal(object):
    def __init__(self, *args):
        self.args = args


def slot(f):
    def inner(device, message, args):
        device._ss.reply(message, f(device, *args))
    f.slot = inner
    return f


def coslot(f):
    f = coroutine(f)

    @coroutine
    def inner(device, message, args):
        try:
            device._ss.reply(message, (yield from f(device, *args)))
        except CancelledError:
            raise
        except Exception:
            logger = logging.getLogger(device.deviceId)
            logger.exception('exception in slot "{}" of device "{}"'.
                             format(f.__qualname__, device.deviceId))

    def outer(device, message, args):
        async(inner(device, message, args))

    f.slot = outer
    return f


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


class SignalSlotable(Configurable):
    """The base class for all objects connected to the broker

    This contains everything to talk to the internet, especially
    the handling of signals and slots, but also heartbeats.

    Every coroutine is automatically a slot and can be called from the
    outside. Signals must be declared as follows:

    ::

        from karabo.api_2 import String, Int
        from karabo.api_2 import SignalSlotable, Signal

        class Spam(SignalSlotable):
            signalHam = Signal(String(), Int())"""
    signalChanged = Signal(HashType(), String())

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
        self._sethash = {"ignore": "this"}
        for k in dir(type(self)):
            if isinstance(getattr(self, k, None), Signal):
                setattr(self, k, BoundSignal(self, k, getattr(self, k)))
        super().__init__(configuration)
        self.deviceId = self._deviceId_
        self._devices = {}
        self.__randPing = random.randint(2, 0x7fffffff)

    def startInstance(self, server=None, *, loop=None):
        """Start this (device) instance

        This sets up everything for the instance to run, and then runs
        all initializing code. It returns the task in which this initializing
        code is running."""
        if loop is None:
            loop = get_event_loop()
        self._ss = loop.getBroker(self.deviceId, type(self).__name__)
        self._ss.info = Hash("heartbeatInterval", self.heartbeatInterval)
        self._sethash = {}
        if server is not None:
            server.addChild(self.deviceId, self)
        return loop.create_task(self.run_async(), self)


    # slotPing _is_ a slot, but not using the official decorator.
    # See the definition of 'inner' below.
    def slotPing(self, instanceId, rand, track=None):
        """return our info to show that we are here"""
        # during startup, we ping possible other instances with our name,
        # no response means that we are alone. To avoid that we respond
        # ourselves, we set self.__randPing to a random value and pass it
        # as the parameter rand, so that we know we pinged ourselves.
        # Once we know we are alone, self.__randPing is set to 0 meaning
        # that we start responding to other pings.
        if rand:
            if instanceId == self.deviceId and self.__randPing != rand:
                return self._ss.info
        elif self.__randPing == 0:
            self._ss.emit("call", {instanceId: ["slotPingAnswer"]},
                          self.deviceId, self._ss.info)

    def inner(self, message, args):
        ret = self.slotPing(*args)
        # In contrast to normal slots, let slotPing not send an empty reply.
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
    def run_async(self):
        """start everything needed for this device

        This coroutine is called once a device is started. Overwrite this
        method if you want code to be called at startup time. Don't forget
        to yield from super.

        This method also calls ``self.run``, so if you don't need a coroutine,
        it's easier to overwrite ``run``.

        This method is supposed to return once everything is up and running.
        If you have long-running tasks, start them with async.

        Return a future which represents the Karabo event dispatcher.
        Once this future is done, the entire device is considered dead, and
        all other still running tasks should be cancelled as well."""
        self.mainloop = async(self._ss.main(self))
        try:
            yield from wait_for(
                self.call(self.deviceId, "slotPing", self.deviceId,
                          self.__randPing, False), timeout=1)
            try:
                yield from self.slotKillDevice()
            except CancelledError:
                pass
            raise KaraboError('deviceId "{}" already in use'.
                              format(self.deviceId))
        except TimeoutError:
            pass
        self.run()
        self.__randPing = 0  # Start answering on slotPing with argument rand=0
        async(self._ss.notify_network())

    @coslot
    def slotKillDevice(self):
        self.mainloop.cancel()

    def call(self, device, target, *args):
        reply = "{}-{}".format(self.deviceId, time.monotonic().hex())
        self._ss.call("call", {device: [target]}, reply, args)
        future = Future(loop=self._ss.loop)
        self._ss.repliers[reply] = future
        future.add_done_callback(lambda _: self._ss.repliers.pop(reply))
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

    def updateInstanceInfo(self, info):
        self.info.merge(info)
        self._ss.emit('call', {'*': ['slotInstanceUpdated']},
                      self.deviceId, self.info)

    def setValue(self, attr, value):
        self.__dict__[attr.key] = value
        update = not self._sethash
        self._sethash[attr.key] = value
        if update:
            self._ss.loop.call_soon_threadsafe(self.update)

    def update(self):
        hash = Hash()
        while self._sethash:
            k, v = self._sethash.popitem()
            hash[k] = v
        if hash:
            self.signalChanged(hash, self.deviceId)

    def setChildValue(self, key, value):
        if not self._sethash:
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
        for f in loop.changedFutures:
            f.set_result(None)

    @coroutine
    def onCancelled(self, slot):
        pass

    @coroutine
    def onException(self, slot, exception, traceback):
        pass

    def _onException(self, slot, exc, tb):
        logger = logging.getLogger(self.deviceId)

        if isinstance(exc, CancelledError):
            m = self.onCancelled
            args = slot,
        else:
            m = self.onException
            args = slot, exc, tb
            if slot is None:
                logger.error('error in device "%s"', self.deviceId,
                             exc_info=(type(exc), exc, tb))
            else:
                logger.error('error in slot "%s" of device "%s"',
                             slot.key, self.deviceId,
                             exc_info=(type(exc), exc, tb))

        @coroutine
        def logException(coro):
            try:
                yield from coro
            except:
                logger.exception("error in error handler")

        if iscoroutinefunction(m):
            async(logException(m(*args)))
        else:
            async(logException(get_event_loop().start_thread(m, *args)))
