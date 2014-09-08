# To change this template, choose Tools | Templates
# and open the template in the editor.

import unittest
import time
from karabo.karathon import *


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


if __name__ == '__main__':
    unittest.main()

