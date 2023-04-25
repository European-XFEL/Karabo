/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   SchemaXmlSerializer.hh
 * Author: <burkhard.heisen@xsmail.com>
 *
 * Created on February 21, 2013, 8:42 AM
 *
 */

#ifndef _KARABO_IO_SCHEMA_XML_SERIALIZER_HH
#define _KARABO_IO_SCHEMA_XML_SERIALIZER_HH

#include <karabo/util/Schema.hh>

#include "TextSerializer.hh"

namespace karabo {

    namespace io {

        /**
         * @class SchemaXmlSerializer
         * @brief The SchemaXmlSerializer provides an implementation of TextSerializer
         *        for the karabo::util::Hash
         */
        class SchemaXmlSerializer : public TextSerializer<karabo::util::Schema> {
            TextSerializer<karabo::util::Hash>::Pointer m_serializer;

           public:
            KARABO_CLASSINFO(SchemaXmlSerializer, "Xml", "1.0")

            static void expectedParameters(karabo::util::Schema& expected);

            SchemaXmlSerializer(const karabo::util::Hash& hash);

            void save(const karabo::util::Schema& object, std::string& archive);

            void load(karabo::util::Schema& object, const std::string& archive);

            virtual ~SchemaXmlSerializer() {}
        };
    } // namespace io
} // namespace karabo

KARABO_REGISTER_CONFIGURATION_BASE_CLASS(karabo::io::TextSerializer<karabo::util::Schema>)

#endif
