/* 
 * File:   DateTimeString.cc
 * Author: <luis.maia@xfel.eu>
 * 
 * Created on March 19, 2014, 3:32 AM
 * 
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <CoreServices/CoreServices.h>
typedef int clockid_t;
#define CLOCK_REALTIME 0
#endif

#include "DateTimeString.hh"

namespace karabo {
    namespace util {


        DateTimeString::DateTimeString() :
        m_dateString("19700101"),
        m_timeString("000000"),
        m_fractionalSecondString("0"),
        m_timeZoneString("+0000"),
        m_dateTimeString("19700101T000000"),
        m_dateTimeStringAll("19700101T000000+0000"),
        m_timeZoneSignal("+"),
        m_timeZoneHours(0),
        m_timeZoneMinutes(0) {
        }


        DateTimeString::DateTimeString(const std::string& timePoint) {
            *this = iso8601KaraboApiStringToDateTimeString(timePoint);
        }


        DateTimeString::DateTimeString(const std::string& inputDateStr, const std::string& inputTimeStr,
                                       const std::string& inputFractionSecondStr, const std::string& inputTimeZoneStr) :
        m_dateString(inputDateStr),
        m_timeString(inputTimeStr),
        m_fractionalSecondString(inputFractionSecondStr),
        m_timeZoneString(inputTimeZoneStr),
        m_dateTimeString(inputDateStr + "T" + inputTimeStr) {

            if (m_fractionalSecondString == "") {
                m_fractionalSecondString = "0";
                m_dateTimeStringAll = inputDateStr + "T" + inputTimeStr + inputTimeZoneStr;
            } else {
                m_dateTimeStringAll = inputDateStr + "T" + inputTimeStr + "." + inputFractionSecondStr + inputTimeZoneStr;
            }

            if (inputTimeZoneStr == "Z" || inputTimeZoneStr == "z" || inputTimeZoneStr == "") {
                m_timeZoneSignal = "+";
                m_timeZoneHours = 0;
                m_timeZoneMinutes = 0;
            } else {

                std::string timeZoneHour;
                std::string timeZoneMinute;

                if (inputTimeZoneStr.find(':') != std::string::npos) {
                    size_t pos = inputTimeZoneStr.find(':');
                    timeZoneHour = inputTimeZoneStr.substr(1, pos - 1);
                    timeZoneMinute = inputTimeZoneStr.substr(pos + 1, inputTimeZoneStr.size());
                } else {
                    timeZoneHour = inputTimeZoneStr.substr(1, 2);
                    timeZoneMinute = inputTimeZoneStr.substr(3, inputTimeZoneStr.size());
                }

                m_timeZoneSignal = inputTimeZoneStr[0];
                m_timeZoneHours = boost::lexical_cast<int>(timeZoneHour);
                m_timeZoneMinutes = boost::lexical_cast<int>(timeZoneMinute);

            }

            if (DateTimeString::DateTimeString::isStringValidIso8601(m_dateTimeStringAll) == false) {
                throw KARABO_PARAMETER_EXCEPTION("Illegal time string sent by user (not a valid ISO-8601 format)");
            }

            if (DateTimeString::DateTimeString::isStringKaraboValidIso8601(m_dateTimeStringAll) == false) {
                throw KARABO_PARAMETER_EXCEPTION("Illegal time string sent by user (not a valid KARABO API ISO-8601 format)");
            }

        }


        DateTimeString::~DateTimeString() {
        }


        const bool DateTimeString::isStringValidIso8601(const std::string& timePoint) {
            // Original regular expression:
            // ^((((\+?|-{0,3})(\d{4}|\d{2})(?!\d{2}\b)|-\d?)((-?)((0[1-9]|1[0-2])((-?)([12]\d|0[1-9]|3[01]))?|W(((-?[1-7]))|([0-4]\d|5[0-2])(-?[1-7])?)|(00[1-9]|0[1-9]\d|[12]\d{2}|3([0-5]\d|6[1-6]))))?)([T]((((\+?|-{0,3})(([01]\d|2[0-3])((:?)([0-5]\d)?)((:?)([0-5]\d)?)|24\:?(00)?:?(00)?)|([-]{1,2}[0-5]\d([\.,]\d+)?))([\.,]\d+(?!:))?)))([zZ]|([\+-])([01]\d|2[0-3]):?([0-5]\d)?)?|(((\+?|-{0,3})(\d{4}|\d{2})(?!\d{2}\b)|(-\d)?)((-?)((0[1-9]|1[0-2])((-?)([12]\d|0[1-9]|3[01]))?|W(((-?[1-7]))|([0-4]\d|5[0-2])(-?[1-7])?)|(00[1-9]|0[1-9]\d|[12]\d{2}|3([0-5]\d|6[1-6]))))?)|((((\+?|-{0,3})(([01]\d|2[0-3])((:?)([0-5]\d)?)((:?)([0-5]\d)?)|24\:?(00)?:?(00)?)|([-]{1,2}[0-5]\d([\.,]\d+)?))([\.,]\d+(?!:))?))([zZ]|([\+-])([01]\d|2[0-3]):?([0-5]\d)?)?)$
            // Regex visualizer: https://www.debuggex.com/
            // Converted using online Java converter: http://www.regexplanet.com/advanced/java/index.html
            static const boost::regex e("^((((\\+?|-{0,3})(\\d{4}|\\d{2})(?!\\d{2}\\b)|-\\d?)((-?)((0[1-9]|1[0-2])((-?)([12]\\d|0[1-9]|3[01]))?|W(((-?[1-7]))|([0-4]\\d|5[0-2])(-?[1-7])?)|(00[1-9]|0[1-9]\\d|[12]\\d{2}|3([0-5]\\d|6[1-6]))))?)([T]((((\\+?|-{0,3})(([01]\\d|2[0-3])((:?)([0-5]\\d)?)((:?)([0-5]\\d)?)|24\\:?(00)?:?(00)?)|([-]{1,2}[0-5]\\d([\\.,]\\d+)?))([\\.,]\\d+(?!:))?)))([zZ]|([\\+-])([01]\\d|2[0-3]):?([0-5]\\d)?)?|(((\\+?|-{0,3})(\\d{4}|\\d{2})(?!\\d{2}\\b)|(-\\d)?)((-?)((0[1-9]|1[0-2])((-?)([12]\\d|0[1-9]|3[01]))?|W(((-?[1-7]))|([0-4]\\d|5[0-2])(-?[1-7])?)|(00[1-9]|0[1-9]\\d|[12]\\d{2}|3([0-5]\\d|6[1-6]))))?)|((((\\+?|-{0,3})(([01]\\d|2[0-3])((:?)([0-5]\\d)?)((:?)([0-5]\\d)?)|24\\:?(00)?:?(00)?)|([-]{1,2}[0-5]\\d([\\.,]\\d+)?))([\\.,]\\d+(?!:))?))([zZ]|([\\+-])([01]\\d|2[0-3]):?([0-5]\\d)?)?)$");
            if (timePoint != "") {
                return boost::regex_match(timePoint, e);
            } else {
                return false;
            }
        }


        const bool DateTimeString::isStringKaraboValidIso8601(const std::string& timePoint) {
            // Original regular expression:
            // ^((\d{4})-(0[1-9]|1[0-2])-([12]\d|0[1-9]|3[01])T([01]\d|2[0-3]):([0-5]\d):([0-5]\d)([\.,]\d+(?!:))?([zZ]|([\+-])([01]\d|2[0-3]):([0-5]\d))?|(\d{4})(0[1-9]|1[0-2])([12]\d|0[1-9]|3[01])T([01]\d|2[0-3])([0-5]\d)([0-5]\d)([\.,]\d+(?!:))?([zZ]|([\+-])([01]\d|2[0-3])([0-5]\d))?)$
            // Regex visualizer: https://www.debuggex.com/
            // Converted using online Java converter: http://www.regexplanet.com/advanced/java/index.html
            static const boost::regex e("^((\\d{4})-(0[1-9]|1[0-2])-([12]\\d|0[1-9]|3[01])T([01]\\d|2[0-3]):([0-5]\\d):([0-5]\\d)([\\.,]\\d+(?!:))?([zZ]|([\\+-])([01]\\d|2[0-3]):([0-5]\\d))?|(\\d{4})(0[1-9]|1[0-2])([12]\\d|0[1-9]|3[01])T([01]\\d|2[0-3])([0-5]\\d)([0-5]\\d)([\\.,]\\d+(?!:))?([zZ]|([\\+-])([01]\\d|2[0-3])([0-5]\\d))?)$");
            if (timePoint != "") {
                return boost::regex_match(timePoint, e);
            } else {
                return false;
            }
        }


        const DateTimeString DateTimeString::iso8601KaraboApiStringToDateTimeString(const std::string& timePoint) {

            karabo::util::DateTimeString t = karabo::util::DateTimeString();
            if (t.isStringValidIso8601(timePoint) == false) {
                throw KARABO_PARAMETER_EXCEPTION("Illegal time string sent by user (not a valid ISO-8601 format)");
            }

            if (t.isStringKaraboValidIso8601(timePoint) == false) {
                throw KARABO_PARAMETER_EXCEPTION("Illegal time string sent by user (not a valid KARABO API ISO-8601 format)");
            }

            // Copy current string and replace some characters to allow a cleaner code
            std::string currentTimePoint = timePoint;
            std::replace(currentTimePoint.begin(), currentTimePoint.end(), ',', '.');
            std::replace(currentTimePoint.begin(), currentTimePoint.end(), 'z', 'Z');

            // Goal variables
            std::string date = "";
            std::string time = "";
            std::string fractionalSeconds = "0";
            std::string timezone = "";

            //Auxiliary variables
            size_t pos = 0;
            std::string rest = "";


            //Separate Date (years, months and days) from the string
            // NOTE: This must be the first operation because the character '-' is use to separate the date and also in the Time Zone
            pos = currentTimePoint.find('T');
            date = currentTimePoint.substr(0, pos);
            rest = currentTimePoint.substr(pos + 1, currentTimePoint.size());


            //Separate Fractional seconds and Time zone from the string
            if (rest.find('Z') != std::string::npos ||
                    rest.find('+') != std::string::npos ||
                    rest.find('-') != std::string::npos) {
                if (rest.find('Z') != std::string::npos) {
                    pos = rest.find('Z');
                } else if (rest.find('+') != std::string::npos) {
                    pos = rest.find('+');
                } else if (rest.find('-') != std::string::npos) {
                    pos = rest.find('-');
                }

                timezone = rest.substr(pos, rest.size());
                rest = rest.substr(0, pos);
            } else {
                timezone = "";
                rest = rest;
            }


            //Separate Time (hours, minutes, seconds and fractional seconds) from the string
            if (rest.find('.') != std::string::npos) {
                pos = rest.find('.');
                fractionalSeconds = rest.substr(pos + 1, rest.size());
                time = rest.substr(0, pos);
            } else {
                fractionalSeconds = "0";
                time = rest;
            }


            // Create DateTimeString instance to be returned
            return DateTimeString(date, time, fractionalSeconds, timezone);
        }


        const std::locale formats[] = {
            //std::locale(std::locale::classic(), new boost::posix_time::time_input_facet("%Y-%m-%dT%H:%M:%S%z")), //2012-12-25T13:25:36-0700
            //std::locale(std::locale::classic(), new boost::posix_time::time_input_facet("%Y%m%dT%H%M%S.%f%z")), //19951231T235959.9942+0000
            std::locale(std::locale::classic(), new boost::posix_time::time_input_facet("%Y%m%dT%H%M%S%.f")), //19951231T235959.789333123456789123
            std::locale(std::locale::classic(), new boost::posix_time::time_input_facet("%Y-%m-%dT%H:%M:%S")) //2012-12-25T13:25:36
        };
        const size_t formats_n = sizeof (formats) / sizeof (formats[0]);


        const unsigned long long DateTimeString::ptimeToSecondsSinceEpoch(boost::posix_time::ptime& pt) {
            static boost::posix_time::ptime timet_start(boost::gregorian::date(1970, 1, 1));
            boost::posix_time::time_duration diff = pt - timet_start;
            return diff.total_seconds();
        }


        const unsigned long long DateTimeString::getSecondsSinceEpoch() {
            std::string dateAndTimeStr = m_dateTimeString;

            // Try to convert String to PTIME taking into consideration the date formats defined above
            boost::posix_time::ptime ptimeLocal;
            for (size_t i = 0; i < formats_n; ++i) {
                std::istringstream is(dateAndTimeStr);
                is.imbue(formats[i]);
                is >> ptimeLocal;
                if (ptimeLocal != boost::posix_time::ptime()) break;
            }

            boost::posix_time::ptime ptimeUtc;
            if (m_timeZoneSignal == "-") {
                ptimeUtc = ptimeLocal - boost::posix_time::hours(m_timeZoneHours) - boost::posix_time::minutes(m_timeZoneMinutes);
            } else {
                ptimeUtc = ptimeLocal + boost::posix_time::hours(m_timeZoneHours) + boost::posix_time::minutes(m_timeZoneMinutes);
            }

            return ptimeToSecondsSinceEpoch(ptimeUtc);
        }
        
        template<>
        const std::string DateTimeString::getFractionalSecondString() const {
            return boost::lexical_cast<std::string>(m_fractionalSecondString);
        }

        template<>
        const unsigned long long DateTimeString::getFractionalSecondString() const {
            return boost::lexical_cast<unsigned long long>(m_fractionalSecondString);
        }
    }
}
