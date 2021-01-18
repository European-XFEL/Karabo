from asyncio import (
    coroutine, gather, get_event_loop, iscoroutinefunction, set_event_loop,
    wait_for)
from contextlib import contextmanager, ExitStack
from functools import partial, wraps
from unittest import TestCase
from unittest.mock import Mock
import uuid

from karabo.middlelayer_api.eventloop import EventLoop


def async_tst(f=None, *, timeout=None):
    if f is None:
        assert timeout is not None, 'timeout must be given!'
        return partial(async_tst, timeout=timeout)
    if not iscoroutinefunction(f):
        f = coroutine(f)
    return sync_tst(f, timeout=timeout)


def sync_tst(f=None, *, timeout=None):
    if f is None:
        assert timeout is not None, 'timeout must be given!'
        return partial(sync_tst, timeout=timeout)

    # Default timeout is 30 seconds. Slower tests can give longer values
    timeout = 30 if timeout is None else timeout

    @wraps(f)
    def wrapper(self):
        task = self.loop.create_task(
            self.loop.run_coroutine_or_thread(f, self), self.lead)
        self.loop.run_until_complete(wait_for(task, timeout))
    return wrapper


class DeviceTest(TestCase):
    @classmethod
    def setUpClass(cls):
        with ExitStack() as cls.exit_stack:
            cls.loop = EventLoop()
            cls.old_event_loop = get_event_loop()
            set_event_loop(cls.loop)
            cls.exit_stack.enter_context(cls.lifetimeManager())
            cls.exit_stack = cls.exit_stack.pop_all()

    @classmethod
    def tearDownClass(cls):
        try:
            with cls.exit_stack:
                pass
        finally:
            # return the event loop at the conditions we found it.
            # multiple tests might fail if we do not do this.
            # also: closing the event loop in a `finally`
            # as insurance against failures in exit_stack
            cls.loop.stop()
            cls.loop.close()
            set_event_loop(cls.old_event_loop)

    @classmethod
    @contextmanager
    def lifetimeManager(cls):
        """This context manager is run around the test class"""
        cls.lead = Mock()
        cls.lead.deviceId = "test-mdl-{}".format(uuid.uuid4())
        cls.lead._ss = Mock()
        cls.lead._ss.loop = cls.loop
        cls.lead._ss.tasks = set()
        yield

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
