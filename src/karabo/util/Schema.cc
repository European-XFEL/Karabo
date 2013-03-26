/*
 * $Id: Schema.cc 4587 2011-10-21 10:52:13Z heisenb@DESY.DE $
 *
 * File:   Schema.cc
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on August 11, 2010, 3:44 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */
#include <iostream>
#include <iosfwd>
#include <deque>
#include <set>

#include "Exception.hh"
#include "Schema.hh"
#include "Hash.hh"

namespace karabo {
    namespace util {

        using namespace std;


        Schema::Schema() : m_rootName("") {
        };


        Schema::Schema(const std::string& classId, const Schema::AssemblyRules& rules) :
        m_currentAccessMode(rules.m_accessMode), m_currentState(rules.m_state), m_currentAccessRole(rules.m_accessRole), m_rootName(classId) {

            //this->setRoot(classId);

        }


        void Schema::setRootName(const std::string& rootName) {
            m_rootName = rootName;
        }


        karabo::util::Hash& Schema::getParameterHash() {
            return m_hash;
        }


        const karabo::util::Hash& Schema::getParameterHash() const {
            return m_hash;
        }


        std::vector<std::string> Schema::getParameters(const std::string& path) const {
            std::vector<std::string> tmp;
            if (path.empty())
                m_hash.getKeys(tmp);
            else if (m_hash.is<Hash > (path))
                m_hash.get<Hash > (path).getKeys(tmp);

            return tmp;
        }


        void Schema::setAssemblyRules(const Schema::AssemblyRules& rules) {
            m_currentAccessMode = rules.m_accessMode;
            m_currentState = rules.m_state;
            m_currentAccessRole = rules.m_accessRole;
        }


        Schema::AssemblyRules Schema::getAssemblyRules() const {
            Schema::AssemblyRules rules;
            rules.m_accessMode = m_currentAccessMode;
            rules.m_accessRole = m_currentAccessRole;
            rules.m_state = m_currentState;
            return rules;
        }


        const std::string& Schema::getRootName() const {
            return m_rootName;
        }

        //**********************************************
        //              Node property                  *
        //**********************************************


        bool Schema::isLeaf(const std::string& path) const {
            return m_hash.getAttribute<int>(path, "nodeType") == LEAF;
        }


        bool Schema::isNode(const std::string& path) const {
            return m_hash.getAttribute<int>(path, "nodeType") == NODE;
        }


        bool Schema::isChoiceOfNodes(const std::string& path) const {
            return m_hash.getAttribute<int>(path, "nodeType") == CHOICE_OF_NODES;
        }


        bool Schema::isListOfNodes(const std::string& path) const {
            return m_hash.getAttribute<int>(path, "nodeType") == LIST_OF_NODES;
        }


        int Schema::getNodeType(const std::string& path) const {
            return m_hash.getAttribute<int>(path, "nodeType");
        }


        bool Schema::isCommand(const std::string& path) const {
            if (this->isLeaf(path) && m_hash.getAttribute<int>(path, "leafType") == COMMAND) return true;
            else return false;
        }


        bool Schema::isProperty(const std::string& path) const {
            if (this->isLeaf(path) && m_hash.getAttribute<int>(path, "leafType") == PROPERTY) return true;
            else return false;
        }

        //**********************************************
        //                Value Type                   *
        //**********************************************


        const string& Schema::getValueType(const std::string& path) const {
            return m_hash.getAttribute<string > (path, "valueType");
        }

        //**********************************************
        //                Access Mode                  *
        //**********************************************


        void Schema::setAccessMode(const std::string& path, const AccessType& value) {
            m_hash.setAttribute<int>(path, "accessMode", value);
        }


        bool Schema::hasAccessMode(const std::string& path) const {
            return m_hash.hasAttribute(path, "accessMode");
        }


        bool Schema::isAccessInitOnly(const std::string& path) const {
            return m_hash.getAttribute<int>(path, "accessMode") == INIT;
        }


        bool Schema::isAccessReadOnly(const std::string& path) const {
            return m_hash.getAttribute<int>(path, "accessMode") == READ;
        }


        bool Schema::isAccessReconfigurable(const std::string& path) const {
            return m_hash.getAttribute<int>(path, "accessMode") == WRITE;
        }


        int Schema::getAccessMode(const std::string& path) const {
            return m_hash.getAttribute<int>(path, "accessMode");
        }

        //**********************************************
        //                DisplayedName                *
        //**********************************************


        void Schema::setDisplayedName(const std::string& path, const std::string& value) {
            m_hash.setAttribute(path, "displayedName", value);
        }


        bool Schema::hasDisplayedName(const std::string& path) const {
            return m_hash.hasAttribute(path, "displayedName");
        }


        const std::string& Schema::getDisplayedName(const std::string& path) const {
            return m_hash.getAttribute<std::string > (path, "displayedName");
        }

        //**********************************************
        //                Description                  *
        //**********************************************


        void Schema::setDescription(const std::string& path, const std::string& value) {
            m_hash.setAttribute(path, "description", value);
        }


        bool Schema::hasDescription(const std::string& path) const {
            return m_hash.hasAttribute(path, "description");
        }


        const std::string& Schema::getDescription(const std::string& path) const {
            return m_hash.getAttribute<std::string > (path, "description");
        }

        //**********************************************
        //                DefaultValue                 *
        //**********************************************


        bool Schema::hasDefaultValue(const std::string& path) const {
            return m_hash.hasAttribute(path, "defaultValue");
        }

        //**********************************************
        //                Assignment                   *
        //**********************************************


        void Schema::setAssignment(const std::string& path, const Schema::AssignmentType& value) {
            m_hash.setAttribute(path, "assignment", value);
        }


        bool Schema::hasAssignment(const std::string& path) const {
            return m_hash.hasAttribute(path, "assignment");
        }


        bool Schema::isAssignmentMandatory(const std::string& path) const {
            return m_hash.getAttribute<int>(path, "assignment") == Schema::MANDATORY_PARAM;
        }


        bool Schema::isAssignmentOptional(const std::string& path) const {
            return m_hash.getAttribute<int>(path, "assignment") == Schema::OPTIONAL_PARAM;
        }


        bool Schema::isAssignmentInternal(const std::string& path) const {
            return m_hash.getAttribute<int>(path, "assignment") == Schema::INTERNAL_PARAM;
        }


        const int Schema::getAssignment(const std::string& path) const {
            return m_hash.getAttribute<int > (path, "assignment");
        }

        //**********************************************
        //                   Tags                      *
        //**********************************************


        void Schema::setTags(const std::string& path, const std::string& value, const std::string& sep) {
            m_hash.setAttribute(path, "tags", karabo::util::fromString<std::string, std::vector > (value, sep));
        }


        bool Schema::hasTags(const std::string& path) const {
            return m_hash.hasAttribute(path, "tags");
        }


        const std::vector<std::string>& Schema::getTags(const std::string& path) const {
            return m_hash.getAttribute<std::vector<std::string> >(path, "tags");
        }


        //**********************************************
        //                  Alias                      *
        //**********************************************


        void Schema::setDisplayType(const std::string& path, const std::string& value) {
            m_hash.setAttribute(path, "displayType", value);
        }


        bool Schema::hasDisplayType(const std::string& path) const {
            return m_hash.hasAttribute(path, "displayType");
        }


        const string& Schema::getDisplayType(const std::string& path) const {
            return m_hash.getAttribute<string > (path, "displayType");
        }

        //**********************************************
        //                  Alias                      *
        //**********************************************


        bool Schema::hasAlias(const std::string& path) const {
            return m_hash.hasAttribute(path, "alias");
        }
        
        string Schema::getAliasAsString(const std::string& path) const {
            return m_hash.getAttributeAs<string>(path, "alias");
        }
 
        //**********************************************
        //                  Options             *
        //**********************************************


        void Schema::setOptions(const std::string& path, const std::string& value, const std::string& sep) {
            m_hash.setAttribute(path, "options", karabo::util::fromString<std::string, std::vector > (value, sep));
        }


        bool Schema::hasOptions(const std::string& path) const {
            return m_hash.hasAttribute(path, "options");
        }


        const std::vector<std::string>& Schema::getOptions(const std::string& path) const {
            return m_hash.getAttribute<std::vector<std::string> >(path, "options");
        }


        //**********************************************
        //                AllowedStates                *
        //**********************************************


        void Schema::setAllowedStates(const std::string& path, const std::string& value, const std::string& sep) {
            m_hash.setAttribute(path, "allowedStates", karabo::util::fromString<std::string, std::vector > (value, sep));
        }


        bool Schema::hasAllowedStates(const std::string& path) const {
            return m_hash.hasAttribute(path, "allowedStates");
        }


        const vector<string>& Schema::getAllowedStates(const std::string& path) const {
            return m_hash.getAttribute<vector<string> >(path, "allowedStates");
        }

        //**********************************************
        //                  ExpertLevel                *
        //**********************************************


        void Schema::setExpertLevel(const std::string& path, const Schema::ExpertLevelType& value) {
            m_hash.setAttribute(path, "expertLevel", value);
        }


        bool Schema::hasExpertLevel(const std::string& path) const {
            return m_hash.hasAttribute(path, "expertLevel");
        }


        bool Schema::isExpertLevelAdvanced(const std::string& path) const {
            return m_hash.getAttribute<int>(path, "expertLevel") == Schema::ADVANCED;
        }


        bool Schema::isExpertLevelMedium(const std::string& path) const {
            return m_hash.getAttribute<int>(path, "expertLevel") == Schema::MEDIUM;
        }


        bool Schema::isExpertLevelSimple(const std::string& path) const {
            return m_hash.getAttribute<int>(path, "expertLevel") == Schema::SIMPLE;
        }


        const int Schema::getExpertLevel(const std::string& path) const {
            return m_hash.getAttribute<int> (path, "expertLevel");
        }

        //**********************************************
        //                  Unit                       *
        //**********************************************


        void Schema::setUnit(const std::string& path, const Units::Unit& value) {
            m_hash.setAttribute(path, "unitEnum", value);
            pair<string, string> names = Units::getUnit(value);
            m_hash.setAttribute(path, "unitName", names.first);
            m_hash.setAttribute(path, "unitSymbol", names.second);
        }


        bool Schema::hasUnit(const std::string& path) const {
            return m_hash.hasAttribute(path, "unitEnum");
        }


        const int Schema::getUnit(const std::string& path) const {
            return m_hash.getAttribute<int>(path, "unitEnum");
        }


        const std::string& Schema::getUnitName(const std::string& path) const {
            return m_hash.getAttribute<string > (path, "unitName");
        }


        const std::string& Schema::getUnitSymbol(const std::string& path) const {
            return m_hash.getAttribute<string > (path, "unitSymbol");
        }


        //**********************************************
        //                  MetricPrefix               *
        //**********************************************


        void Schema::setMetricPrefix(const std::string& path, const Units::MetricPrefix& value) {
            m_hash.setAttribute(path, "metricPrefixEnum", value);
            pair<string, string> names = Units::getMetricPrefix(value);
            m_hash.setAttribute(path, "metricPrefixName", names.first);
            m_hash.setAttribute(path, "metricPrefixSymbol", names.second);
        }


        bool Schema::hasMetricPrefix(const std::string& path) const {
            return m_hash.hasAttribute(path, "metricPrefixEnum");
        }


        const int Schema::getMetricPrefix(const std::string& path) const {
            return m_hash.getAttribute<int>(path, "metricPrefixEnum");
        }


        const std::string& Schema::getMetricPrefixName(const std::string& path) const {
            return m_hash.getAttribute<string > (path, "metricPrefixName");
        }


        const std::string& Schema::getMetricPrefixSymbol(const std::string& path) const {
            return m_hash.getAttribute<string > (path, "metricPrefixSymbol");
        }

        //**********************************************
        //    Minimum Inclusive value                  *                   *
        //**********************************************


        bool Schema::hasMinInc(const std::string& path) const {
            return m_hash.hasAttribute(path, "minInc");
        }

        //**********************************************
        //    Maximum Inclusive value                  *                   *
        //**********************************************


        bool Schema::hasMaxInc(const std::string& path) const {
            return m_hash.hasAttribute(path, "maxInc");
        }


        //**********************************************
        //    Minimum Exclusive value                  *                   *
        //**********************************************


        bool Schema::hasMinExc(const std::string& path) const {
            return m_hash.hasAttribute(path, "minExc");
        }


        //**********************************************
        //    Maximum Exclusive value                  *                   *
        //**********************************************


        bool Schema::hasMaxExc(const std::string& path) const {
            return m_hash.hasAttribute(path, "maxExc");
        }

        //**********************************************************
        //       Specific functions for LEAF node which is vector  *
        //       Minimum Size of the vector                        *
        //**********************************************************


        void Schema::setMinSize(const std::string& path, const unsigned int& value) {
            m_hash.setAttribute(path, "minSize", value);
        }


        bool Schema::hasMinSize(const std::string& path) const {
            return m_hash.hasAttribute(path, "minSize");
        }


        const unsigned int& Schema::getMinSize(const std::string& path) const {
            return m_hash.getAttribute<unsigned int>(path, "minSize");
        }


        //******************************************************
        //  Specific functions for LEAF node (which is vector):*
        //  Maximum Size of the vector                         *  
        //******************************************************


        void Schema::setMaxSize(const std::string& path, const unsigned int& value) {
            m_hash.setAttribute(path, "maxSize", value);
        }


        bool Schema::hasMaxSize(const std::string& path) const {
            return m_hash.hasAttribute(path, "maxSize");
        }


        const unsigned int& Schema::getMaxSize(const std::string& path) const {
            return m_hash.getAttribute<unsigned int>(path, "maxSize");
        }

        //******************************************************
        //    has/ WarnLow, WarnHigh, AlarmLow, AlarmHigh      *                   *  
        //******************************************************


        bool Schema::hasWarnLow(const std::string& path) const {
            return m_hash.hasAttribute(path, "warnLow");
        }


        bool Schema::hasWarnHigh(const std::string& path) const {
            return m_hash.hasAttribute(path, "warnHigh");
        }


        bool Schema::hasAlarmLow(const std::string& path) const {
            return m_hash.hasAttribute(path, "alarmLow");
        }


        bool Schema::hasAlarmHigh(const std::string& path) const {
            return m_hash.hasAttribute(path, "alarmHigh");
        }

        //******************************************************
        //      min/max for number of nodes in ListElement     *                     *  
        //******************************************************


        void Schema::setMin(const std::string& path, const int& value) {
            m_hash.setAttribute(path, "min", value);
        }


        bool Schema::hasMin(const std::string& path) const {
            return m_hash.hasAttribute(path, "min");
        }


        const int& Schema::getMin(const std::string& path) const {
            return m_hash.getAttribute<int>(path, "min");
        }


        void Schema::setMax(const std::string& path, const int& value) {
            m_hash.setAttribute(path, "max", value);
        }


        bool Schema::hasMax(const std::string& path) const {
            return m_hash.hasAttribute(path, "max");
        }


        const int& Schema::getMax(const std::string& path) const {
            return m_hash.getAttribute<int>(path, "max");
        }


        void Schema::addElement(Hash::Node& node) {

            if (node.hasAttribute("overwrite")) {
                this->overwriteAttributes(node);
                return;
            }

            // Ensure completeness of node parameter description
            ensureParameterDescriptionIsComplete(node); // Will throw in case of error

            // Check whether node is allowed to be added
            bool accessModeOk = isAllowedInCurrentAccessMode(node);
            bool accessRoleOk = isAllowedInCurrentAccessRole(node);
            bool stateOk = isAllowedInCurrentState(node);
            if (!(accessModeOk && accessRoleOk && stateOk)) return;

            this->getParameterHash().setNode(node);
        }


        void Schema::overwriteAttributes(const Hash::Node& node) {

            boost::optional<Hash::Node&> thisNodeOpt = m_hash.find(node.getKey());
            if (thisNodeOpt) {
                const Hash::Attributes& attrs = node.getAttributes();
                for (Hash::Attributes::const_iterator it = attrs.begin(); it != attrs.end(); ++it) {
                    const std::string& attributeKey = it->getKey();
                    if (thisNodeOpt->hasAttribute(attributeKey)) {
                        thisNodeOpt->setAttribute(attributeKey, it->getValueAsAny());
                    }
                }
            }
        }


        void Schema::ensureParameterDescriptionIsComplete(Hash::Node& node) const {
            std::string error;
            if (node.hasAttribute("nodeType")) {
                int type = node.getAttribute<int>("nodeType");
                if (type == Schema::LEAF || type == Schema::CHOICE_OF_NODES) {
                    if (!node.hasAttribute("assignment")) error = "Missing assignment, i.e. assignmentMandatory() / assignmentOptional(). ";
                }
            } else {
                error = "Missing nodeType attribute. ";
            }
            if (!node.hasAttribute("accessMode")) error = "Missing accessMode attribute. ";

            if (!error.empty()) throw KARABO_PARAMETER_EXCEPTION("Bad description for parameter \"" + node.getKey() + "\": " + error);
        }


        bool Schema::isAllowedInCurrentAccessMode(const Hash::Node& node) const {
            return (m_currentAccessMode & node.getAttribute<int>("accessMode"));
        }


        bool Schema::isAllowedInCurrentAccessRole(const Hash::Node& node) const {
            if (node.hasAttribute("allowedRoles")) {
                const vector<string>& allowedRoles = node.getAttribute<vector<string> >("allowedRoles");
                return (std::find(allowedRoles.begin(), allowedRoles.end(), m_currentAccessRole) != allowedRoles.end());
            } else { // If no roles are assigned, access/visibility is always possible
                return true;
            }
        }


        bool Schema::isAllowedInCurrentState(const Hash::Node& node) const {
            if (node.hasAttribute("allowedStates") && !m_currentState.empty()) {
                const vector<string>& allowedStates = node.getAttribute<vector<string> >("allowedStates");
                return (std::find(allowedStates.begin(), allowedStates.end(), m_currentState) != allowedStates.end());
            } else { // If no states are assigned, access/visibility is always possible
                return true;
            }
        }


        ostream& operator<<(std::ostream& os, const Schema& schema) {
            os << "Schema for: " << schema.getRootName() << endl;
            os << schema.m_hash;
            return os;
        }


        void Schema::help(const string& classId) {
            ostringstream stream;
            stream << "----- HELP -----" << endl;
            if (classId.empty()) {
                stream << "Schema: " << getRootName() << endl;
                vector<string> keys = getParameters();


                BOOST_FOREACH(string key, keys) {
                    if (getNodeType(key) == Schema::LEAF) {
                        processingLeaf(key, stream);
                    } else if (getNodeType(key) == Schema::NODE) {
                        processingNode(key, stream);
                    } else if (getNodeType(key) == Schema::CHOICE_OF_NODES) {
                        processingChoiceOfNodes(key, stream);
                    } else if (getNodeType(key) == Schema::LIST_OF_NODES) {
                        processingListOfNodes(key, stream);
                    }
                }
            } else {
                stream << "Schema: " << getRootName() << " , key: " << classId << endl;

                if (getNodeType(classId) == Schema::LEAF) {
                    stream << "LEAF element" << endl;
                    processingLeaf(classId, stream);
                }

                if (getNodeType(classId) == Schema::NODE) {

                    vector<string> keys = getParameters(classId);
                    if (!keys.empty()) {
                        stream << "NODE element" << endl;


                        BOOST_FOREACH(string key, keys) {
                            string path = classId + "." + key;
                            if (getNodeType(path) == Schema::LEAF) {
                                processingLeaf(path, stream);
                            } else if (getNodeType(path) == Schema::NODE) {
                                processingNode(path, stream);
                            } else if (getNodeType(path) == Schema::CHOICE_OF_NODES) {
                                processingChoiceOfNodes(path, stream);
                            } else if (getNodeType(path) == Schema::LIST_OF_NODES) {
                                processingListOfNodes(path, stream);
                            }
                        }
                    } else {
                        stream << "NODE element (contains no other elements)" << endl;
                        processingNode(classId, stream);
                    }
                }

                if (getNodeType(classId) == Schema::CHOICE_OF_NODES) {
                    stream << "CHOICE element" << endl;
                    vector<string> keys = getParameters(classId);


                    BOOST_FOREACH(string key, keys) {
                        string path = classId + "." + key;
                        processingNode(path, stream);
                    }
                }

                if (getNodeType(classId) == Schema::LIST_OF_NODES) {
                    stream << "LIST element" << endl;
                    vector<string> keys = getParameters(classId);


                    BOOST_FOREACH(string key, keys) {
                        string path = classId + "." + key;
                        processingNode(path, stream);
                    }
                }
            }

            //show results:
            cout << "\n" << stream.str();
        }


        void Schema::processingLeaf(const std::string& key, ostringstream & stream) {
            string showKey = extractKey(key);

            string valueType = getValueType(key);

            stream << "\n  ." << showKey << "(" << valueType << ")" << endl;

            processingStandardAttributes(key, stream);

            if (getAccessMode(key) == INIT)
                stream << "     " << "Access mode: initialization" << endl;
            else if (getAccessMode(key) == READ)
                stream << "     " << "Access mode: read only" << endl;
            else if (getAccessMode(key) == WRITE)
                stream << "     " << "Access mode: reconfigurable" << endl;
        }


        void Schema::processingNode(const std::string& key, ostringstream & stream) {
            string showKey = extractKey(key);
            stream << "\n  ." << showKey << "(NODE)" << endl;
            if (hasDescription(key))
                stream << "     " << "Description : " << getDescription(key) << endl;

        }


        void Schema::processingChoiceOfNodes(const std::string& key, ostringstream & stream) {
            string showKey = extractKey(key);
            stream << "\n  ." << showKey << "(CHOICE_OF_NODES)" << endl;
            processingStandardAttributes(key, stream);
        }


        void Schema::processingListOfNodes(const std::string& key, ostringstream & stream) {
            string showKey = extractKey(key);
            stream << "\n  ." << showKey << "(LIST_OF_NODES)" << endl;
            processingStandardAttributes(key, stream);
        }


        void Schema::processingStandardAttributes(const std::string& key, ostringstream & stream) {
            if (getAssignment(key) == OPTIONAL_PARAM)
                stream << "     " << "Assignment : OPTIONAL" << endl;
            else if (getAssignment(key) == MANDATORY_PARAM)
                stream << "     " << "Assignment : MANDATORY" << endl;
            else if (getAssignment(key) == INTERNAL_PARAM)
                stream << "     " << "Assignment : INTERNAL" << endl;

            if (hasDefaultValue(key))
                stream << "     " << "Default value : " << getDefaultValueAs<string > (key) << endl;

            if (hasDescription(key))
                stream << "     " << "Description : " << getDescription(key) << endl;

        }


        string Schema::extractKey(const std::string& key) {
            string newKey;
            if (key.rfind(".") != string::npos) {
                vector<string> tokens;
                boost::split(tokens, key, boost::is_any_of("."));
                int i = tokens.size() - 1;
                newKey = tokens[i];
            } else {
                newKey = key;
            }
            return newKey;
        }


        //        Schema& Schema::addExternalSchema(const Schema& schema) {
        //            Schema& currentElements = get<Schema > ("elements");
        //            const Schema& inputElements = schema.get<Schema > ("elements");
        //            for (const_iterator it = inputElements.begin(); it != inputElements.end(); it++) {
        //                Schema item = it->getValue<Schema > ();
        //                currentElements.addParameter(item);
        //            }
        //            return *this;
        //        }
        //
        //        void Schema::overwriteIfDuplicated(const std::string& key) {
        //            vector<iterator> deleteMe;
        //            for (iterator it = m_container.begin(); it != this->end(); it++) {
        //                const Schema& element = get<Schema > (it);
        //                if (element.has("key") && element.get<string > ("key") == key) {
        //                    deleteMe.push_back(it);
        //                }
        //            }
        //            for (size_t i = 0; i < deleteMe.size(); ++i) {
        //                //this->erase(deleteMe[i]); // TODO
        //            }
        //        }
        //

        //        void Schema::r_generateAliasMaps(Schema& config, std::string path) {
        //            std::string key = path;
        //            iterator keyIt = config.find("key");
        //            iterator aliasIt = config.find("alias");
        //            if (keyIt != config.end()) {
        //                key = config.get<string > (keyIt);
        //                if (!path.empty()) key = path + "." + key;
        //                if (aliasIt != config.end()) {
        //                    m_keyToAlias[key] = aliasIt->second;
        //                    m_aliasToKey[config.getAsString(aliasIt)] = key;
        //                }
        //                m_keys.insert(std::make_pair(key, &config));
        //            }
        //            iterator elementsIt = config.find("elements");
        //            if (elementsIt != config.end()) {
        //                Schema& tmp = config.get<Schema > (elementsIt);
        //                for (iterator it = tmp.begin(); it != tmp.end(); ++it) {
        //                    Schema& param = tmp.get<Schema > (it);
        //                    r_generateAliasMaps(param, key);
        //                }
        //            }
        //            iterator complexTypeIt = config.find("complexType");
        //            if (complexTypeIt != config.end()) {
        //                Schema& tmp = config.get<Schema > (complexTypeIt);
        //                for (iterator it = tmp.begin(); it != tmp.end(); ++it) {
        //                    Schema& entry = tmp.get<Schema > (it);
        //                    string complexKey = key + "." + it->first;
        //                    m_keys.insert(std::make_pair(complexKey, &entry));
        //                    r_generateAliasMaps(entry, complexKey);
        //                }
        //            }
        //        }



        //        Hash Schema::mergeUserInput(const vector<Hash>& userConfigurations) {
        //
        //            Hash mergedConfiguration;
        //
        //            // Set up flags for this merging
        //            m_injectDefaults = true;
        //            m_allowUnrootedConfiguration = false;
        //            m_allowAdditionalKeys = false;
        //            m_allowMissingKeys = false;
        //
        //            // Merge in all user configurations
        //            for (size_t i = 0; i < userConfigurations.size(); ++i) {
        //                Hash userConfiguration = userConfigurations[i];
        //                ostringstream intermediateReport;
        //
        //                r_validate(*this, userConfiguration, mergedConfiguration, intermediateReport);
        //
        //            }
        //
        //            return mergedConfiguration;
        //        }
        //

        //        void Schema::assertSimpleType(const Schema& desc, Hash& uParam, Hash& wParam, std::ostringstream& report, string & scope) const {
        //
        //            const string& key = desc.getAsString("key");
        //
        //            string nextScope = scope + "." + key;
        //            string type = Types::convert(desc.get<Types::ReferenceType > ("simpleType"));
        //
        //            if (!uParam.has(key)) {
        //                report << "Missing simple-type (" << type << ") parameter: \"" << nextScope << "\"" << endl;
        //                return;
        //            }
        //
        //            // Check datatype
        //            Types::ReferenceType eDataType = desc.get<Types::ReferenceType > ("simpleType");
        //            Types::ReferenceType iDataType = uParam.getTypeAsId(key);
        //            if (eDataType != Types::ANY && iDataType != eDataType) {
        //                // TODO Discuss whether we should try to cast whatever is there
        //                if (iDataType != Types::STRING) {
        //                    if (iDataType == Types::HASH) {
        //                        report << "Parameter \"" << nextScope << "\" needs an assignment of type " << type << "(" << eDataType << ")" << endl;
        //                        report << "Type like this: \"" << nextScope << "=your" << type << "\"" << endl;
        //                        return;
        //                    }
        //                    try {
        //                        string value(uParam.getAsString(key));
        //                        uParam.set(key, value);
        //                        uParam.convertFromString(key, eDataType);
        //                    } catch (...) {
        //                        report << "Failed to cast the value of parameter \"" << nextScope << "\" from " << Types::convert(iDataType);
        //                        report << " to " << Types::convert(eDataType);
        //                        return;
        //                    }
        //                } else if (iDataType == Types::STRING) {
        //                    // Try to cast
        //                    try {
        //                        uParam.convertFromString(key, eDataType);
        //                    } catch (...) {
        //                        report << "Failed to cast the value of parameter \"" << nextScope << "\" from " << Types::convert(iDataType);
        //                        report << " to " << Types::convert(eDataType);
        //                    }
        //                } else {
        //                    report << "Value for parameter \"" << nextScope << "\" is of wrong format." << endl
        //                            << "Expected " << Types::convert(eDataType) << " got " << Types::convert(iDataType) << endl;
        //                }
        //            }
        //
        //            // Check ranges
        //            switch (eDataType) {
        //
        //                case Types::ANY:
        //                    // No way to validate any
        //                    break;
        //
        //                case Types::BOOL:
        //                    // No need to validate true/false
        //                    break;
        //
        //                case Types::VECTOR_BOOL:
        //                {
        //                    // validate number of elements in array: minSize, maxSize
        //                    deque<bool> iValueVect = uParam.get < deque<bool> > (key);
        //                    unsigned int iValue = iValueVect.size();
        //                    checkSizeOfVector(desc, scope, key, iValue, report);
        //                }
        //                    break;
        //
        //                case Types::INT8:
        //                case Types::INT16:
        //                case Types::INT32:
        //                case Types::INT64:
        //                case Types::UINT8:
        //                case Types::UINT16:
        //                case Types::UINT32:
        //                case Types::UINT64:
        //                case Types::FLOAT:
        //                case Types::DOUBLE:
        //                {
        //                    double iValue = uParam.getAs<double > (key);
        //                    if (desc.has("options")) {
        //                        checkOptions(scope, iValue, desc, report);
        //                    }
        //                    if (desc.has("minInc")) {
        //                        double eValue = desc.getAs<double>("minInc");
        //                        checkMinInc(scope, iValue, eValue, report);
        //                    } else if (desc.has("minExc")) {
        //                        double eValue = desc.getAs<double>("minExc");
        //                        checkMinExc(scope, iValue, eValue, report);
        //                    }
        //                    if (desc.has("maxInc")) {
        //                        double eValue = desc.getAs<double>("maxInc");
        //                        checkMaxInc(scope, iValue, eValue, report);
        //                    } else if (desc.has("maxExc")) {
        //                        double eValue = desc.getAs<double>("maxExc");
        //                        checkMaxExc(scope, iValue, eValue, report);
        //                    }
        //                }
        //                    break;
        //
        //                case Types::VECTOR_INT8:
        //                {
        //                    vector<signed char> iValueVect = uParam.get < vector<signed char> > (key);
        //                    unsigned int iValue = iValueVect.size();
        //                    checkSizeOfVector(desc, scope, key, iValue, report);
        //                    checkRangeOfVectorElements(desc, scope, key, iValue, iValueVect, report);
        //                }
        //                    break;
        //
        //                case Types::VECTOR_INT16:
        //                {
        //                    vector<signed short> iValueVect = uParam.get < vector<signed short> > (key);
        //                    unsigned int iValue = iValueVect.size();
        //                    checkSizeOfVector(desc, scope, key, iValue, report);
        //                    checkRangeOfVectorElements(desc, scope, key, iValue, iValueVect, report);
        //                }
        //                    break;
        //
        //                case Types::VECTOR_INT32:
        //                {
        //                    vector<signed int> iValueVect = uParam.get < vector<signed int> > (key);
        //                    unsigned int iValue = iValueVect.size();
        //                    checkSizeOfVector(desc, scope, key, iValue, report);
        //                    checkRangeOfVectorElements(desc, scope, key, iValue, iValueVect, report);
        //                }
        //                    break;
        //
        //                case Types::VECTOR_INT64:
        //                {
        //                    vector<signed long long> iValueVect = uParam.get < vector<signed long long> > (key);
        //                    unsigned int iValue = iValueVect.size();
        //                    checkSizeOfVector(desc, scope, key, iValue, report);
        //                    checkRangeOfVectorElements(desc, scope, key, iValue, iValueVect, report);
        //                }
        //                    break;
        //
        //                case Types::VECTOR_UINT8:
        //                {
        //                    vector<unsigned char> iValueVect = uParam.get< vector<unsigned char> > (key);
        //                    unsigned int iValue = iValueVect.size();
        //                    checkSizeOfVector(desc, scope, key, iValue, report);
        //                    checkRangeOfVectorElements(desc, scope, key, iValue, iValueVect, report);
        //                }
        //                    break;
        //
        //                case Types::VECTOR_UINT16:
        //                {
        //                    vector<unsigned short> iValueVect = uParam.get< vector<unsigned short> > (key);
        //                    unsigned int iValue = iValueVect.size();
        //                    checkSizeOfVector(desc, scope, key, iValue, report);
        //                    checkRangeOfVectorElements(desc, scope, key, iValue, iValueVect, report);
        //                }
        //                    break;
        //
        //                case Types::VECTOR_UINT32:
        //                {
        //                    vector<unsigned int> iValueVect = uParam.get< vector<unsigned int> > (key);
        //                    unsigned int iValue = iValueVect.size();
        //                    checkSizeOfVector(desc, scope, key, iValue, report);
        //                    checkRangeOfVectorElements(desc, scope, key, iValue, iValueVect, report);
        //                }
        //                    break;
        //
        //                case Types::VECTOR_UINT64:
        //                {
        //                    vector<unsigned long long> iValueVect = uParam.get< vector<unsigned long long> > (key);
        //                    unsigned int iValue = iValueVect.size();
        //                    checkSizeOfVector(desc, scope, key, iValue, report);
        //                    checkRangeOfVectorElements(desc, scope, key, iValue, iValueVect, report);
        //                }
        //                    break;
        //
        //                case Types::VECTOR_DOUBLE:
        //                {
        //                    vector<double> iValueVect = uParam.get< vector<double> > (key);
        //                    unsigned int iValue = iValueVect.size();
        //                    checkSizeOfVector(desc, scope, key, iValue, report);
        //                    checkRangeOfVectorElements(desc, scope, key, iValue, iValueVect, report);
        //                }
        //                    break;
        //
        //                case Types::VECTOR_FLOAT:
        //                {
        //                    vector<float> iValueVect = uParam.get< vector<float> > (key);
        //                    unsigned int iValue = iValueVect.size();
        //                    checkSizeOfVector(desc, scope, key, iValue, report);
        //                    checkRangeOfVectorElements(desc, scope, key, iValue, iValueVect, report);
        //                }
        //                    break;
        //
        //                case Types::STRING:
        //                {
        //                    string iValue = uParam.get<string > (key);
        //                    if (desc.has("options")) {
        //                        checkOptions(scope, iValue, desc, report);
        //                    }
        //                    if (desc.has("minInc")) {
        //                        string eValue = desc.get<string > ("minInc");
        //                        checkMinInc(scope, iValue, eValue, report);
        //                    } else if (desc.has("minExc")) {
        //                        string eValue = desc.get<string > ("minExc");
        //                        checkMinExc(scope, iValue, eValue, report);
        //                    }
        //                    if (desc.has("maxInc")) {
        //                        string eValue = desc.get<string > ("maxInc");
        //                        checkMaxInc(scope, iValue, eValue, report);
        //                    } else if (desc.has("maxExc")) {
        //                        string eValue = desc.get<string > ("maxExc");
        //                        checkMaxExc(scope, iValue, eValue, report);
        //                    }
        //                }
        //                    break;
        //
        //                case Types::VECTOR_STRING:
        //                {
        //                    // validate number of elements in array: minSize, maxSize
        //                    vector<string> iValueVect = uParam.get< vector<string> > (key);
        //                    unsigned int iValue = iValueVect.size();
        //                    checkSizeOfVector(desc, scope, key, iValue, report);
        //                }
        //                    break;
        //
        //                case Types::PATH:
        //                {
        //                    boost::filesystem::path value = uParam.get<boost::filesystem::path > (key);
        //                    if (desc.has("options")) {
        //                        checkOptions<string > (scope, value.string(), desc, report);
        //                    }
        //                }
        //                    break;
        //
        //                case Types::VECTOR_PATH:
        //                {
        //                    // validate number of elements in array: minSize, maxSize
        //                    vector<boost::filesystem::path> iValueVect = uParam.get< vector<boost::filesystem::path> > (key);
        //                    unsigned int iValue = iValueVect.size();
        //                    checkSizeOfVector(desc, scope, key, iValue, report);
        //                }
        //                    break;
        //                default:
        //                    break;
        //                    //cout << " ### WARNING ### Range check on non-implemented on datatype " << Types::convert(eDataType) << endl;
        //            }
        //            // Set to working
        //            wParam[key] = uParam[key];
        //        }
        //
        //        void Schema::checkSizeOfVector(const Schema& desc, const string& scope, const string& key, unsigned int iValue, std::ostringstream & report) const {
        //            if (desc.has("minSize")) {
        //                unsigned int eValue = desc.getAs<unsigned int > ("minSize");
        //                if (iValue < eValue) {
        //                    report << "Number of elements in array " << scope << "." << key << " must be greater or equal " << eValue << "." << endl;
        //                }
        //            }
        //            if (desc.has("maxSize")) {
        //                unsigned int eValue = desc.getAs<unsigned int > ("maxSize");
        //                if (iValue > eValue) {
        //                    report << "Number of elements in array " << scope << "." << key << " must be less or equal " << eValue << "." << endl;
        //                }
        //            }
        //        }
        //
        //        void Schema::ensureValidCache() {
        //            if (m_keyToAlias.empty()) r_generateAliasMaps(*this, "");
        //        }
        //
        //        void Schema::help(const std::string & classId) {
        //
        //            ostringstream stream;
        //
        //            Schema expected;
        //            string searchPath;
        //
        //            try {
        //
        //                if (this->has("elements")) {
        //                    searchPath = this->get<string > ("root");
        //                    expected.set(searchPath, *this);
        //                    if (!classId.empty()) searchPath += "." + classId;
        //                } else {
        //                    searchPath = classId;
        //                    expected = *this;
        //                }
        //
        //
        //                if (searchPath.empty()) {
        //                    for (Schema::const_iterator it = expected.begin(); it != expected.end(); it++) {
        //                        stream << it->first << endl;
        //                    }
        //                } else if (searchPath.rfind(".") != string::npos) {
        //
        //                    stream << "----- HELP -----\n" << searchPath << endl;
        //
        //                    vector<string> tokens;
        //                    boost::split(tokens, searchPath, boost::is_any_of("."));
        //
        //                    r_processingPathToElem(expected, tokens, stream);
        //
        //                } else {
        //
        //                    stream << "----- HELP -----\n" << searchPath << endl;
        //
        //                    processingExpectParams(expected, searchPath, stream);
        //
        //                    stream << "------------------------------------------------------------" << endl;
        //                }
        //
        //            } catch (...) {
        //                KARABO_RETHROW;
        //            }
        //
        //            // ! show results !
        //            cout << "\n" << stream.str();
        //        }
        //
        //        void Schema::r_processingPathToElem(const Schema& expected, vector<string>& tokens, ostringstream & stream) {
        //
        //            try {
        //                bool tokenFound = false;
        //                size_t tokensSize = tokens.size();
        //                size_t j; //counter for tokens
        //
        //                if (tokensSize == 1) {
        //                    j = 0;
        //                } else if (tokensSize > 1) {
        //                    j = 1;
        //                }
        //
        //                Schema entry;
        //                Schema elements;
        //
        //                if (expected.has("root")) { //we got for processing SINGLE_ELEMENT
        //                    //string rootOfSingleElem = expected.get<string>("root");
        //                    string keyOfSingleElem = expected.get<string > ("key");
        //
        //                    if (tokens[0] == keyOfSingleElem) { //as expected
        //                        elements = expected.get<Schema > ("elements");
        //                    } else {
        //                        stream << "CHECK error. tokens[0]: " << tokens[0] << " != " << keyOfSingleElem << "  :keyOfSingleElem" << endl;
        //                    }
        //                } else {
        //                    entry = expected.get<Schema > (tokens[0]);
        //                    elements = entry.get<Schema > ("elements");
        //                }
        //                string currentToken = tokens[j];
        //
        //                bool lastToken = false;
        //                if (j == tokensSize - 1) {
        //                    lastToken = true;
        //                }
        //
        //                //cout << "currently searching for 'currentToken':" << currentToken << endl;
        //
        //                //iterate over elements 0=>Schema (SCHEMA), 1=>--||--, ... , N=>--||-- and check whether
        //                //one of them has 'key' equal to the given 'currentToken' 
        //                //As soon as such element found: 
        //                //1) check what kind of element it is: simpleType, complexType, or SINGLE_ELEMENT
        //                //2) process this element according to its type
        //                //3) while processing, check counter for tokens: show completed help for last token,
        //                //   or continue search for intermediate token (calling function recursively)
        //
        //                for (Schema::const_iterator ct = elements.begin(); ct != elements.end(); ct++) {
        //
        //                    const Schema& desc = elements.get<Schema > (ct);
        //
        //                    Schema::AssignmentType at = desc.get<Schema::AssignmentType > ("assignment");
        //
        //                    //find name of element (it's 'key'); element can be simple, complex, or SINGLE_ELEMENT
        //                    string elementName;
        //                    elementName = desc.getAsString("key"); //whatever element, find its 'key'
        //
        //                    //cout << "elementName=" << elementName << endl;
        //
        //                    if (elementName == currentToken) { //this is the element we are looking for ('key' equal to our 'token')
        //
        //                        tokenFound = true;
        //
        //                        if (at != Schema::INTERNAL_PARAM) {//consider here cases MANDATORY or OPTIONAL
        //                            if (desc.has("root")) { //it's a SINGLE_ELEMENT
        //
        //                                if (lastToken) { //this is the last token, show help for this element
        //                                    showSingleElement(desc, stream);
        //                                    return;
        //                                }
        //                                //this is not the last token, continue search
        //                                //remove currently found token from the vector and search later
        //                                tokens.erase(tokens.begin(), tokens.begin() + 1);
        //
        //                                //call recursively 
        //                                r_processingPathToElem(desc, tokens, stream);
        //
        //                            } else if (desc.has("complexType")) { //complexType element
        //
        //                                const Schema& complexElem = desc.get<Schema > ("complexType");
        //
        //                                if (lastToken) { //this is the last token, show this element
        //
        //                                    //1st variant: show only keys (OK)
        //                                    //std::set<string> keys = complexElem.keys();
        //                                    //for (std::set<string>::const_iterator iter=keys.begin(); iter!=keys.end(); ++iter){
        //                                    //     stream<< *iter << endl;
        //                                    //}                 
        //
        //                                    //2nd variant: show complete (OK)
        //                                    processingComplexType(complexElem, stream);
        //                                    return;
        //                                }
        //
        //                                //this is not the last token, take next token and continue search
        //                                //increase token's counter 
        //                                j = j + 1;
        //                                //consider next token 
        //                                const string& newToken = tokens[j];
        //
        //                                std::set<string> keys = complexElem.getKeysAsSet();
        //                                std::set<string>::const_iterator itKeys;
        //                                itKeys = keys.find(newToken);
        //
        //                                if (itKeys == keys.end()) { //token not found
        //                                    //this complex element does not contain a key we are looking for
        //                                    stream << "The following element '" << newToken << " ' is not a sub-element of '" << tokens[j - 1] << "'" << endl;
        //                                    return;
        //                                }
        //
        //                                //token was found
        //                                tokenFound = true;
        //
        //                                //remove currently found token from the vector and search further 
        //                                tokens.erase(tokens.begin(), tokens.begin() + 2); //remove 2 tokens       
        //
        //                                if (tokens.size() == 1) { //process element, ready
        //                                    processingExpectParams(complexElem, tokens[0], stream);
        //                                    return;
        //                                } else { //call recursively...
        //                                    r_processingPathToElem(complexElem, tokens, stream);
        //                                }
        //
        //                            } else if (desc.has("simpleType")) { // simple type element, that could be only a last-token 
        //
        //                                stream << "Element '" << elementName << "' is a simple element" << endl;
        //
        //                                processingDescription(desc, stream);
        //
        //                                if (!lastToken) { //if it is not last-token, just show the message to inform user
        //                                    stream << "Element '" << elementName << "' is a simple element! It does not contain any other elements." << endl;
        //                                }
        //
        //                                return;
        //                            }
        //
        //                        } //considering elements with AssignmentType != Schema::INTERNAL (i.e., OPTIONAL or MANDATORY)
        //
        //                    } //token found " if (elementName == currentToken) "
        //
        //                } //iterating over Schema-elements (Schema::const_iterator ct = elements.begin(); ct != elements.end(); ct++)
        //
        //                if (!tokenFound) { //token not found
        //                    stream << "No such element '" << currentToken << "' exists." << endl;
        //                    return;
        //                }
        //
        //            } catch (...) {
        //                KARABO_RETHROW;
        //            }
        //        }
        //
        //        void Schema::showSingleElement(const Schema& desc, ostringstream & stream) {
        //
        //            const Schema& elements = desc.get<Schema > ("elements");
        //
        //            for (Schema::const_iterator it = elements.begin(); it != elements.end(); it++) {
        //                const Schema& description = elements.get<Schema > (it);
        //                processingDescription(description, stream);
        //            }
        //        }
        //
        //        void Schema::processingComplexType(const Schema& complexElem, ostringstream & stream) {
        //
        //            for (Schema::const_iterator itk = complexElem.begin(); itk != complexElem.end(); itk++) {
        //
        //                const Schema& current = complexElem.get<Schema > (itk);
        //
        //                string currentName = current.getAsString("root");
        //
        //                stream << "\n" << currentName << "\n";
        //                processingExpectParams(complexElem, currentName, stream);
        //
        //                stream << "------------------------------------------------------------" << endl;
        //            }
        //
        //        }
        //
        //        void Schema::processingExpectParams(const Schema& expected, const std::string& classId, ostringstream & stream) {
        //            try {
        //
        //                const Schema& entry = expected.get<Schema > (classId);
        //
        //                const Schema& elements = entry.get<Schema > ("elements");
        //
        //                for (Schema::const_iterator it = elements.begin(); it != elements.end(); it++) {
        //
        //                    const Schema& description = elements.get<Schema > (it);
        //
        //                    processingDescription(description, stream);
        //
        //                }
        //            } catch (...) {
        //                KARABO_RETHROW;
        //            }
        //
        //        }
        //
        //        void Schema::processingDescription(const Schema& desc, ostringstream & stream) {
        //
        //            string key("");
        //            string description("");
        //            string type("");
        //            string defaultValue("");
        //            string assignment("");
        //            string options("");
        //            string range("");
        //            string accessTypePrint("");
        //            bool expertLevel = false;
        //            string untName("");
        //            string untSymbol("");
        //            string minSize("");
        //            string maxSize("");
        //            string displayType("");
        //
        //            key = desc.getAsString("key");
        //
        //            if (desc.has("description")) {
        //                desc.get("description", description);
        //            }
        //
        //            if (desc.has("access")) {
        //                AccessType accessT = desc.get<AccessType > ("access");
        //                switch (accessT) {
        //                    case 1: //INIT, init() in expectedParameters
        //                        accessTypePrint = "initialization";
        //                        break;
        //                    case 2: //READ, readOnly() in expectedParameters
        //                        accessTypePrint = "read only";
        //                        break;
        //                    case 4: //WRITE, reconfigurable() in expectedParameters
        //                        accessTypePrint = "reconfiguration";
        //                        break;
        //                }
        //            }
        //
        //            if (desc.has("simpleType")) {
        //                type = Types::convert(desc.get<Types::ReferenceType > ("simpleType"));
        //            } else if (desc.has("complexType")) {
        //                Schema::OccuranceType occ = desc.get<Schema::OccuranceType > ("occurrence");
        //                if (occ == Schema::EITHER_OR) {
        //                    type = "CHOICE";
        //                } else if (occ == Schema::ONE_OR_MORE) {
        //                    type = "NON_EMPTY_LIST";
        //                } else if (occ == Schema::ZERO_OR_MORE) {
        //                    type = "LIST";
        //                }
        //                if (desc.has("displayType")) {
        //                    displayType = desc.get<string > ("displayType");
        //                }
        //
        //            } else {//no simpleType and no complexType, that is, SINGLE_ELEMENT
        //                type = "SINGLE_ELEMENT";
        //                if (desc.has("displayType")) {
        //                    displayType = desc.get<string > ("displayType");
        //                }
        //            }
        //
        //            if (desc.has("default")) {
        //                defaultValue = desc.getAsString("default");
        //                if (type.rfind("VECTOR") != string::npos) { //print '(' if default value is Vector
        //                    defaultValue = "(" + defaultValue + ")";
        //                }
        //            }
        //
        //            if (desc.has("assignment")) {
        //                Schema::AssignmentType at = desc.get<Schema::AssignmentType > ("assignment");
        //                if (at == Schema::OPTIONAL_PARAM) {
        //                    assignment = "OPTIONAL";
        //                } else if (at == Schema::MANDATORY_PARAM) {
        //                    assignment = "MANDATORY";
        //                } else if (at == Schema::INTERNAL_PARAM) {
        //                    assignment = "INTERNAL";
        //                }
        //            }
        //
        //            if (desc.has("options")) {
        //
        //                string opt = desc.getAsString("options");
        //                vector<string> tmp;
        //                boost::split(tmp, opt, is_any_of(","));
        //                if (tmp.size() > 1) {
        //
        //                    for (size_t i = 0; i < tmp.size() - 1; ++i) {
        //                        options += tmp[i] + ", ";
        //                    }
        //                    options += tmp.back();
        //                } else {
        //                    options += desc.getAsString("options");
        //                }
        //
        //            } else {
        //                if (desc.has("minInc")) {
        //                    range += "[" + desc.getAsString("minInc") + ", ";
        //                } else if (desc.has("minExc")) {
        //                    range += "(" + desc.getAsString("minExc") + ", ";
        //                } else {
        //                    if (type.substr(0, 3) == "UIN") { //for example UINT32, ..
        //                        range += "[0, ";
        //                    } else {
        //                        range += "(-infty, ";
        //                    }
        //                }
        //                if (desc.has("maxInc")) {
        //                    range += desc.getAsString("maxInc") + "]";
        //                } else if (desc.has("maxExc")) {
        //                    range += desc.getAsString("maxExc") + ")";
        //                } else {
        //                    if (range.substr(0, 3) == "(-i") {
        //                        range = "";
        //                    } else {
        //                        range += "+infty)";
        //                    }
        //                }
        //            }
        //
        //            if (desc.has("expertLevel")) {
        //                int level = desc.get<int>("expertLevel");
        //                if (level > 0) {
        //                    expertLevel = true;
        //                }
        //            }
        //
        //            if (desc.has("unitName")) {
        //                desc.get("unitName", untName);
        //            }
        //
        //            if (desc.has("unitSymbol")) {
        //                untSymbol = "[" + desc.get<string > ("unitSymbol") + "]";
        //            }
        //
        //            if (desc.has("minSize")) { //minSize of Vector
        //                minSize = desc.getAsString("minSize");
        //            }
        //
        //            if (desc.has("maxSize")) { //maxSize of Vector
        //                maxSize = desc.getAsString("maxSize");
        //            }
        //
        //            stream << "\n    ." << key << " (" << type << ")";
        //            if (expertLevel) {
        //                stream << " [expert level]";
        //            }
        //            stream << endl;
        //
        //            stream << "         Assignment   : " << assignment << endl;
        //            if (description != "") {
        //                stream << "         Description  : " << description << endl;
        //            }
        //            if (defaultValue != "") {
        //                stream << "         Default value: " << defaultValue << endl;
        //            }
        //            if (displayType != "") {
        //                stream << "         Display type : " << displayType << endl;
        //            }
        //            if (options != "") {
        //                stream << "         Options      : " << options << endl;
        //            } else if (range != "") {
        //                stream << "         Range        : " << range << endl;
        //            }
        //            if (minSize != "") {
        //                stream << "         minSize      : " << minSize << endl;
        //            }
        //            if (minSize != "") {
        //                stream << "         maxSize      : " << maxSize << endl;
        //            }
        //            if (untName != "" || untSymbol != "") {
        //                stream << "         Units        : " << untName << " " << untSymbol << endl;
        //            }
        //            if (accessTypePrint != "") {
        //                stream << "         Usage        : " << accessTypePrint << endl;
        //            }
        //
        //        }//processingDescription(const Schema& desc, ostringstream& stream)
        //


    } // namespace util
} // namespace karabo
