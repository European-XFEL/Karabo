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

import inspect
import logging
import random
import re
import sys
import weakref
from asyncio import (
    CancelledError, TimeoutError, ensure_future, gather, get_event_loop,
    iscoroutine, iscoroutinefunction, wait_for)
from collections import defaultdict

from karabo.native import (
    AccessLevel, AccessMode, Assignment, Configurable, Descriptor, Hash, Int32,
    KaraboError, Node, Slot, String, TypeHash, Weak)

from .pipeline import NetworkOutput, OutputChannel
from .proxy import DeviceClientProxyFactory
from .synchronization import FutureDict, firstCompleted
from .utils import get_karabo_version, get_property


class Signal:
    def __init__(self, *args):
        self.args = args


def _log_exception(func, device, message):
    logger = logging.getLogger(device.deviceId)
    _, exception, _ = sys.exc_info()
    # Use properties directly ...
    default = ('Exception in slot "%s" of device "%s" called by "%s"',
               func.__qualname__, device.deviceId,
               device._sigslot.get_property(message, 'signalInstanceId'))
    logmessage = getattr(exception, "logmessage", default)
    level = getattr(exception, "loglevel", logging.ERROR)
    logger.log(level, *logmessage, exc_info=True)


def slot(f, passMessage=False):
    is_coro = iscoroutinefunction(f)
    if not is_coro:
        def wrapper(func, device, name, message, args):
            try:
                device._sigslot.reply(message, func(*args))
            except BaseException as e:
                device._sigslot.replyException(message, e)
                _log_exception(func, device, message)
    else:
        def wrapper(func, device, name, message, args):
            broker = device._sigslot

            async def inner():
                try:
                    ret = await func(*args)
                    broker.reply(message, (ret))
                except BaseException as e:
                    broker.replyException(message, e)
                    _log_exception(func, device, message)

            if passMessage:
                args.append(message)
            get_event_loop().create_task(inner(), instance=device)

    f.slot = wrapper
    return f


# Backward compatibility
coslot = slot


class BoundSignal:
    def __init__(self, device, name, signal):
        self.device = device
        self.name = name
        self.args = signal.args

    def __call__(self, *args):
        args = [d.cast(v) for d, v in zip(self.args, args)]
        self.device._sigslot.emit_signal(self.name, *args)


def get_device_node_initializers(instance):
    """Returns a list of initializers for the DeviceNodes in a configurable.
    """
    ret = []
    klass = instance.__class__
    for key in instance._allattrs:
        descr = getattr(klass, key, None)
        if descr is None:
            # NOTE: This protection is solely for the unsupported
            # NodeTypes, e.g. ListOfNodes
            continue
        if descr.displayType == "deviceNode":
            ret.append(descr.finalize_init(instance))
        elif isinstance(descr, Node):  # recurse Nodes
            node = getattr(instance, descr.key)
            ret.extend(get_device_node_initializers(node))
    return ret


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
    naming_regex = re.compile("[A-Za-z0-9_/-]+")
    signalChanged = Signal(TypeHash(), String())

    __deviceServer = Weak()

    @String(
        displayedName="_DeviceID_",
        description="Do not set this property, it will be set by the "
                    "device-server",
        requiredAccessLevel=AccessLevel.EXPERT,
        assignment=Assignment.INTERNAL, accessMode=AccessMode.INITONLY,
        defaultValue="__none__",
    )
    def _deviceId_(self, value):
        self._deviceId_ = value
        self.logger = logging.getLogger(value)

    deviceId = String(
        displayedName="DeviceID",
        description="The device instance ID uniquely identifies a device "
                    "instance in the distributed system",
        accessMode=AccessMode.READONLY,
    )

    heartbeatInterval = Int32(
        displayedName="Heartbeat interval",
        description="The heartbeat interval",
        assignment=Assignment.OPTIONAL, defaultValue=120,
        accessMode=AccessMode.INITONLY,
        minInc=10,
        requiredAccessLevel=AccessLevel.EXPERT,
    )

    _sigslot = None

    @property
    def signalSlotable(self):
        """Public method to get the signal slotable"""
        return self._sigslot

    def __init__(self, configuration):
        self._sethash = {"ignore": "this"}
        self.is_server = False
        for k in dir(type(self)):
            if isinstance(getattr(self, k, None), Signal):
                setattr(self, k, BoundSignal(self, k, getattr(self, k)))
        super().__init__(configuration)
        if not self.naming_regex.fullmatch(self._deviceId_):
            raise RuntimeError(
                'Device name "{}" does not follow naming convention'
                .format(self._deviceId_))
        self.deviceId = self._deviceId_
        self._proxies = weakref.WeakValueDictionary()
        self._proxy_futures = {}
        self._timers = weakref.WeakSet()
        self.__initialized = False
        self.__removed = False
        self._new_device_futures = FutureDict()

    @property
    def is_initialized(self):
        """Check if the signal slotable is online and initialized

        This property has been added with Karabo 2.14.
        """
        return self._sigslot is not None and self.__initialized

    @property
    def device_server(self):
        """Get the device server instance hosting this device

        This property has been added with Karabo 2.14.
        """
        return self.__deviceServer

    def startInstance(self, server=None, *, loop=None, broadcast=True):
        """Start this (device) instance

        :param broadcast: Defines whether this device receives broadcasts

        This sets up everything for the instance to run, and then runs
        all initializing code. It returns the task in which this initializing
        code is running.
        """
        if loop is None:
            loop = get_event_loop()
        self._sigslot = loop.getBroker(self.deviceId, type(self).__name__,
                                       broadcast)
        self._sethash = {}
        return loop.create_task(self._run(server=server), instance=self)

    async def _assert_name_unique(self):
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
            await wait_for(
                self.call(self.deviceId, "slotPing",
                          self.__randPing), timeout=1)
            raise KaraboError('deviceId "{}" already in use'.
                              format(self.deviceId))
        except TimeoutError:
            pass
        self.__randPing = 0

    # slotPing _is_ a slot, but not using the official decorator.
    # See the definition of 'inner_ping' below.
    def slotPing(self, rand):
        """return our info to show that we are here"""
        if self.__randPing == 0 or self.__randPing != rand:
            # I am fully up (__randPing == 0) or I get a ping from another
            # (rand != __randPing) instance:
            # I reply my existence, providing my instance info.
            # NOTE: __randPing of a booting instance is always > 1,
            #       i.e. rand == 1 will always get this reply.

            return self._sigslot.info

    def inner_ping(func, device, name, message, args):
        ret = func(*args)
        # In contrast to normal slots, let slotPing not send an empty reply.
        if ret is not None:
            device._sigslot.reply(message, ret)

    slotPing.slot = inner_ping
    del inner_ping

    # Also slotDiscver _is_ a slot, but not using the official decorator.
    # See the definition of 'inner_discover' below.
    def slotDiscover(self, requestorId):
        if self.__randPing == 0:
            self.callNoWait(requestorId, "slotDiscoverAnswer",
                            self.deviceId, self._sigslot.info)

    def inner_discover(func, device, name, message, args):
        func(*args)
        if device.is_server:
            # The server has to forward this broadcast to its devices:
            for dev in device.deviceInstanceMap.values():
                dev.slotDiscover(*args)

    slotDiscover.slot = inner_discover
    del inner_discover

    @slot
    def slotHeartbeat(self, networkId, info):
        pass

    @slot
    def slotStopTrackingExistenceOfConnection(self, *args):
        print('received stopTracking...', args)

    @slot
    def slotGetOutputChannelInformation(self, info: Hash):
        channelId = info['channelId']
        try:
            ch = get_property(self, channelId)
        except AttributeError:
            ch = None
        if isinstance(ch, NetworkOutput) and ch.is_serving():
            ret = ch.getInformation("{}:{}".format(
                self.deviceId, channelId))
            ret["memoryLocation"] = "remote"
            return Hash("success", True, "info", ret)

        return Hash("success", False, "info", Hash())

    @slot
    def slotGetOutputChannelNames(self):
        """Return a list of channel names for the DAQ
        """
        ret = []

        for k in dir(self.__class__):
            desc = getattr(self.__class__, k, None)
            if isinstance(desc, Descriptor):
                for name, output in desc.allDescriptors():
                    if isinstance(output, OutputChannel):
                        ret.append(name)
        return ret

    def _initInfo(self):
        """return the info hash at initialization time"""
        return Hash(
            "type", "unknown",
            "heartbeatInterval", self.heartbeatInterval.value,
            "karaboVersion", get_karabo_version())

    def _register_slots(self):
        """Register all available slots on the broker
        """
        for k in dir(self.__class__):
            value = getattr(self, k, None)
            desc = getattr(self.__class__, k, None)
            # internal slots
            if callable(value) and hasattr(value, "slot"):
                self._sigslot.register_slot(k, value)
            # public slots
            elif isinstance(desc, Descriptor):
                for name, slot in desc.allDescriptors():
                    if isinstance(slot, Slot):
                        self._sigslot.register_slot(name, slot)

    async def _run(self, server=None, **kwargs):
        try:
            # add ourselves to the server immediately.
            if server is not None:
                # add deviceId to the instance map of the server
                server.addChild(self.deviceId, self)
                self.__deviceServer = server
            await self._sigslot.ensure_connection()
            self._register_slots()
            ensure_future(self._sigslot.main(self))
            await self._assert_name_unique()
            await self._sigslot.notify_network(self._initInfo())
            await super()._run(**kwargs)
            await wait_for(get_event_loop().run_coroutine_or_thread(
                self.preInitialization), timeout=5)
        except CancelledError:
            # Cancellation is caught here, remove ourselves
            if server is not None:
                self.__removed = True
                server.removeChild(self.deviceId)
        except (TimeoutError, Exception):
            # TimeoutError accounts the preInitialization
            await self.slotKillDevice()
            raise
        else:
            ensure_future(self._initialize_instance())

    async def _initialize_instance(self):
        try:
            initializers = get_device_node_initializers(self)
            if initializers:
                await gather(*initializers)
            # do not simply await because some `onInitialization`
            # could be not a coroutine in Macros.
            await get_event_loop().run_coroutine_or_thread(
                self.onInitialization)
        except CancelledError:
            # NOTE: onInitialization might still be active and creating proxies
            # and we simply catch the CancelledError. There is no need
            # to additionally kill the device, as this will be called
            # by the device server. Calling twice slotKillDevice will
            # deal with cyclic references and crash the server
            #
            # NOTE: Initializers for device nodes might still be active and the
            # Cancellation is caught here.
            pass
        except Exception:
            self.logger.exception("Error in onInitialization")
            await self.slotKillDevice()
        else:
            self.__initialized = True

    async def slotKillDevice(self, message=None):
        """Kill the device on shutdown

        :returns: success boolean if all tasks related to the device
                  are gone.
        """
        if self.__initialized:
            instanceId = (
                self._sigslot.get_property(message, "signalInstanceId")
                if message is not None else "OS signal")
            self.logger.info("Received request to shutdown SignalSlotable"
                             f" from {instanceId}.")
            self.__initialized = False
            try:
                await wait_for(get_event_loop().run_coroutine_or_thread(
                    self.onDestruction), timeout=5)
            except TimeoutError:
                self.logger.exception(
                    "onDestruction took longer than 5 seconds")
            except Exception:
                self.logger.exception("Exception in onDestruction")

        if self.device_server is not None and not self.__removed:
            self.__removed = True
            self.device_server.removeChild(self.deviceId)

        # Stop all timers!
        for timer in list(self._timers):
            timer.destroy()

        if self._sigslot is not None:
            # Returns success
            return await self._sigslot.stop_tasks()

        # No tasks are running
        return True

    slotKillDevice = slot(slotKillDevice, passMessage=True)

    def __del__(self):
        if self._sigslot is not None and self._sigslot.loop.is_running():
            self._sigslot.loop.call_soon_threadsafe(
                self._sigslot.loop.create_task, self.slotKillDevice())

    def create_instance_task(self, coro):
        """Wrap a coroutine into a task and attach it to the instance"""
        text = f"Input must be a of type coroutine, got {type(coro)} instead."
        assert iscoroutine(coro), text
        loop = get_event_loop()
        return loop.create_task(coro, instance=self)

    async def call(self, device, target, *args):
        return (await self._sigslot.request(device, target, *args))

    def callNoWait(self, device, target, *args):
        self._sigslot.call_slot(device, target, None, *args)

    def stopEventLoop(self):
        """Method called by the device server to stop the event loop
        """
        get_event_loop().stop()

    @slot
    def slotHasSlot(self, slot):
        slotCand = getattr(self, slot, None)
        if slotCand is not None:
            if hasattr(slotCand, 'slot'):
                return inspect.ismethod(slotCand)
        return False

    def updateInstanceInfo(self, info):
        self._sigslot.updateInstanceInfo(info)

    def update(self):
        """Update via sending a bulk hash on the network
        """
        if not self._sethash:
            return
        hash_dict = defaultdict(Hash)
        while self._sethash:
            k, (v, desc) = self._sethash.popitem()
            value, attrs = desc.toDataAndAttrs(v)
            tid = attrs.get('tid', 0)
            h = hash_dict[tid]
            h.setElement(k, value, attrs)
        # we have to send our changes in tid order for the DAQ!
        for tid in sorted(hash_dict.keys()):
            h = hash_dict[tid]
            self.signalChanged(h, self.deviceId)

    def setChildValue(self, key, value, desc):
        self._sethash[key] = value, desc
        if self._sigslot is not None:
            self._sigslot.loop.call_soon_threadsafe(self.update)

    @slot
    def slotChanged(self, configuration, deviceId):
        d = self._proxies.get(deviceId)
        if d is not None:
            d._onChanged(configuration)
        get_event_loop().something_changed()

    @slot
    def slotSchemaUpdated(self, schema, deviceId):
        d = self._proxies.get(deviceId)
        if d is not None:
            DeviceClientProxyFactory.updateSchema(d, schema)
        get_event_loop().something_changed()

    @slot
    async def slotInstanceNew(self, instanceId, info):
        if info["type"] == "server":
            for proxy in self._proxies.values():
                if proxy.serverId == instanceId and proxy._alive:
                    proxy._notify_gone()
        self._new_device_futures[instanceId] = info
        proxy = self._proxies.get(instanceId)
        if proxy is not None:
            await proxy._notify_new()

        get_event_loop().something_changed()

    async def _call_once_alive(self, deviceId, slot, *args):
        """try to call slot, wait until device becomes alive if needed"""
        done, pending, error = await firstCompleted(
            newdevice=self._new_device_futures[deviceId],
            call=self.call(deviceId, slot, *args))
        if error:
            raise error.popitem()[1]
        elif "call" in done:
            return done["call"]
        elif "newdevice" in done:
            return (await self.call(deviceId, slot, *args))
        else:
            raise AssertionError("this should not happen")

    @slot
    def slotInstanceUpdated(self, instanceId, info):
        pass

    @slot
    def slotInstanceGone(self, instanceId, info):
        proxy = self._proxies.get(instanceId)
        if proxy is not None:
            proxy._notify_gone()
        get_event_loop().something_changed()

    async def preInitialization(self):
        """This method is called before a device is instantiated

        Subclass this method to validate the `instance` behavior.
        Throwing an exception will shutdown the `instance`.
        NOTE: No network depent tasks should be executed here.
        e.g. no device connection, no pipeline connection,
        DeviceNodes will not be connected yet to the devices.
        """

    async def onInitialization(self):
        """This method is called just after instance is running"""

    async def onDestruction(self):
        """This method is called just before the device ceases existence

        Subclass this method to cancel and cleanup any existing long-term
        tasks quickly. This method will be cancelled if the execution is longer
        than 5 seconds.
        """

    async def onCancelled(self, slot):
        """This method is called if a slot gets cancelled"""

    async def onException(self, slot, exception, traceback):
        """This method is called if an exception in a slot is not caught"""

    def _onException(self, slot, exc, tb):
        logger = logging.getLogger(self.deviceId)
        if isinstance(exc, CancelledError):
            m = self.onCancelled
            args = slot,
        else:
            m = self.onException
            args = slot, exc, tb
            error_name = type(exc).__name__
            if slot is None:
                logger.error(f"{error_name}: {exc}",
                             exc_info=(type(exc), exc, tb))
            else:
                logger.error(f"{error_name} in slot '{slot.key}': {exc}",
                             exc_info=(type(exc), exc, tb))

        async def logException(coro):
            try:
                await coro
            except BaseException:
                logger.exception("error in error handler")

        loop = get_event_loop()
        coro = logException(loop.run_coroutine_or_thread(m, *args))
        return loop.create_task(coro, instance=self)
