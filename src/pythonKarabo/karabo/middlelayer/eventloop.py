# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.

import asyncio
import getpass
import os
import queue
import threading
import weakref
from asyncio import (
    AbstractEventLoop, CancelledError, Future, Queue, SelectorEventLoop,
    TimeoutError, ensure_future, get_event_loop, iscoroutinefunction,
    set_event_loop, sleep, wait_for)
from concurrent.futures import ThreadPoolExecutor
from functools import wraps

from karabo.middlelayer.broker import Broker, check_broker_scheme
from karabo.native import KaraboValue, unit_registry as unit

# Number of threads that can be scheduled in the thread pool executor.
_NUM_THREADS = 200
_SYNC_LOOP = False
_DEFAULT_HOSTS = "tcp://exfl-broker.desy.de:7777,tcp://localhost:7777"


def set_global_sync():
    """Set the global sync loop singleton. This function is only called
    from internal applications"""
    global _SYNC_LOOP
    _SYNC_LOOP = True


def global_sync():
    """Return an indicator if the main thread runs in sync"""
    return _SYNC_LOOP


def ensure_coroutine(coro):
    """Ensure that a function `coro` is a coroutine to play well
    in our eventloops"""
    if iscoroutinefunction(coro):
        return coro

    def create_coroutine(func):
        """Create a coroutine wrapper around the sync function `func`"""
        @wraps(func)
        async def wrapper(*args, **kwargs):
            return func(*args, **kwargs)
        return wrapper

    return create_coroutine(coro)


def synchronize(coro):
    """Decorate coroutine to play well in threads

    Most Karabo functions are coroutines that must run in the main thread.
    This decorator assures that the decorated coroutine will be called in the
    main thread, and returns the return value to the caller.

    If we are already in the main thread, the coroutine is simply returned.
    """
    coro = ensure_coroutine(coro)

    @wraps(coro)
    def wrapper(*args, timeout=-1, wait=True, **kwargs):
        return get_event_loop().sync(coro(*args, **kwargs), timeout, wait)

    if wrapper.__doc__ is not None:
        if not wrapper.__doc__[-1] == "\n":
            wrapper.__doc__ += "\n"
        wrapper.__doc__ += "\n    This is a synchronized coroutine.\n"
    return wrapper


def synchronize_notimeout(coro):
    """same as synchronize, but the timeout is handled by the coroutine"""

    coro = ensure_coroutine(coro)

    @wraps(coro)
    def wrapper(*args, wait=True, **kwargs):
        return get_event_loop().sync(coro(*args, **kwargs), -1, wait)

    if wrapper.__doc__ is not None:
        if not wrapper.__doc__[-1] == "\n":
            wrapper.__doc__ += "\n"
        wrapper.__doc__ += "\n    This is a synchronized coroutine.\n"
    return wrapper


class KaraboFuture:
    """A handle for a result that will be available in the future

    This will be returned by many Karabo methods, if a callback has been
    set to a function or None. It is possible to add more callbacks, to
    wait for completion, or cancel the operation in question.
    """

    def __init__(self, future):
        self.future = ensure_future(future)

    @synchronize
    def add_done_callback(self, fn):
        """Add another callback to the future"""
        loop = get_event_loop()
        instance = loop.instance()

        def func(future):
            loop.create_task(loop.run_coroutine_or_thread(
                fn, self), instance)

        self.future.add_done_callback(func)

    @synchronize
    async def wait(self):
        """Wait for the result to be available, and return the result"""
        return (await self.future)


for f in ["cancel", "cancelled", "done", "result", "exception"]:
    @wraps(getattr(Future, f))
    def method(self, *args, name=f):
        return getattr(self.future, name)(*args)

    setattr(KaraboFuture, f, synchronize(method))


class NoEventLoop(AbstractEventLoop):
    """A fake event loop for a thread

    There is only one Karabo event loop running ever. All other threads
    have this fake event loop running, which is merely a marker that there
    is no event loop. Setting a fake event loop prevents asyncio from
    automatically setting one.
    """
    Queue = queue.Queue
    sync_set = True

    def __init__(self, instance):
        self._instance = instance
        self._cancelled = False
        self.task = None

    def cancel(self):
        """Mark this event loop as cancelled

        This marks the thread of this event loop as cancelled. Subsequent calls
        to Karabo routines will fail with a :exc:`CancelledError`. Currently
        running Karabo routines are cancelled right away.
        """
        self._cancelled = True
        if self.task is not None:
            self._instance._ss.loop.call_soon_threadsafe(self.task.cancel)

    def __await__(self):
        """Awaiting the NoEventLoop cycles and suspend the current task"""
        return sleep(0).__await__()

    def sync(self, coro, timeout, wait):
        """The main synchronization routine

        This injects the coroutine *coro* into the event loop of the main
        thread, with a *timeout*. If *timeout* is a :class:`KaraboValue` its
        unit is used, otherwise it is in seconds. If *wait* is true, we wait
        for the coroutine to execute, otherwise we return a
        :class:`KaraboFuture`.
        """

        if isinstance(timeout, KaraboValue):
            timeout /= unit.second
        if self._cancelled:
            raise CancelledError
        loop = self._instance._ss.loop
        if wait:
            done = threading.Lock()
            done.acquire()
        hastask = threading.Lock()
        hastask.acquire()

        def inner():
            self.task = loop.create_task(coro, instance=self._instance)
            if wait:
                self.task.add_done_callback(lambda _: done.release())
            hastask.release()

        loop.call_soon_threadsafe(inner)
        if wait:
            done.acquire(timeout=timeout)
            hastask.acquire()
            if self.task.done():
                return self.task.result()
            else:
                loop.call_soon_threadsafe(self.task.cancel)
                raise TimeoutError
        else:
            hastask.acquire()
            future = wait_for(self.task,
                              timeout=timeout if timeout != -1 else None)
            return KaraboFuture(loop.create_task(future,
                                                 instance=self._instance))

    def create_future(self):
        return Future(loop=self._instance._ss.loop)

    def create_task(self, coro, name=None, instance=None):
        """Create a task on the main eventloop with this instance"""
        sigslot = self._instance._ss
        # The loop is closing or about to close. In this case no signal
        # slotable is available or the loop is not anymore running. This can
        # happen in CI (CLI) when the eventthread is stopped.
        if sigslot is None:
            return
        loop = sigslot.loop
        if not loop.is_running():
            return
        hastask = threading.Lock()
        hastask.acquire()

        task = None
        instance = instance or self._instance

        def inner():
            nonlocal task
            task = loop.create_task(coro, name=name, instance=instance)
            hastask.release()

        loop.call_soon_threadsafe(inner)
        hastask.acquire()
        return task

    def instance(self):
        return self._instance


class EventLoop(SelectorEventLoop):
    Queue = Queue
    sync_set = False
    global_loop = None

    def __init__(self, topic=None):
        super().__init__()
        self.connection = None
        if EventLoop.global_loop is not None:
            raise RuntimeError("There can be only one Karabo Eventloop")
        EventLoop.global_loop = self

        if topic is not None:
            self.topic = topic
        elif "KARABO_BROKER_TOPIC" in os.environ:
            self.topic = os.environ["KARABO_BROKER_TOPIC"]
        else:
            self.topic = getpass.getuser()
        if self.topic.endswith("_beats"):
            raise RuntimeError(f"Topic ('{self.topic}') must not end with "
                               f"'_beats'")

        self.changedFutures = set()  # call if some property changes
        self._default_executor = ThreadPoolExecutor(_NUM_THREADS)
        self.set_default_executor(self._default_executor)
        self.set_exception_handler(EventLoop.exceptionHandler)

        self.hosts = os.environ.get("KARABO_BROKER", _DEFAULT_HOSTS).split(',')
        check_broker_scheme(self.hosts)

    def exceptionHandler(self, context):
        try:
            instance = context["future"].instance()
            exception = context["exception"]
            trace = None
            if exception is not None:
                trace = exception.__traceback__
            instance._onException(None, exception, trace)
        except BaseException:
            self.default_exception_handler(context)

    def getBroker(self, deviceId, classId, broadcast):
        self.connection, Cls = Broker.create_connection(
            self.hosts, self.connection)
        return Cls(self, deviceId, classId, broadcast)

    def create_task(self, coro, instance=None, name=None):
        """Create a new task, running coroutine *coro*

        As an extension to the standard library method, in Karabo we track
        which device started a task, so that we can cancel them once the
        device goes away.

        *instance* is the device this task should belong to, it defaults
        to the caller's device if existent. Note that a device first
        has to be started with ``startInstance`` before this will work."""
        task = super().create_task(coro, name=name)
        try:
            if instance is None:
                # This try/except is required for MQTT broker support ...
                # ToDo: get rid of this requirement (MQTT)
                try:
                    instance = get_event_loop().instance()
                except RuntimeError:
                    # No event loop for current thread:
                    return task
            instance._ss.tasks.add(task)
            task.add_done_callback(instance._ss.tasks.remove)
            task.instance = weakref.ref(instance)
        except (AttributeError, TypeError):
            # create_task has been called from outside a Karabo context
            # this happens in tests and while bootstrapping.
            pass

        return task

    async def run_coroutine_or_thread(self, f, *args, **kwargs):
        """run the function *f* correctly, as a coroutine or thread

        if *f* is a normal function, it is started in a thread and returns
        a future, if it is a coroutine function, it returns the coroutine
        object."""
        if iscoroutinefunction(f):
            return (await f(*args, **kwargs))
        else:
            loop = NoEventLoop(self.instance())
            exception = None

            def thread():
                nonlocal exception
                set_event_loop(loop)
                try:
                    ret = f(*args, **kwargs)
                    # The lambda assures we are using the newest future
                    self.call_soon_threadsafe(lambda: future.set_result(ret))
                except BaseException as e:
                    # Since Python 3.8 we have to use BaseException here
                    exception = e
                    self.call_soon_threadsafe(
                        lambda: future.set_exception(exception))
                finally:
                    set_event_loop(None)

            self.run_in_executor(None, thread)
            while True:
                # Why `while` loop???
                # well, it is actually documented some lines down. If this
                # coroutine gets cancelled, we do not want to just give up,
                # but transfer the cancellation to the thread we control.
                # So we catch the exception, send it to the controlled thread
                # instead, and continue. This "and continue" is the while loop
                future = Future(loop=self)
                try:
                    return (await future)
                except CancelledError:
                    # Ignore cancelling from the outside, instead cancel the
                    # thread. Forward any resulting exception from the thread
                    # to the caller.
                    if exception is not None:
                        raise
                    else:
                        loop.cancel()

    def start_device(self, device):
        lock = threading.Lock()
        lock.acquire()
        task = None

        async def run_device():
            nonlocal task
            task = asyncio.current_task(loop=None)
            try:
                await device.startInstance()
            finally:
                lock.release()

        self.call_soon_threadsafe(self.create_task, run_device())
        lock.acquire()
        return task.result()

    def instance(self):
        try:
            return asyncio.current_task(loop=self).instance()
        except AttributeError:
            return None

    async def waitForChanges(self):
        f = Future(loop=self)
        self.changedFutures.add(f)
        try:
            await f
        finally:
            self.changedFutures.discard(f)

    def something_changed(self):
        for future in self.changedFutures:
            future.set_result(None)
        self.changedFutures = set()

    def sync(self, coro, timeout, wait):
        return coro

    async def cancel_all_tasks(self):
        """Cancel all running tasks except the current executing this"""
        me = asyncio.current_task(loop=self)
        tasks = [t for t in asyncio.all_tasks(loop=self) if t is not me]
        for t in tasks:
            t.cancel()

    def close(self):
        for t in asyncio.all_tasks(loop=self):
            t.cancel()
        if self.connection is not None:
            if iscoroutinefunction(self.connection.close):
                self.run_until_complete(self.connection.close())
            else:
                self.connection.close()
            self.connection = None
        super().close()
        EventLoop.global_loop = None
