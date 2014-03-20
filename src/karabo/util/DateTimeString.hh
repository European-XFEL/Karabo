/* 
 * File:   DateTimeString.hh
 * Author: <luis.maia@xfel.eu>
 *
 * Created on March 19, 2014, 3:32 AM
 * 
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_UTIL_DATETIMESTRING_HH
#define	KARABO_UTIL_DATETIMESTRING_HH

#include <boost/date_time.hpp>
#include <boost/regex.hpp>

#include "Exception.hh"

namespace karabo {
    namespace util {

        class DateTimeString {
            // Considering the following example: "2013-01-20T20:30:00.123456Z"
            // each string should contain the following values:
            std::string m_dateString; //"2013-01-20"
            std::string m_timeString; //"20:30:00"
            std::string m_fractionalSecondString; //"123456"
            std::string m_timeZoneString; //"Z"

            // Extra field that concatenates date with time
            std::string m_dateTimeString; //"2013-01-20T20:30:00"
            std::string m_dateTimeStringAll; //"2013-01-20T20:30:00.123456Z"

        public:

            /**
             * Constructor without parameters, that creates a instance with epoch Timestamp ("19700101T000000Z")
             */
            DateTimeString();

            /**
             * Constructor from string
             * @param timePoint String that represents a complete and valid date format using Karabo agreed ISO-8601 subset API
             */
            DateTimeString(const std::string& timePoint);

            /**
             * Constructor from string
             * @param inputDateStr String that represents the date part of the Karabo agreed ISO-8601 subset API
             * @param inputTimeStr String that represents the time part of the Karabo agreed ISO-8601 subset API
             * @param inputFractionSecondStr String that represents the fractional part of the Karabo agreed ISO-8601 subset API
             * @param inputTimeZoneStr String that represents the time zone part of the Karabo agreed ISO-8601 subset API
             */
            DateTimeString(const std::string& inputDateStr, const std::string& inputTimeStr,
                    const std::string& inputFractionSecondStr, const std::string& inputTimeZoneStr);


            virtual ~DateTimeString();

            /**
             * Get string that represents the date part of the Karabo agreed ISO-8601 subset API (i.e. "2013-01-20")
             * @return string
             */
            inline const std::string& getDateString() const {
                return m_dateString;
            }

            /**
             * Get string that represents the time part of the Karabo agreed ISO-8601 subset API (i.e. "20:30:00")
             * @return string
             */
            inline const std::string& getTimeString() const {
                return m_timeString;
            }

            /**
             * Get string that represents the fractional part of the Karabo agreed ISO-8601 subset API (i.e. "123456")
             * @return string
             */
            template<typename T>
            inline const T getFractionalSecondString() const {
                return boost::lexical_cast<T>(m_fractionalSecondString);
            }

            /**
             * Get string that represents the time zone part of the Karabo agreed ISO-8601 subset API (i.e. "Z")
             * @return string
             */
            inline const std::string& getTimeZoneString() const {
                return m_timeZoneString;
            }

            /**
             * Get string that represents the date and time part of the Karabo agreed ISO-8601 subset API (i.e. "2013-01-20T20:30:00")
             * @return string
             */
            inline const std::string& getDateTimeString() const {
                return m_dateTimeString;
            }


            /**
             * Validates if a string representing a timestamp is valid according to ISO-8601 definition
             * 
             * @param timePoint String that represents a Timestamp
             * @return boolean (True is string is valid, False otherwise)
             */
            static const bool isStringValidIso8601(const std::string& timePoint);


            /**
             * Validates if a string representing a timestamp is valid according to Karabo agreed ISO-8601 subset API definition
             * Some examples:
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
            //template<typename T>
            //const T getSecondsSinceEpoch();

        private:

            /**
             * Convert a specific boost ptime to the number of seconds since epoch (1970-Jan-1 00:00:00)
             * 
             * @param pt specific boost ptime
             * @return number of seconds since epoch
             */
            static const unsigned long long ptimeToSecondsSinceEpoch(boost::posix_time::ptime& pt);

            /**
             * Creates an DateTimeString from an ISO-8601 formatted string (string must be a complete and valid timestamp using Karabo agreed ISO-8601 subset API)
             * 
             * @param timePoint ISO 8601 formatted string (see formats locale to more information)
             * @return DateTimeString object
             */
            static const DateTimeString iso8601KaraboApiStringToDateTimeString(const std::string& timePoint);

        };

        template<>
        const std::string DateTimeString::getFractionalSecondString() const;
        template<>
        const unsigned long long DateTimeString::getFractionalSecondString() const;

    }
}

#endif	/* DATETIMESTRING_HH */

