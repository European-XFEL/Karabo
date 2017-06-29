"""This module contains some synchronization routines for users"""

import asyncio
from asyncio import (async, coroutine, Future, get_event_loop, iscoroutine,
                     iscoroutinefunction)
from functools import wraps

from .basetypes import KaraboValue, unit_registry as unit
from .eventloop import (EventLoop, KaraboFuture, synchronize,
                        synchronize_notimeout)


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


@synchronize_notimeout
def sleep(delay, result=None):
    """do nothing for *delay* seconds

    if *delay* is a :cls:`~karabo.middlelayer.KaraboValue`, its unit is
    respected.

    This method should be preferred over :func:`time.sleep`, as it is
    interruptable."""
    if isinstance(delay, KaraboValue):
        delay /= unit.second
    return asyncio.sleep(delay, result)


@synchronize
def firstCompleted(*args, **kwargs):
    """wait for the first of futures to return

    Wait for the first future given as keyword argument to return. It returns
    two dicts: one with the names of the done futures and their results, and
    one mapping the name of the pending futures to them, as an example::

        work = background(do_some_work)
        party = background(partey)
        done, pending = firstCompleted(work=work, party=party)

    the result will then be something like ``{"work": 5}`` and
    ``{"party": Future()}``.

    You may also give futures as positional parameters, they will be returned
    by their number::

        done, pending = firstCompleted(work, party)

    will return something like ``{0: 5}`` and ``{1: Future}``.
    """
    kwargs.update(enumerate(args))
    futures = {k: f if isinstance(f, KaraboFuture) else asyncio.async(f)
               for k, f in kwargs.items()}
    names = {
        f.future if isinstance(f, KaraboFuture) else f: k
        for k, f in futures.items()}
    done, pending = yield from asyncio.wait(
        names, return_when=asyncio.FIRST_COMPLETED)
    return ({names[f]: f.result() for f in done},
            {names[f]: futures[names[f]] for f in pending})


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


class FutureDict(object):
    """A dict for values which are only available in the future

    If you want to wait for something that only becomes available in the
    future, use::

        power = yield from futuredict["power"]

    If later the value is available, just write:

        futuredict["power"] = "lightning"

    and everyone waiting on this value will continue.
    """
    def __init__(self):
        self.futures = {}

    @coroutine
    def __getitem__(self, item):
        future = Future()
        futures = self.futures.setdefault(item, set())
        futures.add(future)
        try:
            return (yield from future)
        except:
            futures.discard(future)
            raise

    def __setitem__(self, item, value):
        futures = self.futures.pop(item, ())
        for future in futures:
            if not future.cancelled():
                future.set_result(value)
