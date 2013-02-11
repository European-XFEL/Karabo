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

namespace karabo {
    namespace util {

        using namespace std;

        Schema::Schema(const std::string& classId, const Schema::AssemblyRules& rules) :
        m_injectDefaults(true), m_assumeRootedConfiguration(false), m_allowAdditionalKeys(false), m_allowMissingKeys(false),
        m_currentAccessMode(rules.m_accessMode), m_currentState(rules.m_state), m_currentAccessRole(rules.m_accessRole), m_rootName(classId) {

            this->setRoot(classId);

        }

        void Schema::setRoot(const std::string& rootName) {
            m_hash.set(rootName, Hash()).setAttribute<int>("nodeType", Schema::ROOT);
        }

        karabo::util::Hash& Schema::getRoot() {
            return m_hash.begin()->getValue<Hash > ();
        }

        const karabo::util::Hash& Schema::getRoot() const {
            return m_hash.begin()->getValue<Hash > ();
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

        void Schema::setValidationRules(const Schema::ValidationRules& rules) {
            m_injectDefaults = rules.injectDefaults;
            m_allowAdditionalKeys = rules.allowAdditionalKeys;
            m_allowMissingKeys = rules.allowMissingKeys;
            m_assumeRootedConfiguration = rules.allowUnrootedConfiguration;
        }

        Schema::ValidationRules Schema::getValidationRules() const {
            Schema::ValidationRules rules;
            rules.injectDefaults = m_injectDefaults;
            rules.allowAdditionalKeys = m_allowAdditionalKeys;
            rules.allowMissingKeys = m_allowMissingKeys;
            rules.allowUnrootedConfiguration = m_assumeRootedConfiguration;
            return rules;
        }
        
        const std::string& Schema::getRootName() const {
            return m_rootName;
        }

        void Schema::setDisplayedName(const std::string& path, const std::string& value) {
            m_hash.setAttribute(path, "displayedName", value);
        }

        bool Schema::hasDisplayedName(const std::string& path) const {
            return m_hash.hasAttribute(path, "displayedName");
        }

        const std::string& Schema::getDisplayedName(const std::string& path) const {
            return m_hash.getAttribute<std::string > (path, "displayedName");
        }

        void Schema::setDescription(const std::string& path, const std::string& value) {
            m_hash.setAttribute(path, "description", value);
        }

        bool Schema::hasDescription(const std::string& path) const {
            return m_hash.hasAttribute(path, "description");
        }

        const std::string& Schema::getDescription(const std::string& path) const {
            return m_hash.getAttribute<std::string > (path, "description");
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

            this->getRoot().setNode(node);
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
            if (node.hasAttribute("allowedStates")) {
                const vector<string>& allowedStates = node.getAttribute<vector<string> >("allowedStates");
                return (std::find(allowedStates.begin(), allowedStates.end(), m_currentState) != allowedStates.end());
            } else { // If no states are assigned, access/visibility is always possible
                return true;
            }
        }

        ostream& operator<<(std::ostream& os, const Schema& schema) {
            os << schema.m_hash;
            return os;
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

        std::pair<bool, std::string> Schema::validate(const Hash& input, Hash& validatedOutput) const {

            // In case of failed validation, report why it failed
            ostringstream validationFailedReport;

            if (m_assumeRootedConfiguration) {
                if (input.size() != 1) {
                    return std::make_pair<bool, string > (false, "Expecting a rooted input, i.e. a Hash with exactly one key (describing the classId) at the top level");
                } else {
                    const Hash::Node& node = *(input.begin());
                    const std::string& classId = node.getKey();
                    if (m_rootName != classId) {
                        return std::make_pair<bool, string > (false, "Wrong schema for given input. Schema describes class \"" + m_rootName + "\", whilst input wants to configure class \"" + classId + "\"");
                    }
                    if (node.getType() == Types::HASH) {

                        Hash::Node& tmp = validatedOutput.set(classId, Hash());
                        r_validate(this->getRoot(), node.getValue<Hash > (), tmp.getValue<Hash > (), validationFailedReport, classId);

                        if (validationFailedReport.str().empty()) return std::make_pair<bool, string > (true, "");
                        else return std::make_pair<bool, string > (false, validationFailedReport.str());
                    } else {
                        return std::make_pair<bool, string > (false, "Root-node for given configuration is of wrong type. It must be HASH");
                    }
                }
            } else {
                r_validate(this->getRoot(), input, validatedOutput, validationFailedReport, "");
                if (validationFailedReport.str().empty()) return std::make_pair<bool, string > (true, "");
                else return std::make_pair<bool, string > (false, validationFailedReport.str());
            }
        }

        void Schema::r_validate(const Hash& master, const Hash& user, Hash& working, std::ostringstream& report, std::string scope) const {

            std::set<std::string> keys;
            user.getKeys(keys);

            // Iterate master
            for (Hash::const_iterator it = master.begin(); it != master.end(); ++it) {

                string key(it->getKey());

                string currentScope;
                if (scope.empty()) currentScope = key;
                else currentScope = scope + "." + key;

                int nodeType = it->getAttribute<int>("nodeType");
                bool userHasNode = user.has(key);
                int assignment = it->getAttribute<int>("assignment");
                bool hasDefault = it->hasAttribute("default");

                // Remove current node from all provided
                if (userHasNode) keys.erase(key); 

                if (nodeType == LEAF) {
                    if (!userHasNode) { // Node IS NOT provided
                        if (assignment == Schema::MANDATORY_PARAM) {
                            if (!m_allowMissingKeys) {
                                report << "Missing mandatory parameter: \"" << currentScope << "\"" << endl;
                                return;
                            }
                        } else if (assignment == Schema::OPTIONAL_PARAM && hasDefault && m_injectDefaults) {
                            working.set(key, it->getAttributeAsAny("default"));
                            // validateLeaf(*it, node, report);
                        }
                    } else { // Node IS provided
                        Hash::Node& node = working.setNode(user.getNode(key));
                        // validateLeaf(*it, node, report, currentScope);
                    }
                } else if (nodeType == NODE) {
                    if (!userHasNode) {
                        if (m_injectDefaults) {
                            Hash::Node& workNode = working.set(key, Hash()); // Insert empty node
                            r_validate(it->getValue<Hash > (), Hash(), workNode.getValue<Hash > (), report, currentScope);
                        } else {
                            Hash workFake;
                            r_validate(it->getValue<Hash > (), Hash(), workFake, report, currentScope);
                        }
                    } else {

                        if (user.getType(key) != Types::HASH) {
                            report << "Parameter \"" << currentScope << "\" has incorrect node type, expecting HASH not " << Types::to<ToLiteral > (user.getType(key)) << endl;
                            return;
                        } else {
                            Hash::Node& workNode = working.set(key, Hash()); // Insert empty node
                            r_validate(it->getValue < Hash > (), user.get<Hash > (key), workNode.getValue<Hash > (), report, currentScope);
                        }
                    }
                } else if (nodeType == CHOICE_OF_NODES) {
                    if (!userHasNode) {
                        if (assignment == Schema::MANDATORY_PARAM) {
                            if (!m_allowMissingKeys) {
                                report << "Missing (choice-)parameter: \"" << currentScope << "\"" << endl;
                                return;
                            }
                        } else {
                            if (assignment == Schema::OPTIONAL_PARAM && hasDefault && m_injectDefaults) {
                                std::string optionName = it->getAttribute<string > ("default");
                                Hash::Node& workNode = working.set(key, Hash(optionName, Hash())); // Inject empty choice
                                r_validate(it->getValue<Hash > ().get<Hash > (optionName), Hash(), workNode.getValue<Hash > ().get<Hash>(optionName), report, currentScope + "." + optionName);
                            }
                        }
                    } else { // User has set a node

                        std::set<std::string> validOptions;
                        master.get<Hash > (key).getKeys(validOptions);
                        
                        // If the option has all-default parameters the user lazily may have set the option as string instead of HASH
                        // We will allow for this and silently inject an empty Hash instead
                        if (user.getType(key) == Types::STRING) {
                             
                            cout << "Silently converting from STRING" << endl;
                            string optionName = user.get<string > (key);
                            if (validOptions.find(optionName) != validOptions.end()) { // Is a valid option
                                Hash::Node& workNode = working.set(key, Hash(optionName, Hash())); // Inject empty choice
                                r_validate(it->getValue<Hash > ().get<Hash > (optionName), Hash(), workNode.getValue<Hash > ().get<Hash>(optionName), report, currentScope + "." + optionName);
                            } else {
                                report << "Provided parameter: \"" << optionName << "\" is not a valid option for choice: \"" << key << "\". ";
                                report << "Valid options are: " << karabo::util::toString(validOptions) << endl;
                                return;
                            }
                        } else if (user.getType(key) != Types::HASH) {
                            report << "Parameter \"" << currentScope << "\" has incorrect type, expecting HASH not " << Types::to<ToLiteral > (user.getType(key));
                            return;
                        } else {

                            const Hash& choice = user.get<Hash > (key);
                            if (choice.size() == 0) {
                                if (assignment == Schema::MANDATORY_PARAM) {
                                    if (!m_allowMissingKeys) {
                                        report << "Missing option for choice: \"" << currentScope << "\". ";
                                        report << "Valid options are: " << karabo::util::toString(validOptions) << endl;
                                        return;
                                    }
                                } else {
                                    if (assignment == Schema::OPTIONAL_PARAM && hasDefault && m_injectDefaults) {
                                        std::string optionName = it->getAttribute<string > ("default");
                                        Hash::Node& workNode = working.set(key, Hash(optionName, Hash())); // Inject empty choice
                                        r_validate(it->getValue<Hash > ().get<Hash > (optionName), Hash(), workNode.getValue<Hash > ().get<Hash>(optionName), report, currentScope + "." + optionName);
                                    }
                                }
                            } else if (choice.size() == 1) { // That is what we expect it should be
                                const Hash::Node& usersOption = *(choice.begin());
                                const string& optionName = usersOption.getKey();
                                if (validOptions.find(optionName) != validOptions.end()) { // Is a valid option
                                    Hash::Node& workNode = working.set(key, Hash(optionName, Hash())); // Inject empty choice
                                    r_validate(it->getValue<Hash > ().get<Hash > (optionName), usersOption.getValue<Hash > (), workNode.getValue<Hash > ().get<Hash>(optionName), report, currentScope + "." + optionName);
                                } else {
                                    report << "Provided parameter: \"" << optionName << "\" is not a valid option for choice: \"" << key << "\". ";
                                    report << "Valid options are: " << karabo::util::toString(validOptions) << endl;
                                    return;
                                }
                            } else if (choice.size() > 1) {
                                std::vector<std::string> usersOptions;
                                choice.getKeys(usersOptions);
                                report << "Choice element \"" << key << "\" expects exactly one option, however multiple options (" << karabo::util::toString(usersOptions) << ") were provided. ";
                                report << "Valid options are: " << karabo::util::toString(validOptions) << endl;
                            }
                        }
                    }

                } else if (nodeType == LIST_OF_NODES) {
                    
                }
            }
            
            if (!m_allowAdditionalKeys && !keys.empty()) {
                BOOST_FOREACH(string key, keys) {
                    string currentScope;
                    if (scope.empty()) currentScope = key;
                    else currentScope = scope + "." + key;
                    report << "Encountered unexpected configuration parameter: \"" << currentScope << "\"" << endl;
                }
            }
            
        }

        //        void Schema::r_validate(const Schema& schema, Hash& user, Hash& working, ostringstream& report, string scope) const {
        //
        //            const string& node = schema.getRoot();
        //            const Schema& schemaParams = schema.getParameters();
        //
        //            // Update scope
        //            string nextScope(scope);
        //            if (!nextScope.empty()) nextScope += ".";
        //            nextScope += node;
        //
        //            if (!user.has(node)) { // User configuration does not provide this node
        //
        //                // The only exception is a missing parent node if the correct flag is enabled
        //                if (scope.empty() && m_allowUnrootedConfiguration) {
        //                    // Ok
        //                } else {
        //                    report << "Missing node \"" << node << "\"" << endl;
        //                    return;
        //                }
        //
        //            } else { // User configuration has this node
        //
        //                // Add current node to working configuration
        //                if (!working.has(node)) working.set(node, Hash());
        //
        //                // The valueType for node has to be a Hash object, however no value may be given on the command line 
        //                // and an empty string will be generated. In this case, an empty Hash object is inserted, instead.
        //                Types::ReferenceType nodeValueType = user.getTypeAsId(node);
        //                if (nodeValueType == Types::STRING && user.get<string > (node).empty()) { // Exception from rule
        //                    user.set(node, Hash());
        //                    //working.set(node, Hash());
        //                } else if (nodeValueType != Types::HASH) { // Default behavior
        //                    report << "Invalid assignment \"" << user.getAsString(node) << "\" to scope \"" << nextScope << "\"" << endl;
        //                    report << "Use \"" << nextScope << ".\" instead of \"" << nextScope << "=\"" << endl;
        //                    // showPossibleParameters(mParams, report); TODO
        //                    throw KARABO_PARAMETER_EXCEPTION(report.str());
        //                }
        //
        //            }
        //
        //            // Retrieve user parameters for current classId
        //            Hash& userParams = user.get<Hash > (node);
        //            Hash& workingParams = working.get<Hash > (node);
        //            std::set<string> sufficientParams = userParams.getKeysAsSet();
        //
        //            // Loop all schema-parameters of current scope
        //            for (Schema::const_iterator it = schemaParams.begin(); it != schemaParams.end(); it++) {
        //
        //                // Fetch individual description
        //                const Schema& desc = schemaParams.get<Schema > (it);
        //
        //                string key = desc.getKey();
        //                Schema::AssignmentType assignment = desc.getAssignment();
        //                bool assert = false;
        //
        //                if (!userParams.has(key)) { // User has NOT set parameter
        //
        //                    if (m_allowMissingKeys) { // If missing keys are allowed we do not assert
        //                        assert = false;
        //                    } else { // Pick out the cases where we have to assert
        //
        //                        if (assignment == Schema::OPTIONAL_PARAM && desc.hasDefaultValue()) { // There is a default
        //                            if (m_injectDefaults) {
        //                                applyDefault(key, desc, userParams, workingParams);
        //                                assert = true;
        //                            }
        //                        } else if (assignment == Schema::MANDATORY_PARAM) {
        //                            assert = true;
        //                        } else if (desc.hasRoot()) { // We need to prepare here for later (deeper recursion) default injection
        //                            if (m_injectDefaults) {
        //                                userParams.set(key, Hash());
        //                                if (!workingParams.has(key)) workingParams.set(key, Hash());
        //                                assert = true;
        //                            }
        //                        }
        //                    }
        //
        //                } else { // User has set parameter
        //
        //                    sufficientParams.erase(key); // Remove from list
        //
        //                    if (!workingParams.has(key)) {
        //                        // Do not blindly copy the whole tree, if we are still recursively going down
        //                        if (!desc.isLeaf() && userParams.is<Hash > (key)) {
        //                            workingParams.set(key, Hash());
        //                        } else {
        //                            workingParams[key] = userParams[key];
        //                        }
        //                    }
        //                    assert = true;
        //                }
        //
        //                if (assert) {
        //                    if (desc.isNode()) { // This is a direct child -> shortcut
        //                        r_validate(desc, userParams, workingParams, report, nextScope);
        //                    } else if (desc.isLeaf()) {
        //                        assertSimpleType(desc, userParams, workingParams, report, nextScope);
        //                    } else if (desc.has("complexType")) {
        //                        assertComplexType(desc, userParams, workingParams, report, nextScope);
        //                    }
        //                }
        //            }
        //
        //            // Complain about too many keys (if correct flag is set)
        //            if (!m_allowAdditionalKeys) reportNotNeededInformation(scope, sufficientParams);
        //
        //        }
        //
        //        void Schema::reportNotNeededInformation(const string& scope, const std::set<string>& sufficientParameters) const {
        //            ostringstream notNeededMessage;
        //
        //            BOOST_FOREACH(string key, sufficientParameters) {
        //                string fullKey = key;
        //                if (!scope.empty()) fullKey = scope + "." + key;
        //                notNeededMessage << "Encountered unexpected configuration parameter \"" << fullKey << "\"" << endl;
        //            }
        //            string out(notNeededMessage.str());
        //            if (!out.empty()) {
        //                throw KARABO_PARAMETER_EXCEPTION(out);
        //            }
        //        }
        //
        //        void Schema::applyDefault(const string& key, const Schema& desc, Hash& uParam, Hash & wParam) const {
        //
        //            if (wParam.has(key)) {
        //                uParam[key] = wParam[key];
        //            } else {
        //                Schema::const_iterator it = desc.find("default");
        //                if (desc.has("simpleType")) {
        //                    uParam[key] = it->second;
        //                } else if (desc.has("complexType")) {
        //                    string defaultValue = desc.get<string > ("default");
        //                    Schema::OccuranceType occ = desc.get<Schema::OccuranceType > ("occurrence");
        //                    if (occ == Schema::ONE_OR_MORE || occ == Schema::ZERO_OR_MORE) {
        //                        boost::trim(defaultValue);
        //                        vector<string> v;
        //                        boost::split(v, defaultValue, is_any_of(" ,;"));
        //                        vector<Hash> configs;
        //
        //                        BOOST_FOREACH(string option, v) {
        //                            boost::trim(option);
        //                            if (option != "") {
        //                                configs.push_back(Hash(option, Hash()));
        //                            }
        //                        }
        //                        uParam.set(key, configs);
        //                    } else {
        //                        uParam.set(key, Hash(defaultValue, Hash()));
        //                    }
        //                } else if (desc.has("root")) {
        //                    uParam.set(key, Hash());
        //                }
        //                wParam[key] = uParam[key];
        //            }
        //        }
        //
        //        void Schema::assertComplexType(const Schema& desc, Hash& uParam, Hash& wParam, ostringstream& report, string & scope) const {
        //
        //            const string& key = desc.getAsString("key");
        //
        //            string nextScope = scope + "." + key;
        //            const Schema& mComplex = desc.get<Schema > ("complexType");
        //            Schema::OccuranceType occurance = desc.get<Schema::OccuranceType > ("occurrence");
        //
        //            switch (occurance) {
        //                case Schema::EITHER_OR:
        //                {
        //                    if (!uParam.has(key)) {
        //                        report << "Choice-type parameter \"" << nextScope << "\" is missing" << endl;
        //                    } else {
        //                        Hash& uComplex = uParam.get<Hash > (key);
        //                        if (!wParam.has(key)) {
        //                            wParam.set(key, Hash());
        //                        }
        //                        Hash& wComplex = wParam.get<Hash > (key);
        //                        assertOccuranceEitherOr(mComplex, uComplex, wComplex, report, nextScope);
        //                    }
        //                }
        //                    break;
        //                case Schema::ZERO_OR_MORE:
        //                {
        //                    if (!uParam.has(key)) {
        //                        report << "List-type parameter \"" << nextScope << "\" is missing" << endl;
        //                    } else if (uParam.getTypeAsId(key) != Types::VECTOR_HASH) {
        //                        reportWrongComplexTypeFormat(nextScope);
        //                    } else {
        //                        vector<Hash>& uComplex = uParam.get<vector<Hash> > (key);
        //                        if (!wParam.has(key)) {
        //                            wParam.set(key, vector<Hash > ());
        //                        }
        //                        vector<Hash>& wComplex = wParam.get<vector<Hash> >(key);
        //                        assertOccuranceZeroOrMore(mComplex, uComplex, wComplex, report, nextScope);
        //                    }
        //                }
        //                    break;
        //                case Schema::ONE_OR_MORE:
        //                {
        //
        //                    if (!uParam.has(key)) {
        //                        report << "List-type parameter \"" << nextScope << "\" is missing" << endl;
        //                    } else if (uParam.getTypeAsId(key) != Types::VECTOR_HASH) {
        //                        reportWrongComplexTypeFormat(nextScope);
        //                    } else {
        //                        vector<Hash>& uComplex = uParam.get<vector<Hash> > (key);
        //                        if (!wParam.has(key)) {
        //                            wParam.set(key, vector<Hash > ());
        //                        }
        //                        vector<Hash>& wComplex = wParam.get<vector<Hash> >(key);
        //                        assertOccuranceOneOrMore(mComplex, uComplex, wComplex, report, nextScope);
        //                    }
        //                }
        //                    break;
        //                default:
        //                    throw KARABO_LOGIC_EXCEPTION("This should never happen, faced illegal occurance type");
        //            }
        //        }
        //
        //        void Schema::reportWrongComplexTypeFormat(const string & scope) const {
        //            ostringstream message;
        //            message << "The elements of parameter \"" << scope << "\" may be repeated (list-type parameter), ";
        //            message << "thus expecting one of the following syntax options:" << endl;
        //            message << "(1) \"" << scope << "[0]\"    (Sets/edits an element at a defined position (here 0))" << endl;
        //            message << "(2) \"" << scope << "[next]\" (Appends a new element to the list)" << endl;
        //            message << "(3) \"" << scope << "[last]\" (Addresses the last element in the list)" << endl;
        //            throw KARABO_PARAMETER_EXCEPTION(message.str());
        //        }
        //
        //        void Schema::assertOccuranceEitherOr(const Schema& mComplex, Hash& uComplex, Hash& wComplex, ostringstream& report, const string & scope) const {
        //            if (uComplex.size() == 1) {
        //                const string& classIdChoice = uComplex.begin()->first;
        //                if (!mComplex.has(classIdChoice)) {
        //                    report << "Current choice \"" << classIdChoice << "\" for parameter \"" << scope << "\" is not one of the valid choices: " << String::mapKeyToString(mComplex) << endl;
        //                } else {
        //
        //                    // Prepare working parameter
        //                    if (!wComplex.has(classIdChoice)) {
        //                        wComplex.clear();
        //                        wComplex.set(classIdChoice, Hash());
        //                    }
        //
        //                    // Go recursive
        //                    r_validate(mComplex.get<Schema > (classIdChoice), uComplex, wComplex, report, scope);
        //                }
        //            } else if (uComplex.empty()) {
        //                report << "Please take a choice for parameter \"" << scope << "\". Valid choices are: " << String::mapKeyToString(mComplex) << endl;
        //            } else {
        //                report << "Please select only one choice for parameter \"" << scope << "\". Valid choices are: " << String::mapKeyToString(mComplex) << endl;
        //            }
        //        }
        //
        //        void Schema::assertOccuranceZeroOrMore(const Schema& mComplex, vector<Hash>& uComplex, vector<Hash>& wComplex, ostringstream& report, const string & scope) const {
        //
        //            wComplex.reserve(uComplex.size());
        //            size_t lastElementIdx = uComplex.size() - 1;
        //            for (size_t i = 0; i < uComplex.size(); ++i) {
        //                if (uComplex[i].empty()) {
        //                    if (i < lastElementIdx) { // Empty element in array (i.e. Application.modules[0] )
        //                        // Skip over empty elements
        //                        continue;
        //                    } else {
        //                        uComplex.pop_back();
        //                    }
        //                } else {
        //                    string classIdChoice = uComplex[i].begin()->first;
        //                    if (uComplex[i].empty() || !mComplex.has(classIdChoice)) {
        //                        report << "Current choice \"" << classIdChoice << "\" for list-type parameter \"" << scope << "\" is not one of the valid choices: " << String::mapKeyToString(mComplex) << endl;
        //                    } else {
        //                        // Prepare working parameter
        //                        string newScope = scope + "[" + String::toString(i) + "]";
        //                        if (i >= wComplex.size()) {
        //                            wComplex.resize(i + 1);
        //                        }
        //                        if (!wComplex[i].has(classIdChoice)) {
        //                            wComplex[i].clear();
        //                            wComplex[i].set(classIdChoice, Hash());
        //                        }
        //                        // Go recursive
        //                        r_validate(mComplex.get<Schema > (classIdChoice), uComplex[i], wComplex[i], report, newScope);
        //                    }
        //                }
        //            }
        //        }
        //
        //        void Schema::assertOccuranceOneOrMore(const Schema& mComplex, vector<Hash>& uComplex, vector<Hash>& wComplex, ostringstream& report, const string & scope) const {
        //            assertOccuranceZeroOrMore(mComplex, uComplex, wComplex, report, scope);
        //            if (uComplex.empty()) {
        //                report << "At least one of the choices: " << String::mapKeyToString(mComplex) << " has to be used for list-type parameter \"" << scope << "\"" << endl;
        //                report << "A valid input could be: " << "\"" << scope << "[0]." << mComplex.begin()->first << "\"" << endl;
        //            }
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
