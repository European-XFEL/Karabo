/**
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

#include "AlarmConditionElement.hh"

#include "karabo/data/types/AlarmConditions.hh"
#include "karabo/data/types/Schema.hh"

namespace karabo {
    namespace data {


        AlarmConditionElement::AlarmConditionElement(Schema& expected)
            : GenericElement<AlarmConditionElement>(expected) {}


        AlarmConditionElement& AlarmConditionElement::initialValue(const AlarmCondition& a) {
            m_node->setAttribute(KARABO_SCHEMA_DEFAULT_VALUE, a.asString());
            return *this;
        }

        AlarmConditionElement& AlarmConditionElement::defaultValue(const AlarmCondition& a) {
            m_node->setAttribute(KARABO_SCHEMA_DEFAULT_VALUE, a.asString());
            return *this;
        }

        void AlarmConditionElement::beforeAddition() {
            m_node->setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, Schema::LEAF);
            m_node->setAttribute<std::string>(KARABO_SCHEMA_VALUE_TYPE, ToLiteral::to<Types::STRING>());
            m_node->setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, READ);
            m_node->setAttribute<int>(KARABO_SCHEMA_ASSIGNMENT, Schema::OPTIONAL_PARAM);
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
    } // namespace data
} // namespace karabo
