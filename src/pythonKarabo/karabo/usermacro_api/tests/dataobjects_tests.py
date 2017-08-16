from collections import deque
import unittest
from karabo.usermacros import AcquiredData


class TestAD(unittest.TestCase):
    def test_initialization(self):
        ad = AcquiredData()
        self.assertEqual(ad.__repr__(), "AcquiredData(None, size=10)")
        self.assertEqual(str(ad), "Unknown Experiment: []")

        ad = AcquiredData(107)
        self.assertEqual(ad.__repr__(), "AcquiredData(107, size=10)")
        self.assertEqual(str(ad), "Experiment 107: []")

    def test_ad_fifo_behaviour(self):
        ad = AcquiredData()
        for i in range(20):
            ad.append(i)
        self.assertEqual(ad._fifo, deque(
                         [10, 11, 12, 13, 14, 15, 16, 17, 18, 19],
                         maxlen=10))
        x = next(ad)
        self.assertEqual(x, 10)

        ad = AcquiredData(size=40)
        self.assertEqual(ad._fifo.maxlen, 40)

        ad.append('val')


if __name__ == "__main__":
    unittest.main()
