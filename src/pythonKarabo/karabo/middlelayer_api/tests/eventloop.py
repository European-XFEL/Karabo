from asyncio import coroutine, gather, set_event_loop, wait_for
from functools import wraps
from unittest import TestCase

from karabo.middlelayer_api.eventloop import EventLoop


def async_tst(f):
    @wraps(f)
    def wrapper(self, *args, **kwargs):
        coro = coroutine(f)
        self.loop.run_until_complete(wait_for(self.loop.create_task(
            coro(self, *args, **kwargs), self.instance), 30))
    return wrapper


def sync_tst(f):
    @wraps(f)
    def wrapper(self, *args):
        self.loop.run_until_complete(wait_for(self.loop.create_task(
            self.loop.start_thread(f, self, *args), self.instance), 30))
    return wrapper


class EventLoopTest(TestCase):
    @classmethod
    def setUpClass(cls, *devices):
        cls.loop = EventLoop()
        set_event_loop(cls.loop)
        cls.loop.run_until_complete(
            gather(*(d.startInstance() for d in devices)))
        cls.devices = devices
        cls.instance = devices[0]

    @classmethod
    def tearDownClass(cls):
        cls.loop.run_until_complete(
            gather(*(d.slotKillDevice() for d in cls.devices)))
        del cls.devices
        del cls.instance
        cls.loop.close()


def setEventLoop():
    loop = EventLoop()
    set_event_loop(loop)
    return loop
