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
#include "DateTimeString.hh"

namespace karabo {
    namespace util {


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
            karabo::util::DateTimeString dts = karabo::util::DateTimeString(pTimeStr);
            const unsigned long long secondsSinceEpoch = dts.getSecondsSinceEpoch();
            const unsigned long long fractionalSecond = dts.getFractionalSecondString<unsigned long long>(); //[Default is "0"]

            *this = Epochstamp(secondsSinceEpoch, fractionalSecond); //Use other constructor
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
            return this->toIso8601Internal(precision, extended, "");
        }


        std::string Epochstamp::toIso8601Ext(TIME_UNITS precision, bool extended) const {
            return this->toIso8601Internal(precision, extended, "Z");
        }


        std::string Epochstamp::toIso8601Internal(TIME_UNITS precision, bool extended, const std::string& localTimeZone) const {

            std::string timeZoneSignal;
            int timeZoneHours;
            int timeZoneMinutes;

            if (localTimeZone == "Z" || localTimeZone == "z" || localTimeZone == "") {
                timeZoneSignal = "+";
                timeZoneHours = 0;
                timeZoneMinutes = 0;
            } else {

                std::string timeZoneHour;
                std::string timeZoneMinute;

                if (localTimeZone.find(':') != std::string::npos) {
                    size_t pos = localTimeZone.find(':');
                    timeZoneHour = localTimeZone.substr(1, pos - 1);
                    timeZoneMinute = localTimeZone.substr(pos + 1, localTimeZone.size());
                } else {
                    timeZoneHour = localTimeZone.substr(1, 2);
                    timeZoneMinute = localTimeZone.substr(3, localTimeZone.size());
                }

                timeZoneSignal = localTimeZone[0];
                timeZoneHours = boost::lexical_cast<int>(timeZoneHour);
                timeZoneMinutes = boost::lexical_cast<int>(timeZoneMinute);
            }
            boost::posix_time::time_duration td(timeZoneHours, timeZoneMinutes, 0);




            using namespace boost::posix_time;
            static boost::posix_time::ptime epoch(boost::gregorian::date(1970, 1, 1));


            // The solution is to print out two separated elements:
            // 1. The time in seconds
            // 2. Fractional (second) part with the desired precision
            boost::posix_time::ptime time_point = epoch + seconds(m_seconds);

            if (timeZoneSignal == "+") {
                time_point = time_point + td;
            } else {
                time_point = time_point - td;
            }


            ostringstream oss;
            oss << (extended ? to_iso_extended_string(time_point) : to_iso_string(time_point))
                    << '.' << setw(18 - std::log10((long double) precision)) << setfill('0') << m_fractionalSeconds / precision;

            // If applicable, add information about the time zone
            // Necessary because of method "toIso8601Ext"
            if (localTimeZone != "") {
                oss << localTimeZone;
            }

            return oss.str();
        }


        double Epochstamp::toTimestamp(TIME_UNITS precision) const {
            ostringstream oss;
            oss << m_seconds << '.' << setw(18 - std::log10((long double) precision)) << setfill('0') << m_fractionalSeconds / precision;
            return karabo::util::fromString<double>(oss.str());
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


        std::string Epochstamp::toFormattedString(const std::string& format, const std::string& localTimeZone) const {

            const boost::posix_time::time_facet* facet = new boost::posix_time::time_facet(format.c_str());
            std::string pTime = this->toIso8601Internal(SECOND, false, localTimeZone);
            const boost::posix_time::ptime pt = boost::posix_time::from_iso_string(pTime);

            return getPTime2String(pt, facet);
        }


        bool Epochstamp::hashAttributesContainTimeInformation(const Hash::Attributes& attributes) {
            return (attributes.has("sec") && attributes.has("frac"));
        }


        Epochstamp Epochstamp::fromHashAttributes(const Hash::Attributes& attributes) {
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