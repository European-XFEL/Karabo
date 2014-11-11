/*
 * $Id$
 *
 * File:   InputElement.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on September 8, 2013, 11:48 AM
 *
 * 
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_UTIL_INPUTELEMENT_HH
#define	KARABO_UTIL_INPUTELEMENT_HH

#include <karabo/util/GenericElement.hh>
#include <karabo/util/LeafElement.hh>
#include <karabo/util/Configurator.hh>
#include "Input.hh"

namespace karabo {
    namespace io {

        class InputElement : public karabo::util::GenericElement<InputElement> {

            karabo::util::Schema::AssemblyRules m_parentSchemaAssemblyRules;

            karabo::util::DefaultValue<InputElement, std::string> m_defaultValue;
        public:

            InputElement(karabo::util::Schema& expected) : karabo::util::GenericElement<InputElement>(expected) {
                m_parentSchemaAssemblyRules = expected.getAssemblyRules();
                m_defaultValue.setElement(this);
            }

            template <class ConfigurationBase>
            InputElement& setInputType() {
                using namespace karabo::util;
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
            InputElement& setDataType(const T& = T()) {
                using namespace karabo::util;
                // Create an empty Hash as value of this choice node if not there yet
                if (this->m_node->getType() != Types::HASH) this->m_node->setValue(Hash());
                // Retrieve reference for filling
                Hash& choiceOfNodes = this->m_node->template getValue<Hash > ();

                std::vector<std::string> nodeNames = Configurator<Input<T> >::getRegisteredClasses();
                for (size_t i = 0; i < nodeNames.size(); ++i) {
                    const std::string& nodeName = nodeNames[i];
                    Schema schema = Configurator<Input<T> >::getSchema(nodeName, m_parentSchemaAssemblyRules);
                    Hash::Node& node = choiceOfNodes.template set<Hash > (nodeName, schema.getParameterHash());
                    node.setAttribute(KARABO_SCHEMA_CLASS_ID, nodeName);
                    node.setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, nodeName);
                    node.setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, Schema::NODE);
                    node.setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, INIT);
                }
                return *this;
            }
           
            /**
             * The <b>init</b> method serves for setting up an access type property that allows the element
             * to be included in initial schema.
             * @return reference to the Element (to allow method's chaining)
             */
            virtual InputElement& init() {
                this->m_node->setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, karabo::util::INIT);
                return *this;
            }

            /**
             * The <b>reconfigurable</b> method serves for setting up an access type property that allows the element
             * to be included in initial, reconfiguration and monitoring schemas.
             * @return reference to the Element (to allow method's chaining)
             */
            virtual InputElement& reconfigurable() {
                this->m_node->setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, karabo::util::WRITE);
                return *this;
            }


        protected:

            void beforeAddition() {
                using namespace karabo::util;
                this->m_node->setAttribute<int>(KARABO_SCHEMA_ASSIGNMENT, Schema::MANDATORY_PARAM);
                if (!this->m_node->hasAttribute(KARABO_SCHEMA_ACCESS_MODE)) this->m_node->setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, INIT);
                this->m_node->setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, Schema::CHOICE_OF_NODES);
                this->m_node->setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, "Input");
            }


        };
        typedef InputElement INPUT_ELEMENT;
    }
}



#endif
