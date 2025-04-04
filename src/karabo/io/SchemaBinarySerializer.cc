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
 * File:   SchemaBinarySerializer.cc
 * Author: <djelloul.boukhelef@xfel.eu>
 *
 * Created on July 9, 2013, 8:59 AM
 */
#include "karabo/data/schema/Configurator.hh"
// #include "HashBinarySerializer.hh"
#include "SchemaBinarySerializer.hh"

using namespace std;
using namespace karabo::data;

KARABO_EXPLICIT_TEMPLATE(karabo::io::BinarySerializer<karabo::data::Schema>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::BinarySerializer<Schema>, karabo::io::SchemaBinarySerializer)

namespace karabo {
    namespace io {

        SchemaBinarySerializer::SchemaBinarySerializer(const Hash& input) {
            m_serializer = BinarySerializer<Hash>::create("Bin", input);
        }


        SchemaBinarySerializer::~SchemaBinarySerializer() {}


        void SchemaBinarySerializer::expectedParameters(karabo::data::Schema& expected) {}

        void SchemaBinarySerializer::save(const karabo::data::Schema& object, std::vector<char>& archive) {
            ostringstream os;
            const string& rootName = object.getRootName();
            const unsigned char size = rootName.size();
            os.write((char*)&size, sizeof(size));
            os.write(rootName.c_str(), size);

            string str = os.str();
            archive.insert(archive.end(), str.c_str(), str.c_str() + str.length());
            m_serializer->save2(object.getParameterHash(), archive); // save2(..) just appends to 'archive'!
        }

        void SchemaBinarySerializer::save2(const karabo::data::Schema& object, std::vector<char>& archive) {
            save(object, archive);
        }

        size_t SchemaBinarySerializer::load(karabo::data::Schema& object, const char* archive, const size_t nBytes) {
            std::stringstream is;
            is.rdbuf()->pubsetbuf(const_cast<char*>(archive), nBytes);

            unsigned char size;
            is.read((char*)&size, sizeof(size));
            char rootName[256];
            is.read(rootName, size);
            rootName[size] = 0;
            object.setRootName(rootName);
            Hash hash;
            size_t bytes = m_serializer->load(hash, archive + sizeof(size) + size, nBytes - sizeof(size) - size);
            object.setParameterHash(std::move(hash));
            object.updateAliasMap();
            return bytes + sizeof(size) + size_t(size);
        }
    } // namespace io
} // namespace karabo
