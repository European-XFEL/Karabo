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
 * File:   TableElement.hh
 * Author: haufs
 *
 * Created on August 7, 2015, 9:01 AM
 */

#ifndef KARABO_UTIL_TABLEELEMENT_HH
#define KARABO_UTIL_TABLEELEMENT_HH

#include <any>
#include <utility>
#include <vector>

#include "GenericElement.hh"
#include "LeafElement.hh"
#include "OverwriteElement.hh"
#include "Types.hh"
#include "Validator.hh"


namespace karabo {
    namespace util {

        /// Validation rules to be used when table elements in Hash are merged.
        extern const Validator::ValidationRules tableValidationRules;

        /**
         * @class TableDefaultValue
         * @brief The TableDefaultValue class defines a default value for the TableElement.
         */
        template <typename Element>
        class TableDefaultValue {
            Element* m_genericElement;


           public:
            TableDefaultValue() : m_genericElement(0) {}

            void setElement(Element* el) {
                m_genericElement = el;
            }

            /**
             * The <b>defaultValue</b> method serves for setting up the default value to be used when User
             * configuration does not specify another value.
             * @param val  Default value
             * @return reference to the Element for proper methods chaining
             */
            Element& defaultValue(const std::vector<Hash>& defaultValue) {
                if (m_genericElement->m_nodeSchema.empty()) {
                    throw KARABO_PARAMETER_EXCEPTION("Need to set a node schema first for defaults to be set");
                }

                std::vector<Hash> validated;
                Validator validator(karabo::util::tableValidationRules);

                for (std::vector<Hash>::const_iterator it = defaultValue.begin(); it != defaultValue.end(); ++it) {
                    Hash validatedHash;
                    std::pair<bool, std::string> validationResult =
                          validator.validate(m_genericElement->m_nodeSchema, *it, validatedHash);
                    if (!validationResult.first) {
                        throw KARABO_PARAMETER_EXCEPTION("Node schema didn't validate against present node schema: " +
                                                         validationResult.second);
                    }
                    validated.push_back(validatedHash);
                }

                m_genericElement->getNode().setAttribute(KARABO_SCHEMA_DEFAULT_VALUE, validated);

                return *m_genericElement;
            }

            /**
             * The <b>noDefaultValue</b> serves for setting up the element that does not have a default value.
             * @return reference to the Element for proper methods chaining
             */
            Element& noDefaultValue() {
                return *m_genericElement;
            }
        };

        /**
         * @class TableElement
         * @brief The TableElement represents a vector<Hash> with fixed entries and types
         *
         * The TableElement represents a vector<Hash> with fixed entries and types. This
         * means that each entry in the vector is expected to be a Hash with the
         * same keys and same types, except for those which are set to assignment optional
         * and have a default value.
         *
         * Tables are defined by assigning a rowSchema to them, hence specifying how the
         * Hash entries in the vector should look like. The Schema::Validator is aware of
         * these specifications and will perform validation on these elements.
         */
        class TableElement : public GenericElement<TableElement> {
            friend class TableDefaultValue<TableElement>;

            Schema m_nodeSchema;
            TableDefaultValue<TableElement> m_defaultValue;
            ReadOnlySpecific<TableElement, std::vector<Hash>> m_readOnlySpecific;
            Schema::AssemblyRules m_parentSchemaAssemblyRules;

           public:
            TableElement(Schema& expected) : GenericElement<TableElement>(expected) {
                m_defaultValue.setElement(this);
                m_readOnlySpecific.setElement(this);
                m_parentSchemaAssemblyRules = expected.getAssemblyRules();
            }

            TableElement& minSize(const unsigned int& value) {
                this->m_node->setAttribute(KARABO_SCHEMA_MIN_SIZE, value);
                return *this;
            }

            TableElement& maxSize(const unsigned int& value) {
                this->m_node->setAttribute(KARABO_SCHEMA_MAX_SIZE, value);
                return *this;
            }

            virtual ReadOnlySpecific<TableElement, std::vector<Hash>>& readOnly() {
                if (this->m_node->hasAttribute(KARABO_SCHEMA_ASSIGNMENT)) {
                    const int assignment = this->m_node->template getAttribute<int>(KARABO_SCHEMA_ASSIGNMENT);
                    if (assignment == Schema::MANDATORY_PARAM) {
                        std::string msg("Error in element '");
                        msg.append(this->m_node->getKey())
                              .append("': readOnly() is not compatible with assignmentMandatory()");
                        throw KARABO_LOGIC_EXCEPTION(msg);
                    } else if (assignment == Schema::OPTIONAL_PARAM &&
                               this->m_node->hasAttribute(KARABO_SCHEMA_DEFAULT_VALUE)) {
                        std::string msg("Error in element '");
                        msg.append(this->m_node->getKey())
                              .append("': readOnly() is not compatible with assignmentOptional().defaultValue(v). ")
                              .append("Use readOnly().defaultValue(v) instead.");
                        throw KARABO_LOGIC_EXCEPTION(msg);
                    }
                }
                this->m_node->template setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, READ);
                // Set the assignment and defaults here, as the API would look strange to assign something to a
                // read-only
                this->m_node->template setAttribute<int>(KARABO_SCHEMA_ASSIGNMENT, Schema::OPTIONAL_PARAM);
                this->m_node->setAttribute(KARABO_SCHEMA_DEFAULT_VALUE, std::vector<Hash>());
                return m_readOnlySpecific;
            }

            /**
             * The <b>allowedStates</b> method serves for setting up allowed states for the element
             * @param states A string describing list of possible states.
             * @param sep A separator symbol used for parsing previous argument for list of states
             * @return reference to the Element (to allow method's chaining)
             */
            TableElement& allowedStates(const std::vector<karabo::util::State>& value) {
                const std::string stateString = karabo::util::toString(value);
                this->m_node->setAttribute(KARABO_SCHEMA_ALLOWED_STATES,
                                           karabo::util::fromString<std::string, std::vector>(stateString, ","));
                return *this;
            }

            // overloads for up to six elements

            TableElement& allowedStates(const karabo::util::State& s1) {
                const karabo::util::State arr[] = {s1};
                return allowedStates(std::vector<karabo::util::State>(arr, arr + 1));
            }

            TableElement& allowedStates(const karabo::util::State& s1, const karabo::util::State& s2) {
                const karabo::util::State arr[] = {s1, s2};
                return allowedStates(std::vector<karabo::util::State>(arr, arr + 2));
            }

            TableElement& allowedStates(const karabo::util::State& s1, const karabo::util::State& s2,
                                        const karabo::util::State& s3) {
                const karabo::util::State arr[] = {s1, s2, s3};
                return allowedStates(std::vector<karabo::util::State>(arr, arr + 3));
            }

            TableElement& allowedStates(const karabo::util::State& s1, const karabo::util::State& s2,
                                        const karabo::util::State& s3, const karabo::util::State& s4) {
                const karabo::util::State arr[] = {s1, s2, s3, s4};
                return allowedStates(std::vector<karabo::util::State>(arr, arr + 4));
            }

            TableElement& allowedStates(const karabo::util::State& s1, const karabo::util::State& s2,
                                        const karabo::util::State& s3, const karabo::util::State& s4,
                                        const karabo::util::State& s5) {
                const karabo::util::State arr[] = {s1, s2, s3, s4, s5};
                return allowedStates(std::vector<karabo::util::State>(arr, arr + 5));
            }

            TableElement& allowedStates(const karabo::util::State& s1, const karabo::util::State& s2,
                                        const karabo::util::State& s3, const karabo::util::State& s4,
                                        const karabo::util::State& s5, const karabo::util::State& s6) {
                const karabo::util::State arr[] = {s1, s2, s3, s4, s5, s6};
                return allowedStates(std::vector<karabo::util::State>(arr, arr + 6));
            }

            /**
             * The <b>assignmentMandatory</b> method serves for setting up a mode that requires the value
             * of the element always being specified. No default value is possible.
             * @return reference to the Element (to allow method's chaining)
             */
            virtual TableElement& assignmentMandatory() {
                this->m_node->setAttribute<int>(KARABO_SCHEMA_ASSIGNMENT, Schema::MANDATORY_PARAM);
                return *this;
            }

            /**
             * The <b>assignmentOptional</b> method serves for setting up a mode that allows the value of
             * element be optional, so it can be omitted in configuration. Default value is injected if defined.
             * If you chain functions for definition of expected parameters the next
             * function may be only defaultValue or noDefaultValue.
             * When the default value is not specified (noDefaultValue) you must always check
             * if the parameter has a value set in delivered User configuration.
             * @return reference to DefaultValue object allowing proper <b>defaultValue</b> method chaining.
             *
             * <b>Example:</b>
             * @code
             * SOME_ELEMENT(expected)
             *         ...
             *         .assignmentOptional().defaultValue("client")
             *         ...
             *         .commit();
             * @endcode
             */
            virtual TableDefaultValue<TableElement>& assignmentOptional() {
                this->m_node->setAttribute<int>(KARABO_SCHEMA_ASSIGNMENT, Schema::OPTIONAL_PARAM);
                return m_defaultValue;
            }

            /**
             * The <b>assignmentInternal</b> method serves for setting up the element to be internal. In the code
             * it behaves like optional parameter but it is not exposed to the user. It is omitted when the schema
             * is serialized to XSD. The value of this parameter should be defined programmatically. Conceptually,
             * internal parameter with internal flag can be treated as an argument to the constructor.
             * @return reference to DefaultValue (to allow method's chaining)
             */
            virtual TableDefaultValue<TableElement>& assignmentInternal() {
                this->m_node->setAttribute<int>(KARABO_SCHEMA_ASSIGNMENT, Schema::INTERNAL_PARAM);
                return m_defaultValue;
            }

            /**
             * The <b>init</b> method serves for setting up an access type property that allows the element
             * to be included in initial schema.
             * @return reference to the Element (to allow method's chaining)
             */
            virtual TableElement& init() {
                this->m_node->setValue(std::vector<Hash>());
                this->m_node->setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, INIT);
                return *this;
            }

            /**
             * The <b>reconfigurable</b> method serves for setting up an access type property that allows the element
             * to be included in initial, reconfiguration and monitoring schemas.
             * @return reference to the Element (to allow method's chaining)
             */
            virtual TableElement& reconfigurable() {
                this->m_node->setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, WRITE);
                return *this;
            }

            /**
             * This method is deprecated.  Use <b>setColumns</b> instead.
             * @param schema
             * @return reference to the Element (chaining)
             */
            KARABO_DEPRECATED TableElement& setNodeSchema(const Schema& schema) {
                m_nodeSchema = schema;

                // this->addRow(); //set first element to containt parameter Hash

                return *this;
            }


            /**
             * This method establishes content of the table, i.e. table columns and their types
             * @param schema
             * @return reference to the Element (chaining)
             */
            TableElement& setColumns(const Schema& schema) {
                m_nodeSchema = schema;
                return *this;
            }

            /**
             * This method appends additional columns to the right side of the table.
             * @param schema
             * @return reference to the Element (chaining)
             */
            TableElement& addColumns(const Schema& schema) {
                m_nodeSchema.merge(schema);
                return *this;
            }

            /**
             * This method appends additional columns to the table taken from
             * some class.  The class is template parameter.
             * @return reference to the Element (chaining)
             */
            template <class T>
            TableElement& addColumnsFromClass() {
                // Simply append the expected parameters of T to current node
                Schema schema("dummyRoot", m_parentSchemaAssemblyRules);
                T::_KARABO_SCHEMA_DESCRIPTION_FUNCTION(schema);
                m_nodeSchema.merge(schema);
                return *this;
            }


           protected:
            void beforeAddition();

           private:
            /**
             * @brief Controlling method for row schema sanitization.
             *
             * @throws karabo::util::ParameterException can be throw by
             * some specific sanitization methods.
             */
            void sanitizeRowSchema(karabo::util::Schema& rowSchema);

            /**
             * @brief Turns reconfigurable and init columns into read-only
             * columns when the hosting table is read-only. When the hosting
             * table is init or reconfigurable, turns every init column into
             * a reconfigurable column.
             */
            void sanitizeColumnsAccessModes(karabo::util::Schema& rowSchema);

            /**
             * @brief Makes sure that every non read-only column in a non
             * read-only table has a default value set.
             *
             * Default values are synthesized for the columns that don't have
             * a default value. The synthesized values correspond to the
             * default initializer of the column type (e.g. 0 for int columns,
             * false for bool columns and empty vectors for vector<..> columns).
             *
             * @throw karabo::util::ParameterException if the synthesized column
             * default values are incompatible with any attribute that already
             * existed in the schema, like 'minInc' or 'minSize' or if the
             * type of the column lacking a default value is not supported for
             * table columns.
             */
            void sanitizeNoDefaultColumns(karabo::util::Schema& rowSchema);

            /**
             * @brief Finds, in the TableElement rowSchema, a column of an
             * unsupported type - the TableElement supports a subset of the
             * Leaf types for valid column types.
             *
             * @return std::pair<std::string, karabo::util::Types::ReferenceType>
             * 'first' is the name of the column of the invalid type (empty
             * string if all columns are of supported types) and 'second' is the
             * type of the column with the invalid type (UNKNOWN if there is no
             * column of an invalid type).
             */
            std::pair<std::string, karabo::util::Types::ReferenceType> findUnsupportedColumnType(
                  const karabo::util::Schema& rowSchema);

            void setDefaultValueForColumn(const std::string& colName, const karabo::util::Types::ReferenceType& colType,
                                          karabo::util::Schema& rowSchema);

            void checkNumericDefaultInRange(const std::string& colName, const karabo::util::Schema& rowSchema);

            void checkSimpleDefaultInOptions(const std::string& colName,
                                             const karabo::util::Types::ReferenceType& colType,
                                             const karabo::util::Schema& rowSchema);
        };

        typedef util::TableElement TABLE_ELEMENT;
    } // namespace util
} // namespace karabo


#endif /* TABLEELEMENT_HH */
