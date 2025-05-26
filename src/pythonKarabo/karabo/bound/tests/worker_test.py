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

import time

import pytest

from karabo.bound import Worker


def test_worker_config():
    worker = Worker(timeout=2000)
    worker.start()
    time.sleep(0.1)
    assert not worker.running

    # Threads can be only started once
    worker.set(callback=lambda x: x)
    with pytest.raises(RuntimeError):
        worker.start()

    # test order of arguments
    worker = Worker(None, 2000, 1)
    assert worker.timeout == 2000
    assert worker.single_shot


def test_worker_basic():
    worker = Worker(timeout=2000)
    worker.setTimeout(100)
    assert not worker.is_running()
    assert worker.timeout == 100

    counter = 0

    def increase():
        nonlocal counter
        counter += 1

    assert not worker.is_running()
    worker.set(callback=increase, timeout=50)
    assert worker.timeout == 50
    worker.start()
    assert worker.is_running()
    while counter < 5:
        time.sleep(0.01)
    assert counter == 5
    worker.pause()
    # We can pause
    time.sleep(0.5)
    assert counter == 5
    worker.start()
    while counter < 10:
        time.sleep(0.01)
    worker.stop()
    assert counter == 10
    assert not worker.is_running()

    # Can't set while running
    worker = Worker()
    worker.set(callback=increase, timeout=50)
    worker.start()
    with pytest.raises(RuntimeError):
        worker.set(callback=increase, timeout=50)
    worker.stop()


def test_worker_singleshot():
    counter = 0

    def increase():
        nonlocal counter
        counter += 1

    worker = Worker(increase, 100, single_shot=True)
    assert worker.timeout == 100
    worker.start()
    time.sleep(0.03)
    assert counter == 0
    time.sleep(0.1)
    assert counter == 1
    assert not worker.running
    time.sleep(0.15)
    assert counter == 1
