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

namespace karabo {
    namespace util {

        class ListElement : public GenericElement<ListElement> {
            Schema::AssemblyRules m_parentSchemaAssemblyRules;
        public:

            ListElement(Schema& expected) : GenericElement<ListElement>(expected) {
                Schema::AssemblyRules m_parentSchemaAssemblyRules = expected.getAssemblyRules();
            }

            ListElement& min(const int minNumNodes) {
                this->m_node->setAttribute("min", minNumNodes);
                return *this;
            }

            ListElement& max(const int maxNumNodes) {
                this->m_node->setAttribute("max", maxNumNodes);
                return *this;
            }
            
            ListElement& defaultValue(const std::vector<std::string>& defaultNodes) {
                this->m_node->setAttribute("default", defaultNodes);
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
                    Schema schema = Configurator<ConfigurationBase>::assembleSchema(nodeName, m_parentSchemaAssemblyRules);
                    Hash::Node& node = choiceOfNodes.set<Hash > (nodeName, schema.getRoot());
                    node.setAttribute("classId", nodeName);
                    node.setAttribute("displayType", nodeName);
                    node.setAttribute<int>("nodeType", Schema::NODE);
                    node.setAttribute<int>("accessMode", READ | WRITE | INIT);
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
                Schema schema = karabo::util::confTools::assembleSchema<T > (nodeName, m_parentSchemaAssemblyRules);
                Hash::Node& node = choiceOfNodes.set<Hash > (nodeName, schema.getRoot());
                node.setAttribute("classId", T::classInfo().getClassId());
                node.setAttribute("displayType", T::classInfo().getClassId());
                node.setAttribute<int>("nodeType", Schema::NODE);
                node.setAttribute<int>("accessMode", READ | WRITE | INIT);
                return *this;
            }

        protected:

            void beforeAddition() {
                this->m_node->setAttribute<int>("accessMode", READ | WRITE | INIT);
                this->m_node->setAttribute<int>("nodeType", Schema::LIST_OF_NODES);
            }

        };
        typedef util::ListElement LIST_ELEMENT;
    }
}



#endif	/* KARABO_PACKAGENAME_LISTELEMENT_HH */

