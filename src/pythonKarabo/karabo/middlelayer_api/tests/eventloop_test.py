from asyncio import CancelledError, coroutine, Lock
from functools import wraps
from unittest import TestCase, main
from unittest.mock import Mock

from karabo.middlelayer_api.eventloop import synchronize

from .eventloop import setEventLoop


def thread_tst(f):
    @wraps(f)
    def wrapper(self):
        device = Mock()
        device._ss = Mock()
        device._ss.loop = self.loop
        device._ss.tasks = set()
        task = self.loop.create_task(
            self.loop.run_coroutine_or_thread(f, self), instance=device)
        self.loop.run_until_complete(task)
    return wrapper


class Barrier(object):
    def __init__(self, loop):
        self.lock = Lock(loop=loop)
        self.state = "init"
        self.error = False

    @synchronize
    def block(self):
        try:
            self.state = "unblocked"
            yield from self.lock.acquire()
            self.state = "blocked"
            yield from self.lock.acquire()
            self.state = "end"
            if self.error:
                raise RuntimeError
            else:
                return "something"
        except CancelledError:
            self.state = "cancelled"
            raise

    @synchronize
    def free(self):
        self.lock.release()


class Tests(TestCase):
    def setUp(self):
        self.loop = setEventLoop()

    def tearDown(self):
        self.loop.close()

    @coroutine
    def coro(self, s, t):
        return "this was the coroutine" + s + t

    def test_coroutine(self):
        r = self.loop.run_until_complete(
            self.loop.run_coroutine_or_thread(self.coro, " test", t="."))
        self.assertEqual("this was the coroutine test.", r)

    def thread(self, s, t):
        return "this was the thread" + s + t

    def test_thread(self):
        r = self.loop.run_until_complete(
            self.loop.run_coroutine_or_thread(self.thread, " test", t="."))
        self.assertEqual("this was the thread test.", r)

    @thread_tst
    def test_callback(self):
        mock = Mock()
        barrier = Barrier(self.loop)
        fut = barrier.block(callback=mock)
        mock.assert_not_called()
        barrier.free()
        fut.wait()
        mock.assert_called_once_with(fut)

    @thread_tst
    def test_add_callback(self):
        mock = Mock()
        barrier = Barrier(self.loop)
        fut = barrier.block(callback=None)
        fut.add_done_callback(mock)
        mock.assert_not_called()
        barrier.free()
        fut.wait()
        mock.assert_called_once_with(fut)

    @thread_tst
    def test_cancel(self):
        barrier = Barrier(self.loop)
        fut = barrier.block(callback=None)
        self.assertFalse(fut.cancelled())
        self.assertFalse(fut.done())
        fut.cancel()
        with self.assertRaises(CancelledError):
            fut.wait()
        self.assertEqual(barrier.state, "cancelled")
        self.assertTrue(fut.done())

    @thread_tst
    def test_result(self):
        barrier = Barrier(self.loop)
        fut = barrier.block(callback=None)
        barrier.free()
        self.assertEqual(fut.wait(), "something")
        self.assertEqual(fut.result(), "something")

    @thread_tst
    def test_error(self):
        barrier = Barrier(self.loop)
        barrier.error = True
        fut = barrier.block(callback=None)
        barrier.free()
        with self.assertRaises(RuntimeError):
            fut.wait()
        with self.assertRaises(RuntimeError):
            fut.result()
        self.assertTrue(fut.done())
        self.assertFalse(fut.cancelled())
        self.assertTrue(isinstance(fut.exception(), RuntimeError))


if __name__ == "__main__":
    main()
