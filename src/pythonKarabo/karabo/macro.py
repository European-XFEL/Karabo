from asyncio import (async, coroutine, Future, get_event_loop, set_event_loop,
                     TimeoutError)
import sys
import threading

from karabo import KaraboError, String, Integer, AccessMode
from karabo.eventloop import EventLoop
from karabo.hash import Hash
from karabo.hashtypes import Slot, Descriptor, Type
from karabo.output import threadData
from karabo.signalslot import Proxy, SignalSlotable, waitUntilNew
from karabo.python_device import Device


def Monitor():
    def outer(prop):
        prop.accessMode = AccessMode.READONLY
        prop.monitor = prop.setter
        del prop.setter
        return prop
    return outer


class MacroProxy(Proxy):
    def setValue(self, attr, value):
        ok, msg = self._device._sync(
            self._device.call(self.deviceId, "slotReconfigure",
            Hash(attr.key, value)))
        if not ok:
            raise KaraboError(msg)


class RemoteDevice:
    def __init__(self, id):
        self.id = id


class Macro(Device):
    subclasses = []

    project = String(
        displayedName="Project",
        description="The name of the project this macro belongs to")

    module = String(
        displayedName="Module",
        description="The name of the module in the project")

    print = String(
        displayedName="Printed output",
        description="The output printed to the console",
        defaultValue="")

    printno = Integer(
        displayedName="Number of prints",
        description="The number of prints issued so far",
        defaultValue=0)

    @classmethod
    def register(cls, name, dict):
        Macro._subclasses = {}
        super().register(name, dict)
        Macro.subclasses.append(cls)
        cls._monitors = [m for m in (getattr(cls, a) for a in cls._allattrs)
                         if hasattr(m, "monitor")]

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
                d = yield from super().getDevice(v.id, Base=MacroProxy)
                setattr(self, k, d)
                devices.append(d)
        for d in devices:
            self.async(self._holdDevice(d))

    def _holdDevice(self, d):
        with d:
            while True:
                yield from waitUntilNew(d)
                for m in self._monitors:
                    setattr(self, m.key, m.monitor(self))

    def _sync(self, coro, timeout=-1):
        lock = threading.Lock()
        lock.acquire()
        future = self.async(coro)
        future.add_done_callback(lambda _: lock.release())
        lock.acquire(timeout=timeout)
        if future.done():
            return future.result()
        else:
            future.cancel()
            raise TimeoutError

    def _executeSlot(self, slot):
        threadData.instance = self
        try:
            slot(self)
        finally:
            threadData.instance = None

    def executeSlot(self, slot):
        return get_event_loop().run_in_executor(None, self._executeSlot, slot)

    def getDevice(self, deviceId):
        return self._sync(SignalSlotable.getDevice(self, deviceId,
                                                   Base=MacroProxy))

    def set(self, device, *, timeout=-1, **kwargs):
        return self._sync(SignalSlotable.set(self, device, **kwargs),
                          timeout=timeout)

    def updateDevice(self, device, timeout=-1):
        return self._sync(device.__iter__(), timeout=timeout)

    def waitUntil(self, condition, timeout=-1):
        return self._sync(SignalSlotable.waitUntil(self, condition),
                          timeout=timeout)

    def waitUntilNew(self, proxy, timeout=-1):
        class WUN(waitUntilNew):
            def __getattr__(s, attr):
                assert isinstance(getattr(type(proxy), attr), Type)
                return self._sync(super().__getattr__(attr), timeout)
        return WUN(proxy)


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
        async(o.run_async())
        try:
            loop.run_until_complete(loop.run_in_executor(None, slot.method, o))
        finally:
            loop.close()
