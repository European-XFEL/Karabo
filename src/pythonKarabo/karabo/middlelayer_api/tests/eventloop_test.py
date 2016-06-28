from asyncio import coroutine
from unittest import TestCase, main

from .eventloop import setEventLoop


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


if __name__ == "__main__":
    main()
