from asyncio import coroutine, gather, set_event_loop, wait_for
from functools import wraps

from karabo.middlelayer_api.eventloop import EventLoop


def async_tst(f):
    @wraps(f)
    def wrapper(self, *args, **kwargs):
        coro = coroutine(f)
        loop.run_until_complete(wait_for(loop.create_task(
            coro(self, *args, **kwargs), self.instance), 30))
    return wrapper


def sync_tst(f):
    @wraps(f)
    def wrapper(self, *args):
        loop.run_until_complete(wait_for(loop.create_task(
            loop.start_thread(f, self, *args), self.instance), 30))
    return wrapper


def setEventLoop():
    loop = EventLoop()
    set_event_loop(loop)
    return loop


def startDevices(*devices):
    global loop
    loop = setEventLoop()
    loop.run_until_complete(gather(*(d.startInstance() for d in devices)))
    return loop


def stopDevices(*devices):
    loop.run_until_complete(gather(*(d.slotKillDevice() for d in devices)))
    loop.close()
