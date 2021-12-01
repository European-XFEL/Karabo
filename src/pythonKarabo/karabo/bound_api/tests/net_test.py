import unittest

from karabo.bound import Epochstamp, EventLoop


class Net_TestCase(unittest.TestCase):
    # def setUp(self):
    #     pass

    # def tearDown(self):
    #     pass

    def test_eventloop_post(self):

        done = False

        def func():
            nonlocal done
            done = True

        before = Epochstamp()
        EventLoop.post(func)
        EventLoop.run()
        after = Epochstamp()

        self.assertTrue(done)

        # All this should not take long
        duration = after - before
        durationSec = duration.getTotalSeconds()
        durationSec += duration.getFractions() / 1.e9  # nanosec
        # Saw locally < 2.e-4, but relax here for very busy test systems
        self.assertGreater(0.02, durationSec)

        # Now post with delay
        done = False
        before = Epochstamp()
        EventLoop.post(func, delay=0.2)  # seconds, as is usual in Python
        EventLoop.run()
        after = Epochstamp()

        self.assertTrue(done)

        duration = after - before
        durationSec = duration.getTotalSeconds()
        durationSec += duration.getFractions() / 1.e9  # nanosec
        self.assertGreater(durationSec, .2)
