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
import logging
import os
import re
from contextlib import asynccontextmanager, contextmanager
from functools import reduce, wraps
from time import perf_counter
from types import MethodType
from weakref import WeakMethod, ref

from karabo import __version__ as karabo_version
from karabo.native import Hash

_SCHEME_DELIMITER = "://"
_BROKER_REGEX = re.compile(
    r"^(amqp:\/\/)([A-Za-z0-9]+)(\:)+([A-Za-z0-9]+)"
    r"(\@)+([A-Za-z0-9\-\.]+)(\:)+([0-9]+)")


def ensure_coroutine(coro):
    """Ensure that a function `coro` is a coroutine to play well
    in our eventloops"""
    if asyncio.iscoroutinefunction(coro):
        return coro

    def create_coroutine(func):
        """Create a coroutine wrapper around the sync function `func`"""

        @wraps(func)
        async def wrapper(*args, **kwargs):
            return func(*args, **kwargs)

        return wrapper

    return create_coroutine(coro)


def get_karabo_version():
    """Return the karabo version
    """
    if "KARABO" not in os.environ:
        print("ERROR: $KARABO is not defined. Make sure you have sourced "
              "the 'activate' script.")
    return karabo_version


def get_property_hash(device, properties):
    """Get all properties from `device`

    :param device: The device instance or proxy object
    :param properties: The sequence of properties ('a', 'b') ...

    returns: Hash
    """
    ret = Hash()
    for key in properties:
        value = get_property(device, key)
        desc = value.descriptor
        ret.setElement(key, *desc.toDataAndAttrs(value))
    return ret


def get_property(device, path):
    """Return the property value from a proxy or device

    :param device: The device instance or proxy object
    :param path: The full path of the property as string

    This function is similar to python's builtin ``getattr`` and used with::

        prop = get_property(proxy, 'node.subnode.property')

    which is equivalent to::

        prop = proxy.node.subnode.property
    """
    return reduce(lambda obj, key: getattr(obj, key), path.split('.'), device)


def set_property(device, path, value):
    """Set a property value on a proxy or device

    This function has been added in Karabo >= 2.14.

    :param device: The device instance or proxy object
    :param path: The full path of the property as string
    :param value: The value to be set

    This function is similar to python's builtin ``setattr`` and used with::

        set_property(proxy, 'node.subnode.property', 5)

    which is equivalent to::

        proxy.node.subnode.property = 5
    """
    obj = device
    paths = path.split(".")
    for name in paths[:-1]:
        if not hasattr(obj, name):
            raise AttributeError(
                f"Property {path} is not available on device.")
        obj = getattr(obj, name)
    if not hasattr(obj, paths[-1]):
        raise AttributeError(f"Property {path} is not available on device.")
    setattr(obj, paths[-1], value)


class profiler:
    """A versatile profiling class

    Use this class either as a context manager or as decorator::

        with profiler():
            # classic context manager

        with profiler("Long computation 1"):
            # do something but provide a name for the context

        The class can also be used for decoration of functions (async works)

        @profiler()
        async def do_something()
            # do something

        async with profiler("Async with statement"):
            # do something with name in context

    """

    def __init__(self, name=None):
        self.name = name
        self.t_start = None

    def __enter__(self):
        self.t_start = perf_counter()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        elapsed = perf_counter() - self.t_start
        name = f"{self.name}:" if self.name is not None else ":"
        print(f"With block {name} time elapsed {elapsed}")

    async def __aenter__(self):
        return self.__enter__()

    async def __aexit__(self, exc_type, exc_val, exc_tb):
        return self.__exit__(exc_type, exc_val, exc_tb)

    def __call__(self, func):
        """Decorate a function to profile the execution time"""
        name = func.__name__ if self.name is None else self.name
        if asyncio.iscoroutinefunction(func):
            @wraps(func)
            async def wrapper(*args, **kwargs):
                t_start = perf_counter()
                ret = await func(*args, **kwargs)
                elapsed = perf_counter() - t_start
                print(f"{name} took {elapsed}")
                return ret
        else:
            @wraps(func)
            def wrapper(*args, **kwargs):
                t_start = perf_counter()
                ret = func(*args, **kwargs)
                elapsed = perf_counter() - t_start
                print(f"{name} took {elapsed}")
                return ret

        return wrapper


class AsyncTimer:
    """AsyncTimer class to periodically execute callbacks

    this timer will be deleted once the instance creating it is being deleted.
    :param callback: Callable function to execute. If callback is a bound
                     method, the timer automatically creates a weak
                     reference
    :param timeout: Interval in seconds after which the callback is put on
                    the eventloop
    :param single_shot: Boolean to set if the timer only does a single call
    :param flush_interval: The "ultimate" interval in seconds after which the
                           `callback` will be called even if the timer is
                           restarted (i.e. "snoozed").
                           Default is `None` and implies that a timer can
                           be snoozed indefinitely.

    The timer starts and always defers if started again (snooze behavior)
    """

    def __init__(self, callback, timeout=0, single_shot=False,
                 flush_interval=None, loop=None):
        if loop is None:
            loop = asyncio.get_event_loop()
        self.loop = loop
        instance = loop.instance()
        if instance is None:
            raise RuntimeError("A timer needs an instance to run ...")
        self.instance = ref(instance)
        self._register(instance)

        assert callable(callback)
        if type(callback) is MethodType:
            callback = WeakMethod(callback)

        self._callback = callback
        self._timeout = timeout
        self._single_shot = single_shot
        msg = f"Unexpected flush_interval: {flush_interval}"
        assert flush_interval is None or flush_interval > 0, msg
        self._flush_interval = flush_interval

        # Internally used variables
        self._handle = None
        self._time = None

    def _register(self, instance):
        """Internal method to register a timer on an `instance`"""
        instance._timers.add(self)
        weak_timer = ref(self)

        @contextmanager
        def destroyer():
            try:
                yield
            finally:
                weak = weak_timer()
                if weak is not None:
                    weak.destroy()

        instance._sigslot.enter_context(destroyer())

    def start(self):
        """Start the async timer

        If the timer was already started, the callback is postponed by a
        timeout interval.

        :returns bool: True if started, False otherwise
        """
        # We restarted ourselves, but now the loop is closed. We don't start
        # again.
        if self.loop.is_closed():
            return False
        if self._handle is None:
            self._time = self.loop.time()
            self._handle = self.loop.call_later(
                self._timeout,
                callback=self._channel_callback)
        else:
            if (self._flush_interval is not None and
                    self.loop.time() - self._time >=
                    self._flush_interval):
                # We don't allow postponing the callback anymore
                return
            self._handle.cancel()
            self._handle = self.loop.call_later(
                self._timeout,
                callback=self._channel_callback)
        return True

    def _channel_callback(self):
        """Private method to launch the callback as a task on the eventloop"""
        callback = (self._callback() if isinstance(self._callback, WeakMethod)
                    else self._callback)
        if callback is None:
            # destroyed or the object holding the WeakMethod is deleted
            self._handle = None
            self._time = None
            return

        coro = ensure_coroutine(callback)
        self.loop.call_soon_threadsafe(self.loop.create_task, coro(),
                                       self.instance())
        self._handle = None
        self._time = None
        if not self._single_shot:
            self.start()

    def is_running(self):
        """Check if the timer is running"""
        return self._handle is not None

    def stop(self):
        """Stop the execution of an asynchronous timer

        :returns bool: True if actively stopped, False otherwise
        """
        if self._handle is not None:
            self._time = None
            self._handle.cancel()
            self._handle = None
            return True
        return False

    def __del__(self):
        self.destroy()

    def destroy(self):
        """Stop the timer and delete any callback or loop reference"""
        self.stop()
        self._callback = None
        self.loop = None


@asynccontextmanager
async def countdown(delay=5, exception=True):
    """An async contextmanager to enter a timeout context

    :param delay: timeout limit (delay), default is 5 seconds
    :param exception: Boolean if an exception should be raised on timeout,
                      defaults to `True`
    """
    try:
        async with asyncio.timeout(delay) as ctx:
            yield ctx
    except TimeoutError:
        if exception:
            raise


class suppress:
    """A contextmanager to suppress exceptions in a context

    Note: On exception, a log message to the instance logger is generated
    """

    def __init__(self, *exceptions):
        self._exceptions = exceptions

    def __enter__(self):
        pass

    def __exit__(self, exc_type, exc_value, exc_tb):
        suppressed = exc_type is not None and issubclass(
            exc_type, self._exceptions)
        if suppressed:
            try:
                logger = asyncio.get_event_loop().instance().logger
            except BaseException:
                # Make absolutely sure that this the log message is done!
                logger = logging.getLogger()
            text = "Suppressed exception ..."
            logger.error(text, exc_info=(exc_type, exc_value, exc_tb))

        return suppressed


def check_broker_scheme(urls: list):
    """Check the list of broker hosts

    :param hosts: The list of broker hosts

    This function will raise a `RuntimeError` if the provided list
    contains invalid broker host data.
    """

    def _all_equal(iterable):
        """Check if all elements of an iterable are equal"""
        iterator = iter(iterable)
        try:
            first = next(iterator)
        except StopIteration:
            return True
        return all(first == x for x in iterator)

    def _all_match(iterable, regex):
        """Check if all iterables match the regex"""
        iterator = iter(iterable)
        return all(regex.match(x) for x in iterator)

    if not len(urls):
        raise RuntimeError(
            "The list of broker hosts does contain any url.")

    schemes = [url.split(_SCHEME_DELIMITER)[0] for url in urls]
    if not len(schemes) or not all([scheme in "amqp" for scheme in schemes]):
        raise RuntimeError(
            "The list of broker hosts does provide unknown schemes.")

    if not _all_equal(schemes):
        raise RuntimeError(
            "The list of broker hosts contains different schemes.")

    if not _all_match(urls, _BROKER_REGEX):
        raise RuntimeError(
            "Incomplete broker schemes for amqp provided. It must be "
            "assembled like `amqp://user:password@host:port`.")
