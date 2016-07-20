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


            std::string m_indentation;
            std::string m_xmlns;
            std::string m_prefix;
            bool m_writeDataTypes;
            bool m_readDataTypes;
            bool m_insertXmlNamespace;

            // Helper variables
            std::string m_typeFlag;
            std::string m_artificialRootFlag;
            std::string m_itemFlag;
            bool m_writeCompact;

        public:

            KARABO_CLASSINFO(HashXmlSerializer, "Xml", "1.0")

            static void expectedParameters(karabo::util::Schema& expected);

            HashXmlSerializer(const karabo::util::Hash& hash);

            void save(const karabo::util::Hash& object, std::string& archive);

            void load(karabo::util::Hash& object, const std::string& archive);

            void load(karabo::util::Hash& object, const char* archive);

            void save(const std::vector<karabo::util::Hash>& objects, std::string& archive);

            void load(std::vector<karabo::util::Hash>& objects, const std::string& archive);

            virtual ~HashXmlSerializer() {
            };

        private:

            void createXml(const karabo::util::Hash& object, pugi::xml_node& node) const;

            void createHash(karabo::util::Hash& object, pugi::xml_node node) const;

            void writeAttributes(const karabo::util::Hash::Attributes& attrs, pugi::xml_node& node) const;

            void readAttributes(karabo::util::Hash::Attributes& attrs, const pugi::xml_node& node) const;

            std::pair<std::string, karabo::util::Types::ReferenceType> readXmlAttribute(const std::string& xmlAttribute) const;

            struct CustomWriter : public pugi::xml_writer {


                std::string& _result;

                CustomWriter(std::string & archive) : _result(archive) {
                }

                void write(const void* data, const size_t nBytes) {
                    _result += std::string(static_cast<const char*> (data), nBytes);
                }
            };

            std::string escapeElementName(const std::string& data) const;
            std::string unescapeElementName(const std::string& data) const;

        };
    }
}

KARABO_REGISTER_CONFIGURATION_BASE_CLASS(karabo::io::TextSerializer<karabo::util::Hash>)

#endif	

