from asyncio import async, coroutine, get_event_loop
from contextlib import contextmanager
from functools import wraps
import sys

def parallel(f):
    f = coroutine(f)
    @wraps(f)
    @coroutine
    def wrapper(self, *args, **kwargs):
        try:
            yield from f(self, *args, **kwargs)
        except Exception:
            sys.excepthook(*sys.exc_info())
    return wrapper


def exclusive(f):
    f = coroutine(f)
    @wraps(f)
    def wrapper(self, *args, **kwargs):
        if self.exclusive is not None:
            return
        self.exclusive = async(f(self, *args, **kwargs))
        try:
            yield from self.exclusive
        finally:
            self.exclusive = None
    return parallel(wrapper)


class _NewValue:
    def __init__(self, other):
        self.other = other


    def __iter__(self):
        _, v, _ = yield from self.other
        return v


class NewValue:
    def __init__(self, device, attr=None):
        self.device = device


    @contextmanager
    def __getattr__(self, attr):
        loop = get_event_loop()
        with loop.getValueFuture(self.device, attr) as ret:
            yield _NewValue(ret)


class _NewValueTimestamp:
    def __init__(self, other):
        self.other = other


    def __iter__(self):
        _, v, ts = yield from self.other
        return v, ts


class NewValueTimestamp:
    def __init__(self, device, attr=None):
        self.device = device


    def __getattr__(self, attr):
        loop = get_event_loop()
        ret = loop.getValueFuture(self.device, attr)
        return _NewValueTimestamp(ret)


@coroutine
def waitForChanges():
    yield from get_event_loop().waitForChanges()


@coroutine
def waitUntil(condition):
    """this coroutine waits until the function condition returns true

    An example of usage:
    ::
        device = yield from self.getDevice("someDevice")
        yield from waitUntil(lambda: device.speed > 3)

    Note how one can easily use *lambda* functions to define the condition.
    """
    while not condition():
        yield from waitForChanges()
