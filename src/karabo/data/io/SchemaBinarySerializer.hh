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
 * File:   SchemaBinarySerializer.hh
 * Author: <djelloul.boukhelef@xfel.eu>
 *
 * Created on July 9, 2013, 8:58 AM
 */

#ifndef KARABO_DATA_IO_SCHEMABINARYSERIALIZER_HH
#define KARABO_DATA_IO_SCHEMABINARYSERIALIZER_HH

#include "BinarySerializer.hh"
#include "karabo/data/types/Schema.hh"

namespace karabo {

    namespace data {

        /**
         * @class SchemaBinarySerializer
         * @brief The SchemaBinarySerializer provides an implementation of BinarySerializer
         *        for the karabo::data::Schema
         */
        class SchemaBinarySerializer : public BinarySerializer<karabo::data::Schema> {
            BinarySerializer<karabo::data::Hash>::Pointer m_serializer;

           public:
            KARABO_CLASSINFO(SchemaBinarySerializer, "Bin", "1.0")

            SchemaBinarySerializer(const karabo::data::Hash& hash);
            virtual ~SchemaBinarySerializer();

            static void expectedParameters(karabo::data::Schema& expected);

            /**
             * Save a Schema by appending it to a binary archive
             * @param object to save
             * @param archive to append to - no clear() called
             */
            void save(const karabo::data::Schema& object, std::vector<char>& archive);

            /**
             * Save a Schema by appending it to a binary archive
             * @param object to save
             * @param archive to append to - no clear() called
             */
            void save2(const karabo::data::Schema& object, std::vector<char>& archive);

            size_t load(karabo::data::Schema& object, const char* archive, const size_t nBytes);
        };
    } // namespace data
} // namespace karabo

KARABO_REGISTER_CONFIGURATION_BASE_CLASS(karabo::data::BinarySerializer<karabo::data::Schema>)

#endif /* SCHEMABINARYSERIALIZER_HH */
