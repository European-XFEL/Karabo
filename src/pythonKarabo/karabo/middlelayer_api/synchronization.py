"""This module contains some synchronization routines for users"""

import asyncio
from asyncio import async, get_event_loop, iscoroutine

from .eventloop import synchronize


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
    return (yield from asyncio.gather(*[f.future for f in args],
                                      return_exceptions=return_exceptions))
