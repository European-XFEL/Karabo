from asyncio import CancelledError, coroutine, TimeoutError
from pint import DimensionalityError
from unittest import main
import time

from .eventloop import async_tst, DeviceTest, sync_tst
from karabo.middlelayer import background, gather, sleep, synchronous, unit


class Tests(DeviceTest):
    def setUp(self):
        self.called = False

    @coroutine
    def coro(self, param):
        self.assertEqual(param, "whatever")
        self.called = True
        return "coro called"

    def func(self, param):
        self.assertEqual(param, "something")
        self.called = True
        return "func called"

    @coroutine
    def coro_sleep(self):
        try:
            yield from sleep(1000)
            self.fail()
        except CancelledError:
            self.called = True
            raise

    def func_sleep(self):
        try:
            sleep(1000)
            self.fail()
        except CancelledError:
            self.called = True
            raise

    @async_tst
    def test_coro_coro(self):
        r = yield from background(self.coro, "whatever")
        self.assertEqual(r, "coro called")
        self.assertTrue(self.called)

    @async_tst
    def test_coro_coro_direct(self):
        r = yield from background(self.coro("whatever"))
        self.assertEqual(r, "coro called")
        self.assertTrue(self.called)

    @async_tst
    def test_coro_func(self):
        r = yield from background(self.func, "something")
        self.assertEqual(r, "func called")
        self.assertTrue(self.called)

    @sync_tst
    def test_func_coro(self):
        r = background(self.coro, "whatever").wait()
        self.assertEqual(r, "coro called")
        self.assertTrue(self.called)

    @sync_tst
    def test_func_func(self):
        r = background(self.func, "something").wait()
        self.assertEqual(r, "func called")
        self.assertTrue(self.called)

    @async_tst
    def test_coro_coro_cancel(self):
        task = background(self.coro_sleep)
        yield from sleep(0.01)
        self.assertFalse(self.called)
        task.cancel()
        with self.assertRaises(CancelledError):
            yield from task
        self.assertTrue(self.called)

    @sync_tst
    def test_func_coro_cancel(self):
        task = background(self.coro_sleep)
        sleep(0.01)
        self.assertFalse(self.called)
        task.cancel()
        with self.assertRaises(CancelledError):
            task.wait()
        self.assertTrue(self.called)

    @async_tst
    def test_coro_func_cancel(self):
        task = background(self.func_sleep)
        yield from sleep(0.01)
        self.assertFalse(self.called)
        task.cancel()
        with self.assertRaises(CancelledError):
            yield from task
        yield from sleep(0.01)
        self.assertTrue(self.called)

    @sync_tst
    def test_func_func_cancel(self):
        task = background(self.func_sleep)
        sleep(0.01)
        self.assertFalse(self.called)
        task.cancel()
        with self.assertRaises(CancelledError):
            task.wait()
        self.assertTrue(self.called)

    @sync_tst
    def test_gather(self):
        f1 = sleep(0.01, wait=False)
        f2 = background(self.func, "something")
        g = gather(f1, f2)
        self.assertEqual(g, [None, "func called"])
        self.assertTrue(self.called)

    @sync_tst
    def test_gather_raise(self):
        def raise_error():
            sleep(0.01)
            raise RuntimeError

        def fail_late():
            sleep(0.02)
            self.fail()
        f1 = background(raise_error)
        f2 = background(self.func, "something")
        f3 = background(fail_late)
        with self.assertRaises(RuntimeError):
            gather(f1, f2, f3)
        self.assertTrue(self.called)

    @sync_tst
    def test_gather_noraise(self):
        def raise_error():
            sleep(0.01)
            raise RuntimeError

        def succeed_late():
            sleep(0.02)
            self.called = 2
        f1 = background(raise_error)
        f2 = background(self.func, "something")
        f3 = background(succeed_late)
        g = gather(f1, f2, f3, return_exceptions=True)
        self.assertEqual(len(g), 3)
        self.assertIsInstance(g[0], RuntimeError)
        self.assertEqual(g[1], "func called")
        self.assertIsNone(g[2])
        self.assertEqual(self.called, 2)

    @sync_tst
    def test_sleep(self):
        t = time.time()
        sleep(6)
        self.assertGreater(time.time() - t, 6)

        t = time.time()
        sleep(1 * unit.ms)
        self.assertLess(time.time() - t, 0.8)

        with self.assertRaises(DimensionalityError):
            sleep(1 * unit.meter)
    test_sleep.slow = 1

    @sync_tst
    def test_timeout(self):
        t = time.time()
        with self.assertRaises(TimeoutError):
            sleep(10, timeout=0.01)
        self.assertLess(time.time() - t, 9)

        t = time.time()
        with self.assertRaises(TimeoutError):
            sleep(10, timeout=10 * unit.ms)
        self.assertLess(time.time() - t, 1)

        with self.assertRaises(DimensionalityError):
            sleep(10, timeout=10 * unit.meter)

    @async_tst
    def test_synchronous_async(self):
        @synchronous
        def f(arg):
            nonlocal done
            self.assertEqual(arg, 3)
            done = True
        done = False
        yf = f(3)
        yield from sleep(0.01)
        self.assertFalse(done)
        yield from yf
        self.assertTrue(done)

    @sync_tst
    def test_synchronous_sync(self):
        @synchronous
        def f(arg):
            nonlocal done
            self.assertEqual(arg, 7)
            done = True
        done = False
        f(7)
        self.assertTrue(done)


if __name__ == "__main__":
    main()
