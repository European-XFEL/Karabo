/* 
 * File:   Timestamp2.hh
 * Author: WP76
 *
 * Created on June 19, 2013, 3:22 PM
 */

#ifndef KARABO_UTIL_TIMESTAMP2_HH
#define	KARABO_UTIL_TIMESTAMP2_HH

#include "Epochstamp.hh"
#include "Trainstamp.hh"


namespace karabo {
    namespace util {

        /**
         * This class expresses a time point and holds it in form of an Epochstamp and Trainstamp
         */
        class Timestamp2 {

            Epochstamp m_epochstamp;
            Trainstamp m_trainstamp;

        public:

            
            Timestamp2();
                
            Timestamp2(const Epochstamp& e, const Trainstamp& t);
            
            inline const unsigned long long& getSeconds() const {
                return m_epochstamp.getSeconds();
            }
            
            inline const unsigned long long& getFractionalSeconds() const {
                 return m_epochstamp.getFractionalSeconds();
            }
            
            inline const unsigned long long& getTrainId() const {
                return m_trainstamp.getTrainId();
            }
            
            static bool hashAttributesContainTimeInformation(const Hash::Attributes attributes);
           
            /**
             * Creates an Timestamp2 from three Hash attributes
             * This function throws in case the attributes do no provide the correct information
             * @param attributes Hash attributes
             * @return Timestamp2 object
             */
            static Timestamp2 fromHashAttributes(const Hash::Attributes attributes);

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


            virtual ~Timestamp2();


        private:
            
        };
    }
}

#endif

