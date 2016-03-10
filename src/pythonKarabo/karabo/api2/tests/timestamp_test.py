from datetime import datetime
from unittest import TestCase, main
import time

from karabo.api2.timestamp import Timestamp


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

        t4 = Timestamp(t2)
        self.assertEqual(t3.toTimestamp(), 1240223542)

    def test_hash(self):
        self.assertIsNone(Timestamp.fromHashAttributes(dict()))
        t = Timestamp.fromHashAttributes(dict(sec=1234, frac=5678, tid=22))
        self.assertEqual(t.time, 1234000000000000005678)
        self.assertEqual(t.tid, 22)

    def test_compare(self):
        t1 = Timestamp()
        t2 = Timestamp()
        t3 = Timestamp(t2)

        self.assertLess(t1, t2)
        self.assertEqual(t2, t3)
        self.assertGreater(t2, t1)


if __name__ == "__main__":
    main()
