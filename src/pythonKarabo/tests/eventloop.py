from asyncio import coroutine, gather, set_event_loop
from functools import wraps
import os
import socket

from karabo.eventloop import EventLoop


def async_tst(f):
    @wraps(f)
    def wrapper(self, *args, **kwargs):
        coro = coroutine(f)
        loop.run_until_complete(loop.create_task(coro(self, *args, **kwargs),
                                                 self.instance))
    return wrapper


def sync_tst(f):
    @wraps(f)
    def wrapper(self, *args):
        loop.run_until_complete(loop.create_task(
            loop.start_thread(f, self, *args), self.instance))
    return wrapper


def setEventLoop():
    loop = EventLoop("{}-{}".format(socket.gethostname(), os.getpid()))
    set_event_loop(loop)
    return loop


def startDevices(*devices):
    global loop
    loop = setEventLoop()
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
