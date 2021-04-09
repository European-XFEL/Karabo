import unittest
import threading
import time

from karabo.bound import SignalSlotable, EventLoop


class Xms_TestCase(unittest.TestCase):
    bob_id = "bob"
    alice_id = "alice"

    def setUp(self):
        # start event loop
        self._eventLoopThread = threading.Thread(target=EventLoop.work)
        self._eventLoopThread.daemon = True
        self._eventLoopThread.start()

        # setup bob SignalSlotable with slots for 0-4 arguments
        # that also reply 0-4 values
        self.called = -1
        self.bob = SignalSlotable(self.bob_id)
        self.bob.start()

        def slot0():
            self.called = 0
            self.bob.reply()
        self.bob.registerSlot(slot0)

        def slot1(i):
            self.called = i
            self.bob.reply(i)
        self.bob.registerSlot(slot1)

        def slot2(i, j):
            self.called = i + j
            self.bob.reply(i, j)
        self.bob.registerSlot(slot2)

        def slot3(i, j, k):
            self.called = i + j + k
            self.bob.reply(i, j, k)
        self.bob.registerSlot(slot3)

        def slot4(i, j, k, l1):  # flake does not like l: 'ambiguous var. name'
            self.called = i + j + k + l1
            self.bob.reply(i, j, k, l1)
        self.bob.registerSlot(slot4)

        # setup other SignalSlotable to call the slots
        self.alice = SignalSlotable(self.alice_id)
        self.alice.start()

    def tearDown(self):
        self.alice = None
        self.bob = None

        # Stop the event loop
        EventLoop.stop()
        self._eventLoopThread.join()

    def test_xms_request(self):
        with self.subTest(msg="async receive"):
            self.assertEqual(-1, self.called)  # initial value

            # Helper to check when registered handlers are called
            handled = False

            def wait_for_handled():
                max_count = 1000
                while not handled and max_count > 0:
                    time.sleep(0.01)
                    max_count -= 1

            # Test no arguments
            def handle0():
                nonlocal handled
                handled = True
            req = self.alice.request(self.bob_id, "slot0")
            req.receiveAsync0(handle0)
            wait_for_handled()
            self.assertEqual(0, self.called)

            # Test one argument
            handled = False
            i = 0

            def handle1(ii):
                nonlocal handled, i
                handled = True
                i = ii
            req = self.alice.request(self.bob_id, "slot1", 5)
            req.receiveAsync1(handle1)
            wait_for_handled()
            self.assertEqual(5, self.called)
            self.assertEqual(5, i)

            # Test two arguments
            handled = False
            i = j = 0

            def handle2(ii, jj):
                nonlocal handled, i, j
                handled = True
                i, j = ii, jj
            req = self.alice.request(self.bob_id, "slot2", 5, 7)
            req.receiveAsync2(handle2)
            wait_for_handled()
            self.assertEqual(12, self.called)
            self.assertEqual(5, i)
            self.assertEqual(7, j)

            # Test three arguments
            handled = False
            i = j = k = 0

            def handle3(ii, jj, kk):
                nonlocal handled, i, j, k
                handled = True
                i, j, k = ii, jj, kk
            req = self.alice.request(self.bob_id, "slot3", 5, 7, 11)
            req.receiveAsync3(handle3)
            wait_for_handled()
            self.assertEqual(23, self.called)
            self.assertEqual(5, i)
            self.assertEqual(7, j)
            self.assertEqual(11, k)

            # Test four arguments
            handled = False
            i = j = k = l1 = 0  # flake does not like variable named 'l'

            def handle4(ii, jj, kk, ll):
                nonlocal handled, i, j, k, l1
                handled = True
                i, j, k, l1 = ii, jj, kk, ll
            req = self.alice.request(self.bob_id, "slot4", 5, 7, 11, 13)
            req.receiveAsync4(handle4)
            wait_for_handled()
            self.assertEqual(36, self.called)
            self.assertEqual(5, i)
            self.assertEqual(7, j)
            self.assertEqual(11, k)
            self.assertEqual(13, l1)

            self.called = -1  # reset for next test

        with self.subTest(msg="wait for reply"):
            self.assertEqual(-1, self.called)  # initial value
            to = 2000  # timeout in ms
            res = self.alice.request(self.bob_id, "slot0").waitForReply(to)
            self.assertEqual(0, len(res))
            self.assertEqual(0, self.called)

            res = self.alice.request(self.bob_id, "slot1", 5).waitForReply(to)
            self.assertEqual(1, len(res))
            self.assertEqual(5, res[0])
            self.assertEqual(5, self.called)

            req = self.alice.request(self.bob_id, "slot2", 5, 7)
            res = req.waitForReply(to)
            self.assertEqual(2, len(res))
            self.assertEqual(5, res[0])
            self.assertEqual(7, res[1])
            self.assertEqual(12, self.called)

            req = self.alice.request(self.bob_id, "slot3", 5, 7, 11)
            res = req.waitForReply(to)
            self.assertEqual(3, len(res))
            self.assertEqual(5, res[0])
            self.assertEqual(7, res[1])
            self.assertEqual(11, res[2])
            self.assertEqual(23, self.called)

            req = self.alice.request(self.bob_id, "slot4", 5, 7, 11, 13)
            res = req.waitForReply(to)
            self.assertEqual(4, len(res))
            self.assertEqual(5, res[0])
            self.assertEqual(7, res[1])
            self.assertEqual(11, res[2])
            self.assertEqual(13, res[3])
            self.assertEqual(36, self.called)

            del req
            self.called = -1
