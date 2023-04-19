/**
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */

#include "AlarmConditionElement.hh"

#include "AlarmConditions.hh"
#include "Schema.hh"

namespace karabo {
    namespace util {


        AlarmConditionElement::AlarmConditionElement(Schema& expected)
            : GenericElement<AlarmConditionElement>(expected) {}


        AlarmConditionElement& AlarmConditionElement::initialValue(const AlarmCondition& a) {
            m_node->setAttribute(KARABO_SCHEMA_DEFAULT_VALUE, a.asString());
            return *this;
        }


        void AlarmConditionElement::beforeAddition() {
            m_node->setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, Schema::LEAF);
            m_node->setAttribute<int>(KARABO_SCHEMA_LEAF_TYPE, Schema::ALARM_CONDITION);
            m_node->setAttribute<std::string>(KARABO_SCHEMA_VALUE_TYPE, ToLiteral::to<Types::STRING>());
            m_node->setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, READ);
            m_node->setAttribute<int>(KARABO_SCHEMA_ASSIGNMENT, Schema::OPTIONAL_PARAM);
            m_node->setAttribute<int>(KARABO_SCHEMA_ARCHIVE_POLICY, Schema::EVERY_EVENT);
            m_node->setAttribute<std::string>(KARABO_SCHEMA_CLASS_ID, "AlarmCondition");
            m_node->setAttribute<std::string>(KARABO_SCHEMA_DISPLAY_TYPE, "AlarmCondition");

            // finally protect setting options etc to alarm element via overwrite
            OverwriteElement::Restrictions restrictions;
            restrictions.options = true;
            restrictions.minInc = true;
            restrictions.minExc = true;
            restrictions.maxInc = true;
            restrictions.maxExc = true;
            restrictions.readOnly = true;
            restrictions.reconfigurable = true;
            restrictions.displayedName = true;
            restrictions.overwriteRestrictions = true;
            m_node->setAttribute(KARABO_OVERWRITE_RESTRICTIONS, restrictions.toVectorAttribute());
        }
    } // namespace util
} // namespace karabo
