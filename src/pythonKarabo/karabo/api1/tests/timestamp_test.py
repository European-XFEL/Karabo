# To change this template, choose Tools | Templates
# and open the template in the editor.
import sys

import unittest
import time
import platform

from karabo.api_1 import (Epochstamp, Timestamp, Trainstamp,
                          TimeDuration, Hash, Types,
                          ATTOSEC, FEMTOSEC, PICOSEC, NANOSEC,
                          MICROSEC, MILLISEC)

class  Timestamp_TestCase(unittest.TestCase):
    #def setUp(self):
    #    self.foo = Timestamp_()
    #

    #def tearDown(self):
    #    self.foo.dispose()
    #    self.foo = None

    def test_timestamp_constructors(self):
        
        functionName = "test_timestamp_constructors"
        sleepDelay = 1
        
        try:
            ts01 = Timestamp()
        except Exception as e:
            self.fail(functionName + " creating ts01 default constructor: " + str(e))
        
        time.sleep(sleepDelay)
        
        try:
            ts02 = Timestamp()
        except Exception as e:
            self.fail(functionName + " creating ts02 default constructor: " + str(e))
        
        self.assertEqual(ts01.getSeconds() + sleepDelay, ts02.getSeconds(), "1st timestamp should have minus 'sleepDelay' second(s) than 2nd timestamp");
        self.assertNotEqual(ts01.getFractionalSeconds(), ts02.getFractionalSeconds(), "1st and 2nd timestamp should have differents fractional seconds");
        
        # TODO: This test can be added after create the proper Constructor to Timestamp class
        #try:
        #    ts03 = Timestamp(ts02.getSeconds(), ts02.getFractionalSeconds())
        #except Exception as e:
        #    self.fail(functionName + " creating ts03 with seconds and fractionalSeconds constructor: " + str(e))
        
        #self.assertEqual(ts02.getSeconds(), ts03.getSeconds(), "2st and 3rd timestamp should have the same number of seconds");
        #self.assertEqual(ts02.getFractionalSeconds(), ts03.getFractionalSeconds(), "2st and 3rd timestamp should have the same number of fractional seconds");
        #
        #self.assertEqual(ts01.getSeconds() + sleepDelay, ts03.getSeconds(), "1st timestamp should have minus 'sleepDelay' second(s) than 3rd timestamp");
        #self.assertNotEqual(ts01.getFractionalSeconds(), ts03.getFractionalSeconds(), "1st and 3rd timestamp should have differents fractional seconds");
        
        
    def test_timestamp_to_iso8601_string(self):
        
        functionName = "test_timestamp_to_iso8601_string"
        # ISO8601 compact version
        pTimeStr01 = "20121225T132536.789333123456789123";
        localeNameUS = "en_US.UTF-8";
        
        try:
            es01 = Epochstamp(1356441936, 789333123456789123) #(pTimeStr01)
        except Exception as e:
            self.fail(functionName + " creating Epochstamp01 using the String constructor: " + str(e))
        
        try:
            ts01 = Timestamp(es01, Trainstamp())
            
            #####
            # Function toFormattedStringLocale and toFormattedString only differ in the used locale, since internally 
            # both use the same function: toFormattedStringInternal
            #
            # toFormattedStringLocale - requires that the locale name
            # toFormattedString - uses the System locale
            #
            # In this test only the toFormattedStringLocale, since this way it's possible to run successfully this test 
            # in Systems with a different locale
            #####
            
            # This test can only be run in LINUX since C++ locale support seems completely broken on MAC OSX
            # The solution to make locale works in OSX passes for CLANG use
            if platform.system() == 'Darwin':
                ptime_osx_simple_str = '2012-Dec-25 13:25:36.789333';
                
                # Validate that "UNIVERSAL" format is corrected generated
                pTimeConvertedStr01 = ts01.toFormattedStringLocale(localeNameUS, "%Y%m%dT%H%M%S.%f");
                self.assertEqual(pTimeConvertedStr01, ptime_osx_simple_str, "These strings must be equal");

                # Validate that default "user-friendly" format is corrected generated
                pTimeConvertedStr02 = ts01.toFormattedStringLocale(localeNameUS);
                self.assertEqual(pTimeConvertedStr02, ptime_osx_simple_str, "These strings must be equal");

                # Validate that INPUT "user-friendly" format is corrected generated
                pTimeConvertedStr03 = ts01.toFormattedStringLocale(localeNameUS, "%Y/%m/%d %H:%M:%S");
                self.assertEqual(pTimeConvertedStr03, ptime_osx_simple_str, "These strings must be equal");
                
            else:
                # Validate that "UNIVERSAL" format is corrected generated
                pTimeConvertedStr01 = ts01.toFormattedStringLocale(localeNameUS, "%Y%m%dT%H%M%S.%f");
                self.assertEqual(pTimeConvertedStr01, "20121225T132536.789333", "These strings must be equal");

                # Validate that default "user-friendly" format is corrected generated
                pTimeConvertedStr02 = ts01.toFormattedStringLocale(localeNameUS);
                self.assertEqual(pTimeConvertedStr02, "2012-Dec-25 13:25:36", "These strings must be equal");

                # Validate that INPUT "user-friendly" format is corrected generated
                pTimeConvertedStr03 = ts01.toFormattedStringLocale(localeNameUS, "%Y/%m/%d %H:%M:%S");
                self.assertEqual(pTimeConvertedStr03, "2012/12/25 13:25:36", "These strings must be equal");
            
        except Exception as e:
            self.fail(" creating Timestamp01 using the String constructor: " + str(e))
        
        # Validate time was correctly converted/stored with the correct information
        self.assertEqual(ts01.getSeconds(), 1356441936, "1st timestamp number of seconds must be 1356441936");
        self.assertEqual(ts01.getFractionalSeconds(), 789333123456789123, "1st timestamp number of fractional seconds must be 789333123456789123");

    def test_timeduration(self):

        try:
            durZero = TimeDuration()
            self.assertEqual(durZero.getSeconds(), 0)
            self.assertEqual(durZero.getFractions(), 0)
        except Exception as e:
            self.fail(" testing default constructed time duration to be zero: " + str(e))


        try:
            seconds = 3600 # one hour
            fractionsAtto = 4565460000000 # 456.546 micro seconds

            dur1 = TimeDuration(seconds, fractionsAtto)
            self.assertEqual(dur1.getSeconds(), 0)
            self.assertEqual(dur1.getTotalSeconds(), seconds)
            self.assertEqual(dur1.getMinutes(), 0)
            self.assertEqual(dur1.getTotalMinutes(), 60)
            self.assertEqual(dur1.getHours(), 1)
            self.assertEqual(dur1.getTotalHours(), 1)
            self.assertEqual(dur1.getFractions(ATTOSEC),  fractionsAtto);
            self.assertEqual(dur1.getFractions(FEMTOSEC), fractionsAtto // 1000)
            self.assertEqual(dur1.getFractions(PICOSEC),  fractionsAtto // 1000000)
            self.assertEqual(dur1.getFractions(NANOSEC),  fractionsAtto // 1000000000)
            self.assertEqual(dur1.getFractions(MICROSEC), fractionsAtto // 1000000000000)
            self.assertEqual(dur1.getFractions(MILLISEC), fractionsAtto // 1000000000000000)

        except Exception as e:
            self.fail(" testing construction from seconds and fractions: " + str(e))

        try:
            hash = Hash()
            # Cannot use Hash("seconds", seconds,"fractions", fractionsAtto),
            # but have to set explicitely with type, otherwise it is int32...
            hash.setAs("seconds", seconds, Types.UINT64)
            hash.setAs("fractions", fractionsAtto, Types.UINT64)
            dur2 = TimeDuration(hash)
            self.assertEqual(dur1 - dur2, durZero)
        except Exception as e:
            self.fail(" testing construction from hash: " + str(e))


        try:
            # Test equal comparisons
            durA = TimeDuration(123, 4567890000)
            durB = TimeDuration(123, 4567890000)
            self.assertTrue(durA == durB)
            self.assertTrue(durA <= durB)
            self.assertTrue(durA >= durB)
            self.assertFalse(durA != durB)

            # Test larger/smaller comparisons with equal seconds
            durC = TimeDuration(123, 4567890000)
            durD = TimeDuration(123, 4567890001)
            self.assertTrue(durC != durD)
            self.assertTrue(durC <  durD)
            self.assertTrue(durC <= durD)
            self.assertTrue(durD >  durC)
            self.assertTrue(durD >= durC)
            self.assertFalse(durD <  durC)
            self.assertFalse(durD <= durC)
            self.assertFalse(durC >  durD)
            self.assertFalse(durC >= durD)

            # Test larger/smaller comparisons with equal fractions
            durE = TimeDuration(3, 4567890000)
            durF = TimeDuration(4, 4567890000)
            self.assertTrue(durE != durF)
            self.assertTrue(durE <  durF)
            self.assertTrue(durE <= durF)
            self.assertTrue(durF >  durE)
            self.assertTrue(durF >= durE)
            self.assertFalse(durF <  durE)
            self.assertFalse(durF <= durE)
            self.assertFalse(durE >  durF)
            self.assertFalse(durE >= durF)

            # Test larger/smaller comparisons with seconds smaller, fractions larger
            durG = TimeDuration(444, 4567890000)
            durH = TimeDuration(555, 1234560000)
            self.assertTrue(durG != durH)
            self.assertTrue(durG <  durH)
            self.assertTrue(durG <= durH)
            self.assertTrue(durH >  durG)
            self.assertTrue(durH >= durG)
            self.assertFalse(durH <  durG)
            self.assertFalse(durH <= durG)
            self.assertFalse(durG >  durH)
            self.assertFalse(durG >= durH)

            # Skip here testing operator+, operator- and operator/, see C++ tests.

        except Exception as e:
            self.fail(" testing comparison operators: " + str(e))

if __name__ == '__main__':
    unittest.main()

