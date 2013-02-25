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
            }

            template <class FactoryBase>
            NodeElement& appendParametersOfFactoryMember(const std::string& classId) {
                if (Factory<FactoryBase>::has(classId)) {
                    this->m_node->setAttribute("classId", classId);
                    this->m_node->setAttribute("displayType", FactoryBase::classInfo().getClassId());
                    // Assemble schema (taking into account base classes, etc.) and append to node
                    Schema schema = Configurator<FactoryBase>::assembleSchema(classId, m_parentSchemaAssemblyRules);
                    // The produced schema will be rooted with classId, we however want to put its children
                    // under the defined key and ignore the classId root node
                    this->m_node->template setValue<Hash > (schema.getParameterHash());
                    
                } else {
                    throw KARABO_PARAMETER_EXCEPTION("Can not append class \"" + classId + "\", as it is not registered in factory.");
                }
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
                this->m_node->setAttribute<int>("accessMode", READ|WRITE|INIT);
                this->m_node->setAttribute<int>("nodeType", Schema::NODE);
            }
        };
        typedef NodeElement NODE_ELEMENT;
    }
}

#endif

