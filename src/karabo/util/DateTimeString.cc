/* 
 * File:   DateTimeString.cc
 * Author: luismaia
 * 
 * Created on March 19, 2014, 3:32 AM
 */

#include "DateTimeString.hh"


namespace karabo {
    namespace util {

        DateTimeString::DateTimeString(const std::string& timePoint) :
        m_dateString(""),
        m_timeString(""),
        m_fractionalSecondString(""),
        m_timeZoneString(""),
        m_dateTimeString("") {
        }

        DateTimeString::DateTimeString(const std::string& inputDateStr, const std::string& inputTimeStr,
                const std::string& inputFractionSecondStr, const std::string& inputTimeZoneStr) :
        m_dateString(inputDateStr),
        m_timeString(inputTimeStr),
        m_fractionalSecondString(inputFractionSecondStr),
        m_timeZoneString(inputTimeZoneStr),
        m_dateTimeString(inputDateStr + "T" + inputTimeStr) {
        }

//        DateTimeString::DateTimeString(const DateTimeString& orig) :
//        m_dateString(orig.m_dateString),
//        m_timeString(orig.m_timeString),
//        m_fractionalSecondString(orig.m_fractionalSecondString),
//        m_timeZoneString(orig.m_timeString),
//        m_dateTimeString(orig.m_dateTimeString) {
//        }

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

    }
}
