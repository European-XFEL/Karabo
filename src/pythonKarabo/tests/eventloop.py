from asyncio import coroutine, gather, set_event_loop
from functools import wraps

from karabo.eventloop import EventLoop


def async_tst(f):
    @wraps(f)
    def wrapper(self, *args, **kwargs):
        coro = coroutine(f)
        loop.run_until_complete(coro(self, *args, **kwargs))
    return wrapper

def sync_tst(f):
    @wraps(f)
    def wrapper(self, *args):
        loop.run_until_complete(loop.run_in_executor(None, f, self, *args))
    return wrapper


def startDevices(*devices):
    global loop
    loop = EventLoop()
    set_event_loop(loop)
    if len(devices) > 1:
        loop.run_until_complete(gather(*(d.startInstance() for d in devices)))
    else:
        loop.run_until_complete(devices[0].startInstance())
    return loop


def stopDevices(*devices):
    if len(devices) > 1:
        loop.run_until_complete(gather(*(d.slotKillDevice() for d in devices)))
    else:
        loop.run_until_complete(devices[0].slotKillDevice())
    loop.close()
