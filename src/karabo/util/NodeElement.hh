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
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_UTIL_NODE_ELEMENT_HH
#define	KARABO_UTIL_NODE_ELEMENT_HH

#include "GenericElement.hh"
#include "Configurator.hh"

namespace karabo {
    namespace util {

        class NodeElement : public GenericElement<NodeElement> {

            Schema::AssemblyRules m_parentSchemaAssemblyRules;

        public:

            NodeElement(Schema& expected) : GenericElement<NodeElement>(expected) {
                m_parentSchemaAssemblyRules = expected.getAssemblyRules();
                this->m_node->setValue(Hash()); // A node value always is a Hash
            }

            template <class ConfigurableClass>
            NodeElement& appendParametersOfConfigurableClass(const std::string& classId) {
                this->m_node->setAttribute(KARABO_SCHEMA_CLASS_ID, classId);
                this->m_node->setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, ConfigurableClass::classInfo().getClassId());
                // Assemble schema (taking into account base classes, etc.) and append to node
                Schema schema = Configurator<ConfigurableClass>::getSchema(classId, m_parentSchemaAssemblyRules);
                // The produced schema will be rooted with classId, we however want to put its children
                // under the defined key and ignore the classId root node
                this->m_node->template setValue<Hash > (schema.getParameterHash());
                return *this;
            }

            template <class T>
            NodeElement& appendParametersOf() {
                // Simply append the expected parameters of T to current node
                Schema schema("dummyRoot", m_parentSchemaAssemblyRules);
                T::_KARABO_SCHEMA_DESCRIPTION_FUNCTION(schema);
                this->m_node->template setValue<Hash > (schema.getParameterHash());
                return *this;
            }

        protected:

            void beforeAddition() {
                this->m_node->setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, WRITE);
                this->m_node->setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, Schema::NODE);
            }
        };
        typedef NodeElement NODE_ELEMENT;
    }
}

#endif

