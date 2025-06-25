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
from signal import SIGTERM
from threading import Lock, Thread
from time import sleep

from karabo.bound import Epochstamp, EventLoop


def test_eventloop_post():
    done = False

    def func():
        nonlocal done
        done = True

    before = Epochstamp()
    EventLoop.post(func)
    EventLoop.run()
    after = Epochstamp()

    assert done is True

    # All this should not take long
    duration = after - before
    durationSec = duration.getTotalSeconds()
    durationSec += duration.getFractions() / 1.e9  # nanosec
    # Saw locally < 2.e-4, but relax here for very busy test systems
    assert 0.02 > durationSec

    # Now post with delay
    done = False
    before = Epochstamp()
    EventLoop.post(func, delay=0.2)  # seconds, as is usual in Python
    EventLoop.run()
    after = Epochstamp()

    assert done is True

    duration = after - before
    durationSec = duration.getTotalSeconds()
    durationSec += duration.getFractions() / 1.e9  # nanosec
    assert durationSec > .2

    lock = Lock()
    status = "timeout"

    def callback():
        nonlocal status
        status = "success"
        if lock.locked():
            lock.release()

    lock.acquire()
    assert status == "timeout"
    # post callback with 0.5 sec delay ...
    EventLoop.post(callback, delay=0.5)
    t = Thread(target=EventLoop.work)
    t.start()
    # block here until callback release
    lock.acquire(blocking=True, timeout=2)
    EventLoop.stop()
    lock.release()
    assert status == "success"

    t.join()


def test_eventloop_signalHandler():
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

    assert signal == SIGTERM
