/* 
 * File:   Timestamp2.cc
 * Author: WP76
 * 
 * Created on June 19, 2013, 3:22 PM
 */

#include "Timestamp2.hh"

namespace karabo {
    namespace util {


        Timestamp2::Timestamp2(const Epochstamp& e, const Trainstamp& t) : m_epochstamp(e), m_trainstamp(t) {
        }
        
        
        Timestamp2::~Timestamp2() {
        }

        const unsigned long long& Timestamp2::getSeconds() const {
            return m_epochstamp.getSeconds();
        }
        
        const unsigned long long& Timestamp2::getFraction() const {
            return m_epochstamp.getFraction();
        }
        
        const unsigned long long& Timestamp2::getTrainId() const {
            return m_trainstamp.getTrainId();
        }

        
        Timestamp2 Timestamp2::fromHashAttributes(const Hash::Attributes attributes) {
            return Timestamp2(Epochstamp::fromHashAttributes(attributes), Trainstamp::fromHashAttributes(attributes));
        }


        std::string Timestamp2::toIso8601() const {
            return m_epochstamp.toIso8601();
        }


        void Timestamp2::toHashAttributes(Hash::Attributes& attributes) const {
            m_epochstamp.toHashAttributes(attributes);
            m_trainstamp.toHashAttributes(attributes);
        }
        
        std::string Timestamp2::toFormattedString(const std::string& format) const {
            return m_epochstamp.toFormattedString(format);
        }
    }
}
