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

// To check whether std::format is supported (as is from gcc >= 13)
#include "Validator.hh"

#include <boost/algorithm/string/trim.hpp>
#include <version>

#include "TableElement.hh"
#include "karabo/data/time/Epochstamp.hh"
#include "karabo/data/types/AlarmConditions.hh"
#include "karabo/data/types/FromLiteral.hh"
#include "karabo/data/types/Hash.hh"
#include "karabo/data/types/NDArray.hh"
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

        namespace {
            /**
             * Get size of sequence inside node
             *
             * Throws if node contains an unknown type that cannot be casted to vector<string>
             */
            size_t sequenceSize(const Hash::Node& node) {
                switch (node.getType()) {
                    case Types::VECTOR_STRING:
                        return node.getValue<std::vector<std::string>>().size();

                    case Types::VECTOR_BOOL:
                        return node.getValue<std::vector<bool>>().size();

                    case Types::VECTOR_CHAR:
                        return node.getValue<std::vector<char>>().size();
                    case Types::VECTOR_UINT8:
                        return node.getValue<std::vector<unsigned char>>().size();
                    case Types::VECTOR_INT8:
                        return node.getValue<std::vector<signed char>>().size();
                    case Types::VECTOR_UINT16:
                        return node.getValue<std::vector<unsigned short>>().size();
                    case Types::VECTOR_INT16:
                        return node.getValue<std::vector<short>>().size();

                    case Types::VECTOR_UINT32:
                        return node.getValue<std::vector<unsigned int>>().size();
                    case Types::VECTOR_INT32:
                        return node.getValue<std::vector<int>>().size();

                    case Types::VECTOR_UINT64:
                        return node.getValue<std::vector<unsigned long long>>().size();
                    case Types::VECTOR_INT64:
                        return node.getValue<std::vector<long long>>().size();

                    case Types::VECTOR_FLOAT:
                        return node.getValue<std::vector<float>>().size();
                    case Types::VECTOR_DOUBLE:
                        return node.getValue<std::vector<double>>().size();

                    case Types::VECTOR_COMPLEX_FLOAT:
                        return node.getValue<std::vector<std::complex<float>>>().size();
                    case Types::VECTOR_COMPLEX_DOUBLE:
                        return node.getValue<std::vector<std::complex<double>>>().size();

                    default:
                        try {
                            // Costly try to get a size via casting to vector<string>
                            return node.getValueAs<std::string, std::vector>().size();
                        } catch (const std::exception&) {
                            KARABO_RETHROW;
                            return 0ul; // unreached
                        }
                }
            }
        } // namespace

        static bool isOutputChannelSchema(const karabo::data::Hash::Node& n);
        static bool onlyContainsEmptyHashLeafs(const karabo::data::Hash::Node& n);

        Validator::Validator()
            : m_injectDefaults(true),
              m_allowUnrootedConfiguration(true),
              m_allowAdditionalKeys(false),
              m_allowMissingKeys(false),
              m_injectTimestamps(false),
              m_forceInjectedTimestamp(false),
              m_strict(false) {}


        Validator::Validator(const Validator& other) : Validator(other.getValidationRules()) {}


        Validator::Validator(const ValidationRules rules) : m_timestamp(Epochstamp(0, 0), 0) {
            this->setValidationRules(rules);
        }


        void Validator::setValidationRules(const Validator::ValidationRules& rules) {
            m_injectDefaults = rules.injectDefaults;
            m_allowAdditionalKeys = rules.allowAdditionalKeys;
            m_allowMissingKeys = rules.allowMissingKeys;
            m_allowUnrootedConfiguration = rules.allowUnrootedConfiguration;
            m_injectTimestamps = rules.injectTimestamps;
            m_forceInjectedTimestamp = rules.forceInjectedTimestamp;
            m_strict = rules.strict;
        }


        Validator::ValidationRules Validator::getValidationRules() const {
            Validator::ValidationRules rules;
            rules.injectDefaults = m_injectDefaults;
            rules.allowAdditionalKeys = m_allowAdditionalKeys;
            rules.allowMissingKeys = m_allowMissingKeys;
            rules.allowUnrootedConfiguration = m_allowUnrootedConfiguration;
            rules.injectTimestamps = m_injectTimestamps;
            rules.forceInjectedTimestamp = m_forceInjectedTimestamp;
            rules.strict = m_strict;
            return rules;
        }


        std::pair<bool, string> Validator::validate(const Schema& schema, const Hash& unvalidatedInput,
                                                    Hash& validatedOutput, const Timestamp& timestamp) {
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
                                         std::ostringstream& report, const std::string& scope) {
            // No "injectDefaults", no "additionalKeys", allow "misssingKeys", allow "unrootedConfig",
            // not "strict"
            // Iterate user
            for (Hash::const_iterator uit = user.begin(); uit != user.end(); ++uit) {
                const Hash::Node& userNode = *uit;
                const string& key = userNode.getKey();

                string currentScope;
                if (scope.empty()) currentScope = key;
#if __cpp_lib_format
                else currentScope = std::format("{}.{}", scope, key);
#else
                else currentScope = (scope + ".") += key;
#endif

                auto masterNode = master.find(key);
                if (!masterNode) { // no "additionalKeys" allowed
                    report << "Encountered unexpected configuration parameter: \"" << currentScope << "\"" << endl;
                    return; // Could continue; and get more feedback
                }

                auto nodeType = Schema::NodeType(masterNode->getAttribute<int>(KARABO_SCHEMA_NODE_TYPE));
                const bool hasClassAttribute = masterNode->hasAttribute(KARABO_SCHEMA_CLASS_ID);

                if (nodeType == Schema::LEAF) {
                    this->validateLeaf(*masterNode, userNode, working, report, currentScope);
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
                                   const std::string& scope) {
            if (!m_injectDefaults && !m_allowAdditionalKeys && m_allowMissingKeys && m_allowUnrootedConfiguration &&
                !m_strict) {
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
#if __cpp_lib_format
                else currentScope = std::format("{}.{}", scope, key);
#else
                else currentScope = (scope + ".") += key;
#endif

                auto nodeType = static_cast<Schema::NodeType>(it->getAttribute<int>(KARABO_SCHEMA_NODE_TYPE));
                boost::optional<const Hash::Node&> userNode = user.find(key);
                const bool hasDefault = it->hasAttribute(KARABO_SCHEMA_DEFAULT_VALUE);

                // Remove current node from all provided
                if (userNode) keys.erase(key);

                if (nodeType == Schema::LEAF) {
                    auto assignment = Schema::AssignmentType(it->getAttribute<int>(KARABO_SCHEMA_ASSIGNMENT));

                    if (!userNode) { // Node IS NOT provided
                        if (m_strict) {
                            report << "Missing parameter '" << currentScope << "' in strict mode\n";
                        }
                        if (assignment == Schema::AssignmentType::MANDATORY_PARAM) {
                            if (!m_allowMissingKeys) {
                                report << "Missing mandatory parameter: \"" << currentScope << "\"" << endl;
                                return;
                            }
                        } else if ((assignment == Schema::OPTIONAL_PARAM || assignment == Schema::INTERNAL_PARAM) &&
                                   (hasDefault && m_injectDefaults)) {
                            this->validateLeaf(*it, Hash::Node(key, it->getAttributeAsAny(KARABO_SCHEMA_DEFAULT_VALUE)),
                                               working, report, currentScope);
                        }
                    } else { // Node IS provided
                        this->validateLeaf(*it, *userNode, working, report, currentScope);
                    }
                } else if (nodeType == Schema::NODE) {
                    // This block of code is here to sneak in the rule that we
                    // do not want the pipeline channel to have the schema field
                    // included in its validated configuration.
                    if (isOutputChannelSchema(*it)) {
                        if (!m_strict) working.set(key, Hash());
                        // Having an output.schema/output.schema.A.B...X entry in user's configuration
                        // hash is allright as long as the leaf node ends in an empty Hash.
                        //
                        // FIXME: This exception is a workaround for issue mentioned in:
                        // https://git.xfel.eu/Karabo/Framework/-/merge_requests/7892#note_403479
                        bool userHashHasOutputSchemaEntries = (userNode && !onlyContainsEmptyHashLeafs(*userNode));

                        if (userHashHasOutputSchemaEntries) {
                            report << "Configuring output channel schema is not allowed: '" << currentScope << "'"
                                   << std::endl;
                        }
                        return; // exit because we do not want to process/care about
                                // children of output channel's schema node.
                    }

                    const bool hasClassAttribute = it->hasAttribute(KARABO_SCHEMA_CLASS_ID);
                    if (hasClassAttribute) {
                        const std::string& classId = it->getAttribute<std::string>(KARABO_SCHEMA_CLASS_ID);
                        if (classId == "Slot") {
                            // Slot nodes should not appear in the validated output nor in the input.
                            // Tolerate empty node input for backward compatibility, though.
                            if (userNode &&
                                (userNode->getType() != Types::HASH || !userNode->getValue<Hash>().empty())) {
                                report << "There is configuration provided for Slot '" << currentScope << "'" << endl;
                                return;
                            }
                            continue;
                        } else if (classId == "NDArray") {
                            if (!userNode) {
                                // NDArray is always readOnly and thus may be missing except if we are strict.
                                // Note that it neither has defaults that one could inject here.
                                if (m_strict) {
                                    report << "NDArray is lacking for '" << currentScope << "'.\n";
                                }
                            } else {
                                validateNDArray(it->getValue<Hash>(), userNode->getValue<NDArray>(), key, working,
                                                report, currentScope);
                            }
                        }
                    }
                    if (!userNode) {
                        if (m_injectDefaults && !m_strict) {
                            Hash::Node& workNode = working.set(key, Hash()); // Insert empty node
                            if (hasClassAttribute) {
                                workNode.setAttribute(KARABO_HASH_CLASS_ID,
                                                      it->getAttribute<std::string>(KARABO_SCHEMA_CLASS_ID));
                            }
                            r_validate(it->getValue<Hash>(), Hash(), workNode.getValue<Hash>(), report, currentScope);
                        } else if (m_strict) {
                            report << "Missing node " << currentScope << std::endl;
                        } else {
                            Hash workFake;
                            r_validate(it->getValue<Hash>(), Hash(), workFake, report, currentScope);
                        }
                    } else {
                        if (userNode->getType() != Types::HASH) {
                            if (hasClassAttribute) {
                                // The node reflects a configuration for a class,
                                // what is provided here is the object already -> copy over and shut-up
                                Hash::Node& workNode = working.setNode(*userNode);
                                workNode.setAttribute(KARABO_HASH_CLASS_ID,
                                                      it->getAttribute<std::string>(KARABO_SCHEMA_CLASS_ID));
                                continue;
                            } else {
                                report << "Parameter \"" << currentScope
                                       << "\" has incorrect node type, expecting HASH not "
                                       << Types::to<ToLiteral>(userNode->getType()) << endl;
                                return;
                            }
                        } else {
                            Hash work;
                            r_validate(it->getValue<Hash>(), userNode->getValue<Hash>(), work, report, currentScope);
                            if (!m_strict) working.set(key, std::move(work));
                        }
                    }
                }
            }

            if (!m_allowAdditionalKeys && !keys.empty()) {
                for (const string& key : keys) {
                    string currentScope;
                    if (scope.empty()) currentScope = key;
#if __cpp_lib_format
                    else currentScope = std::format("{}.{}", scope, key);
#else
                    else currentScope = (scope + ".") += key;
#endif
                    report << "Encountered unexpected configuration parameter: \"" << currentScope << "\"" << endl;
                }
            }
        }

        void Validator::validateNDArray(const Hash& master, const NDArray& user, const std::string& key, Hash& working,
                                        std::ostringstream& report, const std::string& scope) {
            if (master.hasAttribute("shape", KARABO_SCHEMA_DEFAULT_VALUE)) {
                // Schema defines a shape - validate it!
                const std::vector<unsigned long long> userDimsVec = user.getShape().toVector();
                const auto& schemaDimsVec =
                      master.getAttribute<std::vector<unsigned long long>>("shape", KARABO_SCHEMA_DEFAULT_VALUE);
                bool mismatch = (userDimsVec.size() != schemaDimsVec.size());
                if (!mismatch) {
                    for (size_t i = 0; i < schemaDimsVec.size(); ++i) {
                        // dimension size 0 in schema means undefined
                        if (schemaDimsVec[i] != 0 && schemaDimsVec[i] != userDimsVec[i]) mismatch = true;
                    }
                }
                if (mismatch) {
                    report << "NDArray shape mismatch for '" << scope << "': should be (" << toString(schemaDimsVec)
                           << "), not (" << toString(userDimsVec) << ")\n";
                }
            }
            const Types::ReferenceType userType = user.getType();
            const auto schemaType =
                  static_cast<Types::ReferenceType>(master.getAttribute<int>("type", KARABO_SCHEMA_DEFAULT_VALUE));
            // Validate data type (but only if specified)
            if (userType != schemaType && schemaType != Types::UNKNOWN) {
                report << "NDArray type mismatch for '" << scope << "': should be " << Types::to<ToLiteral>(schemaType)
                       << ", not " << Types::to<ToLiteral>(userType) << "\n";
            }
            // FIXME: For unknown reasons, an rvalue has to be passed to setValue(..). If passing the normal const
            //        reference, the "_classId" attribute is not set to 'NDArray', but 'Hash'.
            //        Since a copy is anyway needed, this is no performance loss.
            if (!m_strict) {
                working.set(key, NDArray(user));
            }
        }

        struct FindInOptions {
            inline FindInOptions(const Hash::Node& masterNode, const Hash::Node& workNode)
                : result(false), m_masterNode(masterNode), m_workNode(workNode) {}

            template <class T>
            inline void operator()(T*) {
                const vector<T>& options = m_masterNode.getAttribute<vector<T>>(KARABO_SCHEMA_OPTIONS);
                result = std::find(options.begin(), options.end(), m_workNode.getValue<T>()) != options.end();
            }

            bool result;
            const Hash::Node& m_masterNode;
            const Hash::Node& m_workNode;
        };

        void Validator::validateLeaf(const Hash::Node& masterNode, const Hash::Node& userNode, Hash& working,
                                     std::ostringstream& report, const std::string& scope) {
            Types::ReferenceType referenceType =
                  Types::from<FromLiteral>(masterNode.getAttribute<string>(KARABO_SCHEMA_VALUE_TYPE));
            Types::ReferenceType referenceCategory = Types::category(referenceType);
            Types::ReferenceType givenType = userNode.getType();

            // Check data types
            if (m_strict && givenType != referenceType) {
                report << "Expect '" << Types::to<ToLiteral>(referenceType) << "', but got '"
                       << Types::to<ToLiteral>(givenType) << "' for " << scope << "\n";
                return;
            }
            // In m_strict mode, we do not want to copy anything to 'working' (output) and otherwise we want to allow
            // for some casting. So from now on we work on two variables:
            // - workNode: a pointer where to copy things to - non-null only if we want to copy, so we always check it
            // - typeValidatedUserNode: user input, either the type checked userNode, or the workNode we manipulate
            //                          (note we act on it a few lines below to make it the proper type if needed)
            Hash::Node* workNode =
                  (m_strict ? nullptr : &working.setNode(userNode)); // Copy user data, incl. attributes
            const Hash::Node& typeValidatedUserNode = (workNode ? *workNode : userNode);

            if (m_injectTimestamps && workNode) attachTimestampIfNotAlreadyThere(*workNode);

            if (givenType != referenceType) {
                // Cannot be reached if m_strict, i.e. workNode != nullptr
                if (referenceType == Types::VECTOR_HASH && givenType == Types::VECTOR_STRING &&
                    workNode->getValue<vector<string>>().empty()) {
                    // A HACK: Some Python code cannot distinguish between empty VECTOR_HASH and empty VECTOR_STRING
                    //         and in doubt chooses the latter.
                    //         So we tolerate empty vector<string> and overwrite by empty vector<Hash>.
                    workNode->setValue(vector<Hash>());
                    // TableElement cells may be aliasing values. In this case the actual value may be of NoneType
                } else if (!(givenType == Types::NONE && workNode->hasAttribute("isAliasing"))) {
                    // Try casting this guy
                    try {
                        workNode->setType(referenceType);
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
                    const std::string& value = typeValidatedUserNode.getValue<std::string>();
                    if (State::isValid(value)) {
                        // Set the KARABO_INDICATE_STATE_SET bit
                        if (workNode) workNode->setAttribute(KARABO_INDICATE_STATE_SET, true);
                    } else {
                        report << "Value '" << value << "' for parameter \"" << scope
                               << "\" is not a valid state string" << endl;
                    }
                } else if (typeValidatedUserNode.hasAttribute(KARABO_INDICATE_STATE_SET)) {
                    // the KARABO_INDICATE_STATE_SET attribute is being set on an element that is NOT a state element
                    report << "Tried setting non-state element at " << scope << " with state indication attribute"
                           << endl;
                }

                if (classId == "AlarmCondition") {
                    // this node is an alarm condition, we will validate the string against the allowed alarm strings
                    const std::string& value = typeValidatedUserNode.getValue<std::string>();
                    if (AlarmCondition::isValid(value)) {
                        // Set the KARABO_INDICATE_ALARM_SET bit
                        if (workNode) workNode->setAttribute(KARABO_INDICATE_ALARM_SET, true);
                    } else {
                        report << "Value '" << value << "' for parameter \"" << scope
                               << "\" is not a valid alarm string" << endl;
                    }
                } else if (typeValidatedUserNode.hasAttribute(KARABO_INDICATE_ALARM_SET)) {
                    // the KARABO_INDICATE_ALARM_SET attribute is being set on an element that is NOT an alarm condition
                    // element
                    report << "Tried setting non-alarm condition element at " << scope
                           << " with alarm indication attribute" << endl;
                }

                if (workNode) workNode->setAttribute(KARABO_HASH_CLASS_ID, classId);
            }

            // Check ranges
            if (referenceCategory == Types::SIMPLE) {
                if (masterNode.hasAttribute(KARABO_SCHEMA_OPTIONS)) {
                    FindInOptions findInOptions(masterNode, typeValidatedUserNode);
                    templatize(typeValidatedUserNode.getType(), findInOptions);

                    if (!findInOptions.result) {
                        report << "Value '" << typeValidatedUserNode.getValueAs<string>() << "' for parameter \""
                               << scope << "\" is not one of the valid options: "
                               << masterNode.getAttributeAs<string>(KARABO_SCHEMA_OPTIONS) << endl;
                    }
                }

                if (masterNode.hasAttribute(KARABO_SCHEMA_MIN_EXC)) {
                    double minExc = masterNode.getAttributeAs<double>(KARABO_SCHEMA_MIN_EXC);
                    double value = typeValidatedUserNode.getValueAs<double>();
                    if (value <= minExc) {
                        report << "Value " << value << " for parameter \"" << scope << "\" is out of lower bound "
                               << minExc << endl;
                    }
                }

                if (masterNode.hasAttribute(KARABO_SCHEMA_MIN_INC)) {
                    double minInc = masterNode.getAttributeAs<double>(KARABO_SCHEMA_MIN_INC);
                    double value = typeValidatedUserNode.getValueAs<double>();
                    if (value < minInc) {
                        report << "Value " << value << " for parameter \"" << scope << "\" is out of lower bound "
                               << minInc << endl;
                    }
                }

                if (masterNode.hasAttribute(KARABO_SCHEMA_MAX_EXC)) {
                    double maxExc = masterNode.getAttributeAs<double>(KARABO_SCHEMA_MAX_EXC);
                    double value = typeValidatedUserNode.getValueAs<double>();
                    if (value >= maxExc) {
                        report << "Value " << value << " for parameter \"" << scope << "\" is out of upper bound "
                               << maxExc << endl;
                    }
                }

                if (masterNode.hasAttribute(KARABO_SCHEMA_MAX_INC)) {
                    double maxInc = masterNode.getAttributeAs<double>(KARABO_SCHEMA_MAX_INC);
                    double value = typeValidatedUserNode.getValueAs<double>();
                    if (value > maxInc) {
                        report << "Value " << value << " for parameter \"" << scope << "\" is out of upper bound "
                               << maxInc << endl;
                    }
                }

            } else if (referenceCategory == Types::SEQUENCE) {
                if (masterNode.hasAttribute(KARABO_SCHEMA_MIN_SIZE)) {
                    const size_t currentSize = sequenceSize(typeValidatedUserNode);
                    const size_t minSize = masterNode.getAttribute<unsigned int>(KARABO_SCHEMA_MIN_SIZE);
                    if (currentSize < minSize) {
                        report << "Number of elements (" << currentSize << ") for (vector-)parameter \"" << scope
                               << "\" is smaller than lower bound (" << minSize << ")" << endl;
                    }
                }

                if (masterNode.hasAttribute(KARABO_SCHEMA_MAX_SIZE)) {
                    const size_t currentSize = sequenceSize(typeValidatedUserNode);
                    const size_t maxSize = masterNode.getAttribute<unsigned int>(KARABO_SCHEMA_MAX_SIZE);
                    if (currentSize > maxSize) {
                        report << "Number of elements (" << currentSize << ") for (vector-)parameter \"" << scope
                               << "\" is greater than upper bound (" << maxSize << ")" << endl;
                    }
                }
            } else if (referenceCategory == Types::VECTOR_HASH) {
                validateVectorOfHashesLeaf(masterNode, typeValidatedUserNode, workNode, report);
            }
        }

        void Validator::validateVectorOfHashesLeaf(const Hash::Node& masterNode, const Hash::Node& userNode,
                                                   Hash::Node* workNodePtr, std::ostringstream& report) {
            // A vector of hashes may be a table element - if it has a RowSchema attribute
            // it is assumed to be a table element.
            if (masterNode.hasAttribute(KARABO_SCHEMA_ROW_SCHEMA)) {
                const std::string& tableName = masterNode.getKey();

                const auto& rowSchema = masterNode.getAttribute<karabo::data::Schema>(KARABO_SCHEMA_ROW_SCHEMA);

                // Hack (again) case of empty (as checked before) vector<string> from user side
                const std::vector<karabo::data::Hash>& table =
                      (userNode.is<std::vector<std::string>>() ? std::vector<karabo::data::Hash>()
                                                               : userNode.getValue<std::vector<karabo::data::Hash>>());

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
                    ValidationRules rules(data::tableValidationRules);
                    rules.strict = m_strict;
                    Validator rowValidator(rules);
                    for (decltype(table.size()) i = 0; i < table.size(); i++) {
                        data::Hash validatedHash;
                        auto valResult = rowValidator.validate(rowSchema, table[i], validatedHash);
                        if (!valResult.first) {
                            report << valResult.second;
                            break;
                        } else {
                            // Updates the table row - the table validator may have injected columns, converted
                            // values, ....
                            if (workNodePtr) {
                                // i.e. m_strict == false (and otherwise validatedHash.empty() overwrites row!)
                                auto& workTable = workNodePtr->getValue<std::vector<karabo::data::Hash>>();
                                workTable[i] = std::move(validatedHash);
                            }
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
