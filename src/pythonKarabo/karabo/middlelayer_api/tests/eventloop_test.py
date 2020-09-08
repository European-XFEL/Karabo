from asyncio import CancelledError, coroutine, Lock
from unittest import main

from karabo.middlelayer_api.eventloop import synchronize
from karabo.middlelayer_api.synchronization import sleep

from .eventloop import DeviceTest, sync_tst


class Barrier(object):
    def __init__(self, loop):
        self.lock = Lock(loop=loop)
        self.state = "init"
        self.error = False

    @synchronize
    def block(self):
        self.state = "unblocked"
        try:
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
        finally:
            self.lock.release()

    @synchronize
    def free(self):
        self.lock.release()


class Tests(DeviceTest):
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

    @sync_tst
    def test_add_callback(self):
        def callback(future):
            nonlocal called
            sleep(0.001)  # test that we can properly call Karabo functions
            self.assertFalse(called)
            called = True
        called = False
        barrier = Barrier(self.loop)
        fut = barrier.block(wait=False)
        fut.add_done_callback(callback)
        self.assertFalse(called)
        barrier.free()
        fut.wait()
        sleep(0.05)
        self.assertTrue(called)

        called = False
        fut = barrier.block(wait=False, timeout=0.01)
        fut.add_done_callback(callback)
        self.assertFalse(called)
        sleep(0.05)
        self.assertTrue(called)

    @sync_tst
    def test_cancel(self):
        barrier = Barrier(self.loop)
        fut = barrier.block(wait=False)
        self.assertFalse(fut.cancelled())
        self.assertFalse(fut.done())
        fut.cancel()
        with self.assertRaises(CancelledError):
            fut.wait()
        self.assertEqual(barrier.state, "cancelled")
        self.assertTrue(fut.done())

    @sync_tst
    def test_result(self):
        barrier = Barrier(self.loop)
        fut = barrier.block(wait=False)
        barrier.free()
        self.assertEqual(fut.wait(), "something")
        self.assertEqual(fut.result(), "something")

    @sync_tst
    def test_error(self):
        barrier = Barrier(self.loop)
        barrier.error = True
        fut = barrier.block(wait=False)
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
