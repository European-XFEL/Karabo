from datetime import datetime
from unittest import TestCase, main
import time

from karabo.api2.timestamp import Timestamp
from karabo.api2.hash import Hash


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

        t4 = Timestamp(t2)
        self.assertEqual(t3.toTimestamp(), 1240223542)

    def test_hash_read(self):
        self.assertIsNone(Timestamp.fromHashAttributes(dict()))
        t = Timestamp.fromHashAttributes(dict(sec=1234, frac=5678, tid=22))
        self.assertEqual(t.time, 1234000000000000005678)
        self.assertEqual(t.tid, 22)

    def test_hash_write(self):
        h = Hash("a", 3)
        t = Timestamp("2009-09-01 14:23 UTC")
        t.toHashAttributes(h, "a")
        self.assertEqual(h["a", "sec"], 1251814980)
        self.assertEqual(h["a", "frac"], 135584022528)
        self.assertEqual(h["a", "tid"], 0)

    def test_compare(self):
        t1 = Timestamp()
        t2 = Timestamp()
        t3 = Timestamp(t2)

        self.assertLess(t1, t2)
        self.assertEqual(t2, t3)
        self.assertGreater(t2, t1)


if __name__ == "__main__":
    main()
