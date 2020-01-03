/*
 * Author: <gero.flucke@xfel.eu>
 *
 * Created on March 23, 2016, 12:10 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "TableElement.hh"

const karabo::util::Validator::ValidationRules
karabo::util::tableValidationRules(
                                   /* injectDefaults */ true,
                                   /* allowUnrootedConfiguration */ true,
                                   /* allowAdditionalKeys */ false,
                                   /* allowMissingKeys */ false,
                                   /* injectTimestamps */ false
                                   );
namespace karabo {
    namespace util {


        void TableElement::beforeAddition() {

            this->m_node->setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, Schema::LEAF);
            this->m_node->setAttribute<int>(KARABO_SCHEMA_LEAF_TYPE, karabo::util::Schema::PROPERTY);
            this->m_node->setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, "Table");
            this->m_node->setAttribute(KARABO_SCHEMA_VALUE_TYPE, "VECTOR_HASH");
            if (m_nodeSchema.empty()) {
                std::stringstream s;
                s << "Table element '" << this->m_node->getKey() << "' has an empty row schema, "
                        << "likely a call to setColumns(..) is missing.";
                throw KARABO_LOGIC_EXCEPTION(s.str());
            }
            this->m_node->setAttribute(KARABO_SCHEMA_ROW_SCHEMA, m_nodeSchema);


            if (!this->m_node->hasAttribute(KARABO_SCHEMA_ACCESS_MODE)) this->init(); // This is the default

            if (!this->m_node->hasAttribute(KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL)) {

                //for init, reconfigurable elements - set default value of requiredAccessLevel to USER
                if (!this->m_node->hasAttribute(KARABO_SCHEMA_ACCESS_MODE) || //init element
                    this->m_node->getAttribute<int>(KARABO_SCHEMA_ACCESS_MODE) == INIT || //init element
                    this->m_node->getAttribute<int>(KARABO_SCHEMA_ACCESS_MODE) == WRITE) { //reconfigurable element

                    this->userAccess();

                } else { //else set default value of requiredAccessLevel to OBSERVER
                    this->observerAccess();
                }
            }

            //finally protect setting options etc to table element via overwrite
            OverwriteElement::Restrictions restrictions;
            restrictions.options = true;
            restrictions.minInc = true;
            restrictions.minExc = true;
            restrictions.maxInc = true;
            restrictions.maxExc = true;
            m_node->setAttribute(KARABO_OVERWRITE_RESTRICTIONS, restrictions.toVectorAttribute());
        }
    }
}