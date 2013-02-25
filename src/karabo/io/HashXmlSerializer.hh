/* 
 * File:   HashXmlSerializer.hh
 * Author: <burkhard.heisen@xsmail.com>
 * 
 * Created on February 21, 2013, 8:42 AM
 *
 */

#ifndef HASHXMLSERIALIZER_H
#define	HASHXMLSERIALIZER_H

#include <karabo/pugiXml/pugixml.hpp>
#include <karabo/util/Hash.hh>

#include "TextSerializer.hh"

namespace karabo {

    namespace io {

        class HashXmlSerializer : public TextSerializer<karabo::util::Hash> {
            bool _writeDataTypes;
            std::string _indentation;
            bool _writeCompact;

        public:

            KARABO_CLASSINFO(HashXmlSerializer, "Xml", "1.0")

            HashXmlSerializer(const karabo::util::Hash& hash);

            void save(const karabo::util::Hash& object, std::string& archive);

            void load(karabo::util::Hash& object, const std::string& archive);

            virtual ~HashXmlSerializer() {
            };

        private:

            void createXml(const karabo::util::Hash& object, pugi::xml_node& node) const;

            void createHash(karabo::util::Hash& object, pugi::xml_node node) const;

            std::pair<std::string, karabo::util::Types::ReferenceType> decodeXmlAttribute(const std::string& xmlAttribute) const;

            struct CustomWriter : public pugi::xml_writer {
                std::string& _result;

                CustomWriter(std::string & archive) : _result(archive) {
                }

                void write(const void* data, const size_t nBytes) {
                    _result += std::string(static_cast<const char*> (data), nBytes);
                }
            };

        };
    }
}
#endif	

