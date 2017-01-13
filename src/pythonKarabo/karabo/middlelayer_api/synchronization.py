"""This module contains some synchronization routines for users"""

import asyncio
from asyncio import (async, coroutine, get_event_loop, iscoroutine,
                     iscoroutinefunction)
from functools import wraps

from .basetypes import KaraboValue, unit_registry as unit
from .eventloop import EventLoop, KaraboFuture, synchronize


def background(task, *args, timeout=-1):
    """execute a *task* in the background

    run the *task* in the background. This returns a :class:`KaraboFuture`,
    which may be cancelled or waited for as needed.

    A *timeout* may be given after which the function in the background will
    be cancelled.

    Example::

        def do_something_in_background(some_value):
            # so something here

        task = background(do_something_in_background, "some value")
        task.cancel()
    """
    @synchronize
    def inner(task, *args):
        loop = get_event_loop()
        if iscoroutine(task):
            assert not args
            return (yield from task)
        else:
            return (yield from loop.run_coroutine_or_thread(task, *args))

    ret = inner(task, *args, wait=False, timeout=timeout)
    if iscoroutine(ret):
        return async(ret)
    else:
        return ret


@synchronize
def gather(*args, return_exceptions=False):
    """wait until all KaraboFutures given are done

    This function waits until all :class:`KaraboFuture`s passed as
    arguments are done. The function returns a list of the return value
    of all functions.

    If one of the futues raises an exception, gather also immediately raises
    that exception, unless *return_exceptions* indicates that all futures
    should be waited for, and the exception returned instead.
    """
    assert all(isinstance(f, KaraboFuture) for f in args), \
        "Arguments must be of type KaraboFuture"
    return (yield from asyncio.gather(*[f.future for f in args],
                                      return_exceptions=return_exceptions))


@synchronize
def sleep(delay, result=None):
    """do nothing for *delay* seconds

    if *delay* is a :cls:`~karabo.middlelayer.KaraboValue`, its unit is
    respected.

    This method should be preferred over :func:`time.sleep`, as it is
    interruptable."""
    if isinstance(delay, KaraboValue):
        delay /= unit.second
    return asyncio.sleep(delay, result)


def synchronous(func):
    """Decorate a function to declare it synchronous

    If you do not want to use ``yield from`` in your code, decorate it with
    this. The code will be run in a separate thread.

    However, if this code is called from asynchronous code, it has to be
    yielded from!
    """
    @wraps(func)
    def wrapper(*args):
        @coroutine
        def inner():
            return (yield from background(func, *args))
        loop = get_event_loop()
        if loop is EventLoop.global_loop:
            return inner()
        else:
            return func(*args)

    assert not iscoroutinefunction(func)
    return wrapper
