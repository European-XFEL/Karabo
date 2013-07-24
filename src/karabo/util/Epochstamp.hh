/* 
 * File:   EpochStamp.hh
 * Author: WP76
 *
 * Created on June 19, 2013, 3:22 PM
 */

#ifndef KARABO_UTIL_EPOCHSTAMP_HH
#define	KARABO_UTIL_EPOCHSTAMP_HH

#include <boost/date_time.hpp>
#include "Hash.hh"
#include "TimeDuration.hh"

namespace karabo {
    namespace util {

        /**
         * This class expresses a time point and holds it in form of two unsigned 64bit values.
         * The first expressing the elapsed seconds since unix epoch and the second expressing
         * the elapsed atto-seconds since the current second.
         * 
         * The default constructor will initialize this object with the current time.
         * To initialize using an arbitrary point in time the static from functions must be used (e.g. fromIso8601)
         * 
         * 
         */
        class Epochstamp {

            // Number of seconds since 1st of January of 1970
            unsigned long long m_seconds;

            // An attosecond is an SI unit of time equal to 10−18 of a second.
            unsigned long long m_fractionalSeconds;

        public:

            /**
             * The default constructor will use the current time
             */
            Epochstamp();

            /**
             * Constructor from seconds and fraction
             * @param seconds seconds past since the unix epoch
             * @param fraction attoSeconds past since the second under consideration
             */
            Epochstamp(const unsigned long long& seconds, const unsigned long long& fractions);

            explicit Epochstamp(const time_t& tm);
            explicit Epochstamp(const timeval& tv);
            explicit Epochstamp(const timespec& ts);

            virtual ~Epochstamp();

            inline const unsigned long long& getSeconds() const {
                return m_seconds;
            }

            inline const unsigned long long& getFractionalSeconds() const {
                return m_fractionalSeconds;
            }

            Epochstamp& operator =(const time_t& tm);
            Epochstamp& operator =(const timeval& tv);
            Epochstamp& operator =(const timespec& ts);

            bool operator ==(const Epochstamp& other) const;
            bool operator !=(const Epochstamp& other) const;
            bool operator>(const Epochstamp& other) const;
            bool operator >=(const Epochstamp& other) const;
            bool operator<(const Epochstamp& other) const;
            bool operator <=(const Epochstamp& other) const;

            Epochstamp operator +(const TimeDuration& period) const;
            Epochstamp operator -(const TimeDuration& period) const;
            Epochstamp& operator +=(const TimeDuration& period);
            Epochstamp& operator -=(const TimeDuration& period);

            TimeDuration operator -(const Epochstamp& other) const;

            Epochstamp operator ++(int);
            Epochstamp operator --(int);
            Epochstamp& operator ++();
            Epochstamp& operator --();

            time_t getTime() const; // Unix time_t, Resolution = seconds
            timeval getTimeOfDay() const; // Resolution u-sec
            timespec getClockTime() const; // Resolution nano-sec

            void now();
            TimeDuration elpased(const Epochstamp& other = Epochstamp()) const;

            /**
             * Creates an EpochStamp from an ISO 8601 formatted string
             * @param timePoint ISO 8601 formatted string
             * @return EpochStamp object
             */
            static Epochstamp fromIso8601(const std::string& timePoint);

            /**
             * Generates a sting (respecting ISO-8601) for a Timestamp ("ISO 8601"  => "%Y-%m-%dT%H:%M:%S.%f%q")
             * @return ISO 8601 formatted string
             */
            std::string toIso8601(TIME_UNITS precision = MICROSEC, bool extended = false) const;


            static bool hashAttributesContainTimeInformation(const Hash::Attributes attributes);

            /**
             * Creates an EpochStamp from two Hash attributes
             * This function throws in case the attributes do no provide the correct information
             * @param attributes Hash attributes
             * @return EpochStamp object
             */
            static Epochstamp fromHashAttributes(const Hash::Attributes attributes);

            /**
             * Formats as Hash attributes
             * @param attributes container to which the time point information is added
             */
            void toHashAttributes(Hash::Attributes& attributes) const;

            /**
             * Formats to specified format
             * @param format The format of the time point
             * @return formated string
             */
            std::string toFormattedString(const std::string& format = "%Y-%b-%d %H:%M:%S") const;

        private:

            /**
             * Converts fractionalSeconds (attoseconds) into another unit measure
             * @return long long with the desire converted value
             */
            unsigned long long convertFractionalSeconds(std::string destinyUnitMeasure) const;

        };
    }
}



namespace karabo {
    namespace util {

        //
        // inlines
        //

        inline Epochstamp& Epochstamp::operator =(const time_t& tm) {
            m_seconds = static_cast<uint64_t> (tm);
            m_fractionalSeconds = 0ULL;
            return *this;
        }

        inline Epochstamp& Epochstamp::operator =(const timeval& tv) {
            m_seconds = tv.tv_sec;
            m_fractionalSeconds = tv.tv_usec * MICROSEC;
            return *this;
        }

        inline Epochstamp& Epochstamp::operator =(const timespec& ts) {
            m_seconds = ts.tv_sec;
            m_fractionalSeconds = ts.tv_nsec * NANOSEC;
            return *this;
        }

        inline bool Epochstamp::operator ==(const Epochstamp& other) const {
            return (this->m_fractionalSeconds == other.m_fractionalSeconds) && (this->m_seconds == other.m_seconds);
        }

        inline bool Epochstamp::operator !=(const Epochstamp& other) const {
            return !(*this == other);
        }

        inline bool Epochstamp::operator>(const Epochstamp& other) const {
            return (this->m_seconds > other.m_seconds) || ((this->m_seconds == other.m_seconds) && (this->m_fractionalSeconds > other.m_fractionalSeconds));
        }

        inline bool Epochstamp::operator >=(const Epochstamp& other) const {
            return (*this == other) || (*this > other);
        }

        inline bool Epochstamp::operator<(const Epochstamp& other) const {
            return !(*this >= other);
        }

        inline bool Epochstamp::operator <=(const Epochstamp& other) const {
            return !(*this > other);
        }

        inline TimeDuration Epochstamp::operator -(const Epochstamp& other) const {
            TimeDuration td;
            if (m_fractionalSeconds < other.m_fractionalSeconds) {
                return TimeDuration(m_seconds - other.m_seconds - 1ULL, (ONESECOND - other.m_fractionalSeconds) + m_fractionalSeconds);
            } else {
                return TimeDuration(m_seconds - other.m_seconds, m_fractionalSeconds - other.m_fractionalSeconds);
            }
        }

        inline Epochstamp Epochstamp::operator +(const TimeDuration& period) const {
            Epochstamp result(*this);
            result += period;
            return result;
        }

        inline Epochstamp Epochstamp::operator -(const TimeDuration& period) const {
            Epochstamp result(*this);
            result -= period;
            return result;
        }

        inline Epochstamp& Epochstamp::operator +=(const TimeDuration& period) {
            this->m_seconds += period.getTotalSeconds();
            if ((this->m_fractionalSeconds += period.getFractions(ATTOSEC)) > ONESECOND) {
                this->m_fractionalSeconds -= ONESECOND;
                ++this->m_seconds;
            };
            return *this;
        }

        inline Epochstamp& Epochstamp::operator -=(const TimeDuration& period) {
            this->m_seconds -= period.getTotalSeconds();
            if (this->m_fractionalSeconds < period.getFractions()) {
                this->m_fractionalSeconds = (ONESECOND - period.getFractions(ATTOSEC)) + m_fractionalSeconds;
                --this->m_seconds;
            };
            return *this;
        }

        inline Epochstamp Epochstamp::operator ++(int) {
            Epochstamp result(*this);
            ++ * this;
            return result;
        }

        inline Epochstamp Epochstamp::operator --(int) {
            Epochstamp result(*this);
            -- * this;
            return result;
        }

        inline Epochstamp& Epochstamp::operator ++() {
            ++ * this;
            return *this;
        }

        inline Epochstamp& Epochstamp::operator --() {
            -- * this;
            return *this;
        }


    }
}

#endif

