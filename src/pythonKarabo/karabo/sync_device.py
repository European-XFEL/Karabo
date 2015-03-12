from asyncio import async, coroutine, get_event_loop, TimeoutError
import threading
import weakref

from karabo.hash import Hash
from karabo.hashtypes import Slot, Type
from karabo.signalslot import Proxy, waitUntilNew
from karabo.python_device import Device


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
        future = async(coro, loop=self._ss.loop)
        future.add_done_callback(lambda _: lock.release())
        lock.acquire(timeout=timeout)
        if future.done():
            return future.result()
        else:
            future.cancel()
            raise TimeoutError

    def executeSlot(self, slot, message):
        @coroutine
        def inner():
            reply = yield from get_event_loop().start_thread(slot, self)
            self._ss.reply(message, reply)
        return async(inner())

    def executeNoWait(self, device, slot):
        if isinstance(device, Proxy):
            device = device._deviceId
        self._ss.emit("call", {device: [slot]})

    def getDevice(self, deviceId, *, timeout=-1):
        return self._sync(super().getDevice(deviceId, Base=SyncProxy), timeout)

    def updateDevice(self, device, *, timeout=-1):
        return self._sync(device.__iter__(), timeout=timeout)

    def waitUntilNew(self, proxy, *, timeout=-1):
        class WUN(waitUntilNew):
            def __getattr__(s, attr):
                assert isinstance(getattr(type(proxy), attr), Type)
                return self._sync(super().__getattr__(attr), timeout)
        return WUN(proxy)
