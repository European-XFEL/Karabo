/* 
 * File:   EpochStamp.cc
 * Author: WP76
 * 
 * Created on June 19, 2013, 3:22 PM
 */

#include "Epochstamp.hh"

namespace karabo {
    namespace util {
        
        // Placeholder for the time until DB has integrated his clock 
        boost::posix_time::ptime Epochstamp::m_epoch =  boost::posix_time::ptime(boost::gregorian::date(1970, 1, 1));


        Epochstamp::Epochstamp() {
            
            // Placeholder for the time until DB has integrated his clock 
            boost::posix_time::microsec_clock::universal_time();
            boost::posix_time::time_duration diff = boost::posix_time::microsec_clock::universal_time() - m_epoch;
            m_seconds = diff.total_seconds();
            m_fractionalSeconds = diff.fractional_seconds();
        }


        Epochstamp::Epochstamp(const unsigned long long& seconds, const unsigned long long& fraction) :
        m_seconds(seconds), m_fractionalSeconds(fraction) {
        }


        Epochstamp::~Epochstamp() {
        }


        Epochstamp Epochstamp::fromIso8601(const std::string& timePoint) {
            throw KARABO_NOT_IMPLEMENTED_EXCEPTION("To be done by LM");
        }


        std::string Epochstamp::toIso8601() const {
            throw KARABO_NOT_IMPLEMENTED_EXCEPTION("To be done by LM");
        }
        
        bool Epochstamp::hashAttributesContainTimeInformation(const Hash::Attributes attributes) {
            return (attributes.has("sec") && attributes.has("frac"));
        }


        Epochstamp Epochstamp::fromHashAttributes(const Hash::Attributes attributes) {
            unsigned long long seconds, fraction;
            try {
                attributes.get("sec", seconds);
                attributes.get("frac", fraction);
            } catch (const Exception& e) {
                KARABO_RETHROW_AS(KARABO_PARAMETER_EXCEPTION("Provided attributes do not contain proper timestamp information"));
            }
            return Epochstamp(seconds, fraction);
        }


        void Epochstamp::toHashAttributes(Hash::Attributes& attributes) const {
            attributes.set("sec", m_seconds);
            attributes.set("frac", m_fractionalSeconds);
        }


        std::string Epochstamp::toFormattedString(const std::string& format) const {
            throw KARABO_NOT_IMPLEMENTED_EXCEPTION("To be done");
        }
    }
}