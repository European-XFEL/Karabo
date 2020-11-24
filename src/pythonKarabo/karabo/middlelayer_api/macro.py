from asyncio import (coroutine, ensure_future, gather, get_event_loop,
                     set_event_loop, TimeoutError, wait_for)
import atexit
from contextlib import closing
from functools import wraps
import os
import socket
import sys
import threading

from karabo.common.states import State
from karabo.native import AccessLevel, AccessMode, DaqPolicy
from karabo.native import Descriptor, Int32, Slot, String

from .eventloop import EventLoop
from .device import Device
from .device_client import waitUntilNew, getDevice


def Monitor():
    def outer(prop):
        prop.accessMode = AccessMode.READONLY
        prop.monitor = prop.setter
        del prop.setter
        return prop
    return outer


class RemoteDevice:
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

    def start_device(self, device):
        lock = threading.Lock()
        lock.acquire()

        @coroutine
        def run_device():
            yield from device.startInstance(broadcast=False)
            lock.release()
        self.loop.call_soon_threadsafe(ensure_future, run_device())
        lock.acquire()


def _wrapslot(slot, name):
    if slot.allowedStates is None:
        slot.allowedStates = {State.PASSIVE}
    themethod = slot.method

    @wraps(themethod)
    def wrapper(device):
        device._lastloop = get_event_loop()
        device.currentSlot = name
        device.state = State.ACTIVE
        try:
            return themethod(device)
        finally:
            device.currentSlot = ""
            device.state = State.PASSIVE
    slot.method = wrapper


class Macro(Device):
    abstract = True
    subclasses = []
    _lastloop = None

    project = String(
        displayedName="Project",
        description="The name of the project this macro belongs to",
        defaultValue="__none__",
        accessMode=AccessMode.INITONLY,
        requiredAccessLevel=AccessLevel.EXPERT,
        daqPolicy=DaqPolicy.OMIT)

    module = String(
        displayedName="Module",
        description="The name of the module in the project",
        defaultValue="__none__",
        accessMode=AccessMode.INITONLY,
        requiredAccessLevel=AccessLevel.EXPERT,
        daqPolicy=DaqPolicy.OMIT)

    currentSlot = String(
        displayedName="Current Slot",
        description="The name of the slot which is currently running",
        defaultValue="",
        accessMode=AccessMode.READONLY,
        daqPolicy=DaqPolicy.OMIT)

    print = String(
        displayedName="Printed output",
        description="The output printed to the console",
        defaultValue="",
        accessMode=AccessMode.READONLY,
        requiredAccessLevel=AccessLevel.EXPERT,
        daqPolicy=DaqPolicy.OMIT)

    doNotCompressEvents = Int32(
        displayedName="Number of prints",
        description="The number of prints issued so far",
        defaultValue=0,
        accessMode=AccessMode.READONLY,
        requiredAccessLevel=AccessLevel.EXPERT,
        daqPolicy=DaqPolicy.OMIT)

    @Slot(displayedName="Cancel")
    def cancel(self):
        if self._lastloop is not None:
            self._lastloop.cancel()

    @classmethod
    def register(cls, name, dict):
        # configurable subclasses
        Macro._subclasses = {}
        for k, v in dict.items():
            # patch slots for macro state machine behavior
            if isinstance(v, Slot):
                _wrapslot(v, k)
        super().register(name, dict)
        # every macro cls is appended to the subclasses for the macro server
        # to instantiate
        Macro.subclasses.append(cls)
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
        if not isinstance(get_event_loop(), EventLoop):
            EventLoop.global_loop.start_device(self)

    def _initInfo(self):
        info = super(Macro, self)._initInfo()
        info["type"] = "macro"
        info["project"] = self.project
        info["module"] = self.module
        return info

    @coroutine
    def _run(self, **kwargs):
        """ implement the RemoteDevice functionality, upon
        starting the device the devices are searched and then
        assigned to the object's properties """

        yield from super(Macro, self)._run(**kwargs)

        holders = []

        @coroutine
        def connect(key, remote):
            coro = getDevice(remote.id)
            if remote.timeout > 0:
                coro = wait_for(coro, remote.timeout)
            try:
                d = yield from coro
            except TimeoutError:
                self.logger.error('no remote device "{}" for macro "{}"'.
                                  format(remote.id, self.deviceId))
            else:
                setattr(self, key, d)
                holders.append(self.__holdDevice(d))

        ts = type(self)
        attributes = ((k, getattr(ts, k)) for k in dir(ts))
        yield from gather(*(connect(k, v) for k, v in attributes
                            if isinstance(v, RemoteDevice)))
        for h in holders:
            ensure_future(h)
        self.state = State.PASSIVE

    def __holdDevice(self, d):
        """keep the connection to a remote device

        this method holds the connection to the RemoteDevice d, and calls
        all monitors upon changes in this device."""
        with d:
            while True:
                yield from waitUntilNew(d)
                for m in self.__monitors:
                    try:
                        setattr(self, m.key, m.monitor(self))
                    except Exception:
                        self.logger.exception(
                            'exception in monitor "{}" of device "{}"'.
                            format(m.key, self.deviceId))

    def printToConsole(self, data):
        self.print = data
        # Make sure a new timestamp gets attached!
        self.doNotCompressEvents = self.doNotCompressEvents.value + 1
        self.update()

    @classmethod
    def main(cls, argv=None):
        if argv is None:
            argv = sys.argv
        if len(argv) < 2:
            if cls.__doc__ is not None:
                print(cls.__doc__)
                return
            print("usage: {} slot [property=value] ...\n".format(argv[0]))
            print("this calls a slot in macro {} with the given properties\n".
                  format(cls.__name__))
            print("available properties and slots:")
            print("-------------------------------")
            for k in cls._attrs:
                v = getattr(cls, k)
                if isinstance(v, Descriptor):
                    dn = v.displayedName if v.displayedName is not None else ""
                    t = type(v).__name__
                    print("{:10}{:10}{}".format(k, t, dn))
            return
        args = {k: getattr(cls, k).fromstring(v)
                for k, v in (a.split("=", 1) for a in argv[2:])}
        call = argv[1]
        slot = getattr(cls, call)
        assert isinstance(slot, Slot), "only slots can be called"

        loop = EventLoop()
        set_event_loop(loop)
        if "deviceId" in args:
            args["_deviceId_"] = args["deviceId"]
        else:
            bareHostName = socket.gethostname().partition('.')[0]
            args["_deviceId_"] = "{}_{}_{}".format(
                cls.__name__, bareHostName, os.getpid())

        macro = cls(args)

        @coroutine
        def run():
            # Starting a macro from command line should receive broadcasts!
            yield from macro.startInstance()
            future = loop.run_coroutine_or_thread(slot.method, macro)
            yield from loop.create_task(future, macro)
            yield from macro.slotKillDevice()

        with closing(loop):
            loop.run_until_complete(run())
