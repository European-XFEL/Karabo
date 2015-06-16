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

        Epochstamp::Epochstamp(const time_t& tm) : m_seconds(tm), m_fractionalSeconds(0) {
        }

        Epochstamp::Epochstamp(const timeval& tv) : m_seconds(tv.tv_sec), m_fractionalSeconds(tv.tv_usec * 1000000000000ULL) {
        }

        Epochstamp::Epochstamp(const timespec& ts) : m_seconds(ts.tv_sec), m_fractionalSeconds(ts.tv_nsec * 1000000000ULL) {
        }

        Epochstamp::Epochstamp(const std::string& pTime) {
            karabo::util::DateTimeString dts = karabo::util::DateTimeString(pTime);
            m_seconds = dts.getSecondsSinceEpoch();
            m_fractionalSeconds = dts.getFractionalSeconds<unsigned long long>();
        }

        TimeDuration Epochstamp::elapsed(const Epochstamp& other) const {
            if (*this < other) return other - * this;
            else return *this -other;
        }

        time_t Epochstamp::getTime() const {
            return m_seconds;
        }

        timeval Epochstamp::getTimeOfDay() const {
            timeval result = {long(m_seconds), long(m_fractionalSeconds / 1000000000000ULL)};  // std::pow(10, MICROSEC)
            return result;
        }

        timespec Epochstamp::getClockTime() const {
            timespec result = {long(m_seconds), long(m_fractionalSeconds / 1000000000ULL)};    // std::pow(10, NANOSEC)
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
            this->m_seconds = ts.tv_sec;
            this->m_fractionalSeconds = ts.tv_nsec * 1000000000ULL; // in ATTOSEC
        }
#endif

        std::string Epochstamp::toIso8601(TIME_UNITS precision, bool extended) const {
            return this->toIso8601Internal(precision, extended, "");
        }

        std::string Epochstamp::toIso8601Ext(TIME_UNITS precision, bool extended) const {
            return this->toIso8601Internal(precision, extended, "Z");
        }

        std::string Epochstamp::toIso8601Internal(TIME_UNITS precision, bool extended, const std::string& localTimeZone) const {

            const karabo::util::Hash timeZone = karabo::util::DateTimeString::getTimeDurationFromTimeZone(localTimeZone);

            std::string timeZoneSignal = timeZone.get<std::string>("timeZoneSignal");
            int timeZoneHours = timeZone.get<int>("timeZoneHours");
            int timeZoneMinutes = timeZone.get<int>("timeZoneMinutes");
            boost::posix_time::time_duration timeZoneDifference(timeZoneHours, timeZoneMinutes, 0);


            using namespace boost::posix_time;
            static boost::posix_time::ptime epoch(boost::gregorian::date(1970, 1, 1));


            // The solution is to print out two separated elements:
            // 1. The time in seconds
            // 2. Fractional (second) part with the desired precision
            boost::posix_time::ptime time_point = epoch + seconds(m_seconds);

            if (timeZoneSignal == "+") {
                time_point = time_point + timeZoneDifference;
            } else {
                time_point = time_point - timeZoneDifference;
            }

            std::string dateTime = (extended ? to_iso_extended_string(time_point) : to_iso_string(time_point));
            std::string dateTimeWithFractional = this->concatDateTimeWithFractional<std::string, std::string&>(dateTime, precision);

            // If applicable, add information about the time zone
            // Necessary because of method "toIso8601Ext"
            if (localTimeZone != "") {
                dateTimeWithFractional = dateTimeWithFractional + localTimeZone;
            }

            return dateTimeWithFractional;
        }

        template<typename To, typename PT1>
        const To Epochstamp::concatDateTimeWithFractional(const PT1 dateTime, const TIME_UNITS precision) const {
            ostringstream oss;
            oss << dateTime << karabo::util::DateTimeString::fractionalSecondToString(precision, m_fractionalSeconds);
            return boost::lexical_cast<To>(oss.str());
        }

        const double Epochstamp::toTimestamp() const {
            return this->concatDateTimeWithFractional<double, unsigned long long>(m_seconds, MICROSEC);
        }

        std::string Epochstamp::getPTime2String(const boost::posix_time::ptime pt,
                const boost::posix_time::time_facet* facet,
                const std::string& localeName) {
            std::ostringstream datetime_ss;

            // special_locale takes ownership of the p_time_output facet
            std::locale special_locale(std::locale(localeName.c_str()), facet);
            datetime_ss.imbue(special_locale);
            datetime_ss << pt;

            // return timestamp as string
            return datetime_ss.str();
        }

        std::string Epochstamp::toFormattedString(const std::string& format, const std::string& localTimeZone) const {
            return this->toFormattedStringInternal("", format, localTimeZone);
        }

        std::string Epochstamp::toFormattedStringLocale(const std::string& localeName, const std::string& format, const std::string& localTimeZone) const {
            return this->toFormattedStringInternal(localeName, format, localTimeZone);
        }

        std::string Epochstamp::toFormattedStringInternal(const std::string& localeName, const std::string& format, const std::string& localTimeZone) const {
            const boost::posix_time::time_facet* facet = new boost::posix_time::time_facet(format.c_str());
            std::string pTime = this->toIso8601Internal(SECOND, false, localTimeZone);
            const boost::posix_time::ptime pt = boost::posix_time::from_iso_string(pTime);

            return getPTime2String(pt, facet, localeName);
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