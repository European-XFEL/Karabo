
/*
 * File:   timer.cc
 * $Id$
 *
 * Author: <djelloul.boukhelef@xfel.eu>
 *
 * Created on February 24, 2012, 9:46 PM
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <ctime>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <map>
#include <vector>

#include <boost/date_time.hpp>
//#include <boost/date_time/local_time_adjustor.hpp>
#include <boost/date_time/c_local_time_adjustor.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#ifdef _WIN32
#include <windows.h>
#endif

#include "Timer.hh"


namespace karabo {
    namespace util {

        ///    Implementation

        using namespace std;
        using namespace boost;


        HighResolutionTimer::HighResolutionTimer() {
            #ifdef _WIN32
            //Increasing the accuracy of Sleep to 1ms using timeBeginPeriod
            timeBeginPeriod(1); //Add Winmm.lib in Project
            QueryPerformanceFrequency((LARGE_INTEGER*) & m_CpuFrequency); //Determine the frequency
            #endif
        }


        HighResolutionTimer::~HighResolutionTimer() {
            #ifdef _WIN32
            timeEndPeriod(1); //Must be called if timeBeginPeriod() was called
            #endif
        }

        #ifdef _WIN32


        HighResolutionTimer& HighResolutionTimer::getInstance() {
            static HighResolutionTimer timer;
            return timer;
        }


        inline uint32_t HighResolutionTimer::getFrequency() {
            return HighResolutionTimer::getInstance().m_CpuFrequency.nsec;
        }
        #endif

        // A set of operators to manipulate timestamp


        bool operator!=(const timestamp& left, const timestamp& right) {
            return (left.sec != right.sec) || (left.nsec != right.nsec);
        }


        bool operator==(const timestamp& left, const timestamp& right) {
            return !(left != right);
        }


        timestamp operator+(const timestamp& left, const timestamp& right) {
            timestamp tmp(left);
            tmp.nsec += right.nsec;
            tmp.sec += right.sec + tmp.nsec / 1000000000;
            tmp.nsec %= 1000000000;

            return tmp;
        }

        // Calculates the difference between two instant


        timestamp operator-(const timestamp& left, const timestamp& right) {

            timestamp tmp(left);
            if (left.nsec < right.nsec) {
                tmp.sec -= 1;
                tmp.nsec += 1000000000;
            }

            tmp.sec -= right.sec;
            tmp.nsec -= right.nsec;

            return tmp;
        }

        // retrieve the current time
        #ifdef _WIN32


        timestamp HighResolutionTimer::now() {
            static timestamp current_time;

            QueryPerformanceCounter((LARGE_INTEGER*) & current_time);

            double tmp = double(current_time.epoch) / getFrequency();

            current_time.sec = int(tmp);
            current_time.nsec = int((tmp - current_time.sec)*1000000000);

            return current_time;
        }

        #elif __MACH__


        timestamp HighResolutionTimer::now(clockid_t whichtime) {//    CLOCK_REALTIME,  CLOCK_PROCESS_CPUTIME_ID
            static timestamp current_time;
            clock_serv_t cclock;
            mach_timespec_t mts;
            host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
            clock_get_time(cclock, &mts);
            mach_port_deallocate(mach_task_self(), cclock);
            current_time.nsec = mts.tv_nsec;
            current_time.sec = mts.tv_sec;
            return current_time;
        }
        #else


        timestamp HighResolutionTimer::now(clockid_t whichtime) {//    CLOCK_REALTIME,  CLOCK_PROCESS_CPUTIME_ID
            static timestamp current_time;
            timespec tmp;
            if (clock_gettime(whichtime, &tmp) < 0) {
                throw std::runtime_error("Couldn't get the current system time");
            }

            current_time.nsec = tmp.tv_nsec;
            current_time.sec = tmp.tv_sec;

            return current_time;
        }
        #endif

        // Convert time <--> double


        double HighResolutionTimer::time2double(const timestamp& time) {
            return double(time.sec) + double(time.nsec) * 1.0e-9L;
        }


        timestamp HighResolutionTimer::double2time(const double& time) {
            timestamp temp;
            temp.sec = uint32_t(time);
            temp.nsec = uint32_t((time - temp.sec)*1.0e-9L);

            return temp;
        }

        // Convert time <--> timestamp


        uint64_t HighResolutionTimer::time2int(const timestamp& time) {
            //            uint64_t tmp = time.sec;
            //            tmp <<= 32;
            //            tmp += time.nsec;
            return time.epoch;
        }


        timestamp HighResolutionTimer::int2time(const uint64_t& time) {
            timestamp result;
            result.nsec = time & 0xFFFFFFFF;
            result.sec = time >> 32;
            return result;
        }

        // Convert time <--> string


        std::string HighResolutionTimer::time2string(timestamp time, int prec) {
            // strformat = %y %m %d %h %t %s %l %u %n
            using namespace boost::posix_time;
            std::ostringstream oss;
            //            using namespace boost::gregorian;

            //            ptime tm = ;

            //            std::pow(10, precision);
            oss << to_simple_string(boost::date_time::c_local_adjustor<ptime>::utc_to_local(from_time_t(time.sec)));

            if (prec > 0) {
                oss << '.';
                oss << HighResolutionTimer::fractionsOfSecond(time.nsec, prec);
            }


            return oss.str();
        }


        timestamp HighResolutionTimer::string2time(const std::string& time) {
            using namespace boost::posix_time;

            time_duration td(duration_from_string(time));

            timestamp tm;
            tm.sec = td.total_seconds();
            tm.nsec = 0L;

            size_t comma = time.find('.');
            if (comma != string::npos) {
                tm.nsec = boost::lexical_cast<uint32_t > (time.substr(comma + 1, 9));
            }

            return tm;
        }

        // format a timestamp into human readable string 
        // possible formatter are
        // %h: hours,  %m: minutes, %s: seconds, %l:milli-seconds, %u: micro-seconds, %n: nano-seconds


        std::string HighResolutionTimer::format(timestamp time, const char* strformat, int prec) {
            using namespace boost::posix_time;

            time_duration td(0, 0, time.sec, 0);

            std::ostringstream oss;
            oss.fill('0');

            for (int i = 0; strformat[i] != 0; ++i) {
                if (strformat[i] == '%') {
                    switch (strformat[++i]) {
                        case 'H': oss.width(2);
                            oss << td.hours();
                            break;
                        case 'M': oss.width(2);
                            oss << td.minutes();
                            break;
                        case 'S': oss.width(2);
                            oss << td.seconds();
                            break;
                        case 'h': oss.width(1);
                            oss << td.hours();
                            break;
                        case 'm': oss.width(1);
                            oss << td.minutes();
                            break;
                        case 's': oss.width(1);
                            oss << td.seconds();
                            break;
                        case 'l': prec = 3;
                            break;
                        case 'u': prec = 6;
                            break;
                        case 'n': prec = 9;
                            break;
                        default:
                            throw "Unrecognized format";
                    }
                } else {
                    oss << strformat[i];
                }
            }

            if (prec > 0) {
                oss << HighResolutionTimer::fractionsOfSecond(time.nsec, prec);
            }

            return oss.str();
        }

        // return only requested decimals


        std::string HighResolutionTimer::fractionsOfSecond(uint32_t nanosecs, int precision) {
            std::ostringstream oss;

            oss.width(precision);
            oss.fill('0');
            oss.precision(0);

            oss << fixed << double(nanosecs) / pow(10.0, 9 - precision);

            return oss.str();
        }

    }
}
