# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from platform import system
from time import sleep
from unittest import TestCase, main, skipIf

from karabo.native import TimeMixin, Timestamp, get_timestamp

PERIOD = 100000  # microseconds [us]


class TestTimeMixin(TestCase):
    def setUp(self):
        self.tmix = TimeMixin()
        self.tmix.set_reference(0, 0, 0, PERIOD)

    def test_elasped_tid_calculation(self):
        first = Timestamp("2009-04-20T10:32:22 UTC")
        second = Timestamp("2009-04-20T10:32:22 UTC")

        attrs = self.tmix.toDict()
        self.assertEqual(attrs["tid"], 0)
        self.assertEqual(attrs["sec"], 0)
        self.assertEqual(attrs["frac"], 0)

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

    # FIXME: trainID is out of 1 unit for Mac OSX
    @skipIf(system() == "Darwin",
            reason="Train Id mismatch in MacOSX")
    def test_actual_timestamp(self):
        reference = Timestamp()

        # set a new reference tick in the mixin!
        t_dict = reference.toDict()
        self.tmix.set_reference(1000, t_dict['sec'], t_dict['frac'], 100000)
        sleep(1)
        new_timestamp = get_timestamp()
        self.assertEqual(new_timestamp.tid, 1010)


if __name__ == "__main__":
    main()
