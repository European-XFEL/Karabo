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


        Trainstamp Trainstamp::fromHashAttributes(const Hash::Attributes attributes) {
            throw KARABO_NOT_IMPLEMENTED_EXCEPTION("To be done by BH");
        }
        
        void Trainstamp::toHashAttributes(Hash::Attributes& attributes) const {
            throw KARABO_NOT_IMPLEMENTED_EXCEPTION("To be done by BH");
        }
    }
}
