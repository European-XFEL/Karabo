from asyncio import TimeoutError
import threading
import weakref

from karabo.hash import Hash
from karabo.hashtypes import Type
from karabo.signalslot import Proxy, waitUntilNew
from karabo.python_device import Device
from karabo.output import threadData


class SyncProxy(Proxy):
    def setValue(self, attr, value):
        ok, msg = self._device._sync(
            self._device.call(self.deviceId, "slotReconfigure",
            Hash(attr.key, value)))
        if not ok:
            raise KaraboError(msg)


class SyncDevice(Device):
    """A device with a synchronous API"""
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
        threadData.instance = weakref.ref(self)
        try:
            slot(self)
        finally:
            threadData.instance = None

    def executeSlot(self, slot):
        return get_event_loop().run_in_executor(None, self._executeSlot, slot)

    def getDevice(self, deviceId, *, timeout=-1):
        return self._sync(super().getDevice(deviceId, Base=SyncProxy), timeout)

    def set(self, device, *, timeout=-1, **kwargs):
        return self._sync(super().set(device, **kwargs), timeout)

    def updateDevice(self, device, *, timeout=-1):
        return self._sync(device.__iter__(), timeout=timeout)

    def waitUntil(self, condition, *, timeout=-1):
        return self._sync(super().waitUntil(condition), timeout)

    def waitUntilNew(self, proxy, *, timeout=-1):
        class WUN(waitUntilNew):
            def __getattr__(s, attr):
                assert isinstance(getattr(type(proxy), attr), Type)
                return self._sync(super().__getattr__(attr), timeout)
        return WUN(proxy)
