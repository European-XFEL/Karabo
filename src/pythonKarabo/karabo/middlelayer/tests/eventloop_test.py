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
from asyncio import CancelledError, Lock
from unittest import main

from karabo.middlelayer.eventloop import synchronize
from karabo.middlelayer.synchronization import sleep

from .eventloop import DeviceTest, sync_tst


class Barrier:
    def __init__(self):
        self.lock = Lock()
        self.state = "init"
        self.error = False

    @synchronize
    async def block(self):
        self.state = "unblocked"
        try:
            await self.lock.acquire()
            self.state = "blocked"
            await self.lock.acquire()
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
    async def coro(self, s, t):
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
        called = False

        def callback(future):
            nonlocal called
            sleep(0.001)  # test that we can properly call Karabo functions
            self.assertFalse(called)
            called = True

        barrier = Barrier()
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
        barrier = Barrier()
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
        barrier = Barrier()
        fut = barrier.block(wait=False)
        barrier.free()
        self.assertEqual(fut.wait(), "something")
        self.assertEqual(fut.result(), "something")

    @sync_tst
    def test_error(self):
        barrier = Barrier()
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
