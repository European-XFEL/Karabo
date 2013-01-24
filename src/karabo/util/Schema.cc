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
        using namespace boost;

        std::vector<std::string> Schema::getAllParameters() {
            ensureValidCache();
            std::vector<string> tmp;
            for (std::map<std::string, Schema*>::const_iterator it = m_keys.begin(); it != m_keys.end(); ++it) {
                tmp.push_back(it->first);
            }
            return tmp;
        }

        // TODO This should potentially go the constructor now (after Hash/Config refactoring)

        Schema& Schema::initParameterDescription(const string& key, AccessType accessMode, const std::string& currentState) {
            m_inputOrder = 0;
            m_access = accessMode;
            m_currentState = currentState;
            // Set some defaults for factory injected nodes
            if (!has("access")) access(INIT | READ | WRITE);
            if (!has("description")) description("");
            if (!has("displayedName")) displayedName("");
            set("root", key);
            Schema tmp(m_access, m_currentState);
            set("elements", tmp);
            return get<Schema > ("elements");
        }

        Schema& Schema::addElement(Schema& item) {

            // If "overwriteDefault" is available this is a special element which just servers the purpose of overwriting
            // some other defaults specified above in hierarchy
            // This functions does not go recursive this exits immediately
            if (item.has("overwriteDefault")) {
                overwriteNestedDefaults(item);
                return *this;
            }

            // Assert description of expected parameters
            if (!item.has("access")) item.access(INIT); // assume only initial configuration parameter if not specified

            // We stop here if the access type does not fit
            if (!(m_access & item.get<AccessType > ("access"))) return *this;

            // We stop here if the allowed states do not fit the current one
            if (!m_currentState.empty()) {
                if (item.has("allowedStates")) {
                    const std::vector<std::string>& allowedStates = item.get<std::vector<std::string> >("allowedStates");
                    if (std::find(allowedStates.begin(), allowedStates.end(), m_currentState) == allowedStates.end()) {
                        //if (allowedStates.find(m_currentState) == allowedStates.end()) {
                        return *this;
                    }
                }
            }

            // Set access mode for children
            if (item.has("complexType")) item.setAccessMode(m_access);

            // Set current state for children
            if (item.has("complexType")) item.setCurrentState(m_currentState);

            // Setup defaults for other elements
            if (!item.has("expertLevel")) {
                item.set("expertLevel", (int) 0);
            }
            string keyValue;
            if (!item.has("root")) {

                if (!item.has("key")) throw KARABO_PARAMETER_EXCEPTION("Invalid parameter description, no key (variable name) is set");
                if ((!item.has("simpleType")) && (!item.has("complexType"))) throw KARABO_PARAMETER_EXCEPTION("Invalid parameter description. You have to use either of the functions: simpleType, complexType");

                overwriteIfDuplicated(item.get<string > ("key"));
                keyValue = item.get<string > ("key");

            } else {
                keyValue = item.get<string > ("root");
            }
            
            // Check for existence of mandatory meta-information 
            if (!item.has("assignment")) throw KARABO_PARAMETER_EXCEPTION("Invalid parameter description, no assignment type is set for key : " + keyValue);
            if (!item.has("displayedName")) throw KARABO_PARAMETER_EXCEPTION("Invalid parameter description, no displayedName is given for key : " + keyValue);
            
            // Increase the item count and finally add to this
            ostringstream ss;
            ss << "x" << setw(4) << setfill('0') << m_inputOrder++;
            //string key = String::toString(m_inputOrder++, 4);
            string key = ss.str();
            set(key, item);
            return get<Schema > (key);
        }

        Schema& Schema::addExternalSchema(const Schema& schema) {
            Schema& currentElements = get<Schema > ("elements");
            const Schema& inputElements = schema.get<Schema > ("elements");
            for (const_iterator it = inputElements.begin(); it != inputElements.end(); it++) {
                Schema item = inputElements.get<Schema > (it);
                currentElements.addElement(item);
            }
            return *this;
        }

        void Schema::overwriteIfDuplicated(const std::string& key) {
            vector<iterator> deleteMe;
            for (iterator it = this->begin(); it != this->end(); it++) {
                const Schema& element = get<Schema > (it);
                if (element.has("key") && element.get<string > ("key") == key) {
                    deleteMe.push_back(it);
                }
            }
            for (size_t i = 0; i < deleteMe.size(); ++i) {
                this->erase(deleteMe[i]);
            }
        }

        void Schema::overwriteNestedDefaults(Schema& item) {
            string key = item.get<string > ("key");
            vector<string> tokens;
            boost::split(tokens, key, boost::is_any_of("."));
            Schema* level = this;
            bool wasFound = true;
            for (size_t i = 0; i + 1 < tokens.size() - 1; i += 2) {
                wasFound = false;
                for (iterator it = level->begin(); it != level->end(); it++) {
                    Schema& element = level->get<Schema > (it);
                    iterator jt = element.find("complexType");
                    if (jt != element.end() && element.get<string > ("key") == tokens[i]) {
                        Schema& complexTypes = element.get<Schema > (jt);
                        iterator kt = complexTypes.find(tokens[i + 1]);
                        if (kt != complexTypes.end()) {
                            Schema& next = complexTypes.get<Schema > (kt).get<Schema > ("elements");
                            level = &next;
                            wasFound = true;
                            break;
                        }
                    }
                }
                if (!wasFound) break;
            }
            if (wasFound) {
                for (iterator it = level->begin(); it != level->end(); it++) {
                    Schema& element = level->get<Schema > (it);
                    iterator jt = element.find("key");
                    if (jt != element.end()) {
                        if (tokens.back() == element.get<string > (jt)) {
                            element["default"] = item["overwriteDefault"];
                        }
                    }
                }
            }
        }

        void Schema::setAccessMode(AccessType at) {
            m_access = at;
        }

        void Schema::setCurrentState(const std::string& currentState) {
            m_currentState = currentState;
        }

        AccessType Schema::getAccessMode() {
            return m_access;
        }

        const std::string& Schema::getCurrentState() {
            return m_currentState;
        }

        void Schema::key(const std::string& parameterKey) {
            set("key", parameterKey);
        }

        void Schema::displayedName(const std::string& name) {
            set("displayedName", name);
        }

        void Schema::assignment(AssignmentType ass) {
            set("assignment", ass);
        }

        void Schema::occurance(OccuranceType occ) {
            set("occurrence", occ);
        }

        void Schema::options(const std::string& options, const std::string& sep) {
            string s(options);
            boost::trim(s);
            vector<string> v, ret;
            boost::split(v, s, boost::is_any_of(sep));

            BOOST_FOREACH(string option, v) {
                boost::trim(option);
                if (option != "") {
                    ret.push_back(option);
                }
            }
            this->set("options", ret);
        }

        void Schema::allowedStates(const std::string& states, const std::string& sep) {
            string s(states);
            boost::trim(s);
            std::vector<std::string> v;
            std::vector<std::string> allowedStates;
            boost::split(v, s, boost::is_any_of(sep));

            BOOST_FOREACH(string token, v) {
                boost::trim(token);
                if (!token.empty()) {
                    allowedStates.push_back(token);
                }
            }
            this->set("allowedStates", allowedStates);
        }

        void Schema::options(const vector<string>& options) {
            set("options", options);
        }

        void Schema::description(const std::string& description) {
            set("description", description);
        }

        void Schema::expertLevel(ExpertLevelType level) {
            set("expertLevel", (int) level);
        }

        void Schema::access(AccessType at) {
            set("access", at);
        }

        void Schema::simpleType(const Types::ReferenceType type) {
            set("simpleType", type);
        }

        void Schema::complexType(const Schema& type) {
            set("complexType", type);
        }

        void Schema::choiceType(const Schema& elements) {
            complexType(elements);
            occurance(Schema::EITHER_OR);
        }

        void Schema::listType(const Schema& elements) {
            complexType(elements);
            occurance(Schema::ZERO_OR_MORE);
        }

        void Schema::nonEmptyListType(const Schema& elements) {
            complexType(elements);
            occurance(Schema::ONE_OR_MORE);
        }

        void Schema::singleElementType(const Schema& element) {
            append(element);
        }

        void Schema::r_generateAliasMaps(Schema& config, std::string path) {
            std::string key = path;
            iterator keyIt = config.find("key");
            iterator aliasIt = config.find("alias");
            if (keyIt != config.end()) {
                key = config.get<string > (keyIt);
                if (!path.empty()) key = path + "." + key;
                if (aliasIt != config.end()) {
                    m_key2alias[key] = aliasIt->second;
                    m_alias2key[config.getAsString(aliasIt)] = key;
                }
                m_keys.insert(std::make_pair(key, &config));
            }
            iterator elementsIt = config.find("elements");
            if (elementsIt != config.end()) {
                Schema& tmp = config.get<Schema > (elementsIt);
                for (iterator it = tmp.begin(); it != tmp.end(); ++it) {
                    Schema& param = tmp.get<Schema > (it);
                    r_generateAliasMaps(param, key);
                }
            }
            iterator complexTypeIt = config.find("complexType");
            if (complexTypeIt != config.end()) {
                Schema& tmp = config.get<Schema > (complexTypeIt);
                for (iterator it = tmp.begin(); it != tmp.end(); ++it) {
                    Schema& entry = tmp.get<Schema > (it);
                    string complexKey = key + "." + it->first;
                    m_keys.insert(std::make_pair(complexKey, &entry));
                    r_generateAliasMaps(entry, complexKey);
                }
            }
        }

        void Schema::r_toStream(ostream& os, const Hash& config, int depth) const {

            string fill(depth * 2, ' ');
            for (Schema::const_iterator it = config.begin(); it != config.end(); it++) {
                Types::ReferenceType type = config.getTypeAsId(it);
                string typeString = config.getTypeAsString(it);
                bool isHandled = handleStandardTypes(os, config, it, fill);
                if (!isHandled) {
                    switch (type) {
                        case Types::SCHEMA:
                        {
                            os << fill << it->first << " => " << "Schema" << " (" << typeString << ") " << endl;
                            r_toStream(os, config.get<Schema > (it), depth + 1);
                        }
                            break;
                        case Types::VECTOR_HASH:
                        {
                            os << fill << it->first << " => " << "Schema[]" << " (" << typeString << ") " << endl;
                            const vector<Schema>& tmp = config.get<vector<Schema> >(it);
                            for (size_t i = 0; i < tmp.size(); ++i) {
                                os << fill << "[" << i << "]" << endl;
                                r_toStream(os, tmp[i], depth + 1);
                            }
                        }
                            break;
                        case Types::OCCURANCE_TYPE:
                            os << fill << it->first << " => " << config.get<Schema::OccuranceType > (it) << " (" << typeString << ") " << endl;
                            break;
                        case Types::ASSIGNMENT_TYPE:
                            os << fill << it->first << " => " << config.get<Schema::AssignmentType > (it) << " (" << typeString << ") " << endl;
                            break;
                        case Types::EXPERT_LEVEL_TYPE:
                            os << fill << it->first << " => " << config.get<Schema::ExpertLevelType > (it) << " (" << typeString << ") " << endl;
                            break;
                        case Types::ACCESS_TYPE:
                            os << fill << it->first << " => " << config.get<AccessType > (it) << " (" << typeString << ") " << endl;
                            break;
                        default:
                            os << fill << it->first << " => " << "UNKNOWN" << " (" << "UNKNOWN" << ") " << endl;
                    }
                }
            }
        }

        Hash Schema::mergeUserInput(const vector<Hash>& userConfigurations) {

            Hash mergedConfiguration;

            // Set up flags for this merging
            m_injectDefaults = true;
            m_allowUnrootedConfiguration = false;
            m_allowAdditionalKeys = false;
            m_allowMissingKeys = false;

            // Merge in all user configurations
            for (size_t i = 0; i < userConfigurations.size(); ++i) {
                Hash userConfiguration = userConfigurations[i];
                ostringstream intermediateReport;

                r_validate(*this, userConfiguration, mergedConfiguration, intermediateReport);

            }

            return mergedConfiguration;
        }

        Hash Schema::validate(const Hash& user, bool injectDefaults, bool allowUnrootedConfiguration, bool allowAdditionalKeys, bool allowMissingKeys) {

            // Ensure that this schema class is rooted
            if (!this->hasRoot()) throw KARABO_LOGIC_EXCEPTION("This schema class is not rooted. Produce the schema using the static methods of the factory base class");

            // Set up flags for this validation
            m_injectDefaults = injectDefaults;
            m_allowUnrootedConfiguration = allowUnrootedConfiguration;
            m_allowAdditionalKeys = allowAdditionalKeys;
            m_allowMissingKeys = allowMissingKeys;


            // The result of the validation
            Hash workingConfiguration;

            // In case of failed validation, report containing details
            ostringstream validationFailedReport;

            Hash tmp(user);

            r_validate(*this, tmp, workingConfiguration, validationFailedReport);

            // Convert to string
            string report(validationFailedReport.str());

            if (!report.empty()) { // Should be empty upon successful assertion
                throw KARABO_PARAMETER_EXCEPTION("Parameter validation failed.\n\n" + report);
            }

            // Return valid configuration
            return workingConfiguration;
        }

        Hash Schema::injectRootedDefaults(const Hash& user) const {
            Hash injected(user);
            if (this->hasRoot()) {
                const std::string& root = this->getRoot();
                if (!injected.has(root)) {
                    injected.set(root, Hash());
                    r_injectDefaults(*this, injected.get<Hash > (root));
                } else {
                    r_injectDefaults(*this, injected);
                }
            } else {
                throw KARABO_LOGIC_EXCEPTION("Schema is un-rooted, use MyFactoryBase::expectedParameters(\"MyFactorizedClass\") for schema creation");
            }
            return injected;
        }

        Hash Schema::injectUnrootedDefaults(const Hash& user) const {
            Hash injected(user);
            if (this->hasRoot()) {
                r_injectDefaults(*this, injected);
            } else {
                throw KARABO_LOGIC_EXCEPTION("Schema is un-rooted, use MyFactoryBase::expectedParameters(\"MyFactorizedClass\") for schema creation");
            }
            return injected;
        }

        void Schema::r_injectDefaults(const Schema& schema, Hash& hash) const {

            if (schema.hasParameters()) {

                const Schema& parameters = schema.getParameters();
                for (Schema::const_iterator it = parameters.begin(); it != parameters.end(); ++it) {

                    const Schema& parameter = parameters.get<Schema > (it);

                    if (parameter.isLeaf()) {
                        const std::string& key = parameter.getKey();
                        Schema::const_iterator it = parameter.find("default");
                        if (it != parameter.end()) {
                            if (!hash.has(key)) hash[key] = it->second;
                        }

                    } else if (parameter.isNode()) {
                        const std::string& root = parameter.getRoot();
                        if (!hash.has(root)) hash.set(root, Hash());
                        r_injectDefaults(parameter, hash.get<Hash > (root));

                    } else if (parameter.isChoiceOfNodes()) {
                        const std::string& key = parameter.getKey();
                        if (parameter.hasDefaultValue()) {
                            const std::string& defaultNode = parameter.getDefaultValue<std::string > ();
                            std::string path = key + "." + defaultNode;
                            if (!hash.has(key)) hash.setFromPath(path, Hash());
                            r_injectDefaults(parameter.getParameters().get<Schema > (defaultNode), hash.getFromPath<Hash > (path));
                        }

                    } else if (parameter.isListOfNodes() || parameter.isNonEmptyListOfNodes()) {
                        const std::string& key = parameter.getKey();
                        if (parameter.hasDefaultValue()) {
                            const std::vector<std::string>& defaultNodes = parameter.getDefaultValue<std::vector<std::string> >();
                            for (size_t i = 0; i < defaultNodes.size(); ++i) {
                                const std::string& defaultNode = defaultNodes[i];
                                std::string path = key + "." + defaultNode;
                                if (!hash.hasFromPath(path)) hash.setFromPath(path, Hash());
                                r_injectDefaults(parameter.getParameters().get<Schema > (defaultNode), hash.getFromPath<Hash > (path));
                            }
                        }
                    }
                }
            }
        }

        void Schema::r_validate(const Schema& schema, Hash& user, Hash& working, ostringstream& report, string scope) const {

            const string& node = schema.getRoot();
            const Schema& schemaParams = schema.getParameters();

            // Update scope
            string nextScope(scope);
            if (!nextScope.empty()) nextScope += ".";
            nextScope += node;

            if (!user.has(node)) { // User configuration does not provide this node

                // The only exception is a missing parent node if the correct flag is enabled
                if (scope.empty() && m_allowUnrootedConfiguration) {
                    // Ok
                } else {
                    report << "Missing node \"" << node << "\"" << endl;
                    return;
                }

            } else { // User configuration has this node

                // Add current node to working configuration
                if (!working.has(node)) working.set(node, Hash());

                // The valueType for node has to be a Hash object, however no value may be given on the command line 
                // and an empty string will be generated. In this case, an empty Hash object is inserted, instead.
                Types::ReferenceType nodeValueType = user.getTypeAsId(node);
                if (nodeValueType == Types::STRING && user.get<string > (node).empty()) { // Exception from rule
                    user.set(node, Hash());
                    //working.set(node, Hash());
                } else if (nodeValueType != Types::HASH) { // Default behavior
                    report << "Invalid assignment \"" << user.getAsString(node) << "\" to scope \"" << nextScope << "\"" << endl;
                    report << "Use \"" << nextScope << ".\" instead of \"" << nextScope << "=\"" << endl;
                    // showPossibleParameters(mParams, report); TODO
                    throw KARABO_PARAMETER_EXCEPTION(report.str());
                }

            }

            // Retrieve user parameters for current classId
            Hash& userParams = user.get<Hash > (node);
            Hash& workingParams = working.get<Hash > (node);
            std::set<string> sufficientParams = userParams.getKeysAsSet();

            // Loop all schema-parameters of current scope
            for (Schema::const_iterator it = schemaParams.begin(); it != schemaParams.end(); it++) {

                // Fetch individual description
                const Schema& desc = schemaParams.get<Schema > (it);

                string key = desc.getKey();
                Schema::AssignmentType assignment = desc.getAssignment();
                bool assert = false;

                if (!userParams.has(key)) { // User has NOT set parameter

                    if (m_allowMissingKeys) { // If missing keys are allowed we do not assert
                        assert = false;
                    } else { // Pick out the cases where we have to assert

                        if (assignment == Schema::OPTIONAL_PARAM && desc.hasDefaultValue()) { // There is a default
                            if (m_injectDefaults) {
                                applyDefault(key, desc, userParams, workingParams);
                                assert = true;
                            }
                        } else if (assignment == Schema::MANDATORY_PARAM) {
                            assert = true;
                        } else if (desc.hasRoot()) { // We need to prepare here for later (deeper recursion) default injection
                            if (m_injectDefaults) {
                                userParams.set(key, Hash());
                                if (!workingParams.has(key)) workingParams.set(key, Hash());
                                assert = true;
                            }
                        }
                    }

                } else { // User has set parameter

                    sufficientParams.erase(key); // Remove from list

                    if (!workingParams.has(key)) {
                        // Do not blindly copy the whole tree, if we are still recursively going down
                        if (!desc.isLeaf() && userParams.is<Hash > (key)) {
                            workingParams.set(key, Hash());
                        } else {
                            workingParams[key] = userParams[key];
                        }
                    }
                    assert = true;
                }

                if (assert) {
                    if (desc.isNode()) { // This is a direct child -> shortcut
                        r_validate(desc, userParams, workingParams, report, nextScope);
                    } else if (desc.isLeaf()) {
                        assertSimpleType(desc, userParams, workingParams, report, nextScope);
                    } else if (desc.has("complexType")) {
                        assertComplexType(desc, userParams, workingParams, report, nextScope);
                    }
                }
            }

            // Complain about too many keys (if correct flag is set)
            if (!m_allowAdditionalKeys) reportNotNeededInformation(scope, sufficientParams);

        }

        void Schema::reportNotNeededInformation(const string& scope, const std::set<string>& sufficientParameters) const {
            ostringstream notNeededMessage;

            BOOST_FOREACH(string key, sufficientParameters) {
                string fullKey = key;
                if (!scope.empty()) fullKey = scope + "." + key;
                notNeededMessage << "Encountered unexpected configuration parameter \"" << fullKey << "\"" << endl;
            }
            string out(notNeededMessage.str());
            if (!out.empty()) {
                throw KARABO_PARAMETER_EXCEPTION(out);
            }
        }

        void Schema::applyDefault(const string& key, const Schema& desc, Hash& uParam, Hash & wParam) const {

            if (wParam.has(key)) {
                uParam[key] = wParam[key];
            } else {
                Schema::const_iterator it = desc.find("default");
                if (desc.has("simpleType")) {
                    uParam[key] = it->second;
                } else if (desc.has("complexType")) {
                    string defaultValue = desc.get<string > ("default");
                    Schema::OccuranceType occ = desc.get<Schema::OccuranceType > ("occurrence");
                    if (occ == Schema::ONE_OR_MORE || occ == Schema::ZERO_OR_MORE) {
                        boost::trim(defaultValue);
                        vector<string> v;
                        boost::split(v, defaultValue, is_any_of(" ,;"));
                        vector<Hash> configs;

                        BOOST_FOREACH(string option, v) {
                            boost::trim(option);
                            if (option != "") {
                                configs.push_back(Hash(option, Hash()));
                            }
                        }
                        uParam.set(key, configs);
                    } else {
                        uParam.set(key, Hash(defaultValue, Hash()));
                    }
                } else if (desc.has("root")) {
                    uParam.set(key, Hash());
                }
                wParam[key] = uParam[key];
            }
        }

        void Schema::assertComplexType(const Schema& desc, Hash& uParam, Hash& wParam, ostringstream& report, string & scope) const {

            const string& key = desc.getAsString("key");

            string nextScope = scope + "." + key;
            const Schema& mComplex = desc.get<Schema > ("complexType");
            Schema::OccuranceType occurance = desc.get<Schema::OccuranceType > ("occurrence");

            switch (occurance) {
                case Schema::EITHER_OR:
                {
                    if (!uParam.has(key)) {
                        report << "Choice-type parameter \"" << nextScope << "\" is missing" << endl;
                    } else {
                        Hash& uComplex = uParam.get<Hash > (key);
                        if (!wParam.has(key)) {
                            wParam.set(key, Hash());
                        }
                        Hash& wComplex = wParam.get<Hash > (key);
                        assertOccuranceEitherOr(mComplex, uComplex, wComplex, report, nextScope);
                    }
                }
                    break;
                case Schema::ZERO_OR_MORE:
                {
                    if (!uParam.has(key)) {
                        report << "List-type parameter \"" << nextScope << "\" is missing" << endl;
                    } else if (uParam.getTypeAsId(key) != Types::VECTOR_HASH) {
                        reportWrongComplexTypeFormat(nextScope);
                    } else {
                        vector<Hash>& uComplex = uParam.get<vector<Hash> > (key);
                        if (!wParam.has(key)) {
                            wParam.set(key, vector<Hash > ());
                        }
                        vector<Hash>& wComplex = wParam.get<vector<Hash> >(key);
                        assertOccuranceZeroOrMore(mComplex, uComplex, wComplex, report, nextScope);
                    }
                }
                    break;
                case Schema::ONE_OR_MORE:
                {

                    if (!uParam.has(key)) {
                        report << "List-type parameter \"" << nextScope << "\" is missing" << endl;
                    } else if (uParam.getTypeAsId(key) != Types::VECTOR_HASH) {
                        reportWrongComplexTypeFormat(nextScope);
                    } else {
                        vector<Hash>& uComplex = uParam.get<vector<Hash> > (key);
                        if (!wParam.has(key)) {
                            wParam.set(key, vector<Hash > ());
                        }
                        vector<Hash>& wComplex = wParam.get<vector<Hash> >(key);
                        assertOccuranceOneOrMore(mComplex, uComplex, wComplex, report, nextScope);
                    }
                }
                    break;
                default:
                    throw KARABO_LOGIC_EXCEPTION("This should never happen, faced illegal occurance type");
            }
        }

        void Schema::reportWrongComplexTypeFormat(const string & scope) const {
            ostringstream message;
            message << "The elements of parameter \"" << scope << "\" may be repeated (list-type parameter), ";
            message << "thus expecting one of the following syntax options:" << endl;
            message << "(1) \"" << scope << "[0]\"    (Sets/edits an element at a defined position (here 0))" << endl;
            message << "(2) \"" << scope << "[next]\" (Appends a new element to the list)" << endl;
            message << "(3) \"" << scope << "[last]\" (Addresses the last element in the list)" << endl;
            throw KARABO_PARAMETER_EXCEPTION(message.str());
        }

        void Schema::assertOccuranceEitherOr(const Schema& mComplex, Hash& uComplex, Hash& wComplex, ostringstream& report, const string & scope) const {
            if (uComplex.size() == 1) {
                const string& classIdChoice = uComplex.begin()->first;
                if (!mComplex.has(classIdChoice)) {
                    report << "Current choice \"" << classIdChoice << "\" for parameter \"" << scope << "\" is not one of the valid choices: " << String::mapKeyToString(mComplex) << endl;
                } else {

                    // Prepare working parameter
                    if (!wComplex.has(classIdChoice)) {
                        wComplex.clear();
                        wComplex.set(classIdChoice, Hash());
                    }

                    // Go recursive
                    r_validate(mComplex.get<Schema > (classIdChoice), uComplex, wComplex, report, scope);
                }
            } else if (uComplex.empty()) {
                report << "Please take a choice for parameter \"" << scope << "\". Valid choices are: " << String::mapKeyToString(mComplex) << endl;
            } else {
                report << "Please select only one choice for parameter \"" << scope << "\". Valid choices are: " << String::mapKeyToString(mComplex) << endl;
            }
        }

        void Schema::assertOccuranceZeroOrMore(const Schema& mComplex, vector<Hash>& uComplex, vector<Hash>& wComplex, ostringstream& report, const string & scope) const {

            wComplex.reserve(uComplex.size());
            size_t lastElementIdx = uComplex.size() - 1;
            for (size_t i = 0; i < uComplex.size(); ++i) {
                if (uComplex[i].empty()) {
                    if (i < lastElementIdx) { // Empty element in array (i.e. Application.modules[0] )
                        // Skip over empty elements
                        continue;
                    } else {
                        uComplex.pop_back();
                    }
                } else {
                    string classIdChoice = uComplex[i].begin()->first;
                    if (uComplex[i].empty() || !mComplex.has(classIdChoice)) {
                        report << "Current choice \"" << classIdChoice << "\" for list-type parameter \"" << scope << "\" is not one of the valid choices: " << String::mapKeyToString(mComplex) << endl;
                    } else {
                        // Prepare working parameter
                        string newScope = scope + "[" + String::toString(i) + "]";
                        if (i >= wComplex.size()) {
                            wComplex.resize(i + 1);
                        }
                        if (!wComplex[i].has(classIdChoice)) {
                            wComplex[i].clear();
                            wComplex[i].set(classIdChoice, Hash());
                        }
                        // Go recursive
                        r_validate(mComplex.get<Schema > (classIdChoice), uComplex[i], wComplex[i], report, newScope);
                    }
                }
            }
        }

        void Schema::assertOccuranceOneOrMore(const Schema& mComplex, vector<Hash>& uComplex, vector<Hash>& wComplex, ostringstream& report, const string & scope) const {
            assertOccuranceZeroOrMore(mComplex, uComplex, wComplex, report, scope);
            if (uComplex.empty()) {
                report << "At least one of the choices: " << String::mapKeyToString(mComplex) << " has to be used for list-type parameter \"" << scope << "\"" << endl;
                report << "A valid input could be: " << "\"" << scope << "[0]." << mComplex.begin()->first << "\"" << endl;
            }
        }

        void Schema::assertSimpleType(const Schema& desc, Hash& uParam, Hash& wParam, std::ostringstream& report, string & scope) const {

            const string& key = desc.getAsString("key");

            string nextScope = scope + "." + key;
            string type = Types::convert(desc.get<Types::ReferenceType > ("simpleType"));

            if (!uParam.has(key)) {
                report << "Missing simple-type (" << type << ") parameter: \"" << nextScope << "\"" << endl;
                return;
            }

            // Check datatype
            Types::ReferenceType eDataType = desc.get<Types::ReferenceType > ("simpleType");
            Types::ReferenceType iDataType = uParam.getTypeAsId(key);
            if (eDataType != Types::ANY && iDataType != eDataType) {
                // TODO Discuss whether we should try to cast whatever is there
                if (iDataType != Types::STRING) {
                    if (iDataType == Types::HASH) {
                        report << "Parameter \"" << nextScope << "\" needs an assignment of type " << type << "(" << eDataType << ")" << endl;
                        report << "Type like this: \"" << nextScope << "=your" << type << "\"" << endl;
                        return;
                    }
                    try {
                        string value(uParam.getAsString(key));
                        uParam.set(key, value);
                        uParam.convertFromString(key, eDataType);
                    } catch (...) {
                        report << "Failed to cast the value of parameter \"" << nextScope << "\" from " << Types::convert(iDataType);
                        report << " to " << Types::convert(eDataType);
                        return;
                    }
                } else if (iDataType == Types::STRING) {
                    // Try to cast
                    try {
                        uParam.convertFromString(key, eDataType);
                    } catch (...) {
                        report << "Failed to cast the value of parameter \"" << nextScope << "\" from " << Types::convert(iDataType);
                        report << " to " << Types::convert(eDataType);
                    }
                } else {
                    report << "Value for parameter \"" << nextScope << "\" is of wrong format." << endl
                            << "Expected " << Types::convert(eDataType) << " got " << Types::convert(iDataType) << endl;
                }
            }

            // Check ranges
            switch (eDataType) {

                case Types::ANY:
                    // No way to validate any
                    break;

                case Types::BOOL:
                    // No need to validate true/false
                    break;

                case Types::VECTOR_BOOL:
                {
                    // validate number of elements in array: minSize, maxSize
                    deque<bool> iValueVect = uParam.get < deque<bool> > (key);
                    unsigned int iValue = iValueVect.size();
                    checkSizeOfVector(desc, scope, key, iValue, report);
                }
                    break;

                case Types::INT8:
                case Types::INT16:
                case Types::INT32:
                case Types::INT64:
                case Types::UINT8:
                case Types::UINT16:
                case Types::UINT32:
                case Types::UINT64:
                case Types::FLOAT:
                case Types::DOUBLE:
                {
                    double iValue = uParam.getNumeric<double > (key);
                    if (desc.has("options")) {
                        checkOptions(scope, iValue, desc, report);
                    }
                    if (desc.has("minInc")) {
                        double eValue = desc.getNumeric<double>("minInc");
                        checkMinInc(scope, iValue, eValue, report);
                    } else if (desc.has("minExc")) {
                        double eValue = desc.getNumeric<double>("minExc");
                        checkMinExc(scope, iValue, eValue, report);
                    }
                    if (desc.has("maxInc")) {
                        double eValue = desc.getNumeric<double>("maxInc");
                        checkMaxInc(scope, iValue, eValue, report);
                    } else if (desc.has("maxExc")) {
                        double eValue = desc.getNumeric<double>("maxExc");
                        checkMaxExc(scope, iValue, eValue, report);
                    }
                }
                    break;

                case Types::VECTOR_INT8:
                {
                    vector<signed char> iValueVect = uParam.get < vector<signed char> > (key);
                    unsigned int iValue = iValueVect.size();
                    checkSizeOfVector(desc, scope, key, iValue, report);
                    checkRangeOfVectorElements(desc, scope, key, iValue, iValueVect, report);
                }
                    break;

                case Types::VECTOR_INT16:
                {
                    vector<signed short> iValueVect = uParam.get < vector<signed short> > (key);
                    unsigned int iValue = iValueVect.size();
                    checkSizeOfVector(desc, scope, key, iValue, report);
                    checkRangeOfVectorElements(desc, scope, key, iValue, iValueVect, report);
                }
                    break;

                case Types::VECTOR_INT32:
                {
                    vector<signed int> iValueVect = uParam.get < vector<signed int> > (key);
                    unsigned int iValue = iValueVect.size();
                    checkSizeOfVector(desc, scope, key, iValue, report);
                    checkRangeOfVectorElements(desc, scope, key, iValue, iValueVect, report);
                }
                    break;

                case Types::VECTOR_INT64:
                {
                    vector<signed long long> iValueVect = uParam.get < vector<signed long long> > (key);
                    unsigned int iValue = iValueVect.size();
                    checkSizeOfVector(desc, scope, key, iValue, report);
                    checkRangeOfVectorElements(desc, scope, key, iValue, iValueVect, report);
                }
                    break;

                case Types::VECTOR_UINT8:
                {
                    vector<unsigned char> iValueVect = uParam.get< vector<unsigned char> > (key);
                    unsigned int iValue = iValueVect.size();
                    checkSizeOfVector(desc, scope, key, iValue, report);
                    checkRangeOfVectorElements(desc, scope, key, iValue, iValueVect, report);
                }
                    break;

                case Types::VECTOR_UINT16:
                {
                    vector<unsigned short> iValueVect = uParam.get< vector<unsigned short> > (key);
                    unsigned int iValue = iValueVect.size();
                    checkSizeOfVector(desc, scope, key, iValue, report);
                    checkRangeOfVectorElements(desc, scope, key, iValue, iValueVect, report);
                }
                    break;

                case Types::VECTOR_UINT32:
                {
                    vector<unsigned int> iValueVect = uParam.get< vector<unsigned int> > (key);
                    unsigned int iValue = iValueVect.size();
                    checkSizeOfVector(desc, scope, key, iValue, report);
                    checkRangeOfVectorElements(desc, scope, key, iValue, iValueVect, report);
                }
                    break;

                case Types::VECTOR_UINT64:
                {
                    vector<unsigned long long> iValueVect = uParam.get< vector<unsigned long long> > (key);
                    unsigned int iValue = iValueVect.size();
                    checkSizeOfVector(desc, scope, key, iValue, report);
                    checkRangeOfVectorElements(desc, scope, key, iValue, iValueVect, report);
                }
                    break;

                case Types::VECTOR_DOUBLE:
                {
                    vector<double> iValueVect = uParam.get< vector<double> > (key);
                    unsigned int iValue = iValueVect.size();
                    checkSizeOfVector(desc, scope, key, iValue, report);
                    checkRangeOfVectorElements(desc, scope, key, iValue, iValueVect, report);
                }
                    break;

                case Types::VECTOR_FLOAT:
                {
                    vector<float> iValueVect = uParam.get< vector<float> > (key);
                    unsigned int iValue = iValueVect.size();
                    checkSizeOfVector(desc, scope, key, iValue, report);
                    checkRangeOfVectorElements(desc, scope, key, iValue, iValueVect, report);
                }
                    break;

                case Types::STRING:
                {
                    string iValue = uParam.get<string > (key);
                    if (desc.has("options")) {
                        checkOptions(scope, iValue, desc, report);
                    }
                    if (desc.has("minInc")) {
                        string eValue = desc.get<string > ("minInc");
                        checkMinInc(scope, iValue, eValue, report);
                    } else if (desc.has("minExc")) {
                        string eValue = desc.get<string > ("minExc");
                        checkMinExc(scope, iValue, eValue, report);
                    }
                    if (desc.has("maxInc")) {
                        string eValue = desc.get<string > ("maxInc");
                        checkMaxInc(scope, iValue, eValue, report);
                    } else if (desc.has("maxExc")) {
                        string eValue = desc.get<string > ("maxExc");
                        checkMaxExc(scope, iValue, eValue, report);
                    }
                }
                    break;

                case Types::VECTOR_STRING:
                {
                    // validate number of elements in array: minSize, maxSize
                    vector<string> iValueVect = uParam.get< vector<string> > (key);
                    unsigned int iValue = iValueVect.size();
                    checkSizeOfVector(desc, scope, key, iValue, report);
                }
                    break;

                case Types::PATH:
                {
                    boost::filesystem::path value = uParam.get<boost::filesystem::path > (key);
                    if (desc.has("options")) {
                        checkOptions<string > (scope, value.string(), desc, report);
                    }
                }
                    break;

                case Types::VECTOR_PATH:
                {
                    // validate number of elements in array: minSize, maxSize
                    vector<boost::filesystem::path> iValueVect = uParam.get< vector<boost::filesystem::path> > (key);
                    unsigned int iValue = iValueVect.size();
                    checkSizeOfVector(desc, scope, key, iValue, report);
                }
                    break;
                default:
                    break;
                    //cout << " ### WARNING ### Range check on non-implemented on datatype " << Types::convert(eDataType) << endl;
            }
            // Set to working
            wParam[key] = uParam[key];
        }

        void Schema::checkSizeOfVector(const Schema& desc, const string& scope, const string& key, unsigned int iValue, std::ostringstream & report) const {
            if (desc.has("minSize")) {
                unsigned int eValue = desc.getNumeric<unsigned int > ("minSize");
                if (iValue < eValue) {
                    report << "Number of elements in array " << scope << "." << key << " must be greater or equal " << eValue << "." << endl;
                }
            }
            if (desc.has("maxSize")) {
                unsigned int eValue = desc.getNumeric<unsigned int > ("maxSize");
                if (iValue > eValue) {
                    report << "Number of elements in array " << scope << "." << key << " must be less or equal " << eValue << "." << endl;
                }
            }
        }

        void Schema::ensureValidCache() {
            if (m_key2alias.empty()) r_generateAliasMaps(*this, "");
        }

        void Schema::help(const std::string & classId) {

            ostringstream stream;

            Schema expected;
            string searchPath;

            try {

                if (this->has("elements")) {
                    searchPath = this->get<string > ("root");
                    expected.set(searchPath, *this);
                    if (!classId.empty()) searchPath += "." + classId;
                } else {
                    searchPath = classId;
                    expected = *this;
                }


                if (searchPath.empty()) {
                    for (Schema::const_iterator it = expected.begin(); it != expected.end(); it++) {
                        stream << it->first << endl;
                    }
                } else if (searchPath.rfind(".") != string::npos) {

                    stream << "----- HELP -----\n" << searchPath << endl;

                    vector<string> tokens;
                    boost::split(tokens, searchPath, boost::is_any_of("."));

                    r_processingPathToElem(expected, tokens, stream);

                } else {

                    stream << "----- HELP -----\n" << searchPath << endl;

                    processingExpectParams(expected, searchPath, stream);

                    stream << "------------------------------------------------------------" << endl;
                }

            } catch (...) {
                KARABO_RETHROW;
            }

            // ! show results !
            cout << "\n" << stream.str();
        }

        void Schema::r_processingPathToElem(const Schema& expected, vector<string>& tokens, ostringstream & stream) {

            try {
                bool tokenFound = false;
                size_t tokensSize = tokens.size();
                size_t j; //counter for tokens

                if (tokensSize == 1) {
                    j = 0;
                } else if (tokensSize > 1) {
                    j = 1;
                }

                Schema entry;
                Schema elements;

                if (expected.has("root")) { //we got for processing SINGLE_ELEMENT
                    //string rootOfSingleElem = expected.get<string>("root");
                    string keyOfSingleElem = expected.get<string > ("key");

                    if (tokens[0] == keyOfSingleElem) { //as expected
                        elements = expected.get<Schema > ("elements");
                    } else {
                        stream << "CHECK error. tokens[0]: " << tokens[0] << " != " << keyOfSingleElem << "  :keyOfSingleElem" << endl;
                    }
                } else {
                    entry = expected.get<Schema > (tokens[0]);
                    elements = entry.get<Schema > ("elements");
                }
                string currentToken = tokens[j];

                bool lastToken = false;
                if (j == tokensSize - 1) {
                    lastToken = true;
                }

                //cout << "currently searching for 'currentToken':" << currentToken << endl;

                //iterate over elements 0=>Schema (SCHEMA), 1=>--||--, ... , N=>--||-- and check whether
                //one of them has 'key' equal to the given 'currentToken' 
                //As soon as such element found: 
                //1) check what kind of element it is: simpleType, complexType, or SINGLE_ELEMENT
                //2) process this element according to its type
                //3) while processing, check counter for tokens: show completed help for last token,
                //   or continue search for intermediate token (calling function recursively)

                for (Schema::const_iterator ct = elements.begin(); ct != elements.end(); ct++) {

                    const Schema& desc = elements.get<Schema > (ct);

                    Schema::AssignmentType at = desc.get<Schema::AssignmentType > ("assignment");

                    //find name of element (it's 'key'); element can be simple, complex, or SINGLE_ELEMENT
                    string elementName;
                    elementName = desc.getAsString("key"); //whatever element, find its 'key'

                    //cout << "elementName=" << elementName << endl;

                    if (elementName == currentToken) { //this is the element we are looking for ('key' equal to our 'token')

                        tokenFound = true;

                        if (at != Schema::INTERNAL_PARAM) {//consider here cases MANDATORY or OPTIONAL
                            if (desc.has("root")) { //it's a SINGLE_ELEMENT

                                if (lastToken) { //this is the last token, show help for this element
                                    showSingleElement(desc, stream);
                                    return;
                                }
                                //this is not the last token, continue search
                                //remove currently found token from the vector and search later
                                tokens.erase(tokens.begin(), tokens.begin() + 1);

                                //call recursively 
                                r_processingPathToElem(desc, tokens, stream);

                            } else if (desc.has("complexType")) { //complexType element

                                const Schema& complexElem = desc.get<Schema > ("complexType");

                                if (lastToken) { //this is the last token, show this element

                                    //1st variant: show only keys (OK)
                                    //std::set<string> keys = complexElem.keys();
                                    //for (std::set<string>::const_iterator iter=keys.begin(); iter!=keys.end(); ++iter){
                                    //     stream<< *iter << endl;
                                    //}                 

                                    //2nd variant: show complete (OK)
                                    processingComplexType(complexElem, stream);
                                    return;
                                }

                                //this is not the last token, take next token and continue search
                                //increase token's counter 
                                j = j + 1;
                                //consider next token 
                                const string& newToken = tokens[j];

                                std::set<string> keys = complexElem.getKeysAsSet();
                                std::set<string>::const_iterator itKeys;
                                itKeys = keys.find(newToken);

                                if (itKeys == keys.end()) { //token not found
                                    //this complex element does not contain a key we are looking for
                                    stream << "The following element '" << newToken << " ' is not a sub-element of '" << tokens[j - 1] << "'" << endl;
                                    return;
                                }

                                //token was found
                                tokenFound = true;

                                //remove currently found token from the vector and search further 
                                tokens.erase(tokens.begin(), tokens.begin() + 2); //remove 2 tokens       

                                if (tokens.size() == 1) { //process element, ready
                                    processingExpectParams(complexElem, tokens[0], stream);
                                    return;
                                } else { //call recursively...
                                    r_processingPathToElem(complexElem, tokens, stream);
                                }

                            } else if (desc.has("simpleType")) { // simple type element, that could be only a last-token 

                                stream << "Element '" << elementName << "' is a simple element" << endl;

                                processingDescription(desc, stream);

                                if (!lastToken) { //if it is not last-token, just show the message to inform user
                                    stream << "Element '" << elementName << "' is a simple element! It does not contain any other elements." << endl;
                                }

                                return;
                            }

                        } //considering elements with AssignmentType != Schema::INTERNAL (i.e., OPTIONAL or MANDATORY)

                    } //token found " if (elementName == currentToken) "

                } //iterating over Schema-elements (Schema::const_iterator ct = elements.begin(); ct != elements.end(); ct++)

                if (!tokenFound) { //token not found
                    stream << "No such element '" << currentToken << "' exists." << endl;
                    return;
                }

            } catch (...) {
                KARABO_RETHROW;
            }
        }

        void Schema::showSingleElement(const Schema& desc, ostringstream & stream) {

            const Schema& elements = desc.get<Schema > ("elements");

            for (Schema::const_iterator it = elements.begin(); it != elements.end(); it++) {
                const Schema& description = elements.get<Schema > (it);
                processingDescription(description, stream);
            }
        }

        void Schema::processingComplexType(const Schema& complexElem, ostringstream & stream) {

            for (Schema::const_iterator itk = complexElem.begin(); itk != complexElem.end(); itk++) {

                const Schema& current = complexElem.get<Schema > (itk);

                string currentName = current.getAsString("root");

                stream << "\n" << currentName << "\n";
                processingExpectParams(complexElem, currentName, stream);

                stream << "------------------------------------------------------------" << endl;
            }

        }

        void Schema::processingExpectParams(const Schema& expected, const std::string& classId, ostringstream & stream) {
            try {

                const Schema& entry = expected.get<Schema > (classId);

                const Schema& elements = entry.get<Schema > ("elements");

                for (Schema::const_iterator it = elements.begin(); it != elements.end(); it++) {

                    const Schema& description = elements.get<Schema > (it);

                    processingDescription(description, stream);

                }
            } catch (...) {
                KARABO_RETHROW;
            }

        }

        void Schema::processingDescription(const Schema& desc, ostringstream & stream) {

            string key("");
            string description("");
            string type("");
            string defaultValue("");
            string assignment("");
            string options("");
            string range("");
            string accessTypePrint("");
            bool expertLevel = false;
            string untName("");
            string untSymbol("");
            string minSize("");
            string maxSize("");
            string displayType("");

            key = desc.getAsString("key");

            if (desc.has("description")) {
                desc.get("description", description);
            }

            if (desc.has("access")) {
                AccessType accessT = desc.get<AccessType > ("access");
                switch (accessT) {
                    case 1: //INIT, init() in expectedParameters
                        accessTypePrint = "initialization";
                        break;
                    case 2: //READ, readOnly() in expectedParameters
                        accessTypePrint = "read only";
                        break;
                    case 4: //WRITE, reconfigurable() in expectedParameters
                        accessTypePrint = "reconfiguration";
                        break;
                }
            }

            if (desc.has("simpleType")) {
                type = Types::convert(desc.get<Types::ReferenceType > ("simpleType"));
            } else if (desc.has("complexType")) {
                Schema::OccuranceType occ = desc.get<Schema::OccuranceType > ("occurrence");
                if (occ == Schema::EITHER_OR) {
                    type = "CHOICE";
                } else if (occ == Schema::ONE_OR_MORE) {
                    type = "NON_EMPTY_LIST";
                } else if (occ == Schema::ZERO_OR_MORE) {
                    type = "LIST";
                }
                if (desc.has("displayType")) {
                    displayType = desc.get<string > ("displayType");
                }

            } else {//no simpleType and no complexType, that is, SINGLE_ELEMENT
                type = "SINGLE_ELEMENT";
                if (desc.has("displayType")) {
                    displayType = desc.get<string > ("displayType");
                }
            }

            if (desc.has("default")) {
                defaultValue = desc.getAsString("default");
                if (type.rfind("VECTOR") != string::npos) { //print '(' if default value is Vector
                    defaultValue = "(" + defaultValue + ")";
                }
            }

            if (desc.has("assignment")) {
                Schema::AssignmentType at = desc.get<Schema::AssignmentType > ("assignment");
                if (at == Schema::OPTIONAL_PARAM) {
                    assignment = "OPTIONAL";
                } else if (at == Schema::MANDATORY_PARAM) {
                    assignment = "MANDATORY";
                } else if (at == Schema::INTERNAL_PARAM) {
                    assignment = "INTERNAL";
                }
            }

            if (desc.has("options")) {

                string opt = desc.getAsString("options");
                vector<string> tmp;
                boost::split(tmp, opt, is_any_of(","));
                if (tmp.size() > 1) {

                    for (size_t i = 0; i < tmp.size() - 1; ++i) {
                        options += tmp[i] + ", ";
                    }
                    options += tmp.back();
                } else {
                    options += desc.getAsString("options");
                }

            } else {
                if (desc.has("minInc")) {
                    range += "[" + desc.getAsString("minInc") + ", ";
                } else if (desc.has("minExc")) {
                    range += "(" + desc.getAsString("minExc") + ", ";
                } else {
                    if (type.substr(0, 3) == "UIN") { //for example UINT32, ..
                        range += "[0, ";
                    } else {
                        range += "(-infty, ";
                    }
                }
                if (desc.has("maxInc")) {
                    range += desc.getAsString("maxInc") + "]";
                } else if (desc.has("maxExc")) {
                    range += desc.getAsString("maxExc") + ")";
                } else {
                    if (range.substr(0, 3) == "(-i") {
                        range = "";
                    } else {
                        range += "+infty)";
                    }
                }
            }

            if (desc.has("expertLevel")) {
                int level = desc.get<int>("expertLevel");
                if (level > 0) {
                    expertLevel = true;
                }
            }

            if (desc.has("unitName")) {
                desc.get("unitName", untName);
            }

            if (desc.has("unitSymbol")) {
                untSymbol = "[" + desc.get<string > ("unitSymbol") + "]";
            }

            if (desc.has("minSize")) { //minSize of Vector
                minSize = desc.getAsString("minSize");
            }

            if (desc.has("maxSize")) { //maxSize of Vector
                maxSize = desc.getAsString("maxSize");
            }

            stream << "\n    ." << key << " (" << type << ")";
            if (expertLevel) {
                stream << " [expert level]";
            }
            stream << endl;

            stream << "         Assignment   : " << assignment << endl;
            if (description != "") {
                stream << "         Description  : " << description << endl;
            }
            if (defaultValue != "") {
                stream << "         Default value: " << defaultValue << endl;
            }
            if (displayType != "") {
                stream << "         Display type : " << displayType << endl;
            }
            if (options != "") {
                stream << "         Options      : " << options << endl;
            } else if (range != "") {
                stream << "         Range        : " << range << endl;
            }
            if (minSize != "") {
                stream << "         minSize      : " << minSize << endl;
            }
            if (minSize != "") {
                stream << "         maxSize      : " << maxSize << endl;
            }
            if (untName != "" || untSymbol != "") {
                stream << "         Units        : " << untName << " " << untSymbol << endl;
            }
            if (accessTypePrint != "") {
                stream << "         Usage        : " << accessTypePrint << endl;
            }

        }//processingDescription(const Schema& desc, ostringstream& stream)

        ostream& operator<<(std::ostream& os, const Schema & schema) {
            ostringstream stream;
            if (schema.has("elements") && schema.has("root")) {
                stream << schema.get<string > ("root") << endl;
                const Schema& elements = schema.get<Schema > ("elements");
                for (Schema::const_iterator it = elements.begin(); it != elements.end(); it++) {
                    const Schema& description = elements.get<Schema > (it);
                    Schema::processingDescription(description, stream);
                }
            } else {
                std::vector<std::string> keys = schema.getKeysAsVector();
                for (size_t i = 0; i < keys.size(); i++) {
                    stream << keys[i] << endl;
                    string keyStr = keys[i];
                    const Schema& sch = schema.get<Schema>(keyStr);
                    if (sch.has("elements") && sch.has("root")) {
                        stream << sch.get<string > ("root") << endl;
                        const Schema& elements = sch.get<Schema > ("elements");
                        for (Schema::const_iterator it = elements.begin(); it != elements.end(); it++) {
                            const Schema& description = elements.get<Schema > (it);
                            Schema::processingDescription(description, stream);
                        }
                    }
                }
            }
            os << stream.str();
            return os;
        }

    } // namespace util
} // namespace karabo
