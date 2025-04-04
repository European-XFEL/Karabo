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
 * File:   TimeDuration.cc
 * Author: boukhelef
 *
 * Created on April 27, 2013, 10:55 PM
 */

#include "TimeDuration.hh"

#include <boost/multiprecision/cpp_int.hpp>
#include <limits>

namespace karabo {
    namespace data {

        std::string TimeDuration::DEFAULT_FORMAT("%s.%N");


        TimeDuration::TimeDuration() : m_Seconds(0ULL), m_Fractions(0ULL) {}


        TimeDuration::TimeDuration(const karabo::data::Hash& hash) {
            fromHash(hash);
        }


        TimeDuration::TimeDuration(const TimeValue seconds, const TimeValue fractions)
            : m_Seconds(seconds), m_Fractions(fractions) {
            sanitize(m_Seconds, m_Fractions);
        }


        TimeDuration::TimeDuration(const int days, const int hours, const int minutes, const TimeValue seconds,
                                   const TimeValue fractions)
            : m_Seconds(days * DAY + hours * HOUR + minutes * MINUTE + seconds), m_Fractions(fractions) {
            sanitize(m_Seconds, m_Fractions);
        }


        TimeDuration::~TimeDuration() {}


        TimeDuration& TimeDuration::set(const TimeValue seconds, const TimeValue fractions) {
            m_Seconds = seconds;
            m_Fractions = fractions;
            sanitize(m_Seconds, m_Fractions);
            return *this;
        }


        TimeDuration& TimeDuration::set(const int days, const int hours, const int minutes, const TimeValue seconds,
                                        const TimeValue fractions) {
            m_Seconds = days * DAY + hours * HOUR + minutes * MINUTE + seconds;
            m_Fractions = fractions;
            sanitize(m_Seconds, m_Fractions);
            return *this;
        }


        TimeDuration& TimeDuration::add(const TimeValue seconds, const TimeValue fractions) {
            return (*this) += TimeDuration(seconds, fractions);
        }


        TimeDuration& TimeDuration::add(const int days, const int hours, const int minutes, const TimeValue seconds,
                                        const TimeValue fractions) {
            return (*this) += TimeDuration(days * DAY + hours * HOUR + minutes * MINUTE + seconds, fractions);
        }


        TimeDuration& TimeDuration::sub(const TimeValue seconds, const TimeValue fractions) {
            m_Seconds -= seconds;
            if (m_Fractions < fractions) {
                m_Fractions = (m_oneSecondInAtto - fractions) + m_Fractions;
                --m_Seconds;
            } else {
                m_Fractions -= fractions;
            }
            return *this;
        }


        TimeDuration& TimeDuration::sub(const int days, const int hours, const int minutes, const TimeValue seconds,
                                        const TimeValue fractions) {
            m_Seconds -= days * DAY + hours * HOUR + minutes * MINUTE + seconds;
            if (m_Fractions < fractions) {
                m_Fractions = (m_oneSecondInAtto - fractions) + m_Fractions;
                --m_Seconds;
            } else {
                m_Fractions -= fractions;
            }
            return *this;
        }


        bool TimeDuration::isNull() const {
            return (m_Seconds == 0ull) && (m_Fractions == 0ull);
        }


        uint64_t TimeDuration::getDays() const {
            return m_Seconds / DAY;
        }


        uint64_t TimeDuration::getHours() const {
            return (m_Seconds / HOUR) % 24;
        }


        TimeValue TimeDuration::getTotalHours() const {
            return m_Seconds / HOUR;
        }


        uint64_t TimeDuration::getMinutes() const {
            return (m_Seconds / MINUTE) % 60;
        }


        TimeValue TimeDuration::getTotalMinutes() const {
            return m_Seconds / MINUTE;
        }


        TimeValue TimeDuration::getSeconds() const {
            return m_Seconds % 60;
        }


        TimeValue TimeDuration::getTotalSeconds() const {
            return m_Seconds;
        }


        TimeValue TimeDuration::getFractions(const TIME_UNITS unit) const {
            int zeros = int(unit);
            unsigned long long multiplier = 1ULL;
            while (zeros-- > 0) multiplier *= 10ULL;
            return m_Fractions / multiplier; // unsigned long long dividing
        }


        std::string TimeDuration::format(const std::string& fmt) const {
            std::ostringstream oss;
            oss.fill('0');
            for (char const* ptr = fmt.c_str(); *ptr; ++ptr) {
                if (*ptr == '%') {
                    ++ptr;
                    switch (*ptr) {
                        case 'd':
                        case 'D':
                            oss << getDays();
                            break;
                        case 'H':
                            oss.width(2);
                            oss << getHours();
                            break;
                        case 'M':
                            oss.width(2);
                            oss << getMinutes();
                            break;
                        case 'S':
                            oss.width(2);
                            oss << getSeconds();
                            break;
                        case 'h':
                            oss.width(0);
                            oss << getHours();
                            break;
                        case 'm':
                            oss.width(0);
                            oss << getMinutes();
                            break;
                        case 's':
                            oss.width(0);
                            oss << getSeconds();
                            break;
                        default: {
                            int w;
                            TIME_UNITS p = ATTOSEC;
                            switch (*ptr) {
                                case 'l':
                                    w = 0, p = MILLISEC;
                                    break;
                                case 'u':
                                    w = 0, p = MICROSEC;
                                    break;
                                case 'n':
                                    w = 0, p = NANOSEC;
                                    break;
                                case 'p':
                                    w = 0, p = PICOSEC;
                                    break;
                                case 'f':
                                    w = 0, p = FEMTOSEC;
                                    break;
                                case 'a':
                                    w = 0, p = ATTOSEC;
                                    break;
                                case 'L':
                                    w = 3, p = MILLISEC;
                                    break;
                                case 'U':
                                    w = 6, p = MICROSEC;
                                    break;
                                case 'N':
                                    w = 9, p = NANOSEC;
                                    break;
                                case 'P':
                                    w = 12, p = PICOSEC;
                                    break;
                                case 'F':
                                    w = 15, p = FEMTOSEC;
                                    break;
                                case 'A':
                                    w = 18, p = ATTOSEC;
                                    break;
                                default:
                                    throw "Unrecognized format";
                            }
                            oss.width(w);
                            oss << getFractions(p);
                        }
                    }
                } else {
                    oss << *ptr;
                }
            }

            return oss.str();
        }


        void TimeDuration::fromHash(const karabo::data::Hash& hash) {
            m_Seconds = hash.get<unsigned long long>("seconds");
            m_Fractions = hash.get<unsigned long long>("fractions");
            sanitize(m_Seconds, m_Fractions);
        }


        void TimeDuration::toHash(karabo::data::Hash& hash) {
            hash.set<unsigned long long>("seconds", getSeconds());
            hash.set<unsigned long long>("fractions", getFractions(ATTOSEC));
        }


        TimeDuration& TimeDuration::operator*=(TimeValue factor) {
            // Do not care about overflowing seconds - that's billions of years...
            m_Seconds *= factor;

            // Multiplying two large 64-bit values (as TimeValue) might overflow, so get 128-bit result from boost:
            using boost::multiprecision::uint128_t;
            uint128_t fractions128bits;
            boost::multiprecision::multiply(fractions128bits, m_Fractions, factor);
            // Now split it into two 64-bit values, i.e.
            static const uint128_t lower64BitsSet = std::numeric_limits<uint64_t>::max();
            // ...lower part
            const auto fractions64bits = static_cast<unsigned long long>(fractions128bits & lower64BitsSet);
            // and higher part.
            const auto fractions64bitsOverflow = static_cast<unsigned long long>(fractions128bits >> 64ull);

            // Treat non-overflow part...
            m_Fractions = fractions64bits % m_oneSecondInAtto;
            m_Seconds += (fractions64bits / m_oneSecondInAtto);

            // ... and add overflow if needed:
            if (fractions64bitsOverflow) {
                // Calculate duration of single overflow
                TimeValue attos = std::numeric_limits<TimeValue>::max(); // all bits are set, i.e. maximum TimeValue
                                                                         // (which is unsigned)!
                TimeValue seconds = 0ull;
                sanitize(seconds, attos);
                // + 1 since overflow is one more than all 64 bits set
                // (no harm if attos + 1 == 1 second - constructor sanitizes)
                TimeDuration overflow(seconds, attos + 1);
                // Multiply with number of overflows and add
                overflow *= fractions64bitsOverflow; // recursion...
                (*this) += overflow;
            }

            return *this;
        }
    } // namespace data
} // namespace karabo
