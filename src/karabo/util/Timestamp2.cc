/* 
 * File:   Timestamp2.cc
 * Author: WP76
 * 
 * Created on June 19, 2013, 3:22 PM
 */

#include "Timestamp2.hh"

namespace karabo {
    namespace util {


        Timestamp2::Timestamp2() : m_epochstamp(Epochstamp()), m_trainstamp(Trainstamp()) {
        }


        Timestamp2::Timestamp2(const Epochstamp& e, const Trainstamp& t) : m_epochstamp(e), m_trainstamp(t) {
        }


        Timestamp2::~Timestamp2() {
        }


        bool Timestamp2::hashAttributesContainTimeInformation(const Hash::Attributes attributes) {
            return (Epochstamp::hashAttributesContainTimeInformation(attributes) && Trainstamp::hashAttributesContainTimeInformation(attributes));
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
