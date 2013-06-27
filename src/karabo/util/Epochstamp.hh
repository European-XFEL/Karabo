/* 
 * File:   EpochStamp.hh
 * Author: WP76
 *
 * Created on June 19, 2013, 3:22 PM
 */

#ifndef KARABO_UTIL_EPOCHSTAMP_HH
#define	KARABO_UTIL_EPOCHSTAMP_HH

#include "Hash.hh"
#include <boost/date_time.hpp>

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
            
            // Placeholder for the time until DB has integrated his clock 
            static boost::posix_time::ptime m_epoch;
            
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
            Epochstamp(const unsigned long long& seconds, const unsigned long long& fraction);
            
            virtual ~Epochstamp();
            
            
            inline const unsigned long long& getSeconds() const {
                return m_seconds;
            }
            
            inline const unsigned long long& getFractionalSeconds() const {
                return m_fractionalSeconds;
            }

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
            std::string toIso8601() const;
            
            
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

#endif

