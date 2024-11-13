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
from datetime import datetime
from unittest import TestCase, main

from ..hash import Hash
from ..timestamp import RESOLUTION, Timestamp


class Tests(TestCase):
    def test_init(self):
        t = Timestamp()
        self.assertLess(abs(t.toTimestamp() - time.time()), 0.001)

        t2 = Timestamp("2009-04-20T10:32:22 UTC")
        self.assertEqual(t2.toTimestamp(), 1240223542)

        t3 = Timestamp(t2.toLocal())
        self.assertEqual(t3.toTimestamp(), 1240223542)
        self.assertEqual(t3.toLocal(),
                         datetime.fromtimestamp(1240223542).isoformat())
        self.assertEqual(str(t3), t3.toLocal())
        self.assertEqual(repr(t3), "2009-04-20T10:32:22 UTC")
        self.assertEqual(Timestamp(repr(t3)), t3)
        self.assertEqual(Timestamp(str(t3)), t3)
        self.assertEqual(t3.toTimestamp(), 1240223542)

        t4 = Timestamp(t2)
        self.assertEqual(t4, t2)

        fval = 1553078935.3968687
        ival = int(fval * RESOLUTION)
        t5 = Timestamp(fval)
        self.assertAlmostEqual(t5.toTimestamp(), fval)
        t6 = Timestamp(ival)
        self.assertAlmostEqual(t6.toTimestamp(), fval)

    def test_toLocal(self):
        local_str = "2022-05-01T11:22:33.123456"
        t = Timestamp(local_str)
        # by default get back local string with 'T' separator
        self.assertEqual(t.toLocal(), local_str)
        # can define any other separator
        self.assertEqual(t.toLocal(' '), "2022-05-01 11:22:33.123456")
        self.assertEqual(t.toLocal(' ', timespec="hours"),
                         "2022-05-01 11")
        self.assertEqual(t.toLocal(' ', timespec="minutes"),
                         "2022-05-01 11:22")
        self.assertEqual(t.toLocal(' ', timespec="seconds"),
                         "2022-05-01 11:22:33")
        self.assertEqual(t.toLocal(' ', timespec="milliseconds"),
                         "2022-05-01 11:22:33.123")

    def test_hash_read(self):
        self.assertIsNone(Timestamp.fromHashAttributes(dict()))
        t = Timestamp.fromHashAttributes(dict(sec=1234, frac=5678, tid=22))
        self.assertEqual(t.time, 1234000000000000005678)
        self.assertEqual(t.tid, 22)

    def test_hash_write(self):
        t = Timestamp("2009-09-01 14:23 UTC")
        d = t.toDict()
        self.assertEqual(d["sec"], 1251814980)
        self.assertEqual(d["frac"], 135584022528)
        self.assertEqual(d["tid"], 0)

    def test_to_hash_attributes(self):
        t = Timestamp("2009-09-01 14:23 UTC")
        h = Hash("akey", "aval", "anotherkey", 5)
        t.toHashAttributes(h)
        self.assertEqual(h.getAttributes("akey"),
                         {"frac": 135584022528,
                          "sec": 1251814980,
                          "tid": 0})
        self.assertEqual(h.getAttributes("anotherkey"),
                         {"frac": 135584022528,
                          "sec": 1251814980,
                          "tid": 0})

    def test_compare(self):
        t1 = Timestamp()
        t2 = Timestamp()
        t3 = Timestamp(t2)

        self.assertLessEqual(t1, t2)
        self.assertEqual(t2, t3)
        self.assertGreaterEqual(t2, t1)

    def test_properties(self):
        t = Timestamp("2009-09-01 14:23 UTC")
        self.assertEqual(t.tid, 0)
        self.assertEqual(t.time_frac, 135584022528)
        self.assertEqual(t.time_sec, 1251814980)

    def test_add_sub(self):
        t1 = Timestamp("2009-04-20T10:32:22 UTC")
        self.assertEqual(t1.toTimestamp(), 1240223542)

        t2 = Timestamp("2009-04-20T10:32:23 UTC")
        self.assertEqual(t2.toTimestamp(), 1240223543)
        self.assertEqual(t2 - t1, 1)
        self.assertEqual(t2 - 1, t1.toTimestamp())

        self.assertEqual(t2 + t1, t1.toTimestamp() + t2.toTimestamp())
        self.assertEqual(t1 + 1, t2.toTimestamp())


if __name__ == "__main__":
    main()
