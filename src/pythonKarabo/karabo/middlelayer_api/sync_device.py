from asyncio import async, coroutine, get_event_loop, TimeoutError
import threading

from .device import Device


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
