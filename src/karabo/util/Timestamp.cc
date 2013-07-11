/* 
 * File:   Timestamp.cc
 * Author: WP76
 * 
 * Created on June 19, 2013, 3:22 PM
 */

#include "Timestamp.hh"

namespace karabo {
    namespace util {


        Timestamp::Timestamp() : m_epochstamp(Epochstamp()), m_trainstamp(Trainstamp()) {
        }


        Timestamp::Timestamp(const Epochstamp& e, const Trainstamp& t) : m_epochstamp(e), m_trainstamp(t) {
        }


        Timestamp::~Timestamp() {
        }


        bool Timestamp::hashAttributesContainTimeInformation(const Hash::Attributes attributes) {
            return (Epochstamp::hashAttributesContainTimeInformation(attributes) && Trainstamp::hashAttributesContainTimeInformation(attributes));
        }


        Timestamp Timestamp::fromHashAttributes(const Hash::Attributes attributes) {
            return Timestamp(Epochstamp::fromHashAttributes(attributes), Trainstamp::fromHashAttributes(attributes));
        }


        std::string Timestamp::toIso8601() const {
            return m_epochstamp.toIso8601();
        }


        void Timestamp::toHashAttributes(Hash::Attributes& attributes) const {
            m_epochstamp.toHashAttributes(attributes);
            m_trainstamp.toHashAttributes(attributes);
        }


        std::string Timestamp::toFormattedString(const std::string& format) const {
            return m_epochstamp.toFormattedString(format);
        }
    }
}
