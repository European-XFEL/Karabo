from asyncio import (async, coroutine, gather, get_event_loop, set_event_loop,
                     TimeoutError, wait_for)
import atexit
from functools import wraps
import sys
import threading
import weakref

from .device import Device
from .device_client import waitUntilNew, Proxy, getDevice
from .enums import AccessLevel, AccessMode
from .eventloop import EventLoop
from .hash import Descriptor, Hash, Int32 as Int, Slot, String, Type


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


class Sentinel:
    """ everyone needing the event thread to run should keep a reference
    to a sentinel. One nobody has a reference anymore, the event thread
    may be collected."""
    def __init__(self, thread):
        self.thread = thread

class EventThread(threading.Thread):
    instance = None

    def __init__(self):
        super().__init__(name="Karabo macro event loop", daemon=True)

    def run(self):
        self.loop = EventLoop()
        set_event_loop(self.loop)
        self.lock.release()
        atexit.register(self.stop)
        try:
            self.loop.run_forever()
        finally:
            atexit.unregister(self.stop)
            self.loop.close()

    @coroutine
    def run_macro(self, macro, configuration):
        yield from macro.startInstance()
        self.lock.release()

    def stop(self, weakref=None):
        self.loop.call_soon_threadsafe(self.loop.stop)

    @classmethod
    def start_macro(cls, macro, conf):
        if cls.instance is None or cls.instance() is None:
            self = cls()
            s = Sentinel(self)
            cls.instance = weakref.ref(s, self.stop)
            self.lock = threading.Lock()
            self.lock.acquire()
            self.start()
            self.lock.acquire()
        else:
            self = cls.instance().thread
        self.loop.call_soon_threadsafe(async, self.run_macro(macro, conf))
        self.lock.acquire()
        return cls.instance()


def _wrapslot(slot, name):
    if slot.allowedStates is None:
        slot.allowedStates = ["Idle..."]
    themethod = slot.themethod

    @wraps(themethod)
    def wrapper(device):
        device._lastloop = get_event_loop()
        device.state = name
        try:
            return themethod(device)
        finally:
            device.state = "Idle..."
    slot.themethod = wrapper


class Macro(Device):
    abstract = True
    subclasses = []

    project = String(
        displayedName="Project",
        description="The name of the project this macro belongs to",
        defaultValue="__none__",
        accessMode=AccessMode.INITONLY,
        requiredAccessLevel=AccessLevel.EXPERT)

    module = String(
        displayedName="Module",
        description="The name of the module in the project",
        defaultValue="__none__",
        accessMode=AccessMode.INITONLY,
        requiredAccessLevel=AccessLevel.EXPERT)

    print = String(
        displayedName="Printed output",
        description="The output printed to the console",
        defaultValue="",
        accessMode=AccessMode.READONLY,
        requiredAccessLevel=AccessLevel.EXPERT)

    printno = Int(
        displayedName="Number of prints",
        description="The number of prints issued so far",
        defaultValue=0,
        accessMode=AccessMode.READONLY,
        requiredAccessLevel=AccessLevel.EXPERT)

    @Slot(displayedName="Cancel")
    def cancel(self):
        self._lastloop.cancel()


    @classmethod
    def register(cls, name, dict):
        Macro._subclasses = {}
        for k, v in dict.items():
            if isinstance(v, Slot):
                _wrapslot(v, k)
        super().register(name, dict)
        Macro.subclasses.append(cls)
        cls.__monitors = [m for m in (getattr(cls, a) for a in cls._allattrs)
                          if hasattr(m, "monitor")]

    def __init__(self, configuration=None, may_start_thread=True, **kwargs):
        if configuration is None:
            configuration = {}
        configuration.update(kwargs)
        super().__init__(configuration)
        if may_start_thread and not isinstance(get_event_loop(), EventLoop):
            self._thread = EventThread.start_macro(self, configuration)

    @coroutine
    def run_async(self):
        """ implement the RemoteDevice functionality, upon
        starting the device the devices are searched and then
        assigned to the object's properties """
        yield from super().run_async()

        info = Hash()
        info["type"] = "macro"
        info["project"] = self.project
        info["module"] = self.module
        self.updateInstanceInfo(info)

        self.state = "SearchRemotes..."
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
            async(h)
        self.state = "Idle..."

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
        self.printno += 1
        self.update()

    @classmethod
    def main(cls):
        if len(sys.argv) < 2:
            if cls.__doc__ is not None:
                print(cls.__doc__)
                return
            print("usage: {} slot [property=value] ...\n".
                  format(sys.argv[0]))
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
        args = {k: getattr(cls, k).fromstring(v) for k, v in
                (a.split("=", 1) for a in sys.argv[2:])}
        call = sys.argv[1]
        slot = getattr(cls, call)
        assert isinstance(slot, Slot), "only slots can be called"

        loop = EventLoop()
        set_event_loop(loop)
        o = cls(args)
        o.startInstance()
        try:
            loop.run_until_complete(loop.create_task(loop.start_thread(
                slot.method, o), o))
            loop.run_until_complete(o.slotKillDevice())
        finally:
            loop.close()
