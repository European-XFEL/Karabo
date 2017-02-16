from __future__ import unicode_literals
from asyncio import (async, CancelledError, coroutine, FIRST_COMPLETED, Future,
                     get_event_loop, sleep, TimeoutError, wait, wait_for)
import logging
import random
import sys
import weakref
import inspect

from .exceptions import KaraboError
from .enums import AccessLevel, Assignment, AccessMode
from .hash import Hash, HashType, Int32, String
from .p2p import NetworkOutput
from .schema import Configurable


class Signal(object):
    def __init__(self, *args):
        self.args = args


def _log_exception(func, device, message):
    logger = logging.getLogger(device.deviceId)
    _, exception, _ = sys.exc_info()
    default = ('Exception in slot "%s" of device "%s" called by "%s"',
               func.__qualname__, device.deviceId,
               message.properties['signalInstanceId'].decode("ascii"))
    logmessage = getattr(exception, "logmessage", default)
    level = getattr(exception, "loglevel", logging.ERROR)
    logger.log(level, *logmessage, exc_info=True)


def slot(f):
    def inner(func, device, message, args):
        try:
            device._ss.reply(message, func(*args))
        except Exception as e:
            device._ss.replyException(message, e)
            _log_exception(func, device, message)
    f.slot = inner
    return f


def coslot(f, passMessage=False):
    f = coroutine(f)

    def outer(func, device, message, args):
        broker = device._ss

        @coroutine
        def inner():
            try:
                broker.reply(message, (yield from func(*args)))
            except Exception as e:
                broker.replyException(message, e)
                _log_exception(func, device, message)
        if passMessage:
            args.append(message)
        async(inner())

    f.slot = outer
    return f


class BoundSignal(object):
    def __init__(self, device, name, signal):
        self.device = device
        self.name = name
        self.connected = {}
        self.args = signal.args

    def connect(self, target, slot):
        self.connected.setdefault(target, set()).add(slot)

    def disconnect(self, target, slot):
        s = self.connected.get(target, None)
        if s is None:
            return False
        else:
            result = slot in s
            s.discard(slot)
            if not s:
                del self.connected[target]
            return result

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

        from karabo.middlelayer import String, Int
        from karabo.middlelayer import SignalSlotable, Signal

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
        self._ss = None
        self._sethash = {"ignore": "this"}
        for k in dir(type(self)):
            if isinstance(getattr(self, k, None), Signal):
                setattr(self, k, BoundSignal(self, k, getattr(self, k)))
        super().__init__(configuration)
        self.deviceId = self._deviceId_
        self._proxies = weakref.WeakValueDictionary()
        self._proxy_futures = {}
        self.__initialized = False
        self._new_device_futures = {}

    def startInstance(self, server=None, *, loop=None):
        """Start this (device) instance

        This sets up everything for the instance to run, and then runs
        all initializing code. It returns the task in which this initializing
        code is running."""
        if loop is None:
            loop = get_event_loop()
        self._ss = loop.getBroker(self.deviceId, type(self).__name__)
        self._sethash = {}
        return loop.create_task(self._run(server=server), self)

    @coroutine
    def _assert_name_unique(self):
        """check that our device Id is unique

        during startup, we ping possible other instances with our name,
        no response means that we are alone. To avoid that we respond
        ourselves, we set self.__randPing to a random value and pass it
        as the parameter rand, so that we know we pinged ourselves.
        Once we know we are alone, self.__randPing is set to 0 meaning
        that we start responding to other pings.
        """
        self.__randPing = random.randint(2, 0x7fffffff)
        try:
            yield from wait_for(
                self.call(self.deviceId, "slotPing", self.deviceId,
                          self.__randPing, False), timeout=1)
            raise KaraboError('deviceId "{}" already in use'.
                              format(self.deviceId))
        except TimeoutError:
            pass
        self.__randPing = 0

    # slotPing _is_ a slot, but not using the official decorator.
    # See the definition of 'inner' below.
    def slotPing(self, instanceId, rand, track=None):
        """return our info to show that we are here"""
        if rand:
            if instanceId == self.deviceId and self.__randPing != rand:
                return self._ss.info
        elif self.__randPing == 0:
            self._ss.emit("call", {instanceId: ["slotPingAnswer"]},
                          self.deviceId, self._ss.info)

    def inner(func, device, message, args):
        ret = func(*args)
        # In contrast to normal slots, let slotPing not send an empty reply.
        if ret is not None:
            device._ss.reply(message, ret)
    slotPing.slot = inner
    del inner

    @slot
    def slotHeartbeat(self, networkId, heartbeatInterval, info):
        pass

    @slot
    def slotStopTrackingExistenceOfConnection(self, *args):
        print('receivet stopTracking...', args)

    @slot
    def slotGetOutputChannelInformation(self, ioChannelId, processId):
        ch = getattr(self, ioChannelId, None)
        if isinstance(ch, NetworkOutput):
            ret = ch.getInformation()
            ret["memoryLocation"] = "remote"
            return True, ret
        else:
            return False, Hash()

    def _initInfo(self):
        """return the info hash at initialization time"""
        return Hash("heartbeatInterval", self.heartbeatInterval.value)

    @coroutine
    def _run(self, server=None, **kwargs):
        try:
            for k in dir(self.__class__):
                v = getattr(self, k, None)
                if callable(v) and hasattr(v, "slot"):
                    self._ss.register_slot(k, v)
            async(self._ss.main(self))
            yield from self._assert_name_unique()
            self._ss.notify_network(self._initInfo())
            if server is not None:
                server.addChild(self.deviceId, self)
            yield from super(SignalSlotable, self)._run(**kwargs)
            yield from get_event_loop().run_coroutine_or_thread(
                self.onInitialization)
            self.__initialized = True
        except:
            yield from self.slotKillDevice()
            raise

    @coslot
    def slotKillDevice(self):
        if self.__initialized:
            self.__initialized = False
            yield from get_event_loop().run_coroutine_or_thread(
                self.onDestruction)
        yield from self._ss.stop_tasks()

    def __del__(self):
        if self._ss is not None and self._ss.loop.is_running():
            self._ss.loop.call_soon_threadsafe(
                self._ss.loop.create_task, self.slotKillDevice())

    @coroutine
    def call(self, device, target, *args):
        return (yield from self._ss.request(device, target, *args))

    def stopEventLoop(self):
        get_event_loop().stop()

    @slot
    def slotConnectToSignal(self, signal, target, slot):
        signalObj = getattr(self, signal, None)
        if signalObj is None:
            return False
        else:
            signalObj.connect(target, slot)
            return True

    @slot
    def slotDisconnectFromSignal(self, signal, target, slot):
        signalObj = getattr(self, signal, None)
        if signalObj is None:
            return False
        else:
            return signalObj.disconnect(target, slot)

    @slot
    def slotHasSlot(self, slot):
        slotCand = getattr(self, slot, None)
        if slotCand is not None:
            if hasattr(slotCand, 'slot'):
                return inspect.ismethod(slotCand)
        return False

    def updateInstanceInfo(self, info):
        self._ss.updateInstanceInfo(info)

    def update(self):
        if not self._sethash:
            return
        h = Hash()
        while self._sethash:
            k, (v, desc) = self._sethash.popitem()
            value, attrs = desc.toDataAndAttrs(v)
            h[k] = value
            h[k, ...].update(attrs)
        if h:
            self.signalChanged(h, self.deviceId)

    def setChildValue(self, key, value, desc):
        self._sethash[key] = value, desc
        if self._ss is not None:
            self._ss.loop.call_soon_threadsafe(self.update)

    @slot
    def slotSchemaUpdated(self, schema, deviceId):
        print("slot schema updated called:", deviceId)

    @slot
    def slotChanged(self, configuration, deviceId):
        d = self._proxies.get(deviceId)
        if d is not None:
            d._onChanged(configuration)
        loop = get_event_loop()
        loop.notifyChanged()

    @slot
    def slotSchemaUpdated(self, schema, deviceId):
        d = self._proxies.get(deviceId)
        if d is not None:
            get_event_loop().notifyChanged()
            d._onSchemaUpdated(schema)

    @slot
    def slotInstanceNew(self, instanceId, info):
        future = self._new_device_futures.pop(instanceId, None)
        if future is not None:
            future.set_result(None)

    @coroutine
    def _call_once_alive(self, deviceId, slot, *args):
        """try to call slot, wait until device becomes alive if needed"""
        newdevice = self._new_device_futures.setdefault(deviceId, Future())
        call = async(self.call(deviceId, slot, *args))
        done, pending = yield from wait([newdevice, call],
                                        return_when=FIRST_COMPLETED)
        for p in pending:
            p.cancel()
        self._new_device_futures.pop(deviceId, None)
        if call in done:
            return call.result()
        elif newdevice in done:
            return (yield from self.call(deviceId, slot, *args))
        else:
            raise AssertionError("this should not happen")

    @slot
    def slotInstanceUpdated(self, instanceId, info):
        pass

    @slot
    def slotInstanceGone(self, instanceId, info):
        device = self._proxies.get(instanceId)
        if device is not None:
            device._notify_gone()

    @coroutine
    def onInitialization(self):
        """This method is called just after everything is initialized"""

    @coroutine
    def onDestruction(self):
        """This method is called just before the device ceases existence"""

    @coroutine
    def onCancelled(self, slot):
        """This method is called if a slot gets cancelled"""

    @coroutine
    def onException(self, slot, exception, traceback):
        """This method is called if an exception in a slot is not caught"""

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

        loop = get_event_loop()
        coro = logException(loop.run_coroutine_or_thread(m, *args))
        return loop.create_task(coro, instance=self)
