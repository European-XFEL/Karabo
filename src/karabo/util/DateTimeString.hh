/* 
 * File:   DateTimeString.hh
 * Author: luismaia
 *
 * Created on March 19, 2014, 3:32 AM
 */

#ifndef KARABO_UTIL_DATETIMESTRING_HH
#define	KARABO_UTIL_DATETIMESTRING_HH


#include <boost/date_time.hpp>
#include <boost/regex.hpp>


namespace karabo {
    namespace util {

        class DateTimeString {

            // Considering the following example: 2013-01-20T20:30:00.123456Z
            // each string should contain the following values:
            std::string m_dateString; //2013-01-20
            std::string m_timeString; //20:30:00
            std::string m_fractionalSecondString; //123456
            std::string m_timeZoneString; //Z

            // Extra field that concatenates date with time
            std::string m_dateTimeString; //2013-01-20T20:30:00

        public:

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


            //            DateTimeString(const DateTimeString& orig);

            virtual ~DateTimeString();

            /**
             * Get the second (resp. fractional of a second) part, 
             * @return unsigned int 64bits
             */
            inline const std::string& getDateString() const {
                return m_dateString;
            }

            inline const std::string& getDateTimeString() const {
                return m_dateTimeString;
            }

            inline const std::string& getFractionalSecondString() const {
                return m_fractionalSecondString;
            }

            inline const std::string& getTimeString() const {
                return m_timeString;
            }

            inline const std::string& getTimeZoneString() const {
                return m_timeZoneString;
            }

            const bool isStringValidIso8601(const std::string& timePoint);

            const bool isStringKaraboValidIso8601(const std::string& timePoint);

        private:

        };
    }
}

#endif	/* DATETIMESTRING_HH */

