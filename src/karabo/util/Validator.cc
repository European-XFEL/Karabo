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
#include "NDArray.hh"


namespace karabo {
    namespace util {


        Validator::Validator()
            : m_injectDefaults(true)
            , m_allowUnrootedConfiguration(true)
            , m_allowAdditionalKeys(false)
            , m_allowMissingKeys(false)
            , m_injectTimestamps(false)
            , m_hasReconfigurableParameter(false) {
        }


        Validator::Validator(const Validator & other) {
            setValidationRules(other.getValidationRules());
            boost::unique_lock<boost::shared_mutex> lock(other.m_rollingStatMutex);
            m_parameterRollingStats = other.m_parameterRollingStats;
        }


        Validator::Validator(const ValidationRules rules)
            : m_hasReconfigurableParameter(false) {
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


            // Clear previous "reconfigurable" flag
            m_hasReconfigurableParameter = false;

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
                bool hasRowSchema = it->hasAttribute(KARABO_SCHEMA_ROW_SCHEMA);

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
                            if (hasRowSchema) node.setAttribute(KARABO_SCHEMA_ROW_SCHEMA, it->getAttribute<Schema>(KARABO_SCHEMA_ROW_SCHEMA));
                            if (it->hasAttribute(KARABO_INDICATE_STATE_SET)) node.setAttribute(KARABO_INDICATE_STATE_SET, true);
                            if (it->hasAttribute(KARABO_INDICATE_ALARM_SET)) node.setAttribute(KARABO_INDICATE_ALARM_SET, true);
                            this->validateLeaf(*it, node, report, currentScope);
                        }
                    } else { // Node IS provided
                        Hash::Node& node = working.setNode(user.getNode(key));
                        if (hasRowSchema) node.setAttribute(KARABO_SCHEMA_ROW_SCHEMA, it->getAttribute<Schema>(KARABO_SCHEMA_ROW_SCHEMA));
                        if (user.hasAttribute(key,KARABO_INDICATE_STATE_SET)) node.setAttribute(KARABO_INDICATE_STATE_SET, true);
                        if (user.hasAttribute(key,KARABO_INDICATE_ALARM_SET)) node.setAttribute(KARABO_INDICATE_ALARM_SET, true);
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
            if(masterNode.hasAttribute(KARABO_SCHEMA_LEAF_TYPE)){
                const int leafType = masterNode.getAttribute<int>(KARABO_SCHEMA_LEAF_TYPE);

                if(leafType == karabo::util::Schema::STATE && !workNode.hasAttribute(KARABO_INDICATE_STATE_SET)){
                    report << "State element at "<< scope <<" may only be set using predefined states"<<endl;
                }

                if(leafType != karabo::util::Schema::STATE && workNode.hasAttribute(KARABO_INDICATE_STATE_SET)){
                    report << "Tried setting non-state element at "<< scope <<" with state object"<<endl;
                }

                if(leafType == karabo::util::Schema::ALARM_CONDITION && !workNode.hasAttribute(KARABO_INDICATE_ALARM_SET)){
                    report << "Alarm Condition element at "<< scope <<" may only be set using predefined alarm conditions"<<endl;
                }

                if(leafType != karabo::util::Schema::ALARM_CONDITION && workNode.hasAttribute(KARABO_INDICATE_ALARM_SET)){
                    report << "Tried setting non-alarm condition element at "<< scope <<" with alarm condition object"<<endl;
                }
            }
                
            if (masterNode.hasAttribute(KARABO_SCHEMA_ACCESS_MODE) && masterNode.getAttribute<int>(KARABO_SCHEMA_ACCESS_MODE) == WRITE)
                m_hasReconfigurableParameter = true;

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

                const karabo::util::Types::ReferenceType workType = workNode.getType();
                if (karabo::util::Types::isNumericPod(workType)) { // Deal with warns and alarms only on POD types
                    workNode.setAttribute(KARABO_ALARM_ATTR, AlarmCondition::NONE.asString());
                    bool stayInAlarm = false;

                    // the order of these checks is important
                    stayInAlarm |= checkAndSetThresholdedAlarmCondition(AlarmCondition::WARN_LOW, masterNode, workNode, report, scope, false);
                    stayInAlarm |= checkAndSetThresholdedAlarmCondition(AlarmCondition::ALARM_LOW, masterNode, workNode, report, scope, false);
                    stayInAlarm |= checkAndSetThresholdedAlarmCondition(AlarmCondition::WARN_HIGH, masterNode, workNode, report, scope, true);
                    stayInAlarm |= checkAndSetThresholdedAlarmCondition(AlarmCondition::ALARM_HIGH, masterNode, workNode, report, scope, true);


                    if (masterNode.hasAttribute(KARABO_SCHEMA_ENABLE_ROLLING_STATS)) {
                        assureRollingStatsInitialized(scope, masterNode.getAttributeAs<double>(KARABO_SCHEMA_ROLLING_STATS_EVAL));
                        RollingWindowStatistics::Pointer rollingStats = m_parameterRollingStats[scope];
                        rollingStats->update(workNode.getValueAs<double>());
                        double variance = rollingStats->getRollingWindowVariance();
                        // the order of these checks is important

                        stayInAlarm |= checkAndSetThresholdedAlarmCondition(AlarmCondition::WARN_VARIANCE_LOW, variance, masterNode, workNode, report, scope, false);
                        stayInAlarm |= checkAndSetThresholdedAlarmCondition(AlarmCondition::ALARM_VARIANCE_LOW, variance, masterNode, workNode, report, scope, false);
                        stayInAlarm |= checkAndSetThresholdedAlarmCondition(AlarmCondition::WARN_VARIANCE_HIGH, variance, masterNode, workNode, report, scope, true);
                        stayInAlarm |= checkAndSetThresholdedAlarmCondition(AlarmCondition::ALARM_VARIANCE_HIGH, variance, masterNode, workNode, report, scope, true);

                    }

                    if (!stayInAlarm) {
                        m_parametersInWarnOrAlarm.erase(scope);
                    }
                }
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
            else if (referenceCategory == Types::NDARRAY) {
                checkNDArrayShape(masterNode, workNode, report, scope);
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


        bool Validator::hasReconfigurableParameter() const {
            return m_hasReconfigurableParameter;
        }


        void Validator::assureRollingStatsInitialized(const std::string & scope, const unsigned int & evalInterval) {
            boost::unique_lock<boost::shared_mutex> lock(m_rollingStatMutex);
            if (m_parameterRollingStats.find(scope) == m_parameterRollingStats.end()) {
                m_parameterRollingStats.insert(std::pair<std::string, RollingWindowStatistics::Pointer>(scope, RollingWindowStatistics::Pointer(new RollingWindowStatistics(evalInterval))));
            }
        }


        RollingWindowStatistics::ConstPointer Validator::getRollingStatistics(const std::string & scope) const {
            boost::shared_lock<boost::shared_mutex> lock(m_rollingStatMutex);
            std::map<std::string, RollingWindowStatistics::Pointer>::const_iterator stats = m_parameterRollingStats.find(scope);
            if (stats == m_parameterRollingStats.end()) {
                KARABO_LOGIC_EXCEPTION("Rolling statistics have not been enabled for '" + scope + "'!");
            }

            return stats->second;
        };


        void Validator::checkNDArrayShape(const Hash::Node& masterNode, Hash::Node& workNode, std::ostringstream& report, const std::string& scope) {
            const NDArrayElementShapeType& schemaShape = masterNode.getAttribute<NDArrayElementShapeType>(KARABO_SCHEMA_ARRAY_SHAPE);
            const Types::ReferenceType givenType = workNode.getType();

            switch (givenType) {
                case Types::NDARRAY_BOOL: {
                    const Dims& shape = workNode.getValue<NDArray<bool> >().getShape();
                    compareNDArrayShapes(schemaShape, shape, report, scope);
                    break;
                }
                case Types::NDARRAY_INT8: {
                    const Dims& shape = workNode.getValue<NDArray<signed char> >().getShape();
                    compareNDArrayShapes(schemaShape, shape, report, scope);
                    break;
                }
                case Types::NDARRAY_UINT8: {
                    const Dims& shape = workNode.getValue<NDArray<unsigned char> >().getShape();
                    compareNDArrayShapes(schemaShape, shape, report, scope);
                    break;
                }
                case Types::NDARRAY_INT16: {
                    const Dims& shape = workNode.getValue<NDArray<signed short> >().getShape();
                    compareNDArrayShapes(schemaShape, shape, report, scope);
                    break;
                }
                case Types::NDARRAY_UINT16: {
                    const Dims& shape = workNode.getValue<NDArray<unsigned short> >().getShape();
                    compareNDArrayShapes(schemaShape, shape, report, scope);
                    break;
                }
                case Types::NDARRAY_INT32: {
                    const Dims& shape = workNode.getValue<NDArray<int> >().getShape();
                    compareNDArrayShapes(schemaShape, shape, report, scope);
                    break;
                }
                case Types::NDARRAY_UINT32: {
                    const Dims& shape = workNode.getValue<NDArray<unsigned int> >().getShape();
                    compareNDArrayShapes(schemaShape, shape, report, scope);
                    break;
                }
                case Types::NDARRAY_INT64: {
                    const Dims& shape = workNode.getValue<NDArray<long long> >().getShape();
                    compareNDArrayShapes(schemaShape, shape, report, scope);
                    break;
                }
                case Types::NDARRAY_UINT64: {
                    const Dims& shape = workNode.getValue<NDArray<unsigned long long> >().getShape();
                    compareNDArrayShapes(schemaShape, shape, report, scope);
                    break;
                }
                case Types::NDARRAY_FLOAT: {
                    const Dims& shape = workNode.getValue<NDArray<float> >().getShape();
                    compareNDArrayShapes(schemaShape, shape, report, scope);
                    break;
                }
                case Types::NDARRAY_DOUBLE: {
                    const Dims& shape = workNode.getValue<NDArray<double> >().getShape();
                    compareNDArrayShapes(schemaShape, shape, report, scope);
                    break;
                }
                default:
                    report << "Unkown Type (" << givenType << ") for (ndarray-)parameter \"" << scope << "\"" << endl;
                    break;
            }
        }


        bool Validator::checkAndSetThresholdedAlarmCondition(const AlarmCondition& alarmCond, const Hash::Node& masterNode, Hash::Node& workNode, std::ostringstream& report, const std::string & scope, bool checkGreater){
            return checkAndSetThresholdedAlarmCondition(alarmCond, workNode.getValueAs<double>(), masterNode, workNode, report, scope, checkGreater);
        }


        bool Validator::checkAndSetThresholdedAlarmCondition(const AlarmCondition& alarmCond, double value, const Hash::Node& masterNode, Hash::Node& workNode, std::ostringstream& report, const std::string & scope, bool checkGreater){
            const std::string & alarmString = alarmCond.asString();
            if (masterNode.hasAttribute(alarmString)) {
                double threshold = masterNode.getAttributeAs<double>(alarmString);
                double value = workNode.getValueAs<double>();
                if ((checkGreater ? value > threshold : value < threshold)) {
                    string msg("Value " + workNode.getValueAs<string>() + " of parameter \"" + scope + "\" went "
                        + (checkGreater ? "above " : "below ") + alarmCond.asBaseString() + " level of "
                        + karabo::util::toString(threshold));
        
                    Hash::Node& desc = m_parametersInWarnOrAlarm.set(scope, Hash("type", alarmString, "message", msg), '\0');
                    m_timestamp.toHashAttributes(desc.getAttributes());   
                    workNode.setAttribute(KARABO_ALARM_ATTR, alarmString);


                    return true; //alarm condition re-raised, do not clear
                } else {
                    return false; // if it is no longer in alarm we may clear
                }
            }
            return false; // nothing to clear
        }


        void Validator::compareNDArrayShapes(const NDArrayElementShapeType& expected, const Dims& observed, std::ostringstream& report, const std::string& scope) {
            NDArrayElementShapeType::const_iterator eit = expected.begin();
            for (size_t idx=0; eit != expected.end() && idx < observed.rank(); ++eit, ++idx) {
                if (*eit == -1) {
                    // Negative dimension => variable.
                    continue;
                }
                if (*eit != static_cast<long long>(observed.extentIn(idx))) {
                    std::string expectedStr = toString<long long>(expected);
                    std::string observedStr = toString<unsigned long long>(observed.toVector());
                    report << "Expected array shape: " << expectedStr << " for (ndarray-)parameter \"" << scope << "\", but got " << observedStr << " instead." << endl;
                    return;
                }
            }
        }

    }
}