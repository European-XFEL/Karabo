from unittest import TestCase, main
from karabo.middlelayer import Timestamp, TimeMixin

PERIOD = 100000  # microseconds [us]


class TestTimeMixin(TestCase):
    def setUp(self):
        self.tmix = TimeMixin()
        self.tmix.set_reference(0, 0, 0, PERIOD)

    def test_elasped_tid_calculation(self):
        first = Timestamp("2009-04-20T10:32:22 UTC")
        second = Timestamp("2009-04-20T10:32:22 UTC")

        elapsed_tid = self.tmix.elapsed_tid(first, second)
        self.assertEqual(elapsed_tid, 0)

        # Going forward in time
        second = Timestamp("2009-04-20T10:32:22.11 UTC")
        elapsed_tid = self.tmix.elapsed_tid(first, second)
        self.assertEqual(elapsed_tid, 1)

        second = Timestamp("2009-04-20T10:32:22.541 UTC")
        elapsed_tid = self.tmix.elapsed_tid(first, second)
        self.assertEqual(elapsed_tid, 5)

        second = Timestamp("2009-04-20T10:32:22.551 UTC")
        elapsed_tid = self.tmix.elapsed_tid(first, second)
        self.assertEqual(elapsed_tid, 5)

        second = Timestamp("2009-04-20T10:32:22.561 UTC")
        elapsed_tid = self.tmix.elapsed_tid(first, second)
        self.assertEqual(elapsed_tid, 5)

        second = Timestamp("2009-04-20T10:32:23.1 UTC")
        elapsed_tid = self.tmix.elapsed_tid(first, second)
        self.assertEqual(elapsed_tid, 10)

        # Going backward in time
        second = Timestamp("2009-04-20T10:32:21.1 UTC")
        elapsed_tid = self.tmix.elapsed_tid(first, second)
        self.assertEqual(elapsed_tid, -10)

        # A different period length
        self.tmix.set_reference(0, 0, 0, 10000)
        second = Timestamp("2009-04-20T10:32:23 UTC")
        elapsed_tid = self.tmix.elapsed_tid(first, second)
        self.assertEqual(elapsed_tid, 100)


if __name__ == "__main__":
    main()