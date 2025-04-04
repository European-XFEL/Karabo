/*
 * $Id$
 *
 * File:   NodeElement.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on July 1, 2011, 11:49 AM
 *
 * Major re-design on February 1, 2013, 8:49 AM
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


#ifndef KARABO_DATA_SCHEMA_NODE_ELEMENT_HH
#define KARABO_DATA_SCHEMA_NODE_ELEMENT_HH

#include "Configurator.hh"
#include "GenericElement.hh"

namespace karabo {
    namespace data {

        /**
         * @class NodeElement
         * @brief The NodeElement groups other elements into a hierarchy
         *
         * The NodeElement can be used to create hierarchies in the expected
         * parameter section of a device.
         *
         * <b>Example:</b>
         * @code
         * NODE_ELEMENT(expected).key("myNode")
         *          .displayedName("This is a Node")
         *          .commit();
         *
         * SOME_ELEMENT(expected).key("myNode.myKeyA")
         *         ...
         *         .assignmentOptional().defaultValue("client")
         *         ...
         *         .commit();
         *
         * SOME_ELEMENT(expected).key("myNode.myKeyB")
         *         ...
         *         .assignmentOptional().defaultValue("client")
         *         ...
         *         .commit();
         * @endcode
         *
         * creates the following hierarchy:
         *
         *   MyNode -> myKeyA
         *          -> myKeyB
         *
         * NodeElements may contain subnodes so that arbitrary compley hierarchies
         * up to a maximum aggregated key-length of 120 characters are possible
         *
         * NodeElements may further be used to provide options for karabo::data::ChoiceElement
         * and list entries for karabo::data::ListElement
         *
         */
        class NodeElement : public GenericElement<NodeElement> {
            Schema::AssemblyRules m_parentSchemaAssemblyRules;

           public:
            NodeElement(Schema& expected)
                : GenericElement<NodeElement>(expected), m_parentSchemaAssemblyRules(expected.getAssemblyRules()) {
                this->m_node->setValue(Hash()); // A node value always is a Hash
            }

            /**
             * Insert the expected parameters of another class of type ConfigurationBase.
             * The class needs to be known by the factory system.
             * @param classId identifying the clas
             * @return
             */
            template <class ConfigurableClass>
            NodeElement& appendParametersOfConfigurableClass(const std::string& classId) {
                this->m_node->setAttribute(KARABO_SCHEMA_CLASS_ID, classId);
                this->m_node->setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, classId);
                // Assemble schema (taking into account base classes, etc.) and append to node
                Schema schema = Configurator<ConfigurableClass>::getSchema(classId, m_parentSchemaAssemblyRules);
                // The produced schema will be rooted with classId, we however want to put its children
                // under the defined key and ignore the classId root node
                this->m_node->setValue(schema.getParameterHash());
                return *this;
            }

            /**
             * Insert the expected parameters of another class of type ConfigurationBase.
             * The class needs to be known by the factory system.
             * @return
             */
            template <class T>
            NodeElement& appendParametersOf() {
                // Simply append the expected parameters of T to current node
                Schema schema("dummyRoot", m_parentSchemaAssemblyRules);
                T::_KARABO_SCHEMA_DESCRIPTION_FUNCTION(schema);
                this->m_node->setValue(schema.getParameterHash());
                this->m_node->setAttribute(KARABO_SCHEMA_CLASS_ID, T::classInfo().getClassId());
                this->m_node->setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, T::classInfo().getClassId());
                return *this;
            }

            /**
             * Append the elements specified in a Schema to the node
             * @param schema
             * @return
             */
            NodeElement& appendSchema(const Schema& schema) {
                this->m_node->setValue(schema.getParameterHash());
                return *this;
            }

            NodeElement& setDaqDataType(const DaqDataType& dataType) {
                this->m_node->setAttribute<int>(KARABO_SCHEMA_DAQ_DATA_TYPE, dataType);
                return *this;
            }

            /**
             * Specify one or more actions that are allowed on this node.
             *
             * If a Karabo device specifies allowed actions for a node, that means that it offers a specific slot
             * interface to operate on this node.
             * Which allowed actions require which interface will be defined elsewhere.
             * @return reference to the Element (to allow method's chaining)
             */
            NodeElement& setAllowedActions(const std::vector<std::string>& actions) {
                this->m_node->setAttribute(KARABO_SCHEMA_ALLOWED_ACTIONS, actions);
                return *this;
            }

           protected:
            void beforeAddition() {
                this->m_node->setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, WRITE);
                this->m_node->setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, Schema::NODE);
            }
        };
        typedef NodeElement NODE_ELEMENT;
    } // namespace data
} // namespace karabo

#endif
