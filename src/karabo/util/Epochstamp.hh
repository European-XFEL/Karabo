/* 
 * File:   EpochStamp.hh
 * Author: WP76
 *
 * Created on June 19, 2013, 3:22 PM
 */

#ifndef KARABO_UTIL_EPOCHSTAMP_HH
#define	KARABO_UTIL_EPOCHSTAMP_HH

#include "Hash.hh"


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

            unsigned long long m_seconds;

            unsigned long long m_fraction;

        public:

            /**
             * The default constructor will use the current time
             */
            Epochstamp();
            
            inline const unsigned long long& getSeconds() const {
                return m_seconds;
            }
            
            inline const unsigned long long& getFraction() const {
                return m_fraction;
            }

            /**
             * Creates an EpochStamp from an ISO 8601 formatted string
             * @param timePoint ISO 8601 formatted string
             * @return EpochStamp object
             */
            static Epochstamp fromIso8601(const std::string& timePoint);

            /**
             * Creates an EpochStamp from two Hash attributes
             * This function throws in case the attributes do no provide the correct information
             * @param attributes Hash attributes
             * @return EpochStamp object
             */
            static Epochstamp fromHashAttributes(const Hash::Attributes attributes);

            /**
             * Formats into ISO 8601
             * @return ISO 8601 formatted string
             */
            std::string toIso8601() const;

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


            virtual ~Epochstamp();


        private:
            
        };
    }
}

#endif

