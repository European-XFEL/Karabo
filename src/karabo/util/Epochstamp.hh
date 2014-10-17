/* 
 * File:   EpochStamp.hh
 * Author: WP76
 *
 * Created on June 19, 2013, 3:22 PM
 */

#ifndef KARABO_UTIL_EPOCHSTAMP_HH
#define	KARABO_UTIL_EPOCHSTAMP_HH

#include <boost/date_time.hpp>
#include <boost/regex.hpp>
#include "Hash.hh"
#include "TimeDuration.hh"

namespace karabo {
    namespace util {

        /**
         * This class expresses a point in time and holds its value in the form of two unsigned 64bit.
         * The first expresses the total number of seconds elapsed since Unix epoch, i.e. 1970-01-01 00:00:00.00.
         * The second expresses the number fractional of a seconds, in atto seconds, since the last time unit (in seconds).
         * 
         * The default constructor initializes a Epochstamp object with the current system time.
         * To initialize using an arbitrary point in time the static from functions must be used (e.g. DateTimeString class)
         * 
         * 
         */
        class Epochstamp {
            // Number of seconds since hour 00:00 of the 1st of January, 1970
            unsigned long long m_seconds;

            // An attosecond is an SI unit of time equal to 10^−18 of a second.
            unsigned long long m_fractionalSeconds;

        public:

            /**
             * The default constructor creates a timestamps using the current time
             */
            Epochstamp();

            /**
             * Constructor from seconds and fraction
             * @param seconds seconds past since the Unix epoch
             * @param fraction attoSeconds past since the second under consideration
             */
            Epochstamp(const unsigned long long& seconds, const unsigned long long& fractions);

            /**
             * Constructor from implicit conversion from other system time structures, 
             * @param time_t
             * @param timeval
             * @param timespec
             */
            explicit Epochstamp(const time_t& tm);
            explicit Epochstamp(const timeval& tv);
            explicit Epochstamp(const timespec& ts);
            explicit Epochstamp(const std::string& pTime);

            virtual ~Epochstamp();

            /**
             * Get the second (resp. fractional of a second) part, 
             * @return unsigned int 64bits
             */
            inline const unsigned long long& getSeconds() const {
                return m_seconds;
            }

            inline const unsigned long long& getFractionalSeconds() const {
                return m_fractionalSeconds;
            }

            /**
             * Portability operators to convert other system time structures to Epochstamp
             * @param tm time_t value
             * @param tv timeval struct
             * @param ts timespec struct
             */
            Epochstamp& operator =(const time_t& tm);
            Epochstamp& operator =(const timeval& tv);
            Epochstamp& operator =(const timespec& ts);

            /**
             * Relational operations between timestamps
             * @param other Epochstamp object
             * @return bool
             */
            bool operator ==(const Epochstamp& other) const;
            bool operator !=(const Epochstamp& other) const;
            bool operator>(const Epochstamp& other) const;
            bool operator >=(const Epochstamp& other) const;
            bool operator<(const Epochstamp& other) const;
            bool operator <=(const Epochstamp& other) const;

            /**
             * Move a timestamp forward/backward with given time distance, 
             * expressed in terms of time duration
             * @param duration TimeDuration object
             * @return Epochstamp object
             */
            Epochstamp operator +(const TimeDuration& duration) const;
            Epochstamp operator -(const TimeDuration& duration) const;
            Epochstamp& operator +=(const TimeDuration& duration);
            Epochstamp& operator -=(const TimeDuration& duration);

            /**
             * Time duration is the difference between two timestamps
             * @param other Epochstamp object
             * @return TimeDuration object
             */
            TimeDuration operator -(const Epochstamp& other) const;

            /**
             * Auto increment/decrement operators
             */
            Epochstamp operator ++(int);
            Epochstamp operator --(int);
            Epochstamp& operator ++();
            Epochstamp& operator --();

            /**
             * Portability functions to convert Epochstamp to other system time structures
             * @return time_t value
             * @return timeval struct
             * @return timespec struct
             */
            time_t getTime() const; // Unix time_t, Resolution = seconds
            timeval getTimeOfDay() const; // Resolution u-sec
            timespec getClockTime() const; // Resolution nano-sec

            /**
             * Retrieve current time for the system. Highest resolution is Nano-seconds
             */
            void now();

            /**
             * Calculate elapsed time duration between two timestamps
             * @param other Epochstamp object (by default, current time point)
             * @return TimeDuration object
             */
            TimeDuration elapsed(const Epochstamp& other = Epochstamp()) const;


            /**
             * Generates a sting (respecting ISO-8601) for object time for INTERNAL usage ("%Y%m%dT%H%M%S%f" => "20121225T132536.789333[123456789123]")
             * 
             * @param precision - Indicates the precision of the fractional seconds (e.g. MILLISEC, MICROSEC, NANOSEC, PICOSEC, FEMTOSEC, ATTOSEC) [Default: MICROSEC]
             * @param extended - "true" returns ISO8601 extended string; "false" returns ISO8601 compact string [Default: false]
             * @return ISO 8601 formatted string (extended or compact)
             */
            std::string toIso8601(TIME_UNITS precision = MICROSEC, bool extended = false) const;

            /**
             * Generates a sting (respecting ISO-8601) for object time for EXTERNAL usage ("%Y%m%dT%H%M%S%f%z" => "20121225T132536.789333[123456789123]Z")
             * 
             * @param precision - Indicates the precision of the fractional seconds (e.g. MILLISEC, MICROSEC, NANOSEC, PICOSEC, FEMTOSEC, ATTOSEC) [Default: MICROSEC]
             * @param extended - "true" returns ISO8601 extended string; "false" returns ISO8601 compact string [Default: false]
             * @return ISO 8601 formatted string with "Z" in the string end ("Z" means the date time zone is using Coordinated Universal Time - UTC)
             */
            std::string toIso8601Ext(TIME_UNITS precision = MICROSEC, bool extended = false) const;


            /**
             * Generates a timestamp as double with seconds.fractional format (fractional precision == MICROSEC)
             * Function necessary to use in graphs plotting in Python code (MICROSEC precision is enough)
             * 
             * @return A double value with the decimal point indicating fractions of seconds
             */
            const double toTimestamp() const;

            /**
             * Formats to specified format time stored in the object
             * 
             * @param format The format of the time point (visit strftime for more info: http://www.cplusplus.com/reference/ctime/strftime/) [Default: "%Y-%b-%d %H:%M:%S"]
             * @param localTimeZone - String that represents an ISO8601 time zone [Default: "Z" == UTC]
             * @return formated string in the specified Time Zone
             */
            std::string toFormattedString(const std::string& format = std::string("%Y-%b-%d %H:%M:%S"), const std::string& localTimeZone = std::string("Z")) const;

            /**
             * Formats to specified format time stored in the object
             * 
             * @param localeName - String that represents the locale to be used [Default: "" == System locale]
             * @param format The format of the time point (visit strftime for more info: http://www.cplusplus.com/reference/ctime/strftime/) [Default: "%Y-%b-%d %H:%M:%S"]
             * @param localTimeZone - String that represents an ISO8601 time zone [Default: "Z" == UTC]
             * @return formated string in the specified Time Zone
             */
            std::string toFormattedStringLocale(const std::string& localeName = std::string(""),
                    const std::string& format = std::string("%Y-%b-%d %H:%M:%S"),
                    const std::string& localTimeZone = std::string("Z")) const;


            static bool hashAttributesContainTimeInformation(const Hash::Attributes& attributes);

            /**
             * Creates an EpochStamp from two Hash attributes
             * This function throws in case the attributes do no provide the correct information
             * @param attributes Hash attributes
             * @return EpochStamp object
             */
            static Epochstamp fromHashAttributes(const Hash::Attributes& attributes);

            /**
             * Formats as Hash attributes
             * @param attributes container to which the time point information is added
             */
            void toHashAttributes(Hash::Attributes& attributes) const;

        private:

            /**
             * Returns timestamp string in "ANY SPECIFIED" format (format must be a valid format)
             * 
             * @param pt Boost ptime of a specific moment in time
             * @param facet Boost time_facet to be applied
             * @param localeName - String that represents the locale to be used
             * @return The specified date/time formatted according to the specified time_facet
             */
            static std::string getPTime2String(const boost::posix_time::ptime pt,
                    const boost::posix_time::time_facet* facet,
                    const std::string& localeName);

            /**
             * Formats to specified format time stored in the object
             * 
             * @param localeName - String that represents the locale to be used [Default: "" == System locale]
             * @param format The format of the time point (visit strftime for more info: http://www.cplusplus.com/reference/ctime/strftime/) [Default: "%Y-%b-%d %H:%M:%S"]
             * @param localTimeZone - String that represents an ISO8601 time zone [Default: "Z" == UTC]
             * @return formated string in the specified Time Zone
             */
            std::string toFormattedStringInternal(const std::string& localeName,
                    const std::string& format,
                    const std::string& localTimeZone) const;

            /**
             * Generates a sting (respecting ISO-8601) for object time for INTERNAL usage ("%Y%m%dT%H%M%S%f" => "20121225T132536.789333[123456789123]")
             * 
             * @param precision - Indicates the precision of the fractional seconds (e.g. MILLISEC, MICROSEC, NANOSEC, PICOSEC, FEMTOSEC, ATTOSEC) [Default: MICROSEC]
             * @param extended - "true" returns ISO8601 extended string; "false" returns ISO8601 compact string [Default: false]
             * @param localTimeZone - String that represents an ISO8601 time zone [Default: "Z" == UTC]
             * @return ISO 8601 formatted string (extended or compact)
             */
            std::string toIso8601Internal(TIME_UNITS precision = MICROSEC, bool extended = false, const std::string& localTimeZone = std::string("Z")) const;


            /**
             * Concatenates date and time information with instance fractional seconds within a specified precision
             * 
             * @param dateTime - Date and time information
             * @param precision - Indicates the precision of the fractional seconds (e.g. MILLISEC, MICROSEC, NANOSEC, PICOSEC, FEMTOSEC, ATTOSEC) [Default: MICROSEC]
             * @return Concatenation result
             */
            template<typename To, typename PT1>
            const To concatDateTimeWithFractional(const PT1 dateTime, const TIME_UNITS precision) const;

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
            m_fractionalSeconds = static_cast<unsigned long long> (ts.tv_nsec) * NANOSEC;
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

        inline Epochstamp Epochstamp::operator +(const TimeDuration& duration) const {
            Epochstamp result(*this);
            result += duration;
            return result;
        }

        inline Epochstamp Epochstamp::operator -(const TimeDuration& duration) const {
            Epochstamp result(*this);
            result -= duration;
            return result;
        }

        inline Epochstamp& Epochstamp::operator +=(const TimeDuration& duration) {
            this->m_seconds += duration.getTotalSeconds();
            if ((this->m_fractionalSeconds += duration.getFractions(ATTOSEC)) > ONESECOND) {
                this->m_fractionalSeconds -= ONESECOND;
                ++this->m_seconds;
            };
            return *this;
        }

        inline Epochstamp& Epochstamp::operator -=(const TimeDuration& duration) {
            // TODO: Sergey--> Fix the case attosecond
            this->m_seconds -= duration.getTotalSeconds();
            if (this->m_fractionalSeconds < duration.getFractions()) {
                this->m_fractionalSeconds = (ONESECOND - duration.getFractions(ATTOSEC)) + this->m_fractionalSeconds;
                --this->m_seconds;
            } else {
                this->m_fractionalSeconds -= duration.getFractions();
            }
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

