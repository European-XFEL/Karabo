/*
 * File:   Validator.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on February 8, 2013, 6:03 PM
 *
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


#ifndef KARABO_DATA_SCHEMA_VALIDATOR_HH
#define KARABO_DATA_SCHEMA_VALIDATOR_HH

#include <boost/foreach.hpp>
#include <map>
#include <mutex>
#include <shared_mutex>

#include "karabo/data/time/Timestamp.hh"
#include "karabo/data/types/NDArray.hh"
#include "karabo/data/types/Schema.hh"
#include "karabo/data/types/StringTools.hh"
#include "karabo/data/types/ToLiteral.hh"
#include "karabo/data/types/Units.hh"
#include "karabo/data/types/karaboDll.hh"

namespace karabo {
    namespace data {

        // Forward declaration
        class AlarmCondition;

        /**
         * @class Validator
         * @brief The Validator valididates configurations against Schemata
         *
         * The Validator class validates configurations stored in a Hash against
         * a Schema describing the Hash. If the schema defines default values
         * these are inserted into the Hash, depending on the assignment policy.
         * Validation will either be successful, if the configuration Hash fullfils
         * the requirements specified in the schema, or fail.
         */
        class Validator {
            // Validation flags
            bool m_injectDefaults;
            bool m_allowUnrootedConfiguration;
            bool m_allowAdditionalKeys;
            bool m_allowMissingKeys;
            bool m_injectTimestamps;
            bool m_forceInjectedTimestamp;
            bool m_strict;

            karabo::data::Timestamp m_timestamp;

           public:
            /**
             * ValidationRules specify the behavior of the Validator if
             * it encounters differences between the input Hash and
             * the Schema defining it. The following rules may be set
             *
             * - injectDefaults: inject default values (if defined) if a value for an element
             *                   defined in the Schema is missing from the input Hash.
             *
             * - allowUnrootedConfiguration: allow for unrooted input Hash, i.e. one that
             *                   doesn't have a classId as the key of the root node
             *
             * - allowAdditionalKeys: allow additional keys in the input Hash that do not map
             *                   to elements specified in the schema
             *
             * - allowMissingKeys: allow for keys missing in the input Hash even if an element for
             *                     this key is present in the schema
             *
             * - injectTimestamps for leaf elements:
             *    - if injectTimestamps is false: no injection
             *    - if injectTimestamps is true and forceInjectedTimestamp is false:
             *                timestamp is injected, but timestamp attributes present are not overwritten
             *    - if injectTimestamps and forceInjectedTimestamp are both true:
             *                                    timestamp is injected and may overwrite previous timestamp attributes
             * - strict: all elements mentioned in the schema must be specified explicitely and match in type
             *           (even readOnly without a default)
             *
             * If any of the above scenarios are encountered during validation and the option is not
             * set to true, i.e. the Validator is not allowed to resolve the issue, validation will
             * fail.
             *
             */
            struct ValidationRules {
                /**
                 * The default constructor of validation rules is least restrictive, i.e. all
                 * resolution options are set to true (except that additional keys are not allowed).
                 */
                ValidationRules()
                    : injectDefaults(true),
                      allowUnrootedConfiguration(true),
                      allowAdditionalKeys(false),
                      allowMissingKeys(true),
                      injectTimestamps(true),
                      forceInjectedTimestamp(false),
                      strict(false) {}

                ValidationRules(bool injectDefaults_, bool allowUnrootedConfiguration_, bool allowAdditionalKeys_,
                                bool allowMissingKeys_, bool injectTimestamps_, bool forceInjectedTimestamp_ = false,
                                bool strict_ = false)
                    : injectDefaults(injectDefaults_),
                      allowUnrootedConfiguration(allowUnrootedConfiguration_),
                      allowAdditionalKeys(allowAdditionalKeys_),
                      allowMissingKeys(allowMissingKeys_),
                      injectTimestamps(injectTimestamps_),
                      forceInjectedTimestamp(forceInjectedTimestamp_),
                      strict(strict_) {}

                bool injectDefaults;
                bool allowUnrootedConfiguration;
                bool allowAdditionalKeys;
                bool allowMissingKeys;
                bool injectTimestamps;
                bool forceInjectedTimestamp;
                bool strict; // Everything must be there, no casting
            };

            /**
             * Construct a Validator with default, i.e. least-restrictive ValidationRules
             */
            Validator();

            /**
             * Copy constructor will just take over other's validation rules
             * @param other
             */
            Validator(const Validator& other);

            /**
             * Construct a Validator with given rules
             * @param rules
             */
            Validator(const ValidationRules rules);

            /**
             * Set the ValidationRules for this Validator
             * @param rules
             */
            void setValidationRules(const Validator::ValidationRules& rules);

            /**
             * Get the currrent ValidationRules for this Validator
             * @return
             */
            Validator::ValidationRules getValidationRules() const;

            /**
             * Validate an unvalidated input against a schema and write the
             * validated output to a reference. If the rules define to inject timestamp
             * inject the current timestamp or the provided one
             * @param schema against which to validate
             * @param unvalidatedInput which should be validated
             * @param validatedOutput validated and altered according to current ValidationRules
             *                        (but not touched if 'strict' validation rule is active)
             * @param timestamp
             * @return a std::pair where the first entry is a Boolean indicating if validation was successful (true),
             *         and the second entry contains a string identifying the first validation failure encountered if
             *         validation has failed.
             *
             * During validation the following input is checked to fulfill the following characteristics
             *
             *  - all elements as defined by the Schema are present, if the validation rules do not allow for default
             * injection of missing keys. In the first case validation of a missing element only passes if a default
             * value for the element has been defined in the schema
             *
             *  - no elements in addition to those defined by the Schema are present, if the rules define that no
             * additional keys are allowed
             *
             *  - that the root element of the input Hash stands alone and represents the class id of a factorizable
             * class if an unrooted configuration is not allowed by the validation rules
             *
             *  - if validation rules are 'strict', also readOnly elements must be present and types must strictly
             *    match, i.e. no casting attempts are performed. That means that there is no need for populating
             *    the validated output which therefore staye empty (timestamps are not attached, irrespective of
             *    respective flags)
             *
             * In addition for the above "sanity" checks, the Validator performs the following tasks:
             *
             *  - for sequence values validate that they fulfill their minimum and maximum size requirements if defined
             * by the Schema
             *
             *  - inject timestamps for leaf elements of validatedOutput - details depending on ValidationRules:
             *    - if injectTimestamps is false: no injection, but timestamp attributes present in unvalidatedInput
             *                                    are transferred to validatedOutput
             *    - if injectTimestamps is true and forceInjectedTimestamp is false:
             *                                    timestamp is injected, but timestamp attributes present in
             * unvalidatedInput are not overwritten, but are transferred to validatedOutput
             *    - if injectTimestamps and forceInjectedTimestamp are both true:
             *                                    timestamp is injected even if another timestamp is in attributes
             *                                    of unvalidatedInput
             */
            std::pair<bool, std::string> validate(const Schema& schema, const Hash& unvalidatedInput,
                                                  Hash& validatedOutput, const Timestamp& timestamp = Timestamp());

           private:
            void validateUserOnly(const Hash& master, const Hash& user, Hash& working, std::ostringstream& report,
                                  const std::string& scope);

            void r_validate(const Hash& master, const Hash& user, Hash& working, std::ostringstream& report,
                            const std::string& scope);

            void validateNDArray(const Hash& master, const NDArray& user, const std::string& key, Hash& working,
                                 std::ostringstream& report, const std::string& scope);

            void validateLeaf(const Hash::Node& masterNode, const Hash::Node& userNode, Hash& working,
                              std::ostringstream& report, const std::string& scope);

            void validateVectorOfHashesLeaf(const Hash::Node& masterNode, const Hash::Node& userNode,
                                            Hash::Node* workNodePtr, std::ostringstream& report);

            void attachTimestampIfNotAlreadyThere(Hash::Node& node);
        };
    } // namespace data
} // namespace karabo

#endif /* VALIDATOR_HH */
