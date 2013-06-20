/* 
 * File:   EpochStamp.cc
 * Author: WP76
 * 
 * Created on June 19, 2013, 3:22 PM
 */

#include "Epochstamp.hh"

namespace karabo {
    namespace util {


        Epochstamp::Epochstamp() {
            throw KARABO_NOT_IMPLEMENTED_EXCEPTION("To be done by DB");
        }


        Epochstamp::~Epochstamp() {
        }

        Epochstamp Epochstamp::fromIso8601(const std::string& timePoint) {
            throw KARABO_NOT_IMPLEMENTED_EXCEPTION("To be done by LM");
        }


        Epochstamp Epochstamp::fromHashAttributes(const Hash::Attributes attributes) {
            throw KARABO_NOT_IMPLEMENTED_EXCEPTION("To be done by BH");
        }


        std::string Epochstamp::toIso8601() const {
            throw KARABO_NOT_IMPLEMENTED_EXCEPTION("To be done by LM");
        }


        void Epochstamp::toHashAttributes(Hash::Attributes& attributes) const {
            throw KARABO_NOT_IMPLEMENTED_EXCEPTION("To be done by BH");
        }
        
        std::string Epochstamp::toFormattedString(const std::string& format) const {
            throw KARABO_NOT_IMPLEMENTED_EXCEPTION("To be done");
        }
    }
}