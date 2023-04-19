/*
 * File:   ByteArrayElement.hh
 *
 * Created on September 2, 2016
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */


#ifndef KARABO_UTIL_BYTEARRAYELEMENT_HH
#define KARABO_UTIL_BYTEARRAYELEMENT_HH

#include "LeafElement.hh"

namespace karabo {
    namespace util {

        class ByteArrayElement : public LeafElement<ByteArrayElement, ByteArray> {
           public:
            ByteArrayElement(Schema& expected) : LeafElement<ByteArrayElement, ByteArray>(expected) {}

           protected:
            void beforeAddition() {
                m_node->template setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, Schema::LEAF);
                m_node->template setAttribute<int>(KARABO_SCHEMA_LEAF_TYPE, karabo::util::Schema::PROPERTY);
                m_node->setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, "ByteArray");
                m_node->setAttribute(KARABO_SCHEMA_VALUE_TYPE, Types::to<ToLiteral>(Types::from<ByteArray>()));

                m_node->setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, READ);
                m_node->setAttribute<int>(KARABO_SCHEMA_ASSIGNMENT, Schema::OPTIONAL_PARAM);
                m_node->setAttribute<int>(KARABO_SCHEMA_ARCHIVE_POLICY, Schema::EVERY_EVENT);

                const UnitType unit = Unit::NOT_ASSIGNED;
                std::pair<std::string, std::string> names = getUnit(unit);
                m_node->template setAttribute<int>(KARABO_SCHEMA_UNIT_ENUM, unit);
                m_node->setAttribute(KARABO_SCHEMA_UNIT_NAME, names.first);
                m_node->setAttribute(KARABO_SCHEMA_UNIT_SYMBOL, names.second);

                const MetricPrefixType metricPrefix = MetricPrefix::NONE;
                names = getMetricPrefix(metricPrefix);
                m_node->template setAttribute<int>(KARABO_SCHEMA_METRIC_PREFIX_ENUM, metricPrefix);
                m_node->setAttribute(KARABO_SCHEMA_METRIC_PREFIX_NAME, names.first);
                m_node->setAttribute(KARABO_SCHEMA_METRIC_PREFIX_SYMBOL, names.second);
            }
        };

        typedef ByteArrayElement BYTEARRAY_ELEMENT;
    } // namespace util
} // namespace karabo

#endif
