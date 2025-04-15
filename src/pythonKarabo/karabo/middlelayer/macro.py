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
import atexit
import os
import socket
import sys
import threading
from asyncio import (
    CancelledError, TimeoutError, current_task, ensure_future, gather,
    get_event_loop, iscoroutinefunction, set_event_loop, wait_for)
from collections import deque
from contextlib import closing
from functools import wraps

from karabo.common.states import State
from karabo.native import (
    AccessLevel, AccessMode, Attribute, Descriptor, Hash, Int32, Slot, String,
    VectorString)

from .device import Device, DeviceClientBase
from .device_client import getDevice, waitUntilNew
from .eventloop import EventLoop
from .signalslot import slot
from .utils import AsyncTimer, countdown

DEFAULT_ACTION_NAME = "_last_action"
PRINT_THROTTLE = 0.5  # seconds


def run_macro(cls: "Macro", call: str, config: dict | None = None):
    """Convenience function to run a macro from command line"""
    assert issubclass(cls, Macro), "Class must be of type `Macro`"
    if config is None:
        config = {}
    if "deviceId" in config:
        config["_deviceId_"] = config["deviceId"]
    else:
        bareHostName = socket.gethostname().partition('.')[0]
        config["_deviceId_"] = "{}_{}_{}".format(
            cls.__name__, bareHostName, os.getpid())

    loop = EventLoop()
    set_event_loop(loop)

    macro = cls(config)
    slot = getattr(cls, call)
    # We are run from a shell and don't have a device server!
    # Hence, we set this internal variable, which will declare this macro
    # as client in the instanceInfo if `_has_server` is `False`!
    macro._has_server = False
    assert isinstance(slot, Slot), "Only slots can be called"

    async def run():
        await macro.startInstance()
        future = loop.run_coroutine_or_thread(slot.method, macro)
        await loop.create_task(future, instance=macro)
        await macro.slotKillDevice()

    with closing(loop):
        loop.run_until_complete(run())


def Monitor():
    def outer(prop):
        prop.accessMode = AccessMode.READONLY
        prop.monitor = prop.setter
        del prop.setter
        return prop

    return outer


class MacroSlot(Slot):
    """A `MacroSlot` is a helper to simplify long macro tasks

    The method of this slot is always executed in a background task. It works
    with both synchronized and asyncio coroutines.

    This Slot by design has an `Active` and `Passive` state mechanism. It is
    allowed to be called in the `Passive` state and sets the Device's
    state to the `Active` state on execution. When the task is finished the
    device state is set to the `Passive` state.

    Both states are Attributes on the `MacroSlot` descriptor and they are
    by default `State.ACTIVE` and `State.PASSIVE`, respectively.
    """
    activeState = Attribute(dtype=State)
    passiveState = Attribute(dtype=State)

    def __init__(self, allowedStates=None, activeState=None,
                 passiveState=None, **kwargs):
        activeState = activeState or State.ACTIVE
        passiveState = passiveState or State.PASSIVE
        msg = "activeState and passiveState can not be the same"
        assert activeState is not passiveState, msg
        allowedStates = allowedStates or {passiveState}
        super().__init__(strict=True, activeState=activeState,
                         passiveState=passiveState,
                         allowedStates=allowedStates, **kwargs)

    def __call__(self, method):

        def background_slot(loop, configurable, device):

            async def inner():
                try:
                    await loop.run_coroutine_or_thread(method, configurable)
                except CancelledError:
                    coro = suppress_exception(loop.run_coroutine_or_thread(
                        device.onCancelled, self))
                    loop.create_task(coro, instance=device)
                except Exception as exc:
                    tb = exc.__traceback__
                    device._onException(self, exc, tb)
                finally:
                    device.currentSlot = ""
                    device.state = self.passiveState

            task = loop.create_task(inner(), instance=device)
            setattr(device, DEFAULT_ACTION_NAME, task)

        name = method.__name__
        if iscoroutinefunction(method):
            @wraps(method)
            async def wrapper(configurable):
                device = configurable.get_root()
                device.state = self.activeState
                device.currentSlot = name
                loop = get_event_loop()
                background_slot(loop, configurable, device)
        else:
            @wraps(method)
            def wrapper(configurable):
                device = configurable.get_root()
                device.state = self.activeState
                device.currentSlot = name
                loop = device._sigslot.loop
                background_slot(loop, configurable, device)

        return super().__call__(wrapper)


class RemoteDevice:
    """A RemoteDevice is the complement of a `DeviceNode`

    This `RemoteDevice` does not appear in the Schema, but creates
    a proxy that can be used under the key specified in the code.
    """

    def __init__(self, id, timeout=5):
        self.id = id
        self.timeout = timeout


class EventThread(threading.Thread):
    def __init__(self):
        super().__init__(name="Karabo macro event loop", daemon=True)

    def run(self):
        self.loop = EventLoop()
        set_event_loop(self.loop)
        atexit.register(self.stop)
        self.loop.call_soon(self.lock.release)
        try:
            self.loop.run_forever()
        finally:
            atexit.unregister(self.stop)
            self.loop.close()

    def start(self):
        self.lock = threading.Lock()
        self.lock.acquire()
        super().start()
        self.lock.acquire()

    def stop(self, weakref=None):
        self.loop.call_soon_threadsafe(self.loop.stop)
        # In Python 3.9 it is possible to shutdown the
        # thread pool executor gracefully here without
        # waiting.

    def start_device(self, device):

        async def run_device():
            await device.startInstance(broadcast=False)

        future = self.loop.run_coroutine_threadsafe(run_device())
        self.loop.run_until_complete(future)


async def suppress_exception(coro):
    """Supress any kind of exception in an executed coroutine on an eventloop
    """
    try:
        await coro
    except BaseException:
        pass


def _wrapslot(slot, name, abstract_passive, abstract_active):
    if slot.allowedStates is None:
        slot.allowedStates = {abstract_passive}
    themethod = slot.method

    # Note: No need to re-raise CancelledErrors, we got cancelled
    # on operator input. However, we must forward the cancellation
    # to the macro!
    if iscoroutinefunction(themethod):
        @wraps(themethod)
        async def wrapper(device):
            device._last_action = current_task()
            device.currentSlot = name
            device.state = abstract_active
            try:
                return (await themethod(device))
            except CancelledError:
                loop = get_event_loop()
                coro = suppress_exception(loop.run_coroutine_or_thread(
                    device.onCancelled, slot))
                loop.create_task(coro, instance=device)
            finally:
                device.currentSlot = ""
                device.state = abstract_passive
    else:
        @wraps(themethod)
        def wrapper(device):
            device._last_action = get_event_loop()
            device.currentSlot = name
            device.state = abstract_active
            try:
                return themethod(device)
            except CancelledError:
                loop = device._sigslot.loop
                coro = suppress_exception(loop.run_coroutine_or_thread(
                    device.onCancelled, slot))
                loop.create_task(coro, instance=device)
            finally:
                device.currentSlot = ""
                device.state = abstract_passive

    slot.method = wrapper


class Macro(Device):
    abstractPassiveState = State.PASSIVE
    abstractActiveState = State.ACTIVE

    klass = None
    _has_server = True
    _last_action = None

    project = String(
        displayedName="Project",
        description="The name of the project this macro belongs to",
        defaultValue="__none__",
        accessMode=AccessMode.INITONLY,
        requiredAccessLevel=AccessLevel.EXPERT,
    )

    module = String(
        displayedName="Module",
        description="The name of the module in the project",
        defaultValue="__none__",
        accessMode=AccessMode.INITONLY,
        requiredAccessLevel=AccessLevel.EXPERT,
    )

    currentSlot = String(
        displayedName="Current Slot",
        description="The name of the slot which is currently running",
        defaultValue="",
        accessMode=AccessMode.READONLY,
    )

    print = String(
        displayedName="Printed output",
        description="The output printed to the console",
        defaultValue="",
        accessMode=AccessMode.READONLY,
        requiredAccessLevel=AccessLevel.EXPERT,
    )

    doNotCompressEvents = Int32(
        displayedName="Number of prints",
        description="The number of prints issued so far",
        defaultValue=0,
        accessMode=AccessMode.READONLY,
        requiredAccessLevel=AccessLevel.EXPERT,
    )

    @Slot(displayedName="Cancel")
    async def cancel(self):
        if self._last_action is not None:
            self._last_action.cancel()
            async with countdown(exception=False):
                await self._last_action

    @classmethod
    def __init_subclass__(cls, **kwargs):
        for k, v in cls.__dict__.items():
            # patch slots for macro state machine behavior
            if isinstance(v, Slot) and not isinstance(v, MacroSlot):
                passive_state = cls.abstractPassiveState
                active_state = cls.abstractActiveState
                _wrapslot(v, k, passive_state, active_state)
        super().__init_subclass__(**kwargs)
        # Overwrite to provide the top-level class
        Macro.klass = cls
        cls.__monitors = [m for m in (getattr(cls, a) for a in cls._allattrs)
                          if hasattr(m, "monitor")]

    def __init__(self, configuration=None, **kwargs):
        """Create a new macro

        If this is done outside the event loop thread (i. e., from the
        command line), the macro is immediately started so that it responds to
        the network.

        If this is done in a server, the macro will be started like any other
        device."""
        if configuration is None:
            configuration = {}
        configuration.update(kwargs)
        super().__init__(configuration)
        self.code = ""
        self.stacked_print = deque(maxlen=100)
        self.stackTimer = None
        self.__local = False
        if not isinstance(get_event_loop(), EventLoop):
            self.__local = True
            EventLoop.global_loop.start_device(self)

    def _get_instance_info_status(self):
        """The instanceInfo status is always `ok` for a macro"""
        return "ok"

    def _initInfo(self):
        info = super()._initInfo()
        info["type"] = "macro" if self._has_server else "client"
        info["project"] = self.project
        info["module"] = self.module
        return info

    async def _run(self, **kwargs):
        """ implement the RemoteDevice functionality, upon
        starting the device the devices are searched and then
        assigned to the object's properties """
        if not self.__local:
            self.stackTimer = AsyncTimer(
                self._timer_callback, timeout=PRINT_THROTTLE)
            self.stackTimer.start()
        await super()._run(**kwargs)
        try:
            await self.__initialize_remotes()
        except CancelledError:
            # we are cancelled from the outside, nothing to do
            pass
        except Exception:
            await self.slotKillDevice()
            raise
        else:
            self.state = self.abstractPassiveState

    async def __watch_remote(self, d):
        """keep the connection to a remote device

        this method holds the connection to the RemoteDevice d, and calls
        all monitors upon changes in this device."""
        async with d:
            while True:
                await waitUntilNew(d)
                for m in self.__monitors:
                    try:
                        setattr(self, m.key, m.monitor(self))
                    except BaseException:
                        self.logger.exception(
                            'exception in monitor "{}" of device "{}"'.
                            format(m.key, self.deviceId))

    async def __initialize_remotes(self):
        """Initialize all RemoteDevice and their monitors"""
        watchers = []

        async def connect(key, remote):
            coro = getDevice(remote.id)
            if remote.timeout > 0:
                coro = wait_for(coro, remote.timeout)

            try:
                d = await coro
            except TimeoutError:
                self.logger.error('no remote device "{}" for macro "{}"'.
                                  format(remote.id, self.deviceId))
            else:
                setattr(self, key, d)
                watchers.append(self.__watch_remote(d))

        ts = type(self)
        attributes = ((k, getattr(ts, k)) for k in dir(ts))
        await gather(*(connect(k, v) for k, v in attributes
                       if isinstance(v, RemoteDevice)))
        for h in watchers:
            ensure_future(h)

    def printToConsole(self, data):
        """Put a data from the std out on the print stack"""
        if data == "\n":
            return
        sp = self.stacked_print
        sp.extend(data.splitlines())

    async def _timer_callback(self):
        """Provide a nice print output for the macro instance and update"""
        if self.stacked_print:
            self.doNotCompressEvents += 1
            self.print = "\n".join(self.stacked_print)
            self.stacked_print.clear()
            self.update()

    @classmethod
    def main(cls, argv=None):
        if argv is None:
            argv = sys.argv
        if len(argv) < 2:
            if cls.__doc__ is not None:
                print(cls.__doc__)
                return
            print(f"usage: {argv[0]} slot [property=value] ...\n")
            print("this calls a slot in macro {} with the given properties\n".
                  format(cls.__name__))
            print("available properties and slots:")
            print("-------------------------------")
            for k in cls._attrs:
                v = getattr(cls, k)
                if isinstance(v, Descriptor):
                    dn = v.displayedName if v.displayedName is not None else ""
                    t = type(v).__name__
                    print(f"{k:10}{t:10}{dn}")
            return
        args = {k: getattr(cls, k).fromstring(v)
                for k, v in (a.split("=", 1) for a in argv[2:])}
        call = argv[1]
        run_macro(cls, call, config=args)

    availableMacros = VectorString(
        displayedName="Available Macros",
        description="Provides the macro's own code",
        accessMode=AccessMode.READONLY,
        defaultValue=["macro"])

    def store_code(self, code):
        """Store the macro code apart from the descriptor mechanism"""
        self.code = code

    @slot
    def requestMacro(self, params):
        name = params.get("name", default="")
        payload = Hash(
            "success", name in self.availableMacros.value,
            "data", self.code)
        return Hash("type", "deviceMacro",
                    "origin", self.deviceId,
                    "payload", payload)


class TopologyMacro(Macro, DeviceClientBase):
    """A utils class for creating macros with topology access"""
