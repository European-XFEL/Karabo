from asyncio import CancelledError, coroutine
from unittest import main

from .eventloop import async_tst, DeviceTest, sync_tst
from karabo.middlelayer import background, sleep


class Tests(DeviceTest):
    def setUp(self):
        self.called = False

    @coroutine
    def coro(self, param):
        self.assertEqual(param, "whatever")
        self.called = True

    def func(self, param):
        self.assertEqual(param, "something")
        self.called = True

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
        yield from background(self.coro, "whatever")
        self.assertTrue(self.called)

    @async_tst
    def test_coro_coro_direct(self):
        yield from background(self.coro("whatever"))
        self.assertTrue(self.called)

    @async_tst
    def test_coro_func(self):
        yield from background(self.func, "something")
        self.assertTrue(self.called)

    @sync_tst
    def test_func_coro(self):
        background(self.coro, "whatever").wait()
        self.assertTrue(self.called)

    @sync_tst
    def test_func_func(self):
        background(self.func, "something").wait()
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


if __name__ == "__main__":
    main()
