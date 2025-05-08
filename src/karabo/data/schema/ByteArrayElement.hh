/*
 * File:   ByteArrayElement.hh
 *
 * Created on September 2, 2016
 *
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


#ifndef KARABO_DATA_SCHEMA_BYTEARRAYELEMENT_HH
#define KARABO_DATA_SCHEMA_BYTEARRAYELEMENT_HH

#include "LeafElement.hh"

namespace karabo {
    namespace data {

        class ByteArrayElement : public LeafElement<ByteArrayElement, ByteArray> {
           public:
            ByteArrayElement(Schema& expected) : LeafElement<ByteArrayElement, ByteArray>(expected) {}

           protected:
            void beforeAddition() {
                m_node->template setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, Schema::LEAF);
                m_node->setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, "ByteArray");
                m_node->setAttribute(KARABO_SCHEMA_VALUE_TYPE, Types::to<ToLiteral>(Types::from<ByteArray>()));

                m_node->setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, READ);
                m_node->setAttribute<int>(KARABO_SCHEMA_ASSIGNMENT, Schema::OPTIONAL_PARAM);
                m_node->setAttribute<int>(KARABO_SCHEMA_ARCHIVE_POLICY, Schema::EVERY_EVENT);

                const Unit unit = Unit::NOT_ASSIGNED;
                std::pair<std::string, std::string> names = getUnit(unit);
                m_node->template setAttribute<int>(KARABO_SCHEMA_UNIT_ENUM, static_cast<int>(unit));
                m_node->setAttribute(KARABO_SCHEMA_UNIT_NAME, names.first);
                m_node->setAttribute(KARABO_SCHEMA_UNIT_SYMBOL, names.second);

                const MetricPrefix metricPrefix = MetricPrefix::NONE;
                names = getMetricPrefix(metricPrefix);
                m_node->template setAttribute<int>(KARABO_SCHEMA_METRIC_PREFIX_ENUM, static_cast<int>(metricPrefix));
                m_node->setAttribute(KARABO_SCHEMA_METRIC_PREFIX_NAME, names.first);
                m_node->setAttribute(KARABO_SCHEMA_METRIC_PREFIX_SYMBOL, names.second);
            }
        };

        typedef ByteArrayElement BYTEARRAY_ELEMENT;
    } // namespace data
} // namespace karabo

#endif
