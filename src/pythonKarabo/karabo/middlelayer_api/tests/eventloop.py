from asyncio import coroutine, gather, set_event_loop, wait_for
from contextlib import contextmanager, ExitStack
from functools import wraps
from unittest import TestCase

from karabo.middlelayer_api.eventloop import EventLoop


def async_tst(f):
    coro = coroutine(f)

    @wraps(coro)
    def wrapper(self, *args, **kwargs):
        task = self.loop.create_task(
            coro(self, *args, **kwargs), self.instance)
        self.loop.run_until_complete(wait_for(task, 30))
    return wrapper


def sync_tst(f):
    @wraps(f)
    def wrapper(self, *args):
        task = self.loop.create_task(
            self.loop.start_thread(f, self, *args), self.instance)
        self.loop.run_until_complete(wait_for(task, 30))
    return wrapper


class DeviceTest(TestCase):
    @classmethod
    def setUpClass(cls):
        with ExitStack() as cls.exit_stack:
            cls.loop = EventLoop()
            set_event_loop(cls.loop)
            cls.exit_stack.enter_context(cls.lifetimeManager())
            cls.exit_stack = cls.exit_stack.pop_all()

    @classmethod
    def tearDownClass(cls):
        with cls.exit_stack:
            pass
        cls.loop.close()

    @classmethod
    @contextmanager
    def lifetimeManager(cls):
        """This context manager is run around the test class"""

    @classmethod
    @contextmanager
    def deviceManager(cls, device, *more):
        """Manage the devices to run during the tests

        `device` is the principal device in whose name the tests are run,
        while `more` are other devices that should run.
        """
        devices = (device,) + more
        cls.loop.run_until_complete(
            gather(*(d.startInstance() for d in devices)))
        cls.devices = devices
        cls.instance = devices[0]
        yield
        cls.loop.run_until_complete(
            gather(*(d.slotKillDevice() for d in cls.devices)))
        del cls.devices
        del cls.instance


def setEventLoop():
    loop = EventLoop()
    set_event_loop(loop)
    return loop
