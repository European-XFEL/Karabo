/* 
 * File:   SchemaXmlSerializer.cc
 * Author: <burkhard.heisen@xsmail.com>
 *
 * Created on February 21, 2013, 8:42 AM
 *
 */

#include <karabo/util/util.hh>
#include "HashXmlSerializer.hh"

#include "SchemaXmlSerializer.hh"
#include "karabo/tests/io/HashXmlSerializer_Test.hh"

using namespace karabo::util;
using namespace std;

namespace karabo {
    namespace io {


        KARABO_REGISTER_FOR_CONFIGURATION(TextSerializer<Schema>, SchemaXmlSerializer)


        void SchemaXmlSerializer::expectedParameters(karabo::util::Schema& expected) {

            HashXmlSerializer::expectedParameters(expected);
        }


        SchemaXmlSerializer::SchemaXmlSerializer(const Hash& input) {
            m_serializer = TextSerializer<Hash>::create("Xml", input);
        }


        void SchemaXmlSerializer::save(const Schema& object, std::string& archive) {
            archive = object.getRootName() + ":";
            m_serializer->save(object.getParameterHash(), archive);
        }
        
        void SchemaXmlSerializer::load(Schema& object, const std::string& archive) {
            size_t pos = archive.find_first_of(':');
            string rootName = archive.substr(0, pos);
            string hashArchive = archive.substr(pos + 1);
            Hash hash; 
            m_serializer->load(hash, hashArchive);
            object.setRootName(rootName);
            object.setParameterHash(hash);
        }
    }
}
