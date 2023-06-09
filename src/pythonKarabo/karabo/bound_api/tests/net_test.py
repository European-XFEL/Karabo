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
import os
import unittest
from signal import SIGTERM
from threading import Thread
from time import sleep

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

    def test_eventloop_signalHandler(self):

        signal = None

        def handler(sig):
            nonlocal signal
            signal = sig

        EventLoop.setSignalHandler(handler)

        # Start worker thread
        t = Thread(target=EventLoop.work)
        t.start()
        # Time for thread to launch - locally, 0.0001 seems fine (but zero not)
        sleep(.1)
        # Send SIGTERM to us: EventLoop will catch it, call handler and stop
        os.kill(os.getpid(), SIGTERM)
        t.join()

        self.assertEqual(signal, SIGTERM)
