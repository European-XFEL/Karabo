/*
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */
/*

 * File:   Validator.cc
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on February 8, 2013, 6:03 PM
 */

#include "Validator.hh"

#include <boost/algorithm/string/trim.hpp>

#include "TableElement.hh"
#include "karabo/data/time/Epochstamp.hh"
#include "karabo/data/types/AlarmConditions.hh"
#include "karabo/data/types/FromLiteral.hh"
#include "karabo/data/types/Hash.hh"
#include "karabo/data/types/Schema.hh"
#include "karabo/data/types/State.hh"
#include "karabo/data/types/StringTools.hh"
#include "karabo/data/types/Types.hh"

using std::endl;
using std::set;
using std::string;
using std::vector;


namespace karabo {
    namespace data {

        static bool isOutputChannelSchema(const karabo::data::Hash::Node& n);
        static bool onlyContainsEmptyHashLeafs(const karabo::data::Hash::Node& n);

        Validator::Validator()
            : m_injectDefaults(true),
              m_allowUnrootedConfiguration(true),
              m_allowAdditionalKeys(false),
              m_allowMissingKeys(false),
              m_injectTimestamps(false),
              m_forceInjectedTimestamp(false),
              m_hasReconfigurableParameter(false) {}


        Validator::Validator(const Validator& other) : Validator(other.getValidationRules()) {}


        Validator::Validator(const ValidationRules rules) : m_hasReconfigurableParameter(false) {
            this->setValidationRules(rules);
        }


        void Validator::setValidationRules(const Validator::ValidationRules& rules) {
            m_injectDefaults = rules.injectDefaults;
            m_allowAdditionalKeys = rules.allowAdditionalKeys;
            m_allowMissingKeys = rules.allowMissingKeys;
            m_allowUnrootedConfiguration = rules.allowUnrootedConfiguration;
            m_injectTimestamps = rules.injectTimestamps;
            m_forceInjectedTimestamp = rules.forceInjectedTimestamp;
        }


        Validator::ValidationRules Validator::getValidationRules() const {
            Validator::ValidationRules rules;
            rules.injectDefaults = m_injectDefaults;
            rules.allowAdditionalKeys = m_allowAdditionalKeys;
            rules.allowMissingKeys = m_allowMissingKeys;
            rules.allowUnrootedConfiguration = m_allowUnrootedConfiguration;
            rules.injectTimestamps = m_injectTimestamps;
            rules.forceInjectedTimestamp = m_forceInjectedTimestamp;
            return rules;
        }


        std::pair<bool, string> Validator::validate(const Schema& schema, const Hash& unvalidatedInput,
                                                    Hash& validatedOutput, const Timestamp& timestamp) {
            // Clear previous "reconfigurable" flag
            m_hasReconfigurableParameter = false;

            // Prepare timestamp if needed
            if (m_injectTimestamps) {
                m_timestamp = timestamp;
            }

            // In case of failed validation, report why it failed
            std::ostringstream validationFailedReport;

            if (!m_allowUnrootedConfiguration) {
                if (unvalidatedInput.size() != 1) {
                    return std::make_pair<bool, string>(false,
                                                        "Expecting a rooted input, i.e. a Hash with exactly one key "
                                                        "(describing the classId) at the top level");
                } else {
                    const Hash::Node& node = *(unvalidatedInput.begin());
                    const std::string& classId = node.getKey();
                    if (schema.getRootName() != classId) {
                        return std::make_pair<bool, string>(
                              false, "Wrong schema for given input. Schema describes class \"" + schema.getRootName() +
                                           "\", whilst input wants to configure class \"" + classId + "\"");
                    }
                    if (node.getType() == Types::HASH) {
                        Hash::Node& tmp = validatedOutput.set(classId, Hash());
                        this->r_validate(schema.getParameterHash(), node.getValue<Hash>(), tmp.getValue<Hash>(),
                                         validationFailedReport, classId);
                        if (validationFailedReport.str().empty()) {
                            return std::make_pair<bool, string>(true, "");
                        } else {
                            return std::make_pair<bool, string>(false, validationFailedReport.str());
                        }
                    } else {
                        return std::make_pair<bool, string>(
                              false, "Root-node for given configuration is of wrong type. It must be HASH");
                    }
                }
            } else {
                this->r_validate(schema.getParameterHash(), unvalidatedInput, validatedOutput, validationFailedReport,
                                 "");
                if (validationFailedReport.str().empty()) {
                    return std::make_pair(true, std::string());
                } else {
                    // Return with report, but trim any trailing newline:
                    std::string report(validationFailedReport.str());
                    boost::algorithm::trim_right(report);
                    return std::make_pair(false, report);
                }
            }
        }


        void Validator::validateUserOnly(const Hash& master, const Hash& user, Hash& working,
                                         std::ostringstream& report, std::string scope) {
            // No "injectDefaults", no "additionalKeys", allow "misssingKeys", allow "unrootedConfig"
            // Iterate user
            for (Hash::const_iterator uit = user.begin(); uit != user.end(); ++uit) {
                const Hash::Node& userNode = *uit;
                const string& key = userNode.getKey();

                string currentScope;
                if (scope.empty()) currentScope = key;
                else currentScope = scope + "." + key;

                auto masterNode = master.find(key);
                if (!masterNode) { // no "additionalKeys" allowed
                    report << "Encountered unexpected configuration parameter: \"" << currentScope << "\"" << endl;
                    return;
                }

                auto nodeType = Schema::NodeType(masterNode->getAttribute<int>(KARABO_SCHEMA_NODE_TYPE));
                const bool hasClassAttribute = masterNode->hasAttribute(KARABO_SCHEMA_CLASS_ID);

                if (nodeType == Schema::LEAF) {
                    Hash::Node& workNode = working.setNode(userNode); // copies also attributes, i.e. timestamp!
                    if (userNode.hasAttribute(KARABO_SCHEMA_CLASS_ID)) {
                        const std::string& classId = userNode.getAttribute<std::string>(KARABO_SCHEMA_CLASS_ID);
                        if (classId == "State") workNode.setAttribute(KARABO_INDICATE_STATE_SET, true);
                        else if (classId == "AlarmCondition") workNode.setAttribute(KARABO_INDICATE_ALARM_SET, true);
                        workNode.setAttribute(KARABO_HASH_CLASS_ID, classId);
                    }
                    this->validateLeaf(*masterNode, workNode, report, currentScope);
                } else if (nodeType == Schema::NODE) {
                    // See comment in `r_validate`...
                    if (isOutputChannelSchema(*masterNode)) {
                        working.set(key, Hash());
                        bool userHashHasOutputSchemaEntries = !onlyContainsEmptyHashLeafs(userNode);

                        if (userHashHasOutputSchemaEntries) {
                            report << "Configuring output channel schema is not allowed: '" << currentScope << "'"
                                   << std::endl;
                        }
                        return; // exit because we do not want to process/care about
                                // children of output channel's schema node.
                    }

                    if (hasClassAttribute && masterNode->getAttribute<std::string>(KARABO_SCHEMA_CLASS_ID) == "Slot") {
                        // Slot nodes should not appear in the validated output nor in the input.
                        // Tolerate empty node input for backward compatibility, though.
                        if (userNode.getType() != Types::HASH || !userNode.getValue<Hash>().empty()) {
                            report << "There is configuration provided for Slot '" << currentScope << "'" << endl;
                            return;
                        }
                        continue;
                    }

                    if (userNode.getType() != Types::HASH) {
                        if (hasClassAttribute) {
                            // The node reflects a configuration for a class,
                            // what is provided here is the object already -> copy over and shut-up
                            Hash::Node& workNode = working.setNode(userNode);
                            workNode.setAttribute(KARABO_HASH_CLASS_ID,
                                                  masterNode->getAttribute<std::string>(KARABO_SCHEMA_CLASS_ID));
                            continue;
                        } else {
                            report << "Parameter \"" << currentScope
                                   << "\" has incorrect node type, expecting HASH not "
                                   << Types::to<ToLiteral>(userNode.getType()) << endl;
                            return;
                        }
                    } else {
                        Hash::Node& workNode = working.set(key, Hash()); // Insert empty node
                        validateUserOnly(masterNode->getValue<Hash>(), userNode.getValue<Hash>(),
                                         workNode.getValue<Hash>(), report, currentScope);
                    }
                }
            }
        }


        void Validator::r_validate(const Hash& master, const Hash& user, Hash& working, std::ostringstream& report,
                                   std::string scope) {
            if (!m_injectDefaults && !m_allowAdditionalKeys && m_allowMissingKeys && m_allowUnrootedConfiguration) {
                validateUserOnly(master, user, working, report, scope);
                return;
            }

            std::set<std::string> keys;
            user.getKeys(keys);

            // Iterate master
            for (Hash::const_iterator it = master.begin(); it != master.end(); ++it) {
                const string& key = it->getKey();

                string currentScope;
                if (scope.empty()) currentScope = key;
                else currentScope = scope + "." + key;

                auto nodeType = static_cast<Schema::NodeType>(it->getAttribute<int>(KARABO_SCHEMA_NODE_TYPE));
                bool userHasNode = user.has(key);
                const bool hasDefault = it->hasAttribute(KARABO_SCHEMA_DEFAULT_VALUE);
                const bool hasClassAttribute = it->hasAttribute(KARABO_SCHEMA_CLASS_ID);

                // Remove current node from all provided
                if (userHasNode) keys.erase(key);

                if (nodeType == Schema::LEAF) {
                    auto assignment = Schema::AssignmentType(it->getAttribute<int>(KARABO_SCHEMA_ASSIGNMENT));

                    if (!userHasNode) { // Node IS NOT provided
                        if (assignment == Schema::AssignmentType::MANDATORY_PARAM) {
                            if (!m_allowMissingKeys) {
                                report << "Missing mandatory parameter: \"" << currentScope << "\"" << endl;
                                return;
                            }
                        } else if ((assignment == Schema::OPTIONAL_PARAM || assignment == Schema::INTERNAL_PARAM) &&
                                   (hasDefault && m_injectDefaults)) {
                            Hash::Node& node = working.set(key, it->getAttributeAsAny(KARABO_SCHEMA_DEFAULT_VALUE));
                            if (hasClassAttribute) {
                                const std::string& classId = it->getAttribute<std::string>(KARABO_SCHEMA_CLASS_ID);
                                if (classId == "State") node.setAttribute(KARABO_INDICATE_STATE_SET, true);
                                else if (classId == "AlarmCondition")
                                    node.setAttribute(KARABO_INDICATE_ALARM_SET, true);
                                node.setAttribute(KARABO_HASH_CLASS_ID, classId);
                            }
                            this->validateLeaf(*it, node, report, currentScope);
                        }
                    } else { // Node IS provided
                        Hash::Node& node =
                              working.setNode(user.getNode(key)); // copies also attributes, i.e. timestamp!
                        if (user.hasAttribute(key, KARABO_SCHEMA_CLASS_ID)) {
                            const std::string& classId = user.getAttribute<std::string>(key, KARABO_SCHEMA_CLASS_ID);
                            if (classId == "State") node.setAttribute(KARABO_INDICATE_STATE_SET, true);
                            else if (classId == "AlarmCondition") node.setAttribute(KARABO_INDICATE_ALARM_SET, true);
                            node.setAttribute(KARABO_HASH_CLASS_ID, classId);
                        }
                        this->validateLeaf(*it, node, report, currentScope);
                    }
                } else if (nodeType == Schema::NODE) {
                    // This block of code is here to sneak in the rule that we
                    // do not want the pipeline channel to have the schema field
                    // included in its validated configuration.
                    if (isOutputChannelSchema(*it)) {
                        working.set(key, Hash());
                        // Having an output.schema/output.schema.A.B...X entry in user's configuration
                        // hash is allright as long as the leaf node ends in an empty Hash.
                        //
                        // FIXME: This exception is a workaround for issue mentioned in:
                        // https://git.xfel.eu/Karabo/Framework/-/merge_requests/7892#note_403479
                        bool userHashHasOutputSchemaEntries =
                              (userHasNode && !onlyContainsEmptyHashLeafs(user.getNode(key)));

                        if (userHashHasOutputSchemaEntries) {
                            report << "Configuring output channel schema is not allowed: '" << currentScope << "'"
                                   << std::endl;
                        }
                        return; // exit because we do not want to process/care about
                                // children of output channel's schema node.
                    }

                    if (hasClassAttribute && it->getAttribute<std::string>(KARABO_SCHEMA_CLASS_ID) == "Slot") {
                        // Slot nodes should not appear in the validated output nor in the input.
                        // Tolerate empty node input for backward compatibility, though.
                        if (userHasNode && (user.getType(key) != Types::HASH || !user.get<Hash>(key).empty())) {
                            report << "There is configuration provided for Slot '" << currentScope << "'" << endl;
                            return;
                        }
                        continue;
                    }
                    if (!userHasNode) {
                        if (m_injectDefaults) {
                            Hash::Node& workNode = working.set(key, Hash()); // Insert empty node
                            if (hasClassAttribute) {
                                workNode.setAttribute(KARABO_HASH_CLASS_ID,
                                                      it->getAttribute<std::string>(KARABO_SCHEMA_CLASS_ID));
                            }
                            r_validate(it->getValue<Hash>(), Hash(), workNode.getValue<Hash>(), report, currentScope);
                        } else {
                            Hash workFake;
                            r_validate(it->getValue<Hash>(), Hash(), workFake, report, currentScope);
                        }
                    } else {
                        if (user.getType(key) != Types::HASH) {
                            if (hasClassAttribute) {
                                // The node reflects a configuration for a class,
                                // what is provided here is the object already -> copy over and shut-up
                                Hash::Node& workNode = working.setNode(user.getNode(key));
                                workNode.setAttribute(KARABO_HASH_CLASS_ID,
                                                      it->getAttribute<std::string>(KARABO_SCHEMA_CLASS_ID));
                                continue;
                            } else {
                                report << "Parameter \"" << currentScope
                                       << "\" has incorrect node type, expecting HASH not "
                                       << Types::to<ToLiteral>(user.getType(key)) << endl;
                                return;
                            }
                        } else {
                            Hash::Node& workNode = working.set(key, Hash()); // Insert empty node
                            r_validate(it->getValue<Hash>(), user.get<Hash>(key), workNode.getValue<Hash>(), report,
                                       currentScope);
                        }
                    }
                }
            }

            if (!m_allowAdditionalKeys && !keys.empty()) {
                for (const string& key : keys) {
                    string currentScope;
                    if (scope.empty()) currentScope = key;
                    else currentScope = scope + "." + key;
                    report << "Encountered unexpected configuration parameter: \"" << currentScope << "\"" << endl;
                }
            }
        }

        struct FindInOptions {
            inline FindInOptions(const Hash::Node& masterNode, Hash::Node& workNode)
                : result(false), m_masterNode(masterNode), m_workNode(workNode) {}

            template <class T>
            inline void operator()(T*) {
                const vector<T>& options = m_masterNode.getAttribute<vector<T>>(KARABO_SCHEMA_OPTIONS);
                result = std::find(options.begin(), options.end(), m_workNode.getValue<T>()) != options.end();
            }

            bool result;
            const Hash::Node& m_masterNode;
            Hash::Node& m_workNode;
        };

        void Validator::validateLeaf(const Hash::Node& masterNode, Hash::Node& workNode, std::ostringstream& report,
                                     std::string scope) {
            if (m_injectTimestamps) attachTimestampIfNotAlreadyThere(workNode);

            Types::ReferenceType referenceType =
                  Types::from<FromLiteral>(masterNode.getAttribute<string>(KARABO_SCHEMA_VALUE_TYPE));
            Types::ReferenceType referenceCategory = Types::category(referenceType);
            Types::ReferenceType givenType = workNode.getType();

            // Check data types
            if (givenType != referenceType) {
                if (referenceType == Types::VECTOR_HASH && givenType == Types::VECTOR_STRING &&
                    workNode.getValue<vector<string>>().empty()) {
                    // A HACK: Some Python code cannot distinguish between empty VECTOR_HASH and empty VECTOR_STRING
                    //         and in doubt chooses the latter.
                    //         So we tolerate empty vector<string> and overwrite by empty vector<Hash>.
                    workNode.setValue(vector<Hash>());
                    // TableElement cells may be aliasing values. In this case the actual value may be of NoneType
                } else if (!(workNode.hasAttribute("isAliasing") && givenType == Types::NONE)) {
                    // Try casting this guy
                    try {
                        workNode.setType(referenceType);
                    } catch (const CastException& e) {
                        report << "Failed to cast the value of parameter \"" << scope << "\" from "
                               << Types::to<ToLiteral>(givenType);
                        report << " to " << Types::to<ToLiteral>(referenceType) << endl;
                        Exception::clearTrace(); // Do not show all the bloody details
                        return;
                    }
                }
            }
            if (masterNode.hasAttribute(KARABO_SCHEMA_CLASS_ID)) {
                const std::string& classId = masterNode.getAttribute<std::string>(KARABO_SCHEMA_CLASS_ID);
                if (classId == "State") {
                    // this node is a state, we will validate the string against the allowed states
                    const std::string& value = workNode.getValue<std::string>();
                    try {
                        State::fromString(value);
                        // if the KARABO_INDICATE_STATE_SET is missing, we add it since the string is valid
                        if (!workNode.hasAttribute(KARABO_INDICATE_STATE_SET)) {
                            workNode.setAttribute(KARABO_INDICATE_STATE_SET, true);
                        }
                    } catch (const LogicException& e) {
                        report << "Value '" << value << "' for parameter \"" << scope
                               << "\" is not a valid state string" << endl;
                        Exception::clearTrace();
                    }
                } else if (workNode.hasAttribute(KARABO_INDICATE_STATE_SET)) {
                    // the KARABO_INDICATE_STATE_SET attribute is being set on an element that is NOT an state element
                    report << "Tried setting non-state element at " << scope << " with state indication attribute"
                           << endl;
                }

                if (classId == "AlarmCondition") {
                    // this node is an alarm condition, we will validate the string against the allowed alarm strings
                    const std::string& value = workNode.getValue<std::string>();
                    try {
                        AlarmCondition::fromString(value);
                        // if the KARABO_INDICATE_ALARM_SET is missing, we add it since the string is valid
                        if (!workNode.hasAttribute(KARABO_INDICATE_ALARM_SET)) {
                            workNode.setAttribute(KARABO_INDICATE_ALARM_SET, true);
                        }
                    } catch (const LogicException& e) {
                        report << "Value '" << value << "' for parameter \"" << scope
                               << "\" is not a valid alarm string" << endl;
                        Exception::clearTrace();
                    }
                } else if (workNode.hasAttribute(KARABO_INDICATE_ALARM_SET)) {
                    // the KARABO_INDICATE_ALARM_SET attribute is being set on an element that is NOT an alarm condition
                    // element
                    report << "Tried setting non-alarm condition element at " << scope
                           << " with alarm indication attribute" << endl;
                }
            }

            if (masterNode.hasAttribute(KARABO_SCHEMA_ACCESS_MODE) &&
                masterNode.getAttribute<int>(KARABO_SCHEMA_ACCESS_MODE) == WRITE)
                m_hasReconfigurableParameter = true;

            // Check ranges
            if (referenceCategory == Types::SIMPLE) {
                if (masterNode.hasAttribute(KARABO_SCHEMA_OPTIONS)) {
                    FindInOptions findInOptions(masterNode, workNode);
                    templatize(workNode.getType(), findInOptions);

                    if (!findInOptions.result) {
                        report << "Value '" << workNode.getValueAs<string>() << "' for parameter \"" << scope
                               << "\" is not one of the valid options: "
                               << masterNode.getAttributeAs<string>(KARABO_SCHEMA_OPTIONS) << endl;
                    }
                }

                if (masterNode.hasAttribute(KARABO_SCHEMA_MIN_EXC)) {
                    double minExc = masterNode.getAttributeAs<double>(KARABO_SCHEMA_MIN_EXC);
                    double value = workNode.getValueAs<double>();
                    if (value <= minExc) {
                        report << "Value " << value << " for parameter \"" << scope << "\" is out of lower bound "
                               << minExc << endl;
                    }
                }

                if (masterNode.hasAttribute(KARABO_SCHEMA_MIN_INC)) {
                    double minInc = masterNode.getAttributeAs<double>(KARABO_SCHEMA_MIN_INC);
                    double value = workNode.getValueAs<double>();
                    if (value < minInc) {
                        report << "Value " << value << " for parameter \"" << scope << "\" is out of lower bound "
                               << minInc << endl;
                    }
                }

                if (masterNode.hasAttribute(KARABO_SCHEMA_MAX_EXC)) {
                    double maxExc = masterNode.getAttributeAs<double>(KARABO_SCHEMA_MAX_EXC);
                    double value = workNode.getValueAs<double>();
                    if (value >= maxExc) {
                        report << "Value " << value << " for parameter \"" << scope << "\" is out of upper bound "
                               << maxExc << endl;
                    }
                }

                if (masterNode.hasAttribute(KARABO_SCHEMA_MAX_INC)) {
                    double maxInc = masterNode.getAttributeAs<double>(KARABO_SCHEMA_MAX_INC);
                    double value = workNode.getValueAs<double>();
                    if (value > maxInc) {
                        report << "Value " << value << " for parameter \"" << scope << "\" is out of upper bound "
                               << maxInc << endl;
                    }
                }

            } else if (referenceCategory == Types::SEQUENCE) {
                int currentSize = 0;
                // "vector<char>" and "vector<unsigned char>" have "toString"
                // and "fromString" specializations in "StringTools.hh" that
                // use Base64 encoding.
                //
                // Calling specific specializations of "getValue" for the
                // "vector<char>" and "vector<unsigned chars>" makes sure that
                // no wrong specialization of "fromString" is used and that
                // the resulting vector's is correct - before,
                // "getValueAs<string, vector>" was called for all types and
                // ended up using the wrong "fromString" specialization for the
                // two mentioned types.
                //
                // Base64 enconding is the right way to handle those two types
                // of vector. It would be possible to remove the Base64
                // enconding for the "vector<unsigned char>", but that would
                // break backward compatibility.
                if (referenceType == Types::VECTOR_CHAR) {
                    currentSize = workNode.getValue<vector<char>>().size();
                } else if (referenceType == Types::VECTOR_UINT8) {
                    currentSize = workNode.getValue<vector<unsigned char>>().size();
                } else {
                    currentSize = workNode.getValueAs<string, vector>().size();
                }

                // TODO Check whether we are really going to validate inner elements of a vector for max/min..., maybe
                // not.
                if (masterNode.hasAttribute(KARABO_SCHEMA_MIN_SIZE)) {
                    int minSize = masterNode.getAttribute<unsigned int>(KARABO_SCHEMA_MIN_SIZE);
                    if (currentSize < minSize) {
                        report << "Number of elements (" << currentSize << ") for (vector-)parameter \"" << scope
                               << "\" is smaller than lower bound (" << minSize << ")" << endl;
                    }
                }

                if (masterNode.hasAttribute(KARABO_SCHEMA_MAX_SIZE)) {
                    int maxSize = masterNode.getAttribute<unsigned int>(KARABO_SCHEMA_MAX_SIZE);
                    if (currentSize > maxSize) {
                        report << "Number of elements (" << currentSize << ") for (vector-)parameter \"" << scope
                               << "\" is greater than upper bound (" << maxSize << ")" << endl;
                    }
                }
            } else if (referenceCategory == Types::VECTOR_HASH) {
                validateVectorOfHashesLeaf(masterNode, workNode, report);
            }
        }


        void Validator::validateVectorOfHashesLeaf(const Hash::Node& masterNode, Hash::Node& workNode,
                                                   std::ostringstream& report) {
            // A vector of hashes may be a table element - if it has a RowSchema attribute
            // it is assumed to be a table element.
            if (masterNode.hasAttribute(KARABO_SCHEMA_ROW_SCHEMA)) {
                const std::string& tableName = masterNode.getKey();

                const auto& rowSchema = masterNode.getAttribute<karabo::data::Schema>(KARABO_SCHEMA_ROW_SCHEMA);
                std::vector<karabo::data::Hash>& table = workNode.getValue<std::vector<karabo::data::Hash>>();

                const long long minSize = !masterNode.hasAttribute(KARABO_SCHEMA_MIN_SIZE)
                                                ? -1ll
                                                : masterNode.getAttribute<unsigned int>(KARABO_SCHEMA_MIN_SIZE);
                const long long maxSize = !masterNode.hasAttribute(KARABO_SCHEMA_MAX_SIZE)
                                                ? -1ll
                                                : masterNode.getAttribute<unsigned int>(KARABO_SCHEMA_MAX_SIZE);

                // Validates that the number of rows is within the specified limits.
                if (minSize >= 0) {
                    if (table.size() < static_cast<unsigned int>(minSize)) {
                        report << "Table at '" << tableName << "' must have at least " << minSize
                               << (minSize == 1ll ? " row" : " rows");
                        report << "; it has " << table.size() << "." << std::endl;
                        return;
                    }
                }
                if (maxSize >= 0) {
                    if (table.size() > static_cast<unsigned int>(maxSize)) {
                        report << "Table at '" << tableName << "' must have no more than " << maxSize
                               << (maxSize == 1ll ? " row" : " rows");
                        report << "; it has " << table.size() << "." << std::endl;
                        return;
                    }
                }

                // Validates each row.
                if (table.size() > 0) {
                    Validator rowValidator(data::tableValidationRules);
                    for (decltype(table.size()) i = 0; i < table.size(); i++) {
                        data::Hash validatedHash;
                        auto valResult = rowValidator.validate(rowSchema, table[i], validatedHash);
                        if (!valResult.first) {
                            report << valResult.second;
                            break;
                        } else {
                            // Updates the table row - the table validator may have injected columns, converted
                            // values, ....
                            table[i] = std::move(validatedHash);
                        }
                    }
                }
            }
        }


        void Validator::attachTimestampIfNotAlreadyThere(Hash::Node& node) {
            if (m_injectTimestamps) {
                Hash::Attributes& attributes = node.getAttributes();
                if (m_forceInjectedTimestamp || !Timestamp::hashAttributesContainTimeInformation(attributes)) {
                    m_timestamp.toHashAttributes(attributes);
                }
            }
        }


        bool Validator::hasReconfigurableParameter() const {
            return m_hasReconfigurableParameter;
        }


        // The schema field of a output pipeline channel is identified with the OutputSchema tag.
        bool isOutputChannelSchema(const karabo::data::Hash::Node& n) {
            if (n.hasAttribute(KARABO_SCHEMA_DISPLAY_TYPE)) {
                const auto& displayType = n.getAttribute<std::string>(KARABO_SCHEMA_DISPLAY_TYPE);
                if (displayType == "OutputSchema") {
                    return true;
                }
            }
            return false;
        }

        /**
         * Recursively checks if the given node strictly contains Hash nodes
         * which ultimately ends in an empty Hash leaf.
         */
        bool onlyContainsEmptyHashLeafs(const karabo::data::Hash::Node& node) {
            if (!node.is<Hash>()) {
                return false;
            }
            for (auto& it : node.getValue<Hash>()) {
                if (!onlyContainsEmptyHashLeafs(it)) {
                    return false;
                }
            }
            return true;
        }
    } // namespace data
} // namespace karabo
