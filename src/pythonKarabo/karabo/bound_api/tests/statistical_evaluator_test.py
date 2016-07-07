import unittest
import threading
import time
import socket


from karabo.bound import RollingWindowStatistics, Hash



class  RollingWindowStatistics_TestCase(unittest.TestCase):

    def test_rolling_window_statistics_mean(self):
        stat = RollingWindowStatistics(10)
        stat.update(-5)
        stat.update(-2)
        stat.update(4)
        self.assertAlmostEqual(stat.getRollingWindowMean(), -1)
        stat.update(2)
        stat.update(1)

        stat.update(-5)
        stat.update(-2)
        stat.update(4)
        stat.update(2)
        stat.update(1)
        self.assertAlmostEqual(stat.getRollingWindowMean(), 0)

        stat.update(-5)
        stat.update(-2)
        stat.update(4)
        stat.update(2)
        stat.update(1)
        self.assertAlmostEqual(stat.getRollingWindowMean(), 0)


    def test_rolling_window_statistics_variance(self):
        stat = RollingWindowStatistics(10)
        stat.update(-5)
        stat.update(-2)
        stat.update(4)
        self.assertAlmostEqual(stat.getRollingWindowVariance(), 21)
        stat.update(-2)
        stat.update(1)

        stat.update(-5)
        stat.update(-2)
        stat.update(4)
        stat.update(-2)
        stat.update(1)
        self.assertAlmostEqual(stat.getRollingWindowVariance(), 10.4)

        stat.update(-5)
        stat.update(-2)
        stat.update(4)
        stat.update(-2)
        stat.update(1)
        self.assertAlmostEqual(stat.getRollingWindowVariance(), 10.4)



    def test_rolling_window_statistics_non_empty_interval(self):
        #problem is we can't import the bp exception type, so we explicitly trigger it from
        #somewhere else and safe it
        bpexception = None
        try:
            #will result in a bp:argumentError
            Hash(1)
        except Exception as e:
            bpexception = type(e)

        self.assertRaises(bpexception, RollingWindowStatistics)



    def test_rolling_window_statistics_no_copy(self):
        import copy
        stat = RollingWindowStatistics(10)
        self.assertRaises(RuntimeError, copy.copy, stat)







if __name__ == '__main__':
    unittest.main()

