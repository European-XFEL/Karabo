/* 
 * File:   TrainStamp.cc
 * Author: WP76
 * 
 * Created on June 19, 2013, 3:22 PM
 */

#include "Trainstamp.hh"

namespace karabo {
    namespace util {


        Trainstamp::Trainstamp() : m_trainId(0) {
        }


        Trainstamp::Trainstamp(const unsigned long long trainId) : m_trainId(trainId) {
        }


        Trainstamp::~Trainstamp() {
        }


        bool Trainstamp::hashAttributesContainTimeInformation(const Hash::Attributes attributes) {
            return attributes.has("tid");
        }


        Trainstamp Trainstamp::fromHashAttributes(const Hash::Attributes& attributes) {
            unsigned long long tid;
            try {
                attributes.get("tid", tid);
            } catch (const Exception& e) {
                KARABO_RETHROW_AS(KARABO_PARAMETER_EXCEPTION("Provided attributes do not contain proper trainId information"));
            }
            return Trainstamp(tid);
        }


        void Trainstamp::toHashAttributes(Hash::Attributes& attributes) const {
            attributes.set("tid", m_trainId);
        }
    }
}
