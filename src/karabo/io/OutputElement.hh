/*
 * $Id$
 *
 * File:   OutputElement.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on September 8, 2013, 11:48 AM
 *
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


#ifndef KARABO_UTIL_OUTPUTELEMENT_HH
#define KARABO_UTIL_OUTPUTELEMENT_HH

#include <karabo/util/Configurator.hh>
#include <karabo/util/GenericElement.hh>
#include <karabo/util/LeafElement.hh>

#include "Output.hh"

namespace karabo {
    namespace io {

        /**
         * @class OutputElement
         * @brief The OutputElement provides acces to karabo::io::Output in terms of
         *        a Schema Element, defined in an expected parameter section.
         */
        class OutputElement : public karabo::util::GenericElement<OutputElement> {
            karabo::util::Schema::AssemblyRules m_parentSchemaAssemblyRules;

           public:
            OutputElement(karabo::util::Schema& expected)
                : karabo::util::GenericElement<OutputElement>(expected),
                  m_parentSchemaAssemblyRules(expected.getAssemblyRules()) {
                this->m_node->setValue(karabo::util::Hash());
            }

            template <class ConfigurationBase>
            OutputElement& setOutputType() {
                using namespace karabo::util;
                // Create an empty Hash as value of this choice node if not there yet
                if (this->m_node->getType() != Types::HASH) this->m_node->setValue(Hash());
                // Retrieve reference for filling
                Hash& choiceOfNodes = this->m_node->template getValue<Hash>();

                std::vector<std::string> nodeNames = Configurator<ConfigurationBase>::getRegisteredClasses();
                for (size_t i = 0; i < nodeNames.size(); ++i) {
                    const std::string& nodeName = nodeNames[i];
                    Schema schema = Configurator<ConfigurationBase>::getSchema(nodeName, m_parentSchemaAssemblyRules);
                    Hash::Node& node = choiceOfNodes.template set<Hash>(nodeName, schema.getParameterHash());
                    node.setAttribute(KARABO_SCHEMA_CLASS_ID, nodeName);
                    node.setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, nodeName);
                    node.setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, Schema::NODE);
                    node.setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, READ | WRITE | INIT);
                }
                return *this;
            }

            /**
             * The <b>init</b> method serves for setting up an access type property that allows the element
             * to be included in initial schema.
             * @return reference to the Element (to allow method's chaining)
             */
            virtual OutputElement& init() {
                this->m_node->setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, karabo::util::INIT);
                return *this;
            }

            /**
             * The <b>reconfigurable</b> method serves for setting up an access type property that allows the element
             * to be included in initial, reconfiguration and monitoring schemas.
             * @return reference to the Element (to allow method's chaining)
             */
            virtual OutputElement& reconfigurable() {
                this->m_node->setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, karabo::util::WRITE);
                return *this;
            }


           protected:
            void beforeAddition() {
                using namespace karabo::util;
                this->m_node->setAttribute<int>(KARABO_SCHEMA_ASSIGNMENT, Schema::MANDATORY_PARAM);
                if (!this->m_node->hasAttribute(KARABO_SCHEMA_ACCESS_MODE))
                    this->m_node->setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, INIT);
                this->m_node->setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, karabo::util::Schema::NODE);
                this->m_node->setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, "Output");
            }
        };
        typedef OutputElement OUTPUT_ELEMENT;
    } // namespace io
} // namespace karabo


#endif
