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
 * File:   TimeDuration.hh
 * Author: boukhelef
 *
 * Created on April 27, 2013, 10:55 PM
 */

#ifndef TIMEDURATION_HH
#define TIMEDURATION_HH

#include <stdint.h>

#include <sstream>
#include <string>

#include "karabo/data/types/Hash.hh"

namespace karabo {
    namespace data {

        enum TIME_UNITS {


            // Fractions == the number of zeros after 1
            ATTOSEC = 0, // Atto-second is the smallest time unit = highest resolution for time values.
            FEMTOSEC = 3 + ATTOSEC,
            PICOSEC = 3 + FEMTOSEC,
            NANOSEC = 3 + PICOSEC,
            MICROSEC = 3 + NANOSEC,
            MILLISEC = 3 + MICROSEC,
            ONESECOND = 3 + MILLISEC,
            NOFRACTION = -1,

            SECOND = 1ULL, // Base unit
            // Multiples
            MINUTE = 60ULL * SECOND,
            HOUR = 60ULL * MINUTE,
            DAY = 24ULL * HOUR
        };

        typedef unsigned long long TimeValue;

        /**
         * This class represents the time duration (length) between two time points
         * The value is hold in form of two unsigned 64bit values. The first expresses
         * the total number of seconds, and the seconds the fractional of a second.
         *
         * The default constructor create a time duration of length zero, i.e. empty.
         */

        class TimeDuration {
           public:
            /**
             * Default constructor creates an empty time duration
             */

            TimeDuration();

            /**
             * Constructs a time duration from Hash. Seconds and fractions are stored as
             * unsigned integer 64bits under the keys "seconds" and "fractions", respectively.
             * @param hash Hash object ("seconds", unsigned int 64bits, "fractions", unsigned int 64bits)
             */

            TimeDuration(const karabo::data::Hash& hash);

            /**
             * Construct a time duration from separate seconds and fractions of seconds.
             * @param seconds
             * @param fractions in Atto seconds
             */
            TimeDuration(const TimeValue seconds, const TimeValue fractions);

            /**
             * Construct a time duration that expand over days, hours, ...
             * @param days
             * @param hours
             * @param minutes
             * @param seconds
             * @param fractions
             */
            TimeDuration(const int days, const int hours, const int minutes, const TimeValue seconds,
                         const TimeValue fractions);

            virtual ~TimeDuration();

            /**
             * Set new length of, expand, or shrink a time duration
             * @param days
             * @param hours
             * @param minutes
             * @param seconds
             * @param fractions
             */

            TimeDuration& set(const TimeValue seconds, const TimeValue fractions);
            TimeDuration& set(const int days, const int hours, const int minutes, const TimeValue seconds,
                              const TimeValue fractions);

            TimeDuration& add(const TimeValue seconds, const TimeValue fractions);
            TimeDuration& add(const int days, const int hours, const int minutes, const TimeValue seconds,
                              const TimeValue fractions);

            TimeDuration& sub(const TimeValue seconds, const TimeValue fractions);
            TimeDuration& sub(const int days, const int hours, const int minutes, const TimeValue seconds,
                              const TimeValue fractions);

            /**
             * Relational operations between time duration objects
             * @param other TimeDuration
             * @return bool
             */
            bool operator==(const TimeDuration& other) const;
            bool operator!=(const TimeDuration& other) const;
            bool operator>(const TimeDuration& other) const;
            bool operator>=(const TimeDuration& other) const;
            bool operator<(const TimeDuration& other) const;
            bool operator<=(const TimeDuration& other) const;

            /**
             * Arithmetic operations over time durations
             * @param other TimeDuration
             * @return TimeDuration
             */
            TimeDuration operator+(const TimeDuration& other) const;
            TimeDuration operator-(const TimeDuration& other) const;
            TimeDuration& operator+=(const TimeDuration& other);
            TimeDuration& operator-=(const TimeDuration& other);

            /**
             * Create a multiple of the TimeDuration by multiplication with an unsigned, i.e. non-negative number.
             * @param factor that the time duration should be multiplied with
             * @return the new TimeDuration
             */
            TimeDuration operator*(TimeValue factor) const;
            /**
             * Multiply by an unsigned, i.e. non-negative number.
             * @param factor that the time duration should be multiplied with
             * @return reference to *this after multiplication
             */
            TimeDuration& operator*=(TimeValue factor);

            /**
             * These multiplications with a signed value create a linker error: TimeDuration cannot be negative.
             */
            TimeDuration operator*(long long illegalSignedFactor) const;
            TimeDuration& operator*=(long long illegalSignedFactor);

            /**
             * Division, i.e. how many times one time duration is bigger/smaller than another one
             * @param other TimeDuration
             * @return long double
             */
            long double operator/(const TimeDuration& other) const;

            operator double() const {
                // Double has a precision of 16 digits, so e.g. for 1 sec + 1 attosec the attosec is lost.
                return getTotalSeconds() + (getFractions(ATTOSEC) * 1.e-18);
            }

            /**
             * Check if the time duration is empty, i.e. of zero length.
             * @return bool
             */

            bool isNull() const;

            /**
             * In the standard mixed format DD::HH::MM::SS::FF,
             * How many days, hours, minutes, and seconds are in the time duration.
             * @return unsigned int 64bits
             */
            uint64_t getDays() const;
            uint64_t getHours() const;
            uint64_t getMinutes() const;
            TimeValue getSeconds() const;

            /**
             * Express the total length of a time duration in hours, minutes, or seconds.
             * @return unsigned int 64bits
             */
            TimeValue getTotalHours() const;
            TimeValue getTotalMinutes() const;
            TimeValue getTotalSeconds() const;

            /**
             * Number of fractions of a second, default resolution is Nano seconds
             * @return unsigned int 64bits
             */
            TimeValue getFractions(const TIME_UNITS unit = NANOSEC) const;

            /**
             * Serialize time duration to string/std::ostream
             * @param format
             * @return string/ostream object holding the string representation of the time period
             */
            std::string format(const std::string& fmt) const;
            friend std::ostream& operator<<(std::ostream& os, const TimeDuration& dr);

            /**
             * Set the output format. This parameter is class variable,
             * thus, having a global effect on future output.
             * The default format is seconds[dot]fractions, with nanoseconds resolution.
             * CAVEAT: 'seconds' is the total number of seconds modulo 60,
             *         i.e. anything that goes beyond a full minute is ignored!
             *
             * @param format string object
             */
            static void setDefaultFormat(const std::string& fmt);

            /**
             * Serialize time duration to and from Hash
             * @param Hash
             */
            void fromHash(const karabo::data::Hash& hash);
            void toHash(karabo::data::Hash& hash);

            operator karabo::data::Hash() {
                karabo::data::Hash hash;
                toHash(hash);
                return hash;
            }

            /// Sanitize, i.e. take care that 'frac' is below 1 second and adjust 'sec' accordingly.
            static inline void sanitize(TimeValue& sec, TimeValue& frac);
            /// One second expressed in attoseconds
            static const TimeValue m_oneSecondInAtto = 1'000'000'000'000'000'000ULL;

           private:
            TimeValue m_Seconds;
            TimeValue m_Fractions;
            static std::string DEFAULT_FORMAT;
        };

    } // namespace data
} // namespace karabo

namespace karabo {
    namespace data {

        inline bool TimeDuration::operator==(const TimeDuration& other) const {
            return (m_Seconds == other.m_Seconds) && (m_Fractions == other.m_Fractions);
        }

        inline bool TimeDuration::operator!=(const TimeDuration& other) const {
            return (m_Seconds != other.m_Seconds) || (m_Fractions != other.m_Fractions);
        }

        inline bool TimeDuration::operator>(const TimeDuration& other) const {
            return (m_Seconds > other.m_Seconds) ||
                   ((m_Seconds == other.m_Seconds) && (m_Fractions > other.m_Fractions));
        }

        inline bool TimeDuration::operator>=(const TimeDuration& other) const {
            return !(*this < other);
        }

        inline bool TimeDuration::operator<(const TimeDuration& other) const {
            return (m_Seconds < other.m_Seconds) ||
                   ((m_Seconds == other.m_Seconds) && (m_Fractions < other.m_Fractions));
        }

        inline bool TimeDuration::operator<=(const TimeDuration& other) const {
            return !(*this > other);
        }

        inline TimeDuration TimeDuration::operator+(const TimeDuration& other) const {
            TimeDuration result(*this);
            result += other;
            return result;
        }

        inline TimeDuration TimeDuration::operator-(const TimeDuration& other) const {
            TimeDuration result(*this);
            result -= other;
            return result;
        }

        inline TimeDuration TimeDuration::operator*(TimeValue factor) const {
            TimeDuration result(*this);
            result *= factor;
            return result;
        }

        void non_implemented_since_TimeDuration_cannot_be_multiplied_with_signed_factor();

        // These fake implementations of operator*/operator*=(long long) must stay in header:
        // Only when used, a linker error is produced.
        inline TimeDuration& TimeDuration::operator*=(long long illegalSignedFactor) {
            non_implemented_since_TimeDuration_cannot_be_multiplied_with_signed_factor();
            return *this;
        }

        inline TimeDuration TimeDuration::operator*(long long illegalSignedFactor) const {
            non_implemented_since_TimeDuration_cannot_be_multiplied_with_signed_factor();
            return TimeDuration();
        }

        inline long double TimeDuration::operator/(const TimeDuration& other) const {
            if (other.isNull()) return std::numeric_limits<long double>::quiet_NaN();

            long double len_this = m_Seconds + m_Fractions * 1e-18L;
            long double len_other = other.m_Seconds + other.m_Fractions * 1e-18L;

            return len_this / len_other;
        }

        inline TimeDuration& TimeDuration::operator+=(const TimeDuration& other) {
            m_Seconds += other.m_Seconds;
            m_Fractions += other.m_Fractions;
            if (m_Fractions >= m_oneSecondInAtto) {
                ++m_Seconds;
                m_Fractions -= m_oneSecondInAtto;
            }

            return *this;
        }

        inline TimeDuration& TimeDuration::operator-=(const TimeDuration& other) {
            m_Seconds -= other.m_Seconds;
            if (m_Fractions < other.m_Fractions) {
                m_Fractions = (m_oneSecondInAtto - other.m_Fractions) + m_Fractions;
                --m_Seconds;
            } else {
                m_Fractions -= other.m_Fractions;
            }

            return *this;
        }


        inline void TimeDuration::setDefaultFormat(const std::string& fmt) {
            DEFAULT_FORMAT = fmt;
        }

        inline std::ostream& operator<<(std::ostream& os, const TimeDuration& duration) {
            return os << duration.format(TimeDuration::DEFAULT_FORMAT);
        }

        inline void TimeDuration::sanitize(TimeValue& sec, TimeValue& frac) {
            if (frac >= m_oneSecondInAtto) {
                // Add full seconds to sec and assign the rest to frac
                sec += frac / m_oneSecondInAtto;
                frac %= m_oneSecondInAtto;
            }
        }
    } // namespace data
} // namespace karabo

#endif /* TIMEDURATION_HH */
