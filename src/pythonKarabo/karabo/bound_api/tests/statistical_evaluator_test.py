# To change this template, choose Tools | Templates
# and open the template in the editor.

import unittest
import threading
import time
import socket

from karabo.bound import RollingWindowStatistics



class  RollingWindowStatistics_TestCase(unittest.TestCase):
    def setUp(self):
        pass

    def tearDown(self):
        pass

    def test_rolling_window_statistics_mean(self):
        try:
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

        except Exception as e:
            self.fail("test_rolling_window_statistics_mean exception: " + str(e))

    def test_rolling_window_statistics_variance(self):
        try:
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


        except Exception as e:
            self.fail("test_rolling_window_statistics_variance exception: " + str(e))


    def test_rolling_window_statistics_non_empty_interval(self):
        try:

            stat = RollingWindowStatistics()

        except Exception as e:
            self.assertEqual("did not match C++ signature" in str(e), True)



    def test_rolling_window_statistics_no_copy(self):
        try:
            import copy
            stat = RollingWindowStatistics(10)
            stat2 = copy.copy(stat)

        except Exception as e:
            self.assertEqual('Pickling of "karathon.RollingWindowStatistics" instances is not enabled' in str(e), True)







if __name__ == '__main__':
    unittest.main()

