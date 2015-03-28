/* 
 * Author: 
 * 
 * Created on March 23, 2015, 10:17 AM
 */

#include "Data.hh"

#include <karabo/util.hpp>

using namespace karabo::util;

namespace karabo {
  namespace xms {
        
        void Data::expectedParameters(karabo::util::Schema& expected) {
            
            UINT32_ELEMENT(expected).key("id")
                    .description("The ID of this data token")
                    .displayedName("ID")
                    .assignmentOptional().noDefaultValue()
                    .commit();
            
            // Creation date
            
            // Flow (for provenance)
        }

        /**
         * This constructor should be used for filling the data object before sending
         */
        Data::Data() : m_hash(new Hash()) {

        }
        
        /**
         * This constructor should be used during if data was received from network
         * TODO: Improve the Configurator factory to call constructors with Hash::Pointer
         * TODO: Improve the validator for copy-free validation
         * NOTE: For the time being an expensive copy is made here
        **/
        Data::Data(const karabo::util::Hash& config) : m_hash(new Hash(config)) {

        }
        
        void Data::setId(unsigned int i) {
            m_hash->set("id", i);
        }
                
        const karabo::util::Hash::Pointer& Data::getHashPointer() const {
            return m_hash;
        }
        
        
        
    }
}
