/*
 * $Id$
 *
 * File:   ListElement.hh
 * Author: <wp76@xfel.eu>
 *
 * Created on July 1, 2011, 11:48 AM
 *
 * Major re-design on February 30, 2013, 17:22 PM
 * 
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_UTIL_LISTELEMENT_HH
#define	KARABO_UTIL_LISTELEMENT_HH

#include "GenericElement.hh"
#include "LeafElement.hh"
#include "Configurator.hh"

namespace karabo {
    namespace util {

        class ListElement : public GenericElement<ListElement> {

            Schema::AssemblyRules m_parentSchemaAssemblyRules;

            DefaultValue<ListElement, std::vector<std::string> > m_defaultValue;
        public:

            ListElement(Schema& expected) : GenericElement<ListElement>(expected) {
                Schema::AssemblyRules m_parentSchemaAssemblyRules = expected.getAssemblyRules();
                m_defaultValue.setElement(this);
            }

            ListElement& min(const int minNumNodes) {
                this->m_node->setAttribute(KARABO_SCHEMA_MIN, minNumNodes);
                return *this;
            }

            ListElement& max(const int maxNumNodes) {
                this->m_node->setAttribute(KARABO_SCHEMA_MAX, maxNumNodes);
                return *this;
            }

            template <class ConfigurationBase>
            ListElement& appendNodesOfConfigurationBase() {
                // Create an empty Hash as value of this choice node if not there yet
                if (this->m_node->getType() != Types::HASH) this->m_node->setValue(Hash());
                // Retrieve reference for filling
                Hash& choiceOfNodes = this->m_node->template getValue<Hash > ();

                std::vector<std::string> nodeNames = Configurator<ConfigurationBase>::getRegisteredClasses();
                for (size_t i = 0; i < nodeNames.size(); ++i) {
                    const std::string& nodeName = nodeNames[i];
                    Schema schema = Configurator<ConfigurationBase>::getSchema(nodeName, m_parentSchemaAssemblyRules);
                    Hash::Node& node = choiceOfNodes.template set<Hash > (nodeName, schema.getParameterHash());
                    node.setAttribute(KARABO_SCHEMA_CLASS_ID, nodeName);
                    node.setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, nodeName);
                    node.setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, Schema::NODE);
                    node.setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, INIT);
                }
                return *this;
            }

            template <class T>
            ListElement& appendAsNode(const std::string& nodeName = "") {
                // Create an empty Hash as value of this choice node if not there yet
                if (this->m_node->getType() != Types::HASH) this->m_node->setValue(Hash());
                // Retrieve reference for filling
                Hash& choiceOfNodes = this->m_node->template getValue<Hash > ();

                // Simply append the expected parameters of T to current node
                if (nodeName.empty()) nodeName = T::classInfo().getClassId();
                Schema schema(nodeName, m_parentSchemaAssemblyRules);
                T::_KARABO_SCHEMA_DESCRIPTION_FUNCTION(schema);
                Hash::Node& node = choiceOfNodes.template set<Hash > (nodeName, schema.getParameterHash());
                node.setAttribute(KARABO_SCHEMA_CLASS_ID, T::classInfo().getClassId());
                node.setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, T::classInfo().getClassId());
                node.setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, Schema::NODE);
                node.setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, INIT);
                return *this;
            }

            /**
             * The <b>assignmentMandatory</b> method serves for setting up a mode that requires the value
             * of the element always being specified. No default value is possible.
             * @return reference to the Element (to allow method's chaining)
             */
            virtual ListElement& assignmentMandatory() {
                this->m_node->setAttribute<int>(KARABO_SCHEMA_ASSIGNMENT, Schema::MANDATORY_PARAM);
                return *this;
            }

            virtual DefaultValue<ListElement, std::vector<std::string> >& assignmentOptional() {
                this->m_node->setAttribute<int>(KARABO_SCHEMA_ASSIGNMENT, Schema::OPTIONAL_PARAM);
                return m_defaultValue;
            }


        protected:

            void beforeAddition() {
                this->m_node->setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, INIT);
                this->m_node->setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, Schema::LIST_OF_NODES);
            }

        };
        typedef util::ListElement LIST_ELEMENT;
    }
}



#endif	/* KARABO_PACKAGENAME_LISTELEMENT_HH */

