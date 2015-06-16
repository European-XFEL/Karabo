/* 
 * File:   Validator.cc
 * Author: <burkhard.heisen@xfel.eu>
 * 
 * Created on February 8, 2013, 6:03 PM
 */

#include "Validator.hh"  
#include "Schema.hh"
#include "FromLiteral.hh"
#include "Epochstamp.hh"

namespace karabo {
    namespace util {


        Validator::Validator() : m_injectDefaults(true), m_allowUnrootedConfiguration(true),
        m_allowAdditionalKeys(false), m_allowMissingKeys(false), m_injectTimestamps(false) {
        }


        Validator::Validator(const ValidationRules rules) {
            this->setValidationRules(rules);
        }


        void Validator::setValidationRules(const Validator::ValidationRules& rules) {
            m_injectDefaults = rules.injectDefaults;
            m_allowAdditionalKeys = rules.allowAdditionalKeys;
            m_allowMissingKeys = rules.allowMissingKeys;
            m_allowUnrootedConfiguration = rules.allowUnrootedConfiguration;
            m_injectTimestamps = rules.injectTimestamps;
        }


        Validator::ValidationRules Validator::getValidationRules() const {
            Validator::ValidationRules rules;
            rules.injectDefaults = m_injectDefaults;
            rules.allowAdditionalKeys = m_allowAdditionalKeys;
            rules.allowMissingKeys = m_allowMissingKeys;
            rules.allowUnrootedConfiguration = m_allowUnrootedConfiguration;
            rules.injectTimestamps = m_injectTimestamps;
            return rules;
        }


        std::pair<bool, std::string> Validator::validate(const Schema& schema, const Hash& unvalidatedInput, Hash& validatedOutput, const Timestamp& timestamp) {

            // Clear all previous warnings and alarms
            m_parametersInWarnOrAlarm.clear();

            // Prepare timestamp if needed
            if (m_injectTimestamps) {
                m_timestamp = timestamp;
            }

            // In case of failed validation, report why it failed
            ostringstream validationFailedReport;

            if (!m_allowUnrootedConfiguration) {
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
                        this->r_validate(schema.getParameterHash(), node.getValue<Hash > (), tmp.getValue<Hash > (), validationFailedReport, classId);

                        if (validationFailedReport.str().empty()) return std::make_pair<bool, string > (true, "");
                        else return std::make_pair<bool, string > (false, validationFailedReport.str());
                    } else {
                        return std::make_pair<bool, string > (false, "Root-node for given configuration is of wrong type. It must be HASH");
                    }
                }
            } else {
                this->r_validate(schema.getParameterHash(), unvalidatedInput, validatedOutput, validationFailedReport, "");
                if (validationFailedReport.str().empty()) return std::make_pair<bool, string > (true, "");
                else return std::make_pair<bool, string > (false, validationFailedReport.str());
            }
        }


        void Validator::r_validate(const Hash& master, const Hash& user, Hash& working, std::ostringstream& report, std::string scope) {
            std::set<std::string> keys;
            user.getKeys(keys);

            // Iterate master
            for (Hash::const_iterator it = master.begin(); it != master.end(); ++it) {

                string key(it->getKey());

                string currentScope;
                if (scope.empty()) currentScope = key;
                else currentScope = scope + "." + key;

                int nodeType = it->getAttribute<int>(KARABO_SCHEMA_NODE_TYPE);
                bool userHasNode = user.has(key);
                bool hasDefault = it->hasAttribute(KARABO_SCHEMA_DEFAULT_VALUE);

                // Remove current node from all provided
                if (userHasNode) keys.erase(key);

                if (nodeType == Schema::LEAF) {

                    int assignment = it->getAttribute<int>(KARABO_SCHEMA_ASSIGNMENT);

                    if (!userHasNode) { // Node IS NOT provided
                        if (assignment == Schema::MANDATORY_PARAM) {
                            if (!m_allowMissingKeys) {
                                report << "Missing mandatory parameter: \"" << currentScope << "\"" << endl;
                                return;
                            }
                        } else if (assignment == Schema::OPTIONAL_PARAM && hasDefault && m_injectDefaults) {
                            Hash::Node& node = working.set(key, it->getAttributeAsAny(KARABO_SCHEMA_DEFAULT_VALUE));
                            this->validateLeaf(*it, node, report, currentScope);
                        }
                    } else { // Node IS provided
                        Hash::Node& node = working.setNode(user.getNode(key));
                        this->validateLeaf(*it, node, report, currentScope);
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

                    int assignment = it->getAttribute<int>(KARABO_SCHEMA_ASSIGNMENT);

                    if (!userHasNode) {
                        if (assignment == Schema::MANDATORY_PARAM) {
                            if (!m_allowMissingKeys) {
                                report << "Missing (choice-)parameter: \"" << currentScope << "\"" << endl;
                                return;
                            }
                        } else if (assignment == Schema::OPTIONAL_PARAM && hasDefault && m_injectDefaults) {
                            std::string optionName = it->getAttribute<string > (KARABO_SCHEMA_DEFAULT_VALUE);
                            Hash::Node& workNode = working.set(key, Hash(optionName, Hash())); // Inject empty choice
                            r_validate(it->getValue<Hash > ().get<Hash > (optionName), Hash(), workNode.getValue<Hash > ().get<Hash > (optionName), report, currentScope + "." + optionName);
                        }
                    } else { // User has set a node

                        std::set<std::string> validOptions;
                        master.get<Hash > (key).getKeys(validOptions);

                        // If the option has all-default parameters the user lazily may have set the option as string instead of HASH
                        // We will allow for this and silently inject an empty Hash instead
                        if (user.getType(key) == Types::STRING) {

                            //cout << "Silently converting from STRING" << endl;
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
                                    std::string optionName = it->getAttribute<string > (KARABO_SCHEMA_DEFAULT_VALUE);
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

                    int assignment = it->getAttribute<int>(KARABO_SCHEMA_ASSIGNMENT);

                    if (!userHasNode) { // Node IS NOT provided
                        if (assignment == Schema::MANDATORY_PARAM) {
                            if (!m_allowMissingKeys) {
                                report << "Missing (list-)parameter: \"" << currentScope << "\"" << endl;
                                return;
                            }
                        } else if ((assignment == Schema::OPTIONAL_PARAM) && hasDefault && m_injectDefaults) {
                            vector<string> optionNames = it->getAttributeAs<string, vector> (KARABO_SCHEMA_DEFAULT_VALUE);
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
                            int optionNamesSize = static_cast<int> (optionNames.size());
                            if (it->hasAttribute(KARABO_SCHEMA_MIN) && (optionNamesSize < it->getAttribute<int>(KARABO_SCHEMA_MIN))) {
                                report << "Too less options given for (list-)parameter: \"" << key << "\". Expecting at least " << it->getAttribute<int>(KARABO_SCHEMA_MIN);
                                return;
                            }
                            if (it->hasAttribute(KARABO_SCHEMA_MAX) && (optionNamesSize > it->getAttribute<int>(KARABO_SCHEMA_MAX))) {
                                report << "Too many options given for (list-)parameter: \"" << key << "\". Expecting at most " << it->getAttribute<int>(KARABO_SCHEMA_MAX);
                                return;
                            }


                            BOOST_FOREACH(string optionName, optionNames) {
                                cout << "Silently converting from STRING" << endl;
                                if (validOptions.find(optionName) != validOptions.end()) { // Is a valid option
                                    Hash tmp;
                                    r_validate(it->getValue<Hash > ().get<Hash > (optionName), Hash(), tmp, report, currentScope + "." + optionName);
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
                            int optionNamesSize = static_cast<int> (userOptions.size());
                            if (it->hasAttribute(KARABO_SCHEMA_MIN) && (optionNamesSize < it->getAttribute<int>(KARABO_SCHEMA_MIN))) {
                                report << "Too less options given for (list-)parameter: \"" << key << "\". Expecting at least " << it->getAttribute<int>(KARABO_SCHEMA_MIN);
                                report << "Valid options are: " << karabo::util::toString(validOptions) << endl;
                                return;
                            }
                            if (it->hasAttribute(KARABO_SCHEMA_MAX) && (optionNamesSize > it->getAttribute<int>(KARABO_SCHEMA_MAX))) {
                                report << "Too many options given for (list-)parameter: \"" << key << "\". Expecting at most " << it->getAttribute<int>(KARABO_SCHEMA_MAX);
                                report << "Valid options are: " << karabo::util::toString(validOptions) << endl;
                                return;
                            }

                            // That is what we expect it should be
                            for (size_t i = 0; i < userOptions.size(); ++i) {
                                const Hash& option = userOptions[i];
                                if (!option.empty()) {
                                    Hash::Node rootNode = *(option.begin());
                                    string optionName = rootNode.getKey();
                                    if (validOptions.find(optionName) != validOptions.end()) { // Is a valid option
                                        Hash tmp;

                                        if (rootNode.getType() == Types::STRING && rootNode.getValue<string>().empty()) {
                                            // Silently taking empty string as Hash
                                            Hash faked;
                                            r_validate(it->getValue<Hash > ().get<Hash > (optionName), faked, tmp, report, currentScope + "." + optionName);
                                        } else {
                                            r_validate(it->getValue<Hash > ().get<Hash > (optionName), rootNode.getValue<Hash>(), tmp, report, currentScope + "." + optionName);
                                        }

                                        workNodes.push_back(Hash(optionName, tmp));
                                        
                                    } else {
                                        report << "Provided parameter: \"" << optionName << "\" is not a valid option for list: \"" << key << "\". ";
                                        report << "Valid options are: " << karabo::util::toString(validOptions) << endl;
                                        return;

                                    }
                                } else {
                                    // No value provided
                                    report << "Missing parameter: \"" << key << "\". ";
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


        void Validator::validateLeaf(const Hash::Node& masterNode, Hash::Node& workNode, std::ostringstream& report, std::string scope) {


            if (m_injectTimestamps) attachTimestampIfNotAlreadyThere(workNode);

            Types::ReferenceType referenceType = Types::from<FromLiteral>(masterNode.getAttribute<string>(KARABO_SCHEMA_VALUE_TYPE));
            Types::ReferenceType referenceCategory = Types::category(referenceType);
            Types::ReferenceType givenType = workNode.getType();

            // Check data types
            if (givenType != referenceType) {
                if (givenType != Types::VECTOR_STRING || workNode.getValue<vector<string> >().size() != 0) {
                    // Try casting this guy
                    try {
                        workNode.setType(referenceType);
                    } catch (const CastException& e) {
                        report << "Failed to cast the value of parameter \"" << scope << "\" from " << Types::to<ToLiteral>(givenType);
                        report << " to " << Types::to<ToLiteral>(referenceType) << endl;
                        Exception::clearTrace(); // Do not show all the bloody details
                        return;
                    }
                }
            }

            // Check ranges
            if (referenceCategory == Types::SIMPLE) {
                if (masterNode.hasAttribute(KARABO_SCHEMA_OPTIONS)) {
                    vector<string> options;
                    masterNode.getAttribute(KARABO_SCHEMA_OPTIONS, options);
                    if (std::find(options.begin(), options.end(), workNode.getValueAs<string>()) == options.end()) {
                        report << "Value " << workNode.getValueAs<string>() << " for parameter \"" << scope << "\" is not one of the valid options: " << karabo::util::toString(options) << endl;
                    }
                }

                if (masterNode.hasAttribute(KARABO_SCHEMA_MIN_EXC)) {
                    double minExc = masterNode.getAttributeAs<double>(KARABO_SCHEMA_MIN_EXC);
                    double value = workNode.getValueAs<double>();
                    if (value <= minExc) {
                        report << "Value " << value << " for parameter \"" << scope << "\" is out of lower bound " << minExc << endl;
                    }
                }

                if (masterNode.hasAttribute(KARABO_SCHEMA_MIN_INC)) {
                    double minInc = masterNode.getAttributeAs<double>(KARABO_SCHEMA_MIN_INC);
                    double value = workNode.getValueAs<double>();
                    if (value < minInc) {
                        report << "Value " << value << " for parameter \"" << scope << "\" is out of lower bound " << minInc << endl;
                    }
                }

                if (masterNode.hasAttribute(KARABO_SCHEMA_MAX_EXC)) {
                    double maxExc = masterNode.getAttributeAs<double>(KARABO_SCHEMA_MAX_EXC);
                    double value = workNode.getValueAs<double>();
                    if (value >= maxExc) {
                        report << "Value " << value << " for parameter \"" << scope << "\" is out of upper bound " << maxExc << endl;
                    }
                }

                if (masterNode.hasAttribute(KARABO_SCHEMA_MAX_INC)) {
                    double maxInc = masterNode.getAttributeAs<double>(KARABO_SCHEMA_MAX_INC);
                    double value = workNode.getValueAs<double>();
                    if (value > maxInc) {
                        report << "Value " << value << " for parameter \"" << scope << "\" is out of upper bound " << maxInc << endl;
                    }
                }

                if (masterNode.hasAttribute(KARABO_SCHEMA_WARN_LOW)) {
                    double threshold = masterNode.getAttributeAs<double>(KARABO_SCHEMA_WARN_LOW);
                    double value = workNode.getValueAs<double>();
                    if (value < threshold) {
                        string msg("Value " + workNode.getValueAs<string>() + " of parameter \"" + scope + "\" went below warn level of " + karabo::util::toString(threshold));
                        m_parametersInWarnOrAlarm.set(scope, Hash("type", "WARN_LOW", "message", msg), '\0');
                        attachTimestampIfNotAlreadyThere(workNode);
                    }
                }

                if (masterNode.hasAttribute(KARABO_SCHEMA_WARN_HIGH)) {
                    double threshold = masterNode.getAttributeAs<double>(KARABO_SCHEMA_WARN_HIGH);
                    double value = workNode.getValueAs<double>();
                    if (value > threshold) {
                        string msg("Value " + workNode.getValueAs<string>() + " of parameter \"" + scope + "\" went above warn level of " + karabo::util::toString(threshold));
                        m_parametersInWarnOrAlarm.set(scope, Hash("type", "WARN_HIGH", "message", msg), '\0');
                        attachTimestampIfNotAlreadyThere(workNode);
                    }
                }

                if (masterNode.hasAttribute(KARABO_SCHEMA_ALARM_LOW)) {
                    double threshold = masterNode.getAttributeAs<double>(KARABO_SCHEMA_ALARM_LOW);
                    double value = workNode.getValueAs<double>();
                    if (value < threshold) {
                        string msg("Value " + workNode.getValueAs<string>() + " of parameter \"" + scope + "\" went below alarm level of " + karabo::util::toString(threshold));
                        m_parametersInWarnOrAlarm.set(scope, Hash("type", "ALARM_LOW", "message", msg), '\0');
                        attachTimestampIfNotAlreadyThere(workNode);
                    }
                }

                if (masterNode.hasAttribute(KARABO_SCHEMA_ALARM_HIGH)) {
                    double threshold = masterNode.getAttributeAs<double>(KARABO_SCHEMA_ALARM_HIGH);
                    double value = workNode.getValueAs<double>();
                    if (value > threshold) {
                        string msg("Value " + workNode.getValueAs<string>() + " of parameter \"" + scope + "\" went above alarm level of " + karabo::util::toString(threshold));
                        m_parametersInWarnOrAlarm.set(scope, Hash("type", "ALARM_HIGH", "message", msg), '\0');
                        attachTimestampIfNotAlreadyThere(workNode);
                    }
                }

                //if (masterNode.hasAttribute(""))
            } else if (referenceCategory == Types::SEQUENCE) {
                int currentSize = workNode.getValueAs<string, vector>().size();

                // TODO Check whether we are really going to validate inner elements of a vector for max/min..., maybe not.

                if (masterNode.hasAttribute(KARABO_SCHEMA_MIN_SIZE)) {
                    int minSize = masterNode.getAttribute<unsigned int>(KARABO_SCHEMA_MIN_SIZE);
                    if (currentSize < minSize) {
                        report << "Number of elements (" << currentSize << " for (vector-)parameter \"" << scope << "\" is smaller than lower bound (" << minSize << ")" << endl;
                    }
                }

                if (masterNode.hasAttribute(KARABO_SCHEMA_MAX_SIZE)) {
                    int maxSize = masterNode.getAttribute<unsigned int>(KARABO_SCHEMA_MAX_SIZE);
                    if (currentSize > maxSize) {
                        report << "Number of elements (" << currentSize << " for (vector-)parameter \"" << scope << "\" is greater than upper bound (" << maxSize << ")" << endl;
                    }
                }
            }
        }


        void Validator::attachTimestampIfNotAlreadyThere(Hash::Node& node) {
            if (m_injectTimestamps) {
                Hash::Attributes& attributes = node.getAttributes();
                if (!Timestamp::hashAttributesContainTimeInformation(attributes)) {
                    m_timestamp.toHashAttributes(attributes);
                }
            }
        }


        bool Validator::hasParametersInWarnOrAlarm() const {
            return !m_parametersInWarnOrAlarm.empty();
        }


        const Hash& Validator::getParametersInWarnOrAlarm() const {
            return m_parametersInWarnOrAlarm;
        }
    }
}