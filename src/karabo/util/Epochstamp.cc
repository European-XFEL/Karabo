/*
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */
/*
 * File:   EpochStamp.cc
 * Author: WP76
 *
 * Created on June 19, 2013, 3:22 PM
 */

#include "Epochstamp.hh"

#include <boost/algorithm/string/trim.hpp>
#include <boost/format.hpp>

#include "DateTimeString.hh"
// Check level of <chrono> library support in compiler
#if __GNUC__ < 13

#include <date/tz.h>
using date::floor;
using date::format;
using date::January;
using date::sys_days;
using hours = std::chrono::duration<long int, std::ratio<3600>>;
using minutes = std::chrono::duration<long int, std::ratio<60>>;
using seconds = std::chrono::duration<long int>;

#else

#include <chrono>
#include <format> // std::format, std::vformat,...
using std::chrono::floor;
using std::chrono::hours;
using std::chrono::January;
using std::chrono::minutes;
using std::chrono::seconds;
using std::chrono::sys_days;

#endif

namespace karabo {
    namespace util {


        Epochstamp::Epochstamp() {
            now();
        }


        Epochstamp::Epochstamp(const unsigned long long& seconds, const unsigned long long& fractions)
            : m_seconds(seconds), m_fractionalSeconds(fractions) {}


        Epochstamp::~Epochstamp() {}


        Epochstamp::Epochstamp(const time_t& tm) : m_seconds(tm), m_fractionalSeconds(0) {}


        Epochstamp::Epochstamp(const timeval& tv)
            : m_seconds(tv.tv_sec), m_fractionalSeconds(tv.tv_usec * 1'000'000'000'000ULL) {}


        Epochstamp::Epochstamp(const timespec& ts)
            : m_seconds(ts.tv_sec), m_fractionalSeconds(ts.tv_nsec * 1'000'000'000ULL) {}


        Epochstamp::Epochstamp(const std::string& pTime) {
            karabo::util::DateTimeString dts = karabo::util::DateTimeString(pTime);
            m_seconds = dts.getSecondsSinceEpoch();
            m_fractionalSeconds = dts.getFractionalSeconds<unsigned long long>();
        }


        TimeDuration Epochstamp::elapsed(const Epochstamp& other) const {
            if (*this < other) return other - *this;
            else return *this - other;
        }


        time_t Epochstamp::getTime() const {
            return m_seconds;
        }


        timeval Epochstamp::getTimeOfDay() const {
            timeval result = {long(m_seconds), long(m_fractionalSeconds / 1'000'000'000'000ULL)};
            return result;
        }


        timespec Epochstamp::getClockTime() const {
            timespec result = {long(m_seconds), long(m_fractionalSeconds / 1'000'000'000ULL)};
            return result;
        }


        // retrieve the current time


        void Epochstamp::now() {
            // C++ 20 API: function 'now()' returns with precision nanoseconds
            auto now = std::chrono::system_clock::now();
            // truncate up to second's precision
            auto secs = floor<seconds>(now).time_since_epoch();
            m_seconds = secs.count();
            m_fractionalSeconds = (now.time_since_epoch() - secs).count();
            m_fractionalSeconds *= 1'000'000'000ULL; // attosecond's precision
        }


        std::string Epochstamp::toIso8601(TIME_UNITS precision, bool extended) const {
            return this->toIso8601Internal(precision, extended, "");
        }


        std::string Epochstamp::toIso8601Ext(TIME_UNITS precision, bool extended) const {
            return this->toIso8601Internal(precision, extended, "Z");
        }


        std::string Epochstamp::toIso8601Internal(TIME_UNITS precision, bool extended,
                                                  const std::string& locZone) const {
            auto secondsSinceEpoch = sys_days(January / 1 / 1970) + seconds(m_seconds);
            std::chrono::time_point<std::chrono::system_clock, seconds> utcTimePoint(secondsSinceEpoch);

            std::string dateTime;

#if __GNUC__ < 13
            std::string fmt = extended ? "%Y-%m-%dT%H:%M:%S" : "%Y%m%dT%H%M%S";
            dateTime = format(fmt, utcTimePoint);
#else
            std::string fmt = extended ? "{0:%Y-%m-%dT%H:%M:%S}" : "{0:%Y%m%dT%H%M%S}";
            dateTime = std::vformat(fmt, std::make_format_args(utcTimePoint));
#endif

            std::string dateTimeWithFractional =
                  this->concatDateTimeWithFractional<std::string, std::string&>(dateTime, precision);
            if (!locZone.empty()) dateTimeWithFractional += locZone;

            return dateTimeWithFractional;
        }


        template <typename To, typename PT1>
        const To Epochstamp::concatDateTimeWithFractional(const PT1 dateTime, const TIME_UNITS precision) const {
            std::ostringstream oss;
            oss << dateTime << karabo::util::DateTimeString::fractionalSecondToString(precision, m_fractionalSeconds);
            return boost::lexical_cast<To>(oss.str());
        }


        const double Epochstamp::toTimestamp() const {
            return this->concatDateTimeWithFractional<double, unsigned long long>(m_seconds, MICROSEC);
        }


        std::string Epochstamp::toFormattedString(const std::string& fmt, const std::string& localTimeZone) const {
            if (fmt.empty()) return this->toFormattedStringInternal("", "%Y-%b-%d %H:%M:%S", localTimeZone);
            return this->toFormattedStringInternal("", fmt, localTimeZone);
        }


        std::string Epochstamp::toFormattedStringLocale(const std::string& localeName, const std::string& fmt,
                                                        const std::string& localTimeZone) const {
            if (fmt.empty()) return this->toFormattedStringInternal(localeName, "%Y-%b-%d %H:%M:%S", localTimeZone);
            return this->toFormattedStringInternal(localeName, fmt, localTimeZone);
        }


        std::string Epochstamp::toFormattedStringInternal(const std::string& localeName, const std::string& fm,
                                                          const std::string& localTimeZone) const {
            using namespace karabo::util;

            assert(!fm.empty());
            auto secondsSinceEpoch = sys_days(January / 1 / 1970) + seconds(m_seconds);
            std::chrono::time_point<std::chrono::system_clock, seconds> timePoint(secondsSinceEpoch);

            Hash tz = DateTimeString::getTimeDurationFromTimeZone(localTimeZone);
            auto zSign = tz.get<std::string>("timeZoneSignal");
            auto zHours = hours(tz.get<int>("timeZoneHours"));
            auto zMinutes = minutes(tz.get<int>("timeZoneMinutes"));

            if (zSign == "+") timePoint += (zHours + zMinutes);
            if (zSign == "-") timePoint -= (zHours + zMinutes);

            std::string dateTime;
            std::string fmt;
#if __GNUC__ < 13
            if (localeName.empty()) {
                fmt = fm;
                dateTime = format(fmt, timePoint);
            } else {
                std::locale loc(localeName);
                fmt = fm;
                dateTime = format(loc, fmt, timePoint);
            }
#else
            if (localeName.empty()) {
                fmt = std::string("{0:") + fm + "}";
                dateTime = std::vformat(fmt, std::make_format_args(timePoint));
            } else {
                std::locale loc(localeName);
                fmt = std::string("{0:L") + fm + "}";
                dateTime = std::vformat(loc, fmt, std::make_format_args(timePoint));
            }
#endif
            // %S (or %T) : If they are not used in format then NO fractional seconds in output
            bool noFractional = (fmt.find("%S") == std::string::npos && fmt.find("%T") == std::string::npos);
            noFractional = noFractional || m_fractionalSeconds == 0ULL;
            if (!noFractional) {
                dateTime += karabo::util::DateTimeString::fractionalSecondToString(MICROSEC, m_fractionalSeconds);
            }

            return dateTime;
        }


        bool Epochstamp::hashAttributesContainTimeInformation(const Hash::Attributes& attributes) {
            return (attributes.has("sec") && attributes.has("frac"));
        }


        Epochstamp Epochstamp::fromHashAttributes(const Hash::Attributes& attributes) {
            unsigned long long seconds, fraction;

            try {
                auto& sec_element = attributes.getNode("sec");
                seconds = sec_element.getValue<decltype(seconds), long long, unsigned int, int>();

                auto& frac_element = attributes.getNode("frac");
                fraction = frac_element.getValue<decltype(fraction), long long, unsigned int, int>();

            } catch (const Exception& e) {
                KARABO_RETHROW_AS(
                      KARABO_PARAMETER_EXCEPTION("Provided attributes do not contain proper timestamp information"));
            }
            return Epochstamp(seconds, fraction);
        }


        void Epochstamp::toHashAttributes(Hash::Attributes& attributes) const {
            attributes.set("sec", m_seconds);
            attributes.set("frac", m_fractionalSeconds);
        }


        std::ostream& operator<<(std::ostream& output, const Epochstamp& stamp) {
            // Full technical precision of 18 digits for seconds since of unix epoch
            std::string txt((boost::format("%d.%018d") % stamp.getSeconds() % stamp.getFractionalSeconds()).str());
            // Now remove trailing zero:
            boost::algorithm::trim_right_if(txt, [](char c) { return c == '0'; });
            output << txt << " s";
            return output;
        }

        bool operator==(const Epochstamp& lhs, const Epochstamp& rhs) {
            return (lhs.m_fractionalSeconds == rhs.m_fractionalSeconds) && (lhs.m_seconds == rhs.m_seconds);
        }

        bool operator!=(const Epochstamp& lhs, const Epochstamp& rhs) {
            return (lhs.m_fractionalSeconds != rhs.m_fractionalSeconds) || (lhs.m_seconds != rhs.m_seconds);
        }

        bool operator>(const Epochstamp& lhs, const Epochstamp& rhs) {
            return (lhs.m_seconds > rhs.m_seconds) ||
                   ((lhs.m_seconds == rhs.m_seconds) && (lhs.m_fractionalSeconds > rhs.m_fractionalSeconds));
        }

        bool operator>=(const Epochstamp& lhs, const Epochstamp& rhs) {
            return (lhs.m_seconds > rhs.m_seconds) ||
                   ((lhs.m_seconds == rhs.m_seconds) && (lhs.m_fractionalSeconds >= rhs.m_fractionalSeconds));
        }

        bool operator<(const Epochstamp& lhs, const Epochstamp& rhs) {
            return (lhs.m_seconds < rhs.m_seconds) ||
                   ((lhs.m_seconds == rhs.m_seconds) && (lhs.m_fractionalSeconds < rhs.m_fractionalSeconds));
        }

        bool operator<=(const Epochstamp& lhs, const Epochstamp& rhs) {
            return (lhs.m_seconds < rhs.m_seconds) ||
                   ((lhs.m_seconds == rhs.m_seconds) && (lhs.m_fractionalSeconds <= rhs.m_fractionalSeconds));
        }

    } // namespace util
} // namespace karabo
