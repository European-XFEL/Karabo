/* 
 * File:   DateTimeString.cc
 * Author: <luis.maia@xfel.eu>
 * 
 * Created on March 19, 2014, 3:32 AM
 * 
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "DateTimeString.hh"

namespace karabo {
    namespace util {


        DateTimeString::DateTimeString() :
        m_date("19700101"),
        m_time("000000"),
        m_fractionalSeconds("000000000000000000"),
        m_timeZone("+0000"),
        m_dateTime("19700101T000000"),
        m_dateTimeStringAll("19700101T000000+0000"),
        m_timeZoneSignal("+"),
        m_timeZoneHours(0),
        m_timeZoneMinutes(0) {
        }


        DateTimeString::DateTimeString(const std::string& timePoint) {
            *this = iso8601KaraboApiStringToDateTimeString(timePoint);
        }


        DateTimeString::DateTimeString(const std::string& inputDate, const std::string& inputTime,
                                       const std::string& inputFractionSecond, const std::string& inputTimeZone) :
        m_date(inputDate),
        m_time(inputTime),
        m_fractionalSeconds(inputFractionSecond),
        m_timeZone(inputTimeZone),
        m_dateTime(inputDate + "T" + inputTime) {

            if (m_fractionalSeconds == "") {
                m_fractionalSeconds = "0";
                m_dateTimeStringAll = inputDate + "T" + inputTime + inputTimeZone;
            } else {
                m_dateTimeStringAll = inputDate + "T" + inputTime + "." + inputFractionSecond + inputTimeZone;
            }


            const karabo::util::Hash timeZone = karabo::util::DateTimeString::getTimeDurationFromTimeZone(inputTimeZone);
            m_timeZoneSignal = timeZone.get<std::string>("timeZoneSignal");
            m_timeZoneHours = timeZone.get<int>("timeZoneHours");
            m_timeZoneMinutes = timeZone.get<int>("timeZoneMinutes");


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
            // ^(((((\+?|-{0,3})(\d{4}|\d{2})(?!\d{2}\b)|(-\d)?)((-?)((0[1-9]|1[0-2])(-([12]\d|0[1-9]|3[01]))?|W(((-[1-7]))|([0-4]\d|5[0-2])(-[1-7])?)|(00[1-9]|0[1-9]\d|[12]\d{2}|3([0-5]\d|6[1-6]))))?)([T]((((\+?|-{0,3})(([01]\d|2[0-3])((:[0-5]\d)?)((:[0-5]\d)?)|24(:00)?(:00)?)|([-]{1,2}[0-5]\d([\.,]\d+)?))([\.,]\d+(?!:))?)))([zZ]|([\+-])([01]\d|2[0-3])(:[0-5]\d)?)?|(((\+?|-{0,3})(\d{4}|\d{2})(?!\d{2}\b)|(-\d)?)((-?)((0[1-9]|1[0-2])(-([12]\d|0[1-9]|3[01]))?|W(((-[1-7]))|([0-4]\d|5[0-2])(-[1-7])?)|(00[1-9]|0[1-9]\d|[12]\d{2}|3([0-5]\d|6[1-6]))))?)|((((\+?|-{0,3})(([01]\d|2[0-3])((:[0-5]\d)?)((:[0-5]\d)?)|24(:00)?(:00)?)|([-]{1,2}[0-5]\d([\.,]\d+)?))([\.,]\d+(?!:))?))([zZ]|([\+-])([01]\d|2[0-3])(:[0-5]\d)?)?)|((((\+?|-{0,3})(\d{4}|\d{2})(?!\d{2}\b)|(-\d)?)((-?)((0[1-9]|1[0-2])(([12]\d|0[1-9]|3[01]))?|W((([1-7]))|([0-4]\d|5[0-2])([1-7])?)|(00[1-9]|0[1-9]\d|[12]\d{2}|3([0-5]\d|6[1-6]))))?)([T]((((\+?|-{0,3})(([01]\d|2[0-3])(([0-5]\d)?)(([0-5]\d)?)|24(00)?(00)?)|([-]{1,2}[0-5]\d([\.,]\d+)?))([\.,]\d+(?!:))?)))([zZ]|([\+-])([01]\d|2[0-3])([0-5]\d)?)?|(((\+?|-{0,3})(\d{4}|\d{2})(?!\d{2}\b)|(-\d)?)((-?)((0[1-9]|1[0-2])(([12]\d|0[1-9]|3[01]))?|W((([1-7]))|([0-4]\d|5[0-2])([1-7])?)|(00[1-9]|0[1-9]\d|[12]\d{2}|3([0-5]\d|6[1-6]))))?)|((((\+?|-{0,3})(([01]\d|2[0-3])(([0-5]\d)?)(([0-5]\d)?)|24(00)?(00)?)|([-]{1,2}[0-5]\d([\.,]\d+)?))([\.,]\d+(?!:))?))([zZ]|([\+-])([01]\d|2[0-3])([0-5]\d)?)?))$
            // Regex visualizer: https://www.debuggex.com/
            // Converted using online Java converter: http://www.regexplanet.com/advanced/java/index.html
            static const boost::regex e("^(((((\\+?|-{0,3})(\\d{4}|\\d{2})(?!\\d{2}\\b)|(-\\d)?)((-?)((0[1-9]|1[0-2])(-([12]\\d|0[1-9]|3[01]))?|W(((-[1-7]))|([0-4]\\d|5[0-2])(-[1-7])?)|(00[1-9]|0[1-9]\\d|[12]\\d{2}|3([0-5]\\d|6[1-6]))))?)([T]((((\\+?|-{0,3})(([01]\\d|2[0-3])((:[0-5]\\d)?)((:[0-5]\\d)?)|24(:00)?(:00)?)|([-]{1,2}[0-5]\\d([\\.,]\\d+)?))([\\.,]\\d+(?!:))?)))([zZ]|([\\+-])([01]\\d|2[0-3])(:[0-5]\\d)?)?|(((\\+?|-{0,3})(\\d{4}|\\d{2})(?!\\d{2}\\b)|(-\\d)?)((-?)((0[1-9]|1[0-2])(-([12]\\d|0[1-9]|3[01]))?|W(((-[1-7]))|([0-4]\\d|5[0-2])(-[1-7])?)|(00[1-9]|0[1-9]\\d|[12]\\d{2}|3([0-5]\\d|6[1-6]))))?)|((((\\+?|-{0,3})(([01]\\d|2[0-3])((:[0-5]\\d)?)((:[0-5]\\d)?)|24(:00)?(:00)?)|([-]{1,2}[0-5]\\d([\\.,]\\d+)?))([\\.,]\\d+(?!:))?))([zZ]|([\\+-])([01]\\d|2[0-3])(:[0-5]\\d)?)?)|((((\\+?|-{0,3})(\\d{4}|\\d{2})(?!\\d{2}\\b)|(-\\d)?)((-?)((0[1-9]|1[0-2])(([12]\\d|0[1-9]|3[01]))?|W((([1-7]))|([0-4]\\d|5[0-2])([1-7])?)|(00[1-9]|0[1-9]\\d|[12]\\d{2}|3([0-5]\\d|6[1-6]))))?)([T]((((\\+?|-{0,3})(([01]\\d|2[0-3])(([0-5]\\d)?)(([0-5]\\d)?)|24(00)?(00)?)|([-]{1,2}[0-5]\\d([\\.,]\\d+)?))([\\.,]\\d+(?!:))?)))([zZ]|([\\+-])([01]\\d|2[0-3])([0-5]\\d)?)?|(((\\+?|-{0,3})(\\d{4}|\\d{2})(?!\\d{2}\\b)|(-\\d)?)((-?)((0[1-9]|1[0-2])(([12]\\d|0[1-9]|3[01]))?|W((([1-7]))|([0-4]\\d|5[0-2])([1-7])?)|(00[1-9]|0[1-9]\\d|[12]\\d{2}|3([0-5]\\d|6[1-6]))))?)|((((\\+?|-{0,3})(([01]\\d|2[0-3])(([0-5]\\d)?)(([0-5]\\d)?)|24(00)?(00)?)|([-]{1,2}[0-5]\\d([\\.,]\\d+)?))([\\.,]\\d+(?!:))?))([zZ]|([\\+-])([01]\\d|2[0-3])([0-5]\\d)?)?))$");
            if (timePoint != "") {
                return boost::regex_match(timePoint, e);
            } else {
                return false;
            }
        }


        const bool DateTimeString::isStringValidIso8601TimeZone(const std::string& iso8601TimeZone) {
            // Original regular expression:
            // ^([zZ]|([\+-])([01]\d|2[0-3])(:?)([0-5]\d))?$
            // Regex visualizer: https://www.debuggex.com/
            // Converted using online Java converter: http://www.regexplanet.com/advanced/java/index.html
            static const boost::regex e("^([zZ]|([\\+-])([01]\\d|2[0-3])(:?)([0-5]\\d))?$");
            return boost::regex_match(iso8601TimeZone, e);
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
                throw KARABO_PARAMETER_EXCEPTION("Illegal time string sent by user (not a valid ISO-8601 format) => '" + timePoint + "'");
            }

            if (t.isStringKaraboValidIso8601(timePoint) == false) {
                throw KARABO_PARAMETER_EXCEPTION("Illegal time string sent by user (not a valid KARABO API ISO-8601 format) => '" + timePoint + "'");
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

            //Calculate fractional Atto second
            fractionalSeconds = karabo::util::DateTimeString::fractionalStringToAttoFractionalString(fractionalSeconds);

            // Create DateTimeString instance to be returned
            return DateTimeString(date, time, fractionalSeconds, timezone);
        }


        const std::locale formats[] = {
            //std::locale(std::locale::classic(), new boost::posix_time::time_input_facet("%Y-%m-%dT%H:%M:%S%z")), //2012-12-25T13:25:36-0700
            //std::locale(std::locale::classic(), new boost::posix_time::time_input_facet("%Y%m%dT%H%M%S.%f%z")), //19951231T235959.9942+0000
            std::locale(std::locale::classic(), new boost::posix_time::time_input_facet("%Y%m%dT%H%M%S%.%f")), //19951231T235959.789333(123456789123)
            std::locale(std::locale::classic(), new boost::posix_time::time_input_facet("%Y-%m-%dT%H:%M:%S")) //2012-12-25T13:25:36
        };
        const size_t formats_n = sizeof (formats) / sizeof (formats[0]);


        const unsigned long long DateTimeString::ptimeToSecondsSinceEpoch(boost::posix_time::ptime& pt) {
            static boost::posix_time::ptime timet_start(boost::gregorian::date(1970, 1, 1));
            boost::posix_time::time_duration diff = pt - timet_start;
            return diff.total_seconds();
        }


        const unsigned long long DateTimeString::getSecondsSinceEpoch() {
            std::string dateAndTime = m_dateTime;

            // Try to convert String to PTIME taking into consideration the date formats defined above
            boost::posix_time::ptime ptimeLocal;
            for (size_t i = 0; i < formats_n; ++i) {
                std::istringstream is(dateAndTime);
                is.imbue(formats[i]);
                is >> ptimeLocal;
                if (ptimeLocal != boost::posix_time::ptime()) break;
            }

            boost::posix_time::time_duration timeZoneDifference(m_timeZoneHours, m_timeZoneMinutes, 0);
            boost::posix_time::ptime ptimeUtc;
            if (m_timeZoneSignal == "+") {
                ptimeUtc = ptimeLocal - timeZoneDifference; //Berlin hour - 1h == London hour
            } else {
                ptimeUtc = ptimeLocal + timeZoneDifference; //Azores hour + 1h == London hour
            }

            return ptimeToSecondsSinceEpoch(ptimeUtc);
        }


        const karabo::util::Hash DateTimeString::getTimeDurationFromTimeZone(const std::string& iso8601TimeZone) {

            // Attention that "" (empty string) is a valid time zone format in the validation REGEX
            karabo::util::DateTimeString dts = karabo::util::DateTimeString();
            if (dts.isStringValidIso8601TimeZone(iso8601TimeZone) == false) {
                throw KARABO_PARAMETER_EXCEPTION("Illegal Time Zone string sent by user (not a valid ISO-8601 format) => '" + iso8601TimeZone + "'");
            }

            std::string timeZoneSignal;
            int timeZoneHours;
            int timeZoneMinutes;

            if (iso8601TimeZone == "Z" || iso8601TimeZone == "z" || iso8601TimeZone == "") {
                timeZoneSignal = "+";
                timeZoneHours = 0;
                timeZoneMinutes = 0;
            } else {

                std::string timeZoneHour;
                std::string timeZoneMinute;

                if (iso8601TimeZone.find(':') != std::string::npos) {
                    size_t pos = iso8601TimeZone.find(':');
                    timeZoneHour = iso8601TimeZone.substr(1, pos - 1);
                    timeZoneMinute = iso8601TimeZone.substr(pos + 1, iso8601TimeZone.size());
                } else {
                    timeZoneHour = iso8601TimeZone.substr(1, 2);
                    timeZoneMinute = iso8601TimeZone.substr(3, iso8601TimeZone.size());
                }

                timeZoneSignal = iso8601TimeZone[0];
                timeZoneHours = boost::lexical_cast<int>(timeZoneHour);
                timeZoneMinutes = boost::lexical_cast<int>(timeZoneMinute);
            }

            //const karabo::util::Hash h("timeZoneSignal", timeZoneSignal, "timeZoneHours", timeZoneHours, "timeZoneMinutes", timeZoneMinutes);
            karabo::util::Hash h;
            h.set<std::string>("timeZoneSignal", timeZoneSignal);
            h.set<int>("timeZoneHours", timeZoneHours);
            h.set<int>("timeZoneMinutes", timeZoneMinutes);

            return h;
        }


        const std::string DateTimeString::fractionalSecondToString(const TIME_UNITS precision, const unsigned long long fractionalSeconds) {

            if (precision == NOFRACTION) return "";

            ostringstream oss;

            // Be carefully with std::log10 calculation
            // The explicit conversion to "double" is necessary, if used smaller types like:
            // "long double"
            // (i.e. [(int) "18 - std::log10((long double) 15)"] == 2)
            // (i.e. [(double) "18 - std::log10((long double) 15)"] == 3)
            // unsigned long long
            // (i.e. [(int) "18 - std::log10((unsigned long long) 15)"] == 3)
            // (i.e. [(double) "18 - std::log10((unsigned long long) 15)"] == 3)
            // Alternatively the best option is to do:
            // ["std::log10(ONESECOND / precision)"] == 3
            int zeros = int(precision);
            unsigned long long multiplier = 1;
            while(zeros-->0) multiplier *= 10ULL;
            int numDigits = std::log10(1000000000000000000ULL / multiplier);
            oss << '.' << setw(numDigits) << setfill('0') << fractionalSeconds / multiplier;
            
            return oss.str();
        }


        const std::string DateTimeString::fractionalStringToAttoFractionalString(const std::string& fractionalSeconds) {
            //Calculate fractional Atto second value (read method description for more information)
            std::ostringstream oss;
            oss << std::setiosflags(std::ios::left) << std::setw(18) << std::setfill('0') << fractionalSeconds;
            return oss.str();
        }


        template<>
        const std::string DateTimeString::getFractionalSeconds() const {
            return boost::lexical_cast<std::string>(m_fractionalSeconds);
        }


        template<>
        const unsigned long long DateTimeString::getFractionalSeconds() const {
            return boost::lexical_cast<unsigned long long>(m_fractionalSeconds);
        }
    }
}
