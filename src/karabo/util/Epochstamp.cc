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


        TimeDuration Epochstamp::elpased(const Epochstamp& other) const {
            return *this -other;
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
            assert();
            clock_serv_t cclock;
            host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock); //REALTIME_CLOCKSYSTEM_CLOCK

            mach_timespec_t mts;
            clock_get_time(cclock, &mts);
            mach_port_deallocate(mach_task_self(), cclock);

            m_TimeValue = mts.tv_sec * resolution + (mts.tv_nsec * resolution) / NANOSEC;
        }

        #else


        void Epochstamp::now() {
            static const clockid_t whichtime = CLOCK_REALTIME; //    CLOCK_REALTIME,  CLOCK_PROCESS_CPUTIME_ID

            static timespec ts;
            clock_gettime(whichtime, &ts);

            *this = ts;
        }
        #endif


        Epochstamp Epochstamp::fromIso8601(const std::string& timePoint) {
            throw KARABO_NOT_IMPLEMENTED_EXCEPTION("To be done by LM");
        }


        std::string Epochstamp::toIso8601() const {
            using namespace boost::posix_time;
            boost::posix_time::ptime epoch(boost::gregorian::date(1970, 1, 1));

            // The boost minimum unit is nanosecond:
            // A nanosecond (ns) is one billionth of a second (10−9 or 1/1,000,000,000 s).
            boost::posix_time::ptime myTimePoint = epoch + seconds(this->m_seconds) + microseconds(this->convertFractionalSeconds("microseconds"));
            return to_iso_string(myTimePoint);
        }


        unsigned long long Epochstamp::convertFractionalSeconds(std::string destinyUnitMeasure) const {

            // A nanosecond (ns) is one billionth of a second (10−9 or 1/1,000,000,000 s).
            if (destinyUnitMeasure == "nanoseconds") {
                throw KARABO_NOT_IMPLEMENTED_EXCEPTION("To be done");
            }
                // A microsecond is an SI unit of time equal to one millionth (10−6 or 1/1,000,000) of a second.
            else if (destinyUnitMeasure == "microseconds") {
                throw KARABO_NOT_IMPLEMENTED_EXCEPTION("To be done");
            }

            // ...
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


        std::string Epochstamp::toFormattedString(const std::string& format) const {
            throw KARABO_NOT_IMPLEMENTED_EXCEPTION("To be done");
        }
    }
}