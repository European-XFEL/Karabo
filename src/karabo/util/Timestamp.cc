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


        const Epochstamp Timestamp::fromIso8601(const std::string& timePoint) {
            return karabo::util::Epochstamp::fromIso8601(timePoint);
        }


        const Epochstamp Timestamp::fromIso8601Ext(const std::string& timePoint) {
            return karabo::util::Epochstamp::fromIso8601Ext(timePoint);
        }


        std::string Timestamp::toIso8601(TIME_UNITS precision, bool extended) const {
            return m_epochstamp.toIso8601(precision, extended);
        }


        std::string Timestamp::toIso8601Ext(TIME_UNITS precision, bool extended) const {
            return m_epochstamp.toIso8601Ext(precision, extended);
        }


        std::string Timestamp::toFormattedString(const std::string& format) const {
            return m_epochstamp.toFormattedString(format);
        }


        void Timestamp::toHashAttributes(Hash::Attributes& attributes) const {
            m_epochstamp.toHashAttributes(attributes);
            m_trainstamp.toHashAttributes(attributes);
        }
    }
}

