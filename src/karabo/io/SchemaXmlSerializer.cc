/*
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */
/*
 * File:   SchemaXmlSerializer.cc
 * Author: <burkhard.heisen@xsmail.com>
 *
 * Created on February 21, 2013, 8:42 AM
 *
 */

#include "SchemaXmlSerializer.hh"

#include "HashXmlSerializer.hh"
#include "karabo/data/schema/Configurator.hh"

using namespace karabo::data;
using namespace std;

KARABO_EXPLICIT_TEMPLATE(karabo::io::TextSerializer<karabo::data::Schema>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::TextSerializer<karabo::data::Schema>, karabo::io::SchemaXmlSerializer)

namespace karabo {
    namespace io {

        void SchemaXmlSerializer::expectedParameters(karabo::data::Schema& expected) {
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
