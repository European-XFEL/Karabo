from asyncio import (async, coroutine, Future, get_event_loop, set_event_loop,
                     Task, TimeoutError)
import atexit
import sys
import threading
import weakref

from karabo import KaraboError, String, Integer, AccessLevel, AccessMode
from karabo.eventloop import EventLoop
from karabo.hash import Hash
from karabo.hashtypes import Slot, Descriptor, Type
from karabo.signalslot import Proxy, SignalSlotable, waitUntilNew
from karabo.sync_device import SyncDevice, SyncProxy


def Monitor():
    def outer(prop):
        prop.accessMode = AccessMode.READONLY
        prop.monitor = prop.setter
        del prop.setter
        return prop
    return outer


class RemoteDevice:
    def __init__(self, id):
        self.id = id


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
        self.loop.stop()

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


class Macro(SyncDevice):
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

    printno = Integer(
        displayedName="Number of prints",
        description="The number of prints issued so far",
        defaultValue=0,
        accessMode=AccessMode.READONLY,
        requiredAccessLevel=AccessLevel.EXPERT)

    @classmethod
    def register(cls, name, dict):
        Macro._subclasses = {}
        super().register(name, dict)
        Macro.subclasses.append(cls)
        cls._monitors = [m for m in (getattr(cls, a) for a in cls._allattrs)
                         if hasattr(m, "monitor")]

    def __init__(self, configuration=None, may_start_thread=True, **kwargs):
        if configuration is None:
            configuration = {}
        configuration.update(kwargs)
        super().__init__(configuration)
        if may_start_thread and not isinstance(get_event_loop(), EventLoop):
            self._thread = EventThread.start_macro(self, configuration)

    def initInfo(self):
        super().initInfo()
        self.info["type"] = "macro"
        self.info["project"] = self.project
        self.info["module"] = self.module

    @coroutine
    def run_async(self):
        yield from super().run_async()
        devices = []
        for k in dir(type(self)):
            v = getattr(type(self), k)
            if isinstance(v, RemoteDevice):
                d = yield from SignalSlotable.getDevice(self, v.id,
                                                        Base=SyncProxy)
                setattr(self, k, d)
                devices.append(d)
        for d in devices:
            async(self._holdDevice(d))

    def _holdDevice(self, d):
        with d:
            while True:
                yield from waitUntilNew(d)
                for m in self._monitors:
                    setattr(self, m.key, m.monitor(self))

    def printToConsole(self, data):
        self.print = data
        self.printno += 1

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
            loop.run_until_complete(loop.run_in_executor(None, slot.method, o))
        finally:
            loop.close()
