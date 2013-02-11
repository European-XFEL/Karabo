/* 
 * File:   Validator.cc
 * Author: <burkhard.heisen@xfel.eu>
 * 
 * Created on February 8, 2013, 6:03 PM
 */

#include "Validator.hh"
#include "Schema.hh"

namespace karabo {
    namespace util {
        
        Validator::Validator() : m_injectDefaults(true), m_assumeRootedConfiguration(false), 
                m_allowAdditionalKeys(false), m_allowMissingKeys(false) {
        }
        
        Validator::Validator(const ValidationRules rules) {
            this->setValidationRules(rules);
        }
        
        void Validator::setValidationRules(const Validator::ValidationRules& rules) {
            m_injectDefaults = rules.injectDefaults;
            m_allowAdditionalKeys = rules.allowAdditionalKeys;
            m_allowMissingKeys = rules.allowMissingKeys;
            m_assumeRootedConfiguration = rules.allowUnrootedConfiguration;
        }

        Validator::ValidationRules Validator::getValidationRules() const {
            Validator::ValidationRules rules;
            rules.injectDefaults = m_injectDefaults;
            rules.allowAdditionalKeys = m_allowAdditionalKeys;
            rules.allowMissingKeys = m_allowMissingKeys;
            rules.allowUnrootedConfiguration = m_assumeRootedConfiguration;
            return rules;
        }

        std::pair<bool, std::string> Validator::validate(const Schema& schema, const Hash& unvalidatedInput, Hash& validatedOutput) const {

            // In case of failed validation, report why it failed
            ostringstream validationFailedReport;

            if (m_assumeRootedConfiguration) {
                if (unvalidatedInput.size() != 1) {
                    return std::make_pair<bool, string > (false, "Expecting a rooted input, i.e. a Hash with exactly one key (describing the classId) at the top level");
                } else {
                    const Hash::Node& node = *(unvalidatedInput.begin());
                    const std::string& classId = node.getKey();
                    if (schema.getRootName() != classId) {
                        return std::make_pair<bool, string > (false, "Wrong schema for given input. Schema describes class \"" + schema.getRootName() + "\", whilst input wants to configure class \"" + classId + "\"");
                    }
                    if (node.getType() == Types::HASH) {

                        Hash::Node& tmp = validatedOutput.set(classId, Hash());
                        this->r_validate(schema.getRoot(), node.getValue<Hash > (), tmp.getValue<Hash > (), validationFailedReport, classId);

                        if (validationFailedReport.str().empty()) return std::make_pair<bool, string > (true, "");
                        else return std::make_pair<bool, string > (false, validationFailedReport.str());
                    } else {
                        return std::make_pair<bool, string > (false, "Root-node for given configuration is of wrong type. It must be HASH");
                    }
                }
            } else {
                this->r_validate(schema.getRoot(), unvalidatedInput, validatedOutput, validationFailedReport, "");
                if (validationFailedReport.str().empty()) return std::make_pair<bool, string > (true, "");
                else return std::make_pair<bool, string > (false, validationFailedReport.str());
            }
        }

        void Validator::r_validate(const Hash& master, const Hash& user, Hash& working, std::ostringstream& report, std::string scope) const {
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

                if (nodeType == Schema::LEAF) {
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
                } else if (nodeType == Schema::NODE) {
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
                } else if (nodeType == Schema::CHOICE_OF_NODES) {
                    if (!userHasNode) {
                        if (assignment == Schema::MANDATORY_PARAM) {
                            if (!m_allowMissingKeys) {
                                report << "Missing (choice-)parameter: \"" << currentScope << "\"" << endl;
                                return;
                            }
                        } else if (assignment == Schema::OPTIONAL_PARAM && hasDefault && m_injectDefaults) {
                            std::string optionName = it->getAttribute<string > ("default");
                            Hash::Node& workNode = working.set(key, Hash(optionName, Hash())); // Inject empty choice
                            r_validate(it->getValue<Hash > ().get<Hash > (optionName), Hash(), workNode.getValue<Hash > ().get<Hash > (optionName), report, currentScope + "." + optionName);
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
                                r_validate(it->getValue<Hash > ().get<Hash > (optionName), Hash(), workNode.getValue<Hash > ().get<Hash > (optionName), report, currentScope + "." + optionName);
                            } else {
                                report << "Provided parameter: \"" << optionName << "\" is not a valid option for choice: \"" << key << "\". ";
                                report << "Valid options are: " << karabo::util::toString(validOptions) << endl;
                                return;
                            }
                        } else if (user.getType(key) != Types::HASH) {
                            report << "Parameter \"" << currentScope << "\" has incorrect type, expecting HASH not " << Types::to<ToLiteral > (user.getType(key)) << endl;
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
                                } else if (assignment == Schema::OPTIONAL_PARAM && hasDefault && m_injectDefaults) {
                                    std::string optionName = it->getAttribute<string > ("default");
                                    Hash::Node& workNode = working.set(key, Hash(optionName, Hash())); // Inject empty choice
                                    r_validate(it->getValue<Hash > ().get<Hash > (optionName), Hash(), workNode.getValue<Hash > ().get<Hash > (optionName), report, currentScope + "." + optionName);
                                }
                            } else if (choice.size() == 1) { // That is what we expect it should be
                                const Hash::Node& usersOption = *(choice.begin());
                                const string& optionName = usersOption.getKey();
                                if (validOptions.find(optionName) != validOptions.end()) { // Is a valid option
                                    Hash::Node& workNode = working.set(key, Hash(optionName, Hash())); // Inject empty choice
                                    r_validate(it->getValue<Hash > ().get<Hash > (optionName), usersOption.getValue<Hash > (), workNode.getValue<Hash > ().get<Hash > (optionName), report, currentScope + "." + optionName);
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
                } else if (nodeType == Schema::LIST_OF_NODES) {
                    if (!userHasNode) { // Node IS NOT provided
                        if (assignment == Schema::MANDATORY_PARAM) {
                            if (!m_allowMissingKeys) {
                                report << "Missing (list-)parameter: \"" << currentScope << "\"" << endl;
                                return;
                            }
                        } else if ((assignment == Schema::OPTIONAL_PARAM) && hasDefault && m_injectDefaults) {
                            const vector<string>& optionNames = it->getAttribute<vector<string> > ("default");
                            Hash::Node& workNode = working.set(key, std::vector<Hash>()); // TODO use bindReference here
                            vector<Hash>& workNodes = workNode.getValue<vector<Hash> >();
                            BOOST_FOREACH(string optionName, optionNames) {
                                Hash tmp;
                                r_validate(it->getValue<Hash > ().get<Hash > (optionName), Hash(), tmp, report, currentScope + "." + optionName);
                                workNodes.push_back(Hash(optionName, tmp));
                            }
                        }
                    
                    } else { // Node IS provided
                        std::set<std::string> validOptions;
                        master.get<Hash > (key).getKeys(validOptions);
                        Hash::Node& workNode = working.set(key, std::vector<Hash>()); // TODO use bindReference here
                        vector<Hash>& workNodes = workNode.getValue<vector<Hash> >();
                        
                        // If the options have all-default parameters the user lazily may have set the option as string instead of HASH
                        // We will allow for this and silently inject an empty Hashes instead
                        if (user.getType(key) == Types::VECTOR_STRING) {
                            const vector<string> optionNames(user.get<vector<string> >(key));
                            int optionNamesSize = static_cast<int>(optionNames.size());
                            if (it->hasAttribute("min") && (optionNamesSize < it->getAttribute<int>("min"))) {
                                report << "Too less options given for (list-)parameter: \"" << key << "\". Expecting at least " << it->getAttribute<int>("min");
                                return;
                            }
                            if (it->hasAttribute("max") && (optionNamesSize > it->getAttribute<int>("max"))) {
                                report << "Too many options given for (list-)parameter: \"" << key << "\". Expecting at most " << it->getAttribute<int>("max");
                                return;
                            }
                            
                            BOOST_FOREACH(string optionName, optionNames) {
                                cout << "Silently converting from STRING" << endl;
                                if (validOptions.find(optionName) != validOptions.end()) { // Is a valid option
                                    Hash tmp;
                                    //r_validate(it->getValue<Hash > ().get<Hash > (optionName), Hash(), tmp, report, currentScope + "." + optionName);
                                    workNodes.push_back(Hash(optionName, tmp));
                                } else {
                                    report << "Provided parameter: \"" << optionName << "\" is not a valid option for list: \"" << key << "\". ";
                                    report << "Valid options are: " << karabo::util::toString(validOptions) << endl;
                                    return;
                                }
                            }
                        } else if (user.getType(key) != Types::VECTOR_HASH) {
                            report << "Parameter \"" << currentScope << "\" has incorrect type, expecting VECTOR_HASH not " << Types::to<ToLiteral > (user.getType(key)) << endl;
                            return;
                        } else {

                            const vector<Hash>& userOptions = user.get<vector<Hash> > (key);
                             int optionNamesSize = static_cast<int>(userOptions.size());
                            if (it->hasAttribute("min") && (optionNamesSize < it->getAttribute<int>("min"))) {
                                report << "Too less options given for (list-)parameter: \"" << key << "\". Expecting at least " << it->getAttribute<int>("min");
                                report << "Valid options are: " << karabo::util::toString(validOptions) << endl;
                                return;
                            }
                            if (it->hasAttribute("max") && (optionNamesSize > it->getAttribute<int>("max"))) {
                                report << "Too many options given for (list-)parameter: \"" << key << "\". Expecting at most " << it->getAttribute<int>("max");
                                report << "Valid options are: " << karabo::util::toString(validOptions) << endl;
                                return;
                            }
                            
                            // That is what we expect it should be
                            for (size_t i = 0; i < userOptions.size(); ++i) {
                                const Hash& option = userOptions[i];
                                Hash::Node rootNode = *(option.begin());
                                string optionName = rootNode.getKey();
                                if (validOptions.find(optionName) != validOptions.end()) { // Is a valid option
                                    Hash tmp;
                                    //r_validate(it->getValue<Hash > ().get<Hash > (optionName), rootNode.getValue<Hash>(), tmp, report, currentScope + "." + optionName);
                                    workNodes.push_back(Hash(optionName, tmp));
                                } else {
                                    report << "Provided parameter: \"" << optionName << "\" is not a valid option for list: \"" << key << "\". ";
                                    report << "Valid options are: " << karabo::util::toString(validOptions) << endl;
                                    return;
                                    
                                }
                            }
                        }
                    }
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
    }
}