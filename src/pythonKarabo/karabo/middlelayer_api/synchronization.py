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
"""This module contains some synchronization routines for users"""

import asyncio
import logging
from asyncio import (
    CancelledError, Future, ensure_future, get_event_loop, iscoroutine,
    iscoroutinefunction)
from functools import wraps

from karabo.native import KaraboValue, unit_registry as unit

from .eventloop import (
    EventLoop, KaraboFuture, synchronize, synchronize_notimeout)


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
    async def inner(task, *args):
        loop = get_event_loop()
        try:
            if iscoroutine(task):
                assert not args
                return (await task)
            else:
                return (await loop.run_coroutine_or_thread(task, *args))
        except CancelledError:
            raise
        except Exception:
            try:
                logger = logging.getLogger(loop.instance().deviceId)
            except BaseException:
                # Make absolutely sure that this the log message is done!
                logger = logging.getLogger()
            logger.exception("Error in background task ...")
            raise

    ret = inner(task, *args, wait=False, timeout=timeout)
    if iscoroutine(ret):
        task = ensure_future(ret)
        return task
    else:
        return ret


@synchronize
async def gather(*args, return_exceptions=False):
    """wait until all KaraboFutures given are done

    This function waits until all :class:`KaraboFuture`s passed as
    arguments are done. The function returns a list of the return value
    of all functions.

    If one of the futures raises an exception, gather also immediately raises
    that exception, unless *return_exceptions* indicates that all futures
    should be waited for, and the exception returned instead.
    """
    assert all(isinstance(f, KaraboFuture) for f in args), \
        "Arguments must be of type KaraboFuture, for `asyncio.Futures` "\
        " please use `asyncio.gather` instead"
    return (await asyncio.gather(*[f.future for f in args],
                                 return_exceptions=return_exceptions))


@synchronize
async def processEvents():
    """Process the stacked events on the event loop
    """
    await asyncio.sleep(0)


@synchronize_notimeout
async def sleep(delay, result=None):
    """do nothing for *delay* seconds

    if *delay* is a :class:`~karabo.middlelayer.KaraboValue`, its unit is
    respected.

    This method should be preferred over :func:`time.sleep`, as it is
    interruptable."""
    if isinstance(delay, KaraboValue):
        delay /= unit.second
    return (await asyncio.sleep(delay, result))


async def _wait(return_when, *args, timeout=None, cancel_pending=True,
                **kwargs):
    kwargs.update(enumerate(args))
    futures = {k: f if isinstance(f, KaraboFuture) else ensure_future(f)
               for k, f in kwargs.items()}
    names = {
        f.future if isinstance(f, KaraboFuture) else f: k
        for k, f in futures.items()}

    # Make firstCompleted and allCompleted compliant with KaraboValues
    if timeout is not None and isinstance(timeout, KaraboValue):
        timeout /= unit.second
    try:
        done, pending = await asyncio.wait(names, return_when=return_when,
                                           timeout=timeout)
    except CancelledError:
        for fut in names:
            if fut.done() and not fut.cancelled():
                # we need to retrieve the exception which is reported otherwise
                fut.exception()
            else:
                fut.cancel()
        raise

    if cancel_pending:
        for fut in pending:
            fut.cancel()

    return ({names[f]: f.result() for f in done
             if not f.cancelled() and f.exception() is None},
            {names[f]: futures[names[f]] for f in pending},
            {names[f]: None if f.cancelled() else f.exception()
             for f in done if f.cancelled() or f.exception() is not None})


@synchronize_notimeout
async def firstCompleted(*args, **kwargs):
    """wait for the first of coroutines or futures to return

    Wait for the first future given as keyword argument to return. It returns
    three dicts: one with the names of the done futures and their results,
    one mapping the name of the pending futures to them, and a third one
    mapping the names of failed futures to their exceptions, as an example::

        work = background(do_some_work)
        party = background(partey)
        sleep = background(zzz)
        done, pending, error = firstCompleted(
            work=work, party=party, sleep=sleep)

    the result will then be something like ``{"work": 5}``,
    ``{"party": Future()}`` and ``{"sleep": RuntimeError()}``.

    You may also give futures as positional parameters, they will be returned
    by their number::

        done, pending, error = firstCompleted(work, party, sleep)

    will return something like ``{0: 5}``, ``{1: Future()}`` and
    ``{2: RuntimeError()}``.

    Cancelled futures will be listed in the error dict, mapped to ``None``.
    You may also give a timemout.
    Futures still pending will be cancelled before return, unless you
    set `cancel_pending` to `False`.
    """
    return (await _wait(asyncio.FIRST_COMPLETED, *args, **kwargs))


@synchronize_notimeout
async def allCompleted(*args, **kwargs):
    """wait until all futures are done

    This function is an improved version of :func:`asyncio.gather`,
    which also works if a future fails, and a timeout can be set.

    For the returned dicts, see :func:`firstCompleted`.
    """
    return (await _wait(asyncio.ALL_COMPLETED, *args, **kwargs))


@synchronize_notimeout
async def firstException(*args, **kwargs):
    """wait until a future raises an exception

    If no future raises an exception, this is the same as
    :func:`allCompleted`.

    This function is an improved version of :func:`asyncio.gather`,
    which also works if a future fails, and a timeout can be set.

    For the returned dicts, see :func:`firstCompleted`.
    """
    return (await _wait(asyncio.FIRST_EXCEPTION, *args, **kwargs))


def synchronous(func):
    """Decorate a function to declare it synchronous

    If you do not want to use ``await`` in your code, decorate it with
    this. The code will be run in a separate thread.

    However, if this code is called from asynchronous code, it has to be
    yielded from!
    """
    @wraps(func)
    def wrapper(*args):
        async def inner():
            return (await background(func, *args))
        loop = get_event_loop()
        if loop is EventLoop.global_loop:
            return inner()
        else:
            return func(*args)

    assert not iscoroutinefunction(func)
    return wrapper


class FutureDict:
    """A dict for values which are only available in the future

    If you want to wait for something that only becomes available in the
    future, use::

        power = await futuredict["power"]

    If later the value is available, just write:

        futuredict["power"] = "lightning"

    and everyone waiting on this value will continue.
    """

    def __init__(self):
        self.futures = {}

    async def __getitem__(self, item):
        future = Future()
        futures = self.futures.setdefault(item, set())
        futures.add(future)
        try:
            return (await future)
        except BaseException:
            futures.discard(future)
            raise

    def __setitem__(self, item, value):
        futures = self.futures.pop(item, ())
        for future in futures:
            if not future.cancelled():
                future.set_result(value)
