from collections import deque
import unittest
from karabo.middlelayer import Hash
from karabo.usermacros import AcquiredOffline, AcquiredOnline
from karabo.usermacro_api.dataobjects import AcquiredData


class TestAD(unittest.TestCase):
    def test_initialization(self):
        """Test AcquiredData object initialization"""
        ad = AcquiredData()
        self.assertEqual(ad.__repr__(), "AcquiredData(None, size=10)")
        self.assertEqual(str(ad), "Unknown Experiment: []")

        ad = AcquiredData(107)
        self.assertEqual(ad.__repr__(), "AcquiredData(107, size=10)")
        self.assertEqual(str(ad), "Experiment 107: []")

    def test_ad_fifo_behaviour(self):
        """Test the fifo behaviour"""
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

        ad.append('1')
        ad.append('2')
        self.assertEqual(ad[0], '1')
        self.assertEqual(ad[-1], '2')


class TestAcquiredOnline(unittest.TestCase):
    def test_initalization(self):
        """Test AcquiredOnline object initialization"""
        ao = AcquiredOnline()
        expRep = "AcquiredOnline(None, size=10, channel=None)"
        self.assertEqual(ao.__repr__(), expRep)

        ao = AcquiredOnline(10, 'source:channel')
        expRep = "AcquiredOnline(10, size=10, channel=source:channel)"
        self.assertEqual(ao.__repr__(), expRep)

        ao.append(10, 10)

        self.assertEqual(len(ao), 1)
        self.assertEqual(str(ao[0]), "<data{}: 10, meta{}: 10>")
        next(ao)
        self.assertEqual(len(ao), 0)

    def test_flattening(self):
        """Test AcquiredOnline Hash flatenning"""
        ao = AcquiredOnline()
        expOut = Hash([('x', -1), ('y', -2), ('a', 1), ('b', 2), ('c', 3)])
        inputHash = Hash([('x', -1),
                          ('y', -2),
                          ('z', Hash([('a', 1), ('b', 2), ('c', 3)]))])
        # Equality testing isn't implemented in k-hashes, so check the repr
        self.assertEqual(str(expOut), str(ao.flatten(inputHash)))


if __name__ == "__main__":
    unittest.main()
