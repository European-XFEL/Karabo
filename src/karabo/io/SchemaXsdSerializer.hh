/*
 * $Id: SchemaXsdSerializer.hh 6608 2012-06-25 19:41:16Z wrona $
 *
 * File:   SchemaXsdSerializer.hh
 * Author: <irina.kozlova@xfel.eu>
 *
 * Created on March 12, 2013, 10:31 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_IO_XSDSERIALIZER_HH
#define	KARABO_IO_XSDSERIALIZER_HH

#include <karabo/pugiXml/pugixml.hpp>
#include <boost/foreach.hpp>

#include "TextSerializer.hh"

namespace karabo {

    namespace io {

/**
         * The SchemaXsdSerializer class
         */
        class SchemaXsdSerializer : public TextSerializer<karabo::util::Schema> {


            std::string m_indentation;
            std::string m_defaultNamespace;
            std::string m_xmlnsa;
            bool m_writeCompact;

        public:

            KARABO_CLASSINFO(SchemaXsdSerializer, "Xsd", "1.0")

            static void expectedParameters(karabo::util::Schema& expected);

            SchemaXsdSerializer(const karabo::util::Hash& input);

            virtual ~SchemaXsdSerializer() {
            };

            void load(karabo::util::Schema& object, const std::string& archive);

            void save(const karabo::util::Schema& object, std::string& archive);

        private: // functions

            void r_createXsd(const karabo::util::Schema& schema, pugi::xml_node& node, const bool isChildNodeOfListElement, const std::string& key = "") const;

            void appendAttributes(const karabo::util::Schema& schema, const std::string& key, pugi::xml_node& node) const;

            void leafToXsd(const karabo::util::Schema& schema, const std::string& key, pugi::xml_node& node) const;

            void choiceOfNodesToXsd(const karabo::util::Schema& schema, const std::string& key, pugi::xml_node& node) const;

            void listOfNodesToXsd(const karabo::util::Schema& schema, const std::string& key, pugi::xml_node& node) const;

            void createDocumentationNode(const karabo::util::Schema& schema, const std::string& key, pugi::xml_node& node, const bool isVector = false) const;

            bool annotationExists(const karabo::util::Schema& schema, const std::string& key) const;

            std::string extractKey(const std::string& key) const;

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
