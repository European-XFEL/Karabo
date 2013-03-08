/* 
 * File:   timer.hh
 * Author: boukhelef
 *
 * Created on February 13, 2012, 1:16 PM
 */

#ifndef TIMER_HH
#define	TIMER_HH

#include <ctime>
#include <string>

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
typedef int clockid_t;
#define CLOCK_REALTIME 0
#endif

#include <boost/date_time.hpp>
#include "karaboDll.hh"

namespace karabo {
    namespace util {

        //--< high_resolution_timer >---------------------------------------------------
        // High resolution timer using QPC/QPF
        // Problem : QPC sometimes jumps forward on some multi-core processors
        //------------------------------------------------------------------------------
        #ifdef _WIN32
        typedef unsigned long long int uint64_t;
        typedef unsigned int uint32_t;
        #else
        #include <stdint.h>
        #endif

        typedef union {

            uint64_t epoch;

            struct {

                uint64_t nsec;
                uint64_t sec;
            };
        } timestamp;

        // Class to implement high resolution timer (ie. nano seconds)
        // TODO: generalize this class to Windows platform using QueryPerformanceCounter API

        // TODO: merge it with util::Time class

        class KARABO_DECLSPEC HighResolutionTimer {

            #ifdef _WIN32
            timestamp m_CpuFrequency;
            #endif
        public:

            HighResolutionTimer();
            ~HighResolutionTimer();

            #ifdef _WIN32
            static HighResolutionTimer& getInstance();
            static inline uint32_t getFrequency();
            #endif
            // Return the current time (Real-time / Thread-time ...)
            #ifdef _WIN32
            static timestamp now();
            #else
            static timestamp now(clockid_t whichtime = CLOCK_REALTIME);
            #endif

            // Convert time <--> double
            static double time2double(const timestamp& time);
            static timestamp double2time(const double& time);

            // Convert time <--> timestamp
            static uint64_t time2int(const timestamp& time);
            static timestamp int2time(const uint64_t& time);

            // Convert time into string
            //            static std::string time2string(const timestamp& time);
            //            static timestamp string2time(const std::string& time);
            static timestamp string2time(const std::string& time);
            static std::string time2string(timestamp time, int prec = 0);

            static std::string format(timestamp time, const char* strformat, int prec = 0);

        private:
            static std::string fractionsOfSecond(uint32_t nanosecs, int precision);
        };

        KARABO_DECLSPEC bool operator!=(const timestamp& left, const timestamp& right);
        KARABO_DECLSPEC bool operator==(const timestamp& left, const timestamp& right);
        KARABO_DECLSPEC timestamp operator+(const timestamp& left, const timestamp& right);
        KARABO_DECLSPEC timestamp operator-(const timestamp& left, const timestamp& right);
    }
}

#endif	/* TIMER_HH */
