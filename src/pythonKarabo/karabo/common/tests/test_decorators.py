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
        # https://git.xfel.eu/Karabo/Framework/-/jobs/357337 showed a
        # diff of 0.2084, so relax delta a bit further
        self.assertAlmostEqual(diff, sleep_time, delta=0.25)


if __name__ == "__main__":
    main()
