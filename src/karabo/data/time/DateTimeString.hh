/*
 * File:   DateTimeString.hh
 * Author: <luis.maia@xfel.eu>
 *
 * Created on March 19, 2014, 3:32 AM
 *
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef KARABO_DATA_TIME_DATETIMESTRING_HH
#define KARABO_DATA_TIME_DATETIMESTRING_HH

#include <chrono>
#include <regex>

#include "TimeDuration.hh"
#include "karabo/data/types/Exception.hh"
#include "karabo/data/types/Hash.hh"

namespace karabo {
    namespace data {

        /**
         * This class expresses a valid date and time information in the form of a char string.
         * To be a valid date and time char string, it must respect:
         * - ISO-8601:2000 Second Edition definition (docs available at http://www.qsl.net/g1smd/isopdf.htm)
         * - Subset (of ISO-8601) defined as Karabo date and time API
         *   * Compact version: yyyymmddThhmmss[.|,]ffffff[Z|z|±hhmm] (max. digits for ffffff is 18)
         *   * Extended version: yyyy-mm-ddThh:mm:ss[.|,]ffffff[Z|z|±hh:mm] (max. digits for ffffff is 18)
         *
         * The default constructor initializes a DateTimeString object with the Epoch values ("19700101T000000Z").
         *
         */
        class DateTimeString {
            // Considering the following example: "2013-01-20T20:30:00.123456Z"
            // each string should contain the following values:
            std::string m_date;              //"2013-01-20"
            std::string m_time;              //"20:30:00"
            std::string m_fractionalSeconds; //"123456"
            std::string m_timeZone;          //"Z" or "+0000" or "-07:00"

            // Extra field that concatenates date with time
            std::string m_dateTime;          //"2013-01-20T20:30:00"
            std::string m_dateTimeStringAll; //"2013-01-20T20:30:00.123456+00:00"
            std::string m_timeZoneSignal;
            int m_timeZoneHours;
            int m_timeZoneMinutes;

           public:
            /**
             * Constructor without parameters, that creates a instance with epoch Timestamp ("19700101T000000Z")
             */
            DateTimeString();

            /**
             * Constructor from string
             * @param timePoint String that represents a complete and valid date format using Karabo agreed ISO-8601
             * subset API
             */
            DateTimeString(const std::string& timePoint);

            /**
             * Constructor from string
             * @param inputDateStr String that represents the date part of the Karabo agreed ISO-8601 subset API
             * @param inputTimeStr String that represents the time part of the Karabo agreed ISO-8601 subset API
             * @param inputFractionSecondStr String that represents the fractional part of the Karabo agreed ISO-8601
             * subset API
             * @param inputTimeZoneStr String that represents the time zone part of the Karabo agreed ISO-8601 subset
             * API
             */
            DateTimeString(const std::string& inputDate, const std::string& inputTime,
                           const std::string& inputFractionSecond, const std::string& inputTimeZone);


            virtual ~DateTimeString();

            /**
             * Get string that represents the date part of the Karabo agreed ISO-8601 subset API (i.e. "2013-01-20")
             * @return string
             */
            inline const std::string& getDate() const {
                return m_date;
            }

            /**
             * Get string that represents the time part of the Karabo agreed ISO-8601 subset API (i.e. "20:30:00")
             * @return string
             */
            inline const std::string& getTime() const {
                return m_time;
            }

            /**
             * Get string or unsigned long long that represents the fractional part of the Karabo agreed ISO-8601 subset
             * API (i.e. "123456")
             * @return string
             */
            template <typename T>
            inline const T getFractionalSeconds() const {
                return boost::lexical_cast<T>(m_fractionalSeconds);
            }

            /**
             * Get string that represents the time zone part of the Karabo agreed ISO-8601 subset API (i.e. "Z")
             * @return string
             */
            inline const std::string& getTimeZone() const {
                return m_timeZone;
            }

            /**
             * Get string that represents the date and time part of the Karabo agreed ISO-8601 subset API (i.e.
             * "2013-01-20T20:30:00")
             * @return string
             */
            inline const std::string& getDateTime() const {
                return m_dateTime;
            }


            /**
             * Validates if a string representing a timestamp is valid according to ISO-8601 definition
             *
             * @param timePoint String that represents a Timestamp
             * @return boolean (True is string is valid, False otherwise)
             */
            static const bool isStringValidIso8601(const std::string& timePoint);


            /**
             * Validates if a string representing a time zone is valid according to ISO-8601 definition
             *
             * @param iso8601TimeZone String that represents a Time Zone (i.e. "+01:00" or "-07:00")
             * @return boolean (True is string is valid, False otherwise)
             */
            static const bool isStringValidIso8601TimeZone(const std::string& iso8601TimeZone);


            /**
             * Validates if a string representing a timestamp is valid according to Karabo agreed ISO-8601 subset API
             * definition Some examples:
             * => Extended strings:
             * - 1985-01-20T23:20:50
             * - 1985-01-20T23:20:50,123
             * - 1985-01-20T23:20:50.123
             * - 1985-01-20T23:20:50.123z
             * - 1985-01-20T23:20:50.123Z
             * - 1985-01-20T23:20:50z
             * - 1985-01-20T23:20:50Z
             * - 1985-01-20T23:20:50+00:00
             * - 1985-01-20T23:20:50-07:00
             * => Compact strings:
             * - 19850120T232050
             * - 19850120T232050,123
             * - 19850120T232050.123
             * - 19850120T232050.123z
             * - 19850120T232050.123Z
             * - 19850120T232050z
             * - 19850120T232050Z
             * - 19850120T232050+0000
             * - 19850120T232050-0700
             *
             * @param timePoint String that represents a Timestamp
             * @return boolean (True is string is valid, False otherwise)
             */
            static const bool isStringKaraboValidIso8601(const std::string& timePoint);


            /**
             * Returns the number of seconds elapsed since epoch for this object
             *
             * @return unsigned long long (Seconds elapsed since Epoch)
             */
            const unsigned long long getSecondsSinceEpoch();


            /**
             * Converts a fractional second value into it's value with a smaller precision
             * Because fractional seconds are received as an UNSIGNED LONG LONG, this function assumes the algarisms are
             * the right most algarisms. Due to this reason, if there are missing algarisms to perform the desired
             * precision resolution, zeros will be added to this left of the fractional seconds number received.
             *
             * @param precision - Indicates the precision of the fractional seconds (e.g. MILLISEC, MICROSEC, NANOSEC,
             * PICOSEC, FEMTOSEC, ATTOSEC) [Default: MICROSEC]
             * @param fractionalSeconds - Fractional seconds to be return with the correct desired precision
             * @param skipDot - if true, skip leading dot "." in result [Default: false]
             * @return String started with a "." (dot) (except skipDot is true) and followed by the fractional second
             * till the desired precision
             */
            static const std::string fractionalSecondToString(const TIME_UNITS precision = TIME_UNITS::MICROSEC,
                                                              const unsigned long long fractionalSeconds = 0,
                                                              bool skipDot = false);


            /**
             * Converts a STRING fractional second value into it's value in ATTOSEC precision
             * Because fractional seconds are received as a STRING, this function assumes the algarisms are the left
             * most algarisms. Due to this reason, if there are missing algarisms to perform ATTOSEC resolution, zeros
             * will be added to this right of the fractional seconds number received.
             *
             * @param fractionalSeconds - Fractional seconds to be return with ATTOSEC precision
             * @return String started with a "." (dot) and followed by the fractional second till the desired precision
             */
            static const std::string fractionalStringToAttoFractionalString(const std::string& fractionalSeconds);


            /**
             * Split an ISO-8601 valid Time Zone
             *
             * @param iso8601TimeZone String that represents a Time Zone (i.e. "Z" or "+01:00" or "-07:00") [Default:
             * "Z"]
             * @return Hash containing the Time Zone information in three different keys
             * (<std::string>("timeZoneSignal"), <int>("timeZoneHours"), <int>("timeZoneMinutes"))
             */
            static const karabo::data::Hash getTimeDurationFromTimeZone(const std::string& iso8601TimeZone = "Z");


           private:
            /**
             * Convert a specific clock time point to the number of seconds since epoch (1970-Jan-1 00:00:00)
             *
             * @param tp specific (std::chrono::system_clock) time point
             * @return number of seconds since epoch
             */
            static const unsigned long long ptimeToSecondsSinceEpoch(std::chrono::system_clock::time_point tp);

            /**
             * Creates an DateTimeString from an ISO-8601 formatted string (string must be a complete and valid
             * timestamp using Karabo agreed ISO-8601 subset API)
             *
             * @param timePoint ISO 8601 formatted string (see formats locale to more information)
             * @return DateTimeString object
             */
            static const DateTimeString iso8601KaraboApiStringToDateTimeString(const std::string& timePoint);
        };

        template <>
        const std::string DateTimeString::getFractionalSeconds() const;
        template <>
        const unsigned long long DateTimeString::getFractionalSeconds() const;

    } // namespace data
} // namespace karabo

#endif /* DATETIMESTRING_HH */
