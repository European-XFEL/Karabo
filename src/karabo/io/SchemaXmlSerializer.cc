/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   SchemaXmlSerializer.cc
 * Author: <burkhard.heisen@xsmail.com>
 *
 * Created on February 21, 2013, 8:42 AM
 *
 */

#include "SchemaXmlSerializer.hh"

#include <karabo/util/Configurator.hh>

#include "HashXmlSerializer.hh"

using namespace karabo::util;
using namespace std;

KARABO_EXPLICIT_TEMPLATE(karabo::io::TextSerializer<karabo::util::Schema>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::TextSerializer<karabo::util::Schema>, karabo::io::SchemaXmlSerializer)

namespace karabo {
    namespace io {

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
            object.setParameterHash(std::move(hash));
            object.updateAliasMap();
        }
    } // namespace io
} // namespace karabo
