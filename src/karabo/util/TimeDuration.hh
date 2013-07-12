/* 
 * File:   TimeDuration.hh
 * Author: boukhelef
 *
 * Created on April 27, 2013, 10:55 PM
 */

#ifndef TIMEDURATION_HH
#define	TIMEDURATION_HH

#include <stdint.h>
#include <string>
#include <sstream>
#include <karabo/util/Hash.hh>

namespace karabo {
    namespace util {

        enum TIME_UNITS {

            // Fractions
            //            MILLISEC = 1000ULL * SECOND,
            //            MICROSEC = 1000ULL * MILLISEC,
            //            NANOSEC = 1000ULL * MICROSEC,
            //            PICOSEC = 1000ULL * NANOSEC,
            //            FEMTOSEC = 1000ULL * PICOSEC,
            //            ATTOSEC = 1000ULL * FEMTOSEC
            ATTOSEC = 1ULL, // Atto-second is the smallest time unit = highest resolution for time values.
            FEMTOSEC = 1000ULL * ATTOSEC,
            PICOSEC = 1000ULL * FEMTOSEC,
            NANOSEC = 1000ULL * PICOSEC,
            MICROSEC = 1000ULL * NANOSEC,
            MILLISEC = 1000ULL * MICROSEC,
            ONESECOND = 1000ULL * MILLISEC,

            SECOND = 1ULL, // Base unit
            // Multiples
            MINUTE = 60ULL * SECOND,
            HOUR = 60ULL * MINUTE,
            DAY = 24ULL * HOUR
        };

        typedef unsigned long long TimeValue;

        class TimeDuration {

        public:
            TimeDuration();
            TimeDuration(const karabo::util::Hash& hash);
            TimeDuration(const TimeValue seconds, const TimeValue fractions);
            TimeDuration(const int days, const int hours, const int minutes, const TimeValue seconds, const TimeValue fractions);
            virtual ~TimeDuration();

            TimeDuration& set(const TimeValue seconds, const TimeValue fractions);
            TimeDuration& set(const int days, const int hours, const int minutes, const TimeValue seconds, const TimeValue fractions);

            TimeDuration& add(const TimeValue seconds, const TimeValue fractions);
            TimeDuration& add(const int days, const int hours, const int minutes, const TimeValue seconds, const TimeValue fractions);

            TimeDuration& sub(const TimeValue seconds, const TimeValue fractions);
            TimeDuration& sub(const int days, const int hours, const int minutes, const TimeValue seconds, const TimeValue fractions);

            bool operator ==(const TimeDuration& other) const;
            bool operator !=(const TimeDuration& other) const;
            bool operator>(const TimeDuration& other) const;
            bool operator >=(const TimeDuration& other) const;
            bool operator<(const TimeDuration& other) const;
            bool operator <=(const TimeDuration& other) const;

            TimeDuration operator +(const TimeDuration& period) const;
            TimeDuration operator -(const TimeDuration& period) const;
            TimeDuration& operator +=(const TimeDuration& period);
            TimeDuration& operator -=(const TimeDuration& period);

            uint64_t getDays() const;

            uint64_t getHours() const;
            TimeValue getTotalHours() const;

            uint64_t getMinutes() const;
            TimeValue getTotalMinutes() const;

            TimeValue getSeconds() const;
            TimeValue getTotalSeconds() const;

            TimeValue getFractions(const TIME_UNITS unit = NANOSEC) const;

            std::string format(const std::string& fmt) const;
            friend std::ostream& operator <<(std::ostream& os, const TimeDuration& dr);

            static void setDefaultFormat(const std::string& fmt);

            void fromHash(const karabo::util::Hash& hash);
            void toHash(karabo::util::Hash& hash);

            operator karabo::util::Hash() {
                karabo::util::Hash hash;
                toHash(hash);
                return hash;
            }
        private:
            TimeValue m_Seconds;
            TimeValue m_Fractions;
            static std::string DEFAULT_FORMAT;
        };

    }
}

namespace karabo {
    namespace util {

        inline bool TimeDuration::operator ==(const TimeDuration& other) const {
            return (m_Seconds == other.m_Seconds) && (m_Fractions == other.m_Fractions);
        }

        inline bool TimeDuration::operator !=(const TimeDuration& other) const {
            return (m_Seconds != other.m_Seconds) || (m_Fractions != other.m_Fractions);
        }

        inline bool TimeDuration::operator>(const TimeDuration& other) const {
            return (m_Seconds > other.m_Seconds) ||
                    ((m_Seconds == other.m_Seconds) && (m_Fractions > other.m_Fractions));
        }

        inline bool TimeDuration::operator >=(const TimeDuration& other) const {
            return (m_Seconds >= other.m_Seconds) ||
                    ((m_Seconds == other.m_Seconds) && (m_Fractions >= other.m_Fractions));
        }

        inline bool TimeDuration::operator<(const TimeDuration& other) const {
            return (m_Seconds < other.m_Seconds) ||
                    ((m_Seconds == other.m_Seconds) && (m_Fractions < other.m_Fractions));
        }

        inline bool TimeDuration::operator <=(const TimeDuration& other) const {
            return (m_Seconds <= other.m_Seconds) ||
                    ((m_Seconds == other.m_Seconds) && (m_Fractions <= other.m_Fractions));
        }

        inline TimeDuration TimeDuration::operator +(const TimeDuration& period) const {
            TimeDuration result(*this);
            result += period;
            return result;
        }

        inline TimeDuration TimeDuration::operator -(const TimeDuration& period) const {
            TimeDuration result(*this);
            result -= period;
            return result;
        }

        inline TimeDuration& TimeDuration::operator +=(const TimeDuration& period) {
            m_Seconds += period.m_Seconds;
            m_Fractions += period.m_Fractions;

            if (m_Fractions > ATTOSEC) {
                ++m_Seconds;
                m_Fractions -= ATTOSEC;
            }

            //m_Seconds += m_Fractions / ATTOSEC;
            //m_Fractions %= ATTOSEC;

            return *this;
        }

        inline TimeDuration& TimeDuration::operator -=(const TimeDuration& period) {
            m_Seconds -= period.m_Seconds;

            if (m_Fractions < period.m_Fractions) {
                m_Fractions = (ATTOSEC + m_Fractions) - period.m_Fractions;
                --m_Seconds;
            } else {
                m_Fractions -= period.m_Fractions;
            }

            //m_Seconds += m_Fractions / ATTOSEC;
            //m_Fractions %= ATTOSEC;

            return *this;
        }

        inline void TimeDuration::setDefaultFormat(const std::string& fmt) {
            DEFAULT_FORMAT = fmt;
        }

        inline std::ostream& operator <<(std::ostream& os, const TimeDuration& dr) {
            return os << dr.format(TimeDuration::DEFAULT_FORMAT);
        }


    }
}

#endif	/* TIMEDURATION_HH */

