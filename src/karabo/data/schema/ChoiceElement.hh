/*
 * $Id$
 *
 * File:   ChoiceElement.hh
 * Author: <wp76@xfel.eu>
 *
 * Created on July 1, 2011, 11:48 AM
 *
 * Major re-design on February 30, 2013, 17:22 PM
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


#ifndef KARABO_DATA_SCHEMA_CHOICEELEMENT_HH
#define KARABO_DATA_SCHEMA_CHOICEELEMENT_HH

#include "Configurator.hh"
#include "GenericElement.hh"
#include "LeafElement.hh"

namespace karabo {
    namespace data {

        /**
         * @class ChoiceElement
         * @brief An element allowing choice-access to a list of factorized classes
         *
         * The ChoiceElement can be configured to allow a choice between factorized
         * class registered with it. Two methods exist for adding classes to the
         * list of choices the ChoiceElement knows of:
         *
         * - ChoiceElement::appendNodesOfConfigurationBase is used if another
         *   class of a type known to the factory system is to be added
         *
         * - ChoiceElement::appendAsNode is used to append the entries of a
         *   NodeElement defined in the same expectedParameter function as the
         *   choice element
         *
         * In either case, the user can select from the options during configuration
         *
         */
        class ChoiceElement : public GenericElement<ChoiceElement> {
            Schema::AssemblyRules m_parentSchemaAssemblyRules;

            DefaultValue<ChoiceElement, std::string> m_defaultValue;

           public:
            ChoiceElement(Schema& expected)
                : GenericElement<ChoiceElement>(expected), m_parentSchemaAssemblyRules(expected.getAssemblyRules()) {
                m_defaultValue.setElement(this);
            }

            /**
             * Append the expected parameters of another class of type ConfigurationBase.
             * The class needs to be known by the factory system. It will be identified
             * by its Karabo ClassId in the list of options.
             * @return
             */
            template <class ConfigurationBase>
            ChoiceElement& appendNodesOfConfigurationBase() {
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
                    node.setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, WRITE);
                }
                return *this;
            }

            /**
             * Append the entries found underneath a NodeElement identified by
             * key. The node element needs to be defined prior to and in the same expected
             * parameter function as the ChoiceElement.
             * @param nodeName identifying the node, i.e. the key of the node.
             * @return
             */
            template <class T>
            ChoiceElement& appendAsNode(const std::string& nodeName = "") {
                // Create an empty Hash as value of this choice node if not there yet
                if (this->m_node->getType() != Types::HASH) this->m_node->setValue(Hash());
                // Retrieve reference for filling
                Hash& choiceOfNodes = this->m_node->template getValue<Hash>();

                // Simply append the expected parameters of T to current node
                if (nodeName.empty()) nodeName = T::classInfo().getClassId();
                Schema schema(nodeName, m_parentSchemaAssemblyRules);
                T::_KARABO_SCHEMA_DESCRIPTION_FUNCTION(schema);
                // Schema schema = karabo::data::confTools::assembleSchema<T > (nodeName, m_parentSchemaAssemblyRules);
                Hash::Node& node = choiceOfNodes.template set<Hash>(nodeName, schema.getParameterHash());
                node.setAttribute(KARABO_SCHEMA_CLASS_ID, T::classInfo().getClassId());
                node.setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, T::classInfo().getClassId());
                node.setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, Schema::NODE);
                node.setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, WRITE);
                return *this;
            }

            /**
             * The <b>assignmentMandatory</b> method serves for setting up a mode that requires the value
             * of the element always being specified. No default value is possible.
             * @return reference to the Element (to allow method's chaining)
             */
            virtual ChoiceElement& assignmentMandatory() {
                this->m_node->setAttribute<int>(KARABO_SCHEMA_ASSIGNMENT, Schema::MANDATORY_PARAM);
                return *this;
            }

            /**
             * The <b>assignmentOptional</b> method is used to indicate that this
             * element can optionally be configured. It is followed by either
             * specifying that noDefaulValue() exists, or the defaultValue() is
             * a classId or node key registered in the list of choices using either
             * ChoiceElement::appendAsNode or ChoiceElement::appendNodesOfConfigurationBase.
             * @return
             */
            virtual DefaultValue<ChoiceElement, std::string>& assignmentOptional() {
                this->m_node->setAttribute<int>(KARABO_SCHEMA_ASSIGNMENT, Schema::OPTIONAL_PARAM);
                return m_defaultValue;
            }

            /**
             * The <b>init</b> method serves for setting up an access type property that allows the element
             * to be included in initial schema.
             * @return reference to the Element (to allow method's chaining)
             */
            virtual ChoiceElement& init() {
                this->m_node->setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, INIT);
                return *this;
            }

            /**
             * The <b>reconfigurable</b> method serves for setting up an access type property that allows the element
             * to be included in initial, reconfiguration and monitoring schemas.
             * @return reference to the Element (to allow method's chaining)
             */
            virtual ChoiceElement& reconfigurable() {
                this->m_node->setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, WRITE);
                return *this;
            }


           protected:
            void beforeAddition() {
                if (!this->m_node->hasAttribute(KARABO_SCHEMA_ACCESS_MODE))
                    this->m_node->setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, WRITE);
                this->m_node->setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, Schema::CHOICE_OF_NODES);

                // finally protect setting options etc to choice element via overwrite
                OverwriteElement::Restrictions restrictions;

                restrictions.minInc = true;
                restrictions.minExc = true;
                restrictions.maxInc = true;
                restrictions.maxExc = true;
                restrictions.min = true;
                restrictions.max = true;
                restrictions.minSize = true;
                restrictions.maxSize = true;
                m_node->setAttribute(KARABO_OVERWRITE_RESTRICTIONS, restrictions.toVectorAttribute());
            }
        };
        typedef ChoiceElement CHOICE_ELEMENT;
    } // namespace data
} // namespace karabo


#endif
