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
from unittest import TestCase, main

from karabo.common.decorators import timeit, validate_args


class Tests(TestCase):
    def test_validation(self):
        @validate_args
        def test_func(p1: str, p2: (float, int), p3: (float), p4: int):
            """Function to be validated!"""
            return True

        success = test_func("wuff", 0, 20.0, 5)
        self.assertTrue(success)

        # test p1 as int
        with self.assertRaises(ValueError):
            test_func(3, 0, 20.0, 5)

        # test p4 as float
        with self.assertRaises(ValueError):
            test_func("wuff", 0, 20.0, 5.0)

        # test p1 as bool
        with self.assertRaises(ValueError):
            test_func(True, 0, 20.0, 5)

        # booleans are also integers
        test_func("wuff", 0, 20.0, True)

        # test p1 as bool
        with self.assertRaises(ValueError):
            test_func(True, 0, 20.0, 5)

    def test_timeit(self):
        sleep_time = 0.1

        @timeit
        def slow_func():
            time.sleep(sleep_time)

        before = time.time()
        slow_func()
        after = time.time()
        diff = after - before
        # Some tests on CI showed diffs of 0.2084 or even 0.42559.
        # Indeed, we cannot guarantee any wall clock time diff...
        self.assertGreaterEqual(diff, sleep_time)
        self.assertLess(diff, 1.)  # not a real guarantee


if __name__ == "__main__":
    main()
