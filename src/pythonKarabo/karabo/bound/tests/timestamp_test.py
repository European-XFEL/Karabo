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
# To change this template, choose Tools | Templates
# and open the template in the editor.

import platform
import time
import unittest

from karabo.bound import (
    ATTOSEC, FEMTOSEC, MICROSEC, MILLISEC, NANOSEC, PICOSEC, Epochstamp, Hash,
    TimeDuration, TimeId, Timestamp, Types)


class Timestamp_TestCase(unittest.TestCase):
    # def setUp(self):
    #    self.foo = Timestamp_()
    #

    # def tearDown(self):
    #    self.foo.dispose()
    #    self.foo = None

    def test_timestamp_constructors(self):

        functionName = "test_timestamp_constructors"
        sleepDelay = 1

        try:
            ts01 = Timestamp()
        except Exception as e:
            self.fail(
                functionName + " creating ts01 default constructor: " + str(e))

        time.sleep(sleepDelay)

        try:
            ts02 = Timestamp()
        except Exception as e:
            self.fail(
                functionName + " creating ts02 default constructor: " + str(e))

        # The diff will be a tiny bit larger than sleepDelay, and depending
        # on the fractional seconds of ts01, the full seconds may differ
        # by one more:
        self.assertTrue(
            ts02.getSeconds() - ts01.getSeconds() <= sleepDelay + 1)

        self.assertNotEqual(ts01.getFractionalSeconds(),
                            ts02.getFractionalSeconds(),
                            "1st and 2nd timestamp should have differents"
                            " fractional seconds")

    def test_timestamp_to_iso8601_string(self):

        functionName = "test_timestamp_to_iso8601_string"
        # ISO8601 compact version
        pTimeStr01 = "20121225T132536.789333123456789123"
        self.assertIsNotNone(pTimeStr01)
        localeNameUS = "en_US.UTF-8"

        try:
            es01 = Epochstamp(1356441936, 789333123456789123)  # (pTimeStr01)
        except Exception as e:
            self.fail(
                f"{functionName} creating Epochstamp01 using the String"
                f" constructor: {str(e)}")

        try:
            ts01 = Timestamp(es01, TimeId())

            #####
            # Function toFormattedStringLocale and toFormattedString only
            # differ in the used locale, since internally
            # both use the same function: toFormattedStringInternal
            #
            # toFormattedStringLocale - requires that the locale name
            # toFormattedString - uses the System locale
            #
            # In this test only the toFormattedStringLocale, since this way
            # it's possible to run successfully this test
            # in Systems with a different locale
            #####

            # This test can only be run in LINUX since C++ locale support
            # seems completely broken on MAC OSX
            # The solution to make locale works in OSX passes for CLANG use
            if platform.system() == 'Darwin':
                ptime_osx_simple_str = '2012-Dec-25 13:25:36.789333'

                # Validate that "UNIVERSAL" format is corrected generated
                pTimeConvertedStr01 = ts01.toFormattedStringLocale(
                    localeNameUS, "%Y%m%dT%H%M%S")
                self.assertEqual(pTimeConvertedStr01, ptime_osx_simple_str,
                                 "These strings must be equal")

                # Validate that default "user-friendly" format is
                # corrected generated
                pTimeConvertedStr02 = ts01.toFormattedStringLocale(
                    localeNameUS)
                self.assertEqual(pTimeConvertedStr02, ptime_osx_simple_str,
                                 "These strings must be equal")

                # Validate that INPUT "user-friendly" format is
                # corrected generated
                pTimeConvertedStr03 = ts01.toFormattedStringLocale(
                    localeNameUS, "%Y/%m/%d %H:%M:%S")
                self.assertEqual(pTimeConvertedStr03, ptime_osx_simple_str,
                                 "These strings must be equal")

            else:
                # Validate that "UNIVERSAL" format is corrected generated
                pTimeConvertedStr01 = ts01.toFormattedStringLocale(
                    localeNameUS, "%Y%m%dT%H%M%S")
                self.assertEqual(pTimeConvertedStr01,
                                 "20121225T132536.789333",
                                 "These strings must be equal")

                # Validate that default "user-friendly" format is
                # corrected generated
                pTimeConvertedStr02 = ts01.toFormattedStringLocale(
                    localeNameUS)
                self.assertEqual(pTimeConvertedStr02,
                                 "2012-Dec-25 13:25:36.789333",
                                 "These strings must be equal")

                # Validate that INPUT "user-friendly" format is
                # corrected generated
                pTimeConvertedStr03 = ts01.toFormattedStringLocale(
                    localeNameUS, "%Y/%m/%d %H:%M:%S")
                self.assertEqual(pTimeConvertedStr03,
                                 "2012/12/25 13:25:36.789333",
                                 "These strings must be equal")

        except Exception as e:
            self.fail(
                " creating Timestamp01 using the String constructor: "
                + str(e))

        # Validate time was correctly converted/stored with the
        # correct information
        self.assertEqual(ts01.getSeconds(), 1356441936,
                         "1st timestamp number of seconds must be 1356441936")
        self.assertEqual(ts01.getFractionalSeconds(), 789333123456789123,
                         "1st timestamp number of fractional seconds must be"
                         " 789333123456789123")

    def test_timeduration(self):

        try:
            durZero = TimeDuration()
            self.assertEqual(durZero.getSeconds(), 0)
            self.assertEqual(durZero.getFractions(), 0)
        except Exception as e:
            self.fail(
                " testing default constructed time duration to be zero: "
                + str(e))

        try:
            seconds = 3600  # one hour
            fractionsAtto = 4565460000000  # 456.546 micro seconds

            dur1 = TimeDuration(seconds, fractionsAtto)
            self.assertEqual(dur1.getSeconds(), 0)
            self.assertEqual(dur1.getTotalSeconds(), seconds)
            self.assertEqual(dur1.getMinutes(), 0)
            self.assertEqual(dur1.getTotalMinutes(), 60)
            self.assertEqual(dur1.getHours(), 1)
            self.assertEqual(dur1.getTotalHours(), 1)
            self.assertEqual(dur1.getFractions(ATTOSEC), fractionsAtto)
            self.assertEqual(dur1.getFractions(FEMTOSEC),
                             fractionsAtto // 1000)
            self.assertEqual(dur1.getFractions(PICOSEC),
                             fractionsAtto // 1000000)
            self.assertEqual(dur1.getFractions(NANOSEC),
                             fractionsAtto // 1000000000)
            self.assertEqual(dur1.getFractions(MICROSEC),
                             fractionsAtto // 1000000000000)
            self.assertEqual(dur1.getFractions(MILLISEC),
                             fractionsAtto // 1000000000000000)

        except Exception as e:
            self.fail(
                " testing construction from seconds and fractions: " + str(e))

        try:
            h = Hash()
            # Cannot use Hash("seconds", 987, "fractions", 123),
            # but have to set explicitely with type, otherwise Hash
            # uses int32...
            h.setAs("seconds", 987, Types.UINT64)
            h.setAs("fractions", 123, Types.UINT64)
            dur2 = TimeDuration(h)
            dur3 = TimeDuration(987, 123)
            self.assertEqual(dur3 - dur2, durZero)
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
            self.assertTrue(durC < durD)
            self.assertTrue(durC <= durD)
            self.assertTrue(durD > durC)
            self.assertTrue(durD >= durC)
            self.assertFalse(durD < durC)
            self.assertFalse(durD <= durC)
            self.assertFalse(durC > durD)
            self.assertFalse(durC >= durD)

            # Test larger/smaller comparisons with equal fractions
            durE = TimeDuration(3, 4567890000)
            durF = TimeDuration(4, 4567890000)
            self.assertTrue(durE != durF)
            self.assertTrue(durE < durF)
            self.assertTrue(durE <= durF)
            self.assertTrue(durF > durE)
            self.assertTrue(durF >= durE)
            self.assertFalse(durF < durE)
            self.assertFalse(durF <= durE)
            self.assertFalse(durE > durF)
            self.assertFalse(durE >= durF)

            # Test larger/smaller comparisons with seconds smaller,
            # fractions larger
            durG = TimeDuration(444, 4567890000)
            durH = TimeDuration(555, 1234560000)
            self.assertTrue(durG != durH)
            self.assertTrue(durG < durH)
            self.assertTrue(durG <= durH)
            self.assertTrue(durH > durG)
            self.assertTrue(durH >= durG)
            self.assertFalse(durH < durG)
            self.assertFalse(durH <= durG)
            self.assertFalse(durG > durH)
            self.assertFalse(durG >= durH)

            # Skip here testing operator+, operator- and operator/,
            # see C++ tests.

        except Exception as e:
            self.fail(" testing comparison operators: " + str(e))

    def test_time_attributes(self):

        try:
            # Test time attributes
            h = Hash()
            h["timestamp"] = True
            h.setAttribute("timestamp", "tid", 22)
            h.setAttribute("timestamp", "sec", 1234)
            h.setAttribute("timestamp", "frac", 5678)
            attrs = h.getAttributes("timestamp")
            self.assertEqual(
                Timestamp.hashAttributesContainTimeInformation(attrs), True)

            tm = Timestamp.fromHashAttributes(attrs)

            self.assertEqual(tm.getTid(), 22)
            self.assertEqual(tm.getSeconds(), 1234)
            self.assertEqual(tm.getFractionalSeconds(), 5678)
        except Exception as e:
            self.fail(" testing conversion from attributes: " + str(e))

        try:
            # Test Train ID upgrade to uint64 without explicit Types.UINT64
            h = Hash()
            h["timestamp"] = True
            h.setAttribute("timestamp", "tid", 2 ** 40)
            h.setAttribute("timestamp", "sec", 1234)
            h.setAttribute("timestamp", "frac", 5678)

            attrs = h.getAttributes("timestamp")
            self.assertEqual(
                Timestamp.hashAttributesContainTimeInformation(attrs), True)
            tm = Timestamp.fromHashAttributes(attrs)

            self.assertEqual(tm.getTid(), 2 ** 40)
            self.assertEqual(tm.getSeconds(), 1234)
            self.assertEqual(tm.getFractionalSeconds(), 5678)
        except Exception as e:
            self.fail(
                " testing conversion from attributes with large train id: "
                + str(e))

        try:
            # Test explicit time attributes set as uint64
            h64 = Hash()
            h64.setAs("tid", 2 ** 40, Types.UINT64)
            h64.setAs("sec", 2 ** 41, Types.UINT64)
            h64.setAs("frac", 2 ** 42, Types.UINT64)

            h["timestamp64"] = True
            h.setAttribute("timestamp64", "tid", h64["tid"])
            h.setAttribute("timestamp64", "sec", h64["sec"])
            h.setAttribute("timestamp64", "frac", h64["frac"])
            attrs = h.getAttributes("timestamp64")
            self.assertEqual(
                Timestamp.hashAttributesContainTimeInformation(attrs), True)
            tm = Timestamp.fromHashAttributes(attrs)

            self.assertEqual(tm.getTid(), 2 ** 40)
            self.assertEqual(tm.getSeconds(), 2 ** 41)
            self.assertEqual(tm.getFractionalSeconds(), 2 ** 42)
        except Exception as e:
            self.fail(" testing conversion from uint64 attributes: " + str(e))

        try:
            # Test time attributes with a mix of uint32 and small uint64 values
            h64 = Hash()
            h64.setAs("tid", 2 ** 31, Types.UINT32)
            h64.setAs("sec", 2, Types.UINT64)
            h64.setAs("frac", 3, Types.UINT64)

            h = Hash()
            h["timestamp64"] = True
            h.setAttribute("timestamp64", "tid", h64["tid"])
            h.setAttribute("timestamp64", "sec", h64["sec"])
            h.setAttribute("timestamp64", "frac", h64["frac"])
            attrs = h.getAttributes("timestamp64")
            self.assertEqual(
                Timestamp.hashAttributesContainTimeInformation(attrs), True)
            tm = Timestamp.fromHashAttributes(attrs)

            self.assertEqual(tm.getTid(), 2 ** 31)
            self.assertEqual(tm.getSeconds(), 2)
            self.assertEqual(tm.getFractionalSeconds(), 3)
        except Exception as e:
            self.fail(
                " testing conversion from large uint32 "
                "/ small uint64 attributes: " + str(e))

        try:
            # Test failures in case of negative or string literals
            # as time attributes
            h = Hash()
            h["timestamp"] = True
            h.setAttribute("timestamp", "tid", -1)
            h.setAttribute("timestamp", "sec", "123")
            h.setAttribute("timestamp", "frac", 456)
            attrs = h.getAttributes("timestamp")
            self.assertEqual(
                Timestamp.hashAttributesContainTimeInformation(attrs), True)

            with self.assertRaises(Exception):
                Timestamp.fromHashAttributes(attrs)

            h.setAttribute("timestamp", "sec", 123)

            with self.assertRaises(Exception):
                Timestamp.fromHashAttributes(attrs)

            h.setAttribute("timestamp", "tid", 1)
            tm = Timestamp.fromHashAttributes(h.getAttributes("timestamp"))
            self.assertEqual(tm.getTid(), 1)
            self.assertEqual(tm.getSeconds(), 123)
            self.assertEqual(tm.getFractionalSeconds(), 456)

        except Exception as e:
            self.fail(
                " testing conversion from negative or ill-formed "
                "time attributes: " + str(e))


if __name__ == '__main__':
    unittest.main()
