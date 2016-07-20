from asyncio import coroutine, gather, set_event_loop, wait_for
from contextlib import contextmanager, ExitStack
from functools import wraps
from unittest import TestCase

from karabo.middlelayer_api.eventloop import EventLoop


def async_tst(f):
    return sync_tst(coroutine(f))


def sync_tst(f):
    @wraps(f)
    def wrapper(self):
        task = self.loop.create_task(
            self.loop.run_coroutine_or_thread(f, self), self.lead)
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
    def deviceManager(cls, *devices, lead):
        """Manage the devices to run during the tests

        `devices` are the devices that should be run during the tests,
        and `lead` is the device under which name the tests are running.
        """
        if lead not in devices:
            devices += (lead,)
        cls.loop.run_until_complete(
            gather(*(d.startInstance() for d in devices)))
        cls.lead = lead
        yield
        cls.loop.run_until_complete(
            gather(*(d.slotKillDevice() for d in devices)))
        del cls.lead


def setEventLoop():
    loop = EventLoop()
    set_event_loop(loop)
    return loop
