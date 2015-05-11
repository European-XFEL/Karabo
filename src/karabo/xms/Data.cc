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

            // HASH_ELEMENT(expected).key("header")

            //           UINT32_ELEMENT(expected).key("id")
            //                    .description("The ID of this data token")
            //                    .displayedName("ID")
            //                    .readOnly()
            //                    .commit();
            //            
            //            

            // Creation date

            // Flow (for provenance)
        }


        /**
         * This constructor should be used for filling the data object before sending
         */
        Data::Data() : m_hash(new Hash()) {
        }


        Data::Data(const karabo::util::Hash& config) : m_hash(new Hash(config)) {
        }

        Data::Data(const std::string& key, const Data& other) : m_hash(new Hash(key, other.hash())) {
        }

        Data::Data(const std::string& channelName, const karabo::util::Hash& config) {
            if (!config.has(channelName)) {
                throw KARABO_PARAMETER_EXCEPTION("The provided configuration must contain the channel name as key in the configuration");
            }
            m_hash = Hash::Pointer(new Hash(config.get<Hash>(channelName + ".schema")));
        }


        Data::Data(const karabo::util::Hash::Pointer& hash) : m_hash(hash) {
        }


        Data::Data(const Data& other) {
            m_hash = Hash::Pointer(new Hash(*other.m_hash));
        }


        Data::~Data() {
        }
        
        
        void Data::setNode(const std::string& key, const Data& data) {
            m_hash->set(key, data.hash());
        }


        const karabo::util::Hash::Pointer& Data::hash() const {
            return m_hash;
        }
        
        void Data::attachTimestamp(const karabo::util::Timestamp& ts) {
            for (Hash::iterator it = m_hash->begin(); it != m_hash->end(); ++it) {
                ts.toHashAttributes(it->getAttributes());
            }
        }



        std::ostream& operator<<(std::ostream& os, const Data& data) {
            try {
                cout << *data.m_hash;
            } catch (...) {
                KARABO_RETHROW;
            }
            return os;
        }

    }
}
