/* 
 * File:   EpochStamp.cc
 * Author: WP76
 * 
 * Created on June 19, 2013, 3:22 PM
 */

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <CoreServices/CoreServices.h>
typedef int clockid_t;
#define CLOCK_REALTIME 0
#endif

#include "Epochstamp.hh"

namespace karabo {
    namespace util {


        const std::locale formats[] = {
            //std::locale(std::locale::classic(), new boost::posix_time::time_input_facet("%Y%m%dT%H%M%S%f%z")), //19951231T235959.9942
            std::locale(std::locale::classic(), new boost::posix_time::time_input_facet("%Y%m%dT%H%M%S%f")), //19951231T235959.789333123456789123
            std::locale(std::locale::classic(), new boost::posix_time::time_input_facet("%Y-%m-%dT%H:%M:%S")), //2012-12-25T13:25:36.123456789123456789
            std::locale(std::locale::classic(), new boost::posix_time::time_input_facet("%Y-%m-%d %H:%M:%S")), //2012-12-25 13:25:36.123456789123456789
            std::locale(std::locale::classic(), new boost::posix_time::time_input_facet("%Y/%m/%d %H:%M:%S")),
            std::locale(std::locale::classic(), new boost::posix_time::time_input_facet("%d.%m.%Y %H:%M:%S"))
        };
        const size_t formats_n = sizeof (formats) / sizeof (formats[0]);


        const unsigned long long Epochstamp::ptToSecondsSinceEpoch(boost::posix_time::ptime& pt) {
            static boost::posix_time::ptime timet_start(boost::gregorian::date(1970, 1, 1));
            boost::posix_time::time_duration diff = pt - timet_start;
            return diff.total_seconds();
        }


        const bool Epochstamp::is_valid_iso8601_date_value(const std::string& timePoint) {
            struct tm result;
            char **f;
            char *ret;
            char *formats[] = {
                "%Y",
                "%Y-%m",
                "%y-%m",
                "%Y-%m-%d",
                "%y-%m-%d",
                "%Y%m%d",
                "%y%m%d",
                "%Y-%m-%d %T",
                "%y-%m-%d %T",
                "%Y%m%d%H%M%S",
                "%y%m%d%H%M%S",
                "%Y-%m-%dT%T",
                "%y-%m-%dT%T",
                "%Y-%m-%dT%TZ",
                "%y-%m-%dT%TZ",
                "%Y-%m-%d %TZ",
                "%y-%m-%d %TZ",
                "%Y%m%dT%TZ",
                "%y%m%dT%TZ",
                "%Y%m%d %TZ",
                "%y%m%d %TZ",
                NULL
            };

            //std::vector<std::string> timePointChar = karabo::util::fromString<std::string, std::vector > (timePoint, ".");
            const char *in = timePoint.c_str();
            //char *in = timePointChar;

            memset(&result, 0, sizeof (result));

            for (f = formats; f && *f; f++) {
                ret = strptime(in, *f, &result);
                if (ret && *ret == '\0')
                    return true;
            }

            return false;
        }


        const int Epochstamp::count_chars(const char* string, char ch) {
            int count = 0;
            for (; *string; count += (*string++ == ch));
            return count;
        }


        const bool Epochstamp::isStringValidIso8601(const std::string& timePoint) {
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


        const bool Epochstamp::isStringKaraboValidIso8601(const std::string& timePoint) {
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


        const Epochstamp Epochstamp::fromIso8601(const std::string& timePoint) {

            karabo::util::Epochstamp t = karabo::util::Epochstamp();
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
            string date = "";
            string time = "";
            string fractionalSeconds = "0";
            string timezone = "";

            //Auxiliar variables
            size_t pos = 0;
            //size_t size = currentTimePoint.size();
            string rest = "";

            //Separate Fractional seconds and Time zone from the string
            if (currentTimePoint.find('Z') != std::string::npos ||
                    currentTimePoint.find('+') != std::string::npos ||
                    currentTimePoint.find('+') != std::string::npos) {
                if (currentTimePoint.find('Z') != std::string::npos) {
                    pos = currentTimePoint.find('Z');
                } else if (currentTimePoint.find('+') != std::string::npos) {
                    pos = currentTimePoint.find('+');
                } else if (currentTimePoint.find('+') != std::string::npos) {
                    pos = currentTimePoint.find('+');
                }

                timezone = currentTimePoint.substr(pos, currentTimePoint.size());
                rest = currentTimePoint.substr(0, pos);
            } else {
                timezone = "";
                rest = currentTimePoint;
            }


            //Separate Time (hours, minutes and seconds) from the string
            if (currentTimePoint.find('.') != std::string::npos) {
                pos = rest.find('.');
                fractionalSeconds = rest.substr(pos + 1, rest.size());
                rest = rest.substr(0, pos);
            } else {
                fractionalSeconds = "0";
                //rest = rest;
            }

            //Separate Date (years, months and days) from the string
            pos = currentTimePoint.find('T');
            date = rest.substr(0, pos);
            time = rest.substr(pos + 1, rest.size());


            std::clog << "LMAIA => (timePoint == " << currentTimePoint << ")" << endl;
            std::clog << date << " |T| " << time << " |.| " << fractionalSeconds << " |TZ| " << timezone << endl;
            std::clog << "-----------------------------" << endl;

            //std::vector<std::string> timeParts = karabo::util::fromString<std::string, std::vector > (timePoint, ".");

            //if (timeParts.size() != 2) {
            //    throw KARABO_PARAMETER_EXCEPTION("Illegal time string sent by user (missing '.' character or/and seconds or/and fractional seconds)");
            //}
            //std::string secondsStr = timeParts[0];
            //std::string fractionalSecondsStr = timeParts[1];

            std::string nonFractionalDateTime = date + "T" + time;

            // Try to convert String to PTIME taking into consideration the date formats defined above
            boost::posix_time::ptime pt;
            for (size_t i = 0; i < formats_n; ++i) {
                std::istringstream is(nonFractionalDateTime);
                is.imbue(formats[i]);
                is >> pt;
                if (pt != boost::posix_time::ptime()) break;
            }

            // Assign seconds since epoch
            const unsigned long long secs = ptToSecondsSinceEpoch(pt);
            // Assign fractional seconds (in the current second)
            unsigned long long fraqs = boost::lexical_cast<unsigned long long>(fractionalSeconds);

            // Create Epochstamp to be returned
            return Epochstamp(secs, fraqs);
        }


        const Epochstamp Epochstamp::fromIso8601Ext(const std::string& timePoint) {

            std::string lastChar = timePoint.substr(timePoint.size() - 1, timePoint.size());
            if (lastChar != "Z") {
                throw KARABO_PARAMETER_EXCEPTION("Illegal time string sent by user (missing ending 'Z' character)");
            }

            return fromIso8601(timePoint.substr(0, timePoint.size() - 1));
        }


        std::string Epochstamp::getPTime2String(const boost::posix_time::ptime pt, const boost::posix_time::time_facet* facet) {
            std::ostringstream datetime_ss;

            // special_locale takes ownership of the p_time_output facet
            std::locale special_locale(std::locale(""), facet);

            datetime_ss.imbue(special_locale);

            datetime_ss << pt;

            // return timestamp as string
            return datetime_ss.str();
        }


        Epochstamp::Epochstamp() {
            now();
        }


        Epochstamp::Epochstamp(const unsigned long long& seconds, const unsigned long long& fractions) :
        m_seconds(seconds), m_fractionalSeconds(fractions) {
        }


        Epochstamp::~Epochstamp() {
        }


        Epochstamp::Epochstamp(const time_t& tm) {
            *this = tm;
        }


        Epochstamp::Epochstamp(const timeval& tv) {
            *this = tv;
        }


        Epochstamp::Epochstamp(const timespec& ts) {
            *this = ts;
        }


        Epochstamp::Epochstamp(const std::string& pTimeStr) {
            *this = fromIso8601(pTimeStr);
        }


        TimeDuration Epochstamp::elapsed(const Epochstamp& other) const {
            if (*this < other) return other - * this;
            else return *this -other;
        }


        time_t Epochstamp::getTime() const {
            return m_seconds;
        }


        timeval Epochstamp::getTimeOfDay() const {
            timeval result = {m_seconds, m_fractionalSeconds / MICROSEC};
            return result;
        }


        timespec Epochstamp::getClockTime() const {
            timespec result = {m_seconds, m_fractionalSeconds / NANOSEC};
            return result;
        }

        // retrieve the current time
        #ifdef _WIN32


        void Epochstamp::now() {
            assert();

            LARGE_INTEGER wts;
            QueryPerformanceCounter((LARGE_INTEGER*) & wts);

            m_TimeVale = static_cast<uint64_t> (static_cast<long double> (wts) / getFrequency() * NANOSEC / resolution);
        }

        #elif __MACH__


        void Epochstamp::now() {
            clock_serv_t cclock;
            host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock); //REALTIME_CLOCKSYSTEM_CLOCK

            mach_timespec_t mts;
            clock_get_time(cclock, &mts);
            mach_port_deallocate(mach_task_self(), cclock);

            m_seconds = mts.tv_sec;
            m_fractionalSeconds = mts.tv_nsec * 1000000000ull;
        }

        #else


        void Epochstamp::now() {
            static const clockid_t whichtime = CLOCK_REALTIME; //    CLOCK_REALTIME,  CLOCK_PROCESS_CPUTIME_ID

            static timespec ts;
            clock_gettime(whichtime, &ts);

            *this = ts;
        }
        #endif


        std::string Epochstamp::toIso8601(TIME_UNITS precision, bool extended) const {
            static boost::posix_time::ptime epoch(boost::gregorian::date(1970, 1, 1));
            using namespace boost::posix_time;
            if (0) {

                // The boost minimum unit is nanosecond:
                // A nanosecond (ns) is one billionth of a second (10−9 or 1/1,000,000,000 s).
                boost::posix_time::ptime time_point = epoch + seconds(m_seconds);
                #if defined(BOOST_DATE_TIME_HAS_NANOSECONDS)
                time_point += nanoseconds(m_fractionalSeconds / NANOSEC);
                #else
                time_point += microseconds(m_fractionalSeconds / MICROSEC);
                #endif
                return extended ? to_iso_extended_string(time_point) : to_iso_string(time_point);
            }
            // Another solution is to print out the time in seconds 
            // and then print the fractional part with the desired precision. 
            // Could be microseconds, nanoseconds, or any thing else we want.
            {
                boost::posix_time::ptime time_point = epoch + seconds(m_seconds);

                ostringstream oss;
                oss << (extended ? to_iso_extended_string(time_point) : to_iso_string(time_point))
                        << '.' << setw(18 - std::log10((long double) precision)) << setfill('0') << m_fractionalSeconds / precision;
                return oss.str();
            }
        }


        std::string Epochstamp::toIso8601Ext(TIME_UNITS precision, bool extended) const {
            return this->toIso8601(precision, extended) + "Z";
        }


        double Epochstamp::toTimestamp(TIME_UNITS precision) const {
            ostringstream oss;
            oss << m_seconds << '.' << setw(18 - std::log10((long double) precision)) << setfill('0') << m_fractionalSeconds / precision;
            return karabo::util::fromString<double>(oss.str());
        }


        std::string Epochstamp::toFormattedString(const std::string& format) const {

            const boost::posix_time::time_facet* facet = new boost::posix_time::time_facet(format.c_str());

            std::string pTime = this->toIso8601(SECOND, false);
            const boost::posix_time::ptime pt = boost::posix_time::from_iso_string(pTime);

            return getPTime2String(pt, facet);
        }


        bool Epochstamp::hashAttributesContainTimeInformation(const Hash::Attributes attributes) {
            return (attributes.has("sec") && attributes.has("frac"));
        }


        Epochstamp Epochstamp::fromHashAttributes(const Hash::Attributes attributes) {
            unsigned long long seconds, fraction;
            try {
                attributes.get("sec", seconds);
                attributes.get("frac", fraction);
            } catch (const Exception& e) {
                KARABO_RETHROW_AS(KARABO_PARAMETER_EXCEPTION("Provided attributes do not contain proper timestamp information"));
            }
            return Epochstamp(seconds, fraction);
        }


        void Epochstamp::toHashAttributes(Hash::Attributes& attributes) const {
            attributes.set("sec", m_seconds);
            attributes.set("frac", m_fractionalSeconds);
        }

    }
}