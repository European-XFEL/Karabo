from asyncio import async, get_event_loop, set_event_loop, TimeoutError
import threading

from karabo import KaraboError, String
from karabo.hash import Hash
from karabo.signalslot import Proxy
from karabo.python_device import Device


class MacroProxy(Proxy):
    def setValue(self, attr, value):
        ok, msg = self._device._sync(
            self._device.call(self.deviceId, "slotReconfigure",
            Hash(attr.key, value)))
        if not ok:
            raise KaraboError(msg)


class Macro(Device):
    subclasses = []

    project = String(
        displayedName="Project",
        description="The name of the project this macro belongs to")

    module = String(
        displayedName="Module",
        description="The name of the module in the project")

    @classmethod
    def register(cls, name, dict):
        Macro._subclasses = {}
        super().register(name, dict)
        Macro.subclasses.append(cls)

    def initInfo(self):
        super().initInfo()
        self.info["type"] = "macro"
        self.info["project"] = self.project
        self.info["module"] = self.module

    def run(self):
        super().run()
        self._loop = get_event_loop()

    def _sync(self, coro, timeout=-1):
        lock = threading.Lock()
        lock.acquire()
        future = async(coro, loop=self._loop)
        future.add_done_callback(lambda _: lock.release())
        lock.acquire(timeout=timeout)
        if future.done():
            return future.result()
        else:
            future.cancel()
            raise TimeoutError

    def executeSlot(self, slot):
        return self._loop.run_in_executor(None, slot, self)

    def getDevice(self, deviceId):
        return self._sync(SignalSlotable.getDevice(self, deviceId,
                                                   Base=MacroProxy))

    def set(self, device, *, timeout=-1, **kwargs):
        return self._sync(SignalSlotable.set(self, device, **kwargs),
                          timeout=timeout)

    def update(self, device, timeout=-1):
        return self._sync(device.__iter__(), timeout=timeout)

    def waitUntil(self, condition, timeout=-1):
        return self._sync(SignalSlotable.waitUntil(self, condition),
                          timeout=timeout)
