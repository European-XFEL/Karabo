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
#include "OverwriteElement.hh"

namespace karabo {
    namespace data {
        OverwriteElement::OverwriteElement(Schema& expected) : m_schema(&expected), m_node(nullptr), m_path("") {}


        OverwriteElement& OverwriteElement::key(std::string const& name) {
            m_path = name;
            boost::optional<Hash::Node&> node = m_schema->getParameterHash().find(name);
            if (node) { // exists
                m_node = node.get_ptr();
                if (node->hasAttribute(KARABO_OVERWRITE_RESTRICTIONS)) {
                    m_restrictions.assignFromAttrVector(
                          node->getAttribute<std::vector<bool>>(KARABO_OVERWRITE_RESTRICTIONS));
                }
            } else {
                throw KARABO_PARAMETER_EXCEPTION("Key '" + name +
                                                 "' not in actual schema, thus cannot be overwritten.");
            }
            return *this;
        }

        OverwriteElement& OverwriteElement::setNewDisplayedName(const std::string& name) {
            if (!m_node) throw KARABO_LOGIC_EXCEPTION("Please call key(..) before other methods of OverwriteElement");
            checkIfRestrictionApplies(m_restrictions.displayedName);
            m_node->setAttribute(KARABO_SCHEMA_DISPLAYED_NAME, name);
            return *this;
        }

        OverwriteElement& OverwriteElement::setNewDescription(const std::string& description) {
            if (!m_node) throw KARABO_LOGIC_EXCEPTION("Please call key(..) before other methods of OverwriteElement");
            checkIfRestrictionApplies(m_restrictions.description);
            m_node->setAttribute(KARABO_SCHEMA_DESCRIPTION, description);
            return *this;
        }


        OverwriteElement& OverwriteElement::setNewAssignmentMandatory() {
            if (!m_node) throw KARABO_LOGIC_EXCEPTION("Please call key(..) before other methods of OverwriteElement");
            checkIfRestrictionApplies(m_restrictions.assignmentMandatory);
            m_node->setAttribute<int>(KARABO_SCHEMA_ASSIGNMENT, Schema::MANDATORY_PARAM);
            return *this;
        }

        OverwriteElement& OverwriteElement::setNewAssignmentOptional() {
            if (!m_node) throw KARABO_LOGIC_EXCEPTION("Please call key(..) before other methods of OverwriteElement");
            checkIfRestrictionApplies(m_restrictions.assignmentOptional);
            m_node->setAttribute<int>(KARABO_SCHEMA_ASSIGNMENT, Schema::OPTIONAL_PARAM);
            return *this;
        }

        OverwriteElement& OverwriteElement::setNewAssignmentInternal() {
            if (!m_node) throw KARABO_LOGIC_EXCEPTION("Please call key(..) before other methods of OverwriteElement");
            checkIfRestrictionApplies(m_restrictions.assignmentInternal);
            m_node->setAttribute<int>(KARABO_SCHEMA_ASSIGNMENT, Schema::INTERNAL_PARAM);
            return *this;
        }

        OverwriteElement& OverwriteElement::setNowInit() {
            if (!m_node) throw KARABO_LOGIC_EXCEPTION("Please call key(..) before other methods of OverwriteElement");
            checkIfRestrictionApplies(m_restrictions.init);
            m_node->setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, INIT);
            return *this;
        }

        OverwriteElement& OverwriteElement::setNowReconfigurable() {
            if (!m_node) throw KARABO_LOGIC_EXCEPTION("Please call key(..) before other methods of OverwriteElement");
            checkIfRestrictionApplies(m_restrictions.reconfigurable);
            m_node->setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, WRITE);
            return *this;
        }

        OverwriteElement& OverwriteElement::setNowReadOnly() {
            if (!m_node) throw KARABO_LOGIC_EXCEPTION("Please call key(..) before other methods of OverwriteElement");
            checkIfRestrictionApplies(m_restrictions.readOnly);
            m_node->setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, READ);
            return *this;
        }

        OverwriteElement& OverwriteElement::setNewOptions(const std::string& opts, const std::string& sep) {
            return setNewOptions(opts, true, sep);
        }

        OverwriteElement& OverwriteElement::setNewOptions(const std::vector<karabo::data::State>& opts) {
            checkIfRestrictionApplies(m_restrictions.stateOptions);
            return setNewOptions(toString(opts), false, ",");
        }

        OverwriteElement& OverwriteElement::setNewOptions(const karabo::data::State& s1) {
            const karabo::data::State arr[] = {s1};
            return setNewOptions(std::vector<karabo::data::State>(arr, arr + 1));
        }

        OverwriteElement& OverwriteElement::setNewOptions(const karabo::data::State& s1,
                                                          const karabo::data::State& s2) {
            const karabo::data::State arr[] = {s1, s2};
            return setNewOptions(std::vector<karabo::data::State>(arr, arr + 2));
        }

        OverwriteElement& OverwriteElement::setNewOptions(const karabo::data::State& s1, const karabo::data::State& s2,
                                                          const karabo::data::State& s3) {
            const karabo::data::State arr[] = {s1, s2, s3};
            return setNewOptions(std::vector<karabo::data::State>(arr, arr + 3));
        }

        OverwriteElement& OverwriteElement::setNewOptions(const karabo::data::State& s1, const karabo::data::State& s2,
                                                          const karabo::data::State& s3,
                                                          const karabo::data::State& s4) {
            const karabo::data::State arr[] = {s1, s2, s3, s4};
            return setNewOptions(std::vector<karabo::data::State>(arr, arr + 4));
        }

        OverwriteElement& OverwriteElement::setNewOptions(const karabo::data::State& s1, const karabo::data::State& s2,
                                                          const karabo::data::State& s3, const karabo::data::State& s4,
                                                          const karabo::data::State& s5) {
            const karabo::data::State arr[] = {s1, s2, s3, s4, s5};
            return setNewOptions(std::vector<karabo::data::State>(arr, arr + 5));
        }

        OverwriteElement& OverwriteElement::setNewOptions(const karabo::data::State& s1, const karabo::data::State& s2,
                                                          const karabo::data::State& s3, const karabo::data::State& s4,
                                                          const karabo::data::State& s5,
                                                          const karabo::data::State& s6) {
            const karabo::data::State arr[] = {s1, s2, s3, s4, s5, s6};
            return setNewOptions(std::vector<karabo::data::State>(arr, arr + 6));
        }

        OverwriteElement& OverwriteElement::setNewOptions(const karabo::data::State& s1, const karabo::data::State& s2,
                                                          const karabo::data::State& s3, const karabo::data::State& s4,
                                                          const karabo::data::State& s5, const karabo::data::State& s6,
                                                          const karabo::data::State& s7) {
            const karabo::data::State arr[] = {s1, s2, s3, s4, s5, s6, s7};
            return setNewOptions(std::vector<karabo::data::State>(arr, arr + 7));
        }

        OverwriteElement& OverwriteElement::setNewOptions(const karabo::data::State& s1, const karabo::data::State& s2,
                                                          const karabo::data::State& s3, const karabo::data::State& s4,
                                                          const karabo::data::State& s5, const karabo::data::State& s6,
                                                          const karabo::data::State& s7,
                                                          const karabo::data::State& s8) {
            const karabo::data::State arr[] = {s1, s2, s3, s4, s5, s6, s7, s8};
            return setNewOptions(std::vector<karabo::data::State>(arr, arr + 8));
        }

        OverwriteElement& OverwriteElement::setNewOptions(const std::vector<std::string>& opts) {
            if (!m_node) throw KARABO_LOGIC_EXCEPTION("Please call key(..) before other methods of OverwriteElement");
            checkIfRestrictionApplies(m_restrictions.options);
            m_node->setAttribute(KARABO_SCHEMA_OPTIONS, opts);
            return *this;
        }

        OverwriteElement& OverwriteElement::setNewAllowedStates(const std::vector<karabo::data::State>& states) {
            if (!m_node) throw KARABO_LOGIC_EXCEPTION("Please call key(..) before other methods of OverwriteElement");
            checkIfRestrictionApplies(m_restrictions.allowedStates);
            const std::string stateList = karabo::data::toString(states);
            if (m_node)
                m_node->setAttribute(KARABO_SCHEMA_ALLOWED_STATES,
                                     karabo::data::fromString<std::string, std::vector>(stateList, ","));

            return *this;
        }

        // overloads for up to six states

        OverwriteElement& OverwriteElement::setNewAllowedStates(const karabo::data::State& s1) {
            const karabo::data::State arr[] = {s1};
            return setNewAllowedStates(std::vector<karabo::data::State>(arr, arr + 1));
        }

        OverwriteElement& OverwriteElement::setNewAllowedStates(const karabo::data::State& s1,
                                                                const karabo::data::State& s2) {
            const karabo::data::State arr[] = {s1, s2};
            return setNewAllowedStates(std::vector<karabo::data::State>(arr, arr + 2));
        }

        OverwriteElement& OverwriteElement::setNewAllowedStates(const karabo::data::State& s1,
                                                                const karabo::data::State& s2,
                                                                const karabo::data::State& s3) {
            const karabo::data::State arr[] = {s1, s2, s3};
            return setNewAllowedStates(std::vector<karabo::data::State>(arr, arr + 3));
        }

        OverwriteElement& OverwriteElement::setNewAllowedStates(const karabo::data::State& s1,
                                                                const karabo::data::State& s2,
                                                                const karabo::data::State& s3,
                                                                const karabo::data::State& s4) {
            const karabo::data::State arr[] = {s1, s2, s3, s4};
            return setNewAllowedStates(std::vector<karabo::data::State>(arr, arr + 4));
        }

        OverwriteElement& OverwriteElement::setNewAllowedStates(const karabo::data::State& s1,
                                                                const karabo::data::State& s2,
                                                                const karabo::data::State& s3,
                                                                const karabo::data::State& s4,
                                                                const karabo::data::State& s5) {
            const karabo::data::State arr[] = {s1, s2, s3, s4, s5};
            return setNewAllowedStates(std::vector<karabo::data::State>(arr, arr + 5));
        }

        OverwriteElement& OverwriteElement::setNewAllowedStates(
              const karabo::data::State& s1, const karabo::data::State& s2, const karabo::data::State& s3,
              const karabo::data::State& s4, const karabo::data::State& s5, const karabo::data::State& s6) {
            const karabo::data::State arr[] = {s1, s2, s3, s4, s5, s6};
            return setNewAllowedStates(std::vector<karabo::data::State>(arr, arr + 6));
        }

        OverwriteElement& OverwriteElement::setNowObserverAccess() {
            if (!m_node) throw KARABO_LOGIC_EXCEPTION("Please call key(..) before other methods of OverwriteElement");
            checkIfRestrictionApplies(m_restrictions.observerAccess);
            m_node->setAttribute<int>(KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL, Schema::OBSERVER);
            return *this;
        }

        OverwriteElement& OverwriteElement::setNowOperatorAccess() {
            if (!m_node) throw KARABO_LOGIC_EXCEPTION("Please call key(..) before other methods of OverwriteElement");
            checkIfRestrictionApplies(m_restrictions.operatorAccess);
            m_node->setAttribute<int>(KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL, Schema::OPERATOR);
            return *this;
        }

        OverwriteElement& OverwriteElement::setNowExpertAccess() {
            if (!m_node) throw KARABO_LOGIC_EXCEPTION("Please call key(..) before other methods of OverwriteElement");
            checkIfRestrictionApplies(m_restrictions.expertAccess);
            m_node->setAttribute<int>(KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL, Schema::EXPERT);
            return *this;
        }

        OverwriteElement& OverwriteElement::setNewUnit(const Unit& unit) {
            if (!m_node) throw KARABO_LOGIC_EXCEPTION("Please call key(..) before other methods of OverwriteElement");
            checkIfRestrictionApplies(m_restrictions.unit);
            m_node->setAttribute<int>(KARABO_SCHEMA_UNIT_ENUM, static_cast<int>(unit));
            std::pair<std::string, std::string> names = karabo::data::getUnit(unit);
            m_node->setAttribute(KARABO_SCHEMA_UNIT_NAME, names.first);
            m_node->setAttribute(KARABO_SCHEMA_UNIT_SYMBOL, names.second);
            return *this;
        }

        OverwriteElement& OverwriteElement::setNewMetricPrefix(const MetricPrefix& metricPrefix) {
            if (!m_node) throw KARABO_LOGIC_EXCEPTION("Please call key(..) before other methods of OverwriteElement");
            checkIfRestrictionApplies(m_restrictions.metricPrefix);
            m_node->setAttribute<int>(KARABO_SCHEMA_METRIC_PREFIX_ENUM, static_cast<int>(metricPrefix));
            std::pair<std::string, std::string> names = karabo::data::getMetricPrefix(metricPrefix);
            m_node->setAttribute(KARABO_SCHEMA_METRIC_PREFIX_NAME, names.first);
            m_node->setAttribute(KARABO_SCHEMA_METRIC_PREFIX_SYMBOL, names.second);
            return *this;
        }


        OverwriteElement& OverwriteElement::setNewOverwriteRestrictions(OverwriteElement::Restrictions& restrictions) {
            if (!m_node) throw KARABO_LOGIC_EXCEPTION("Please call key(..) before other methods of OverwriteElement");
            checkIfRestrictionApplies(m_restrictions.overwriteRestrictions);
            if (m_node->hasAttribute(KARABO_OVERWRITE_RESTRICTIONS)) {
                OverwriteElement::Restrictions existing;
                existing.assignFromAttrVector(m_node->getAttribute<std::vector<bool>>(KARABO_OVERWRITE_RESTRICTIONS));
                // now merge
                restrictions.merge(existing);
            }
            m_node->setAttribute(KARABO_OVERWRITE_RESTRICTIONS, restrictions.toVectorAttribute());
            return *this;
        }

#define CASE_CHECK_DEFAULT_IN_OPTIONS(RefType, CppType)                                                        \
    case RefType: {                                                                                            \
        const std::vector<CppType>& options = m_schema->getOptions<CppType>(m_path);                           \
        const auto it = std::find(options.begin(), options.end(), m_schema->getDefaultValue<CppType>(m_path)); \
        if (it == options.end()) {                                                                             \
            std::string defAsStr(m_schema->getDefaultValueAs<std::string>(m_path));                            \
            throw KARABO_LOGIC_EXCEPTION("Default value for " + m_path + " (i.e. '" + defAsStr +=              \
                                         "') not in options: " + karabo::data::toString(options));             \
        }                                                                                                      \
        break;                                                                                                 \
    }


        void OverwriteElement::checkOptions() {
            // Checks consistency of default value and options (caveat: not only for things changed...)
            if (m_schema->hasOptions(m_path) && m_schema->hasDefaultValue(m_path)) {
                switch (m_schema->getValueType(m_path)) {
                    CASE_CHECK_DEFAULT_IN_OPTIONS(Types::BOOL, bool);
                    CASE_CHECK_DEFAULT_IN_OPTIONS(Types::CHAR, char);
                    CASE_CHECK_DEFAULT_IN_OPTIONS(Types::INT8, signed char);
                    CASE_CHECK_DEFAULT_IN_OPTIONS(Types::INT16, short);
                    CASE_CHECK_DEFAULT_IN_OPTIONS(Types::INT32, int);
                    CASE_CHECK_DEFAULT_IN_OPTIONS(Types::INT64, long long);
                    CASE_CHECK_DEFAULT_IN_OPTIONS(Types::UINT8, unsigned char);
                    CASE_CHECK_DEFAULT_IN_OPTIONS(Types::UINT16, unsigned short);
                    CASE_CHECK_DEFAULT_IN_OPTIONS(Types::UINT32, unsigned int);
                    CASE_CHECK_DEFAULT_IN_OPTIONS(Types::UINT64, unsigned long long);
                    CASE_CHECK_DEFAULT_IN_OPTIONS(Types::FLOAT, float);
                    CASE_CHECK_DEFAULT_IN_OPTIONS(Types::DOUBLE, double);
                    CASE_CHECK_DEFAULT_IN_OPTIONS(Types::STRING,
                                                  std::string); // Covers also StateElement
                    default:
                        break; // no options for other types like vectors, etc.
                }
            }
        }
#undef CASE_CHECK_DEFAULT_IN_OPTIONS

        void OverwriteElement::checkBoundaries() {
            switch (m_schema->getValueType(m_path)) {
                case Types::CHAR:
                    checkTypedBoundaries<char>();
                    break;
                case Types::INT8:
                    checkTypedBoundaries<signed char>();
                    break;
                case Types::INT16:
                    checkTypedBoundaries<short>();
                    break;
                case Types::INT32:
                    checkTypedBoundaries<int>();
                    break;
                case Types::INT64:
                    checkTypedBoundaries<long long>();
                    break;
                case Types::UINT8:
                    checkTypedBoundaries<unsigned char>();
                    break;
                case Types::UINT16:
                    checkTypedBoundaries<unsigned short>();
                    break;
                case Types::UINT32:
                    checkTypedBoundaries<unsigned int>();
                    break;
                case Types::UINT64:
                    checkTypedBoundaries<unsigned long long>();
                    break;
                case Types::FLOAT:
                    checkTypedBoundaries<float>();
                    break;
                case Types::DOUBLE:
                    checkTypedBoundaries<double>();
                    break;

                    // Vector types

                case Types::VECTOR_BOOL:
                    checkVectorBoundaries<bool>();
                    break;
                case Types::VECTOR_CHAR:
                    checkVectorBoundaries<char>();
                    break;
                case Types::VECTOR_INT8:
                    checkVectorBoundaries<signed char>();
                    break;
                case Types::VECTOR_UINT8:
                    checkVectorBoundaries<unsigned char>();
                    break;
                case Types::VECTOR_INT16:
                    checkVectorBoundaries<short int>();
                    break;
                case Types::VECTOR_UINT16:
                    checkVectorBoundaries<unsigned short int>();
                    break;
                case Types::VECTOR_INT32:
                    checkVectorBoundaries<int>();
                    break;
                case Types::VECTOR_UINT32:
                    checkVectorBoundaries<unsigned int>();
                    break;
                case Types::VECTOR_INT64:
                    checkVectorBoundaries<long long int>();
                    break;
                case Types::VECTOR_UINT64:
                    checkVectorBoundaries<unsigned long long int>();
                    break;
                case Types::VECTOR_FLOAT:
                    checkVectorBoundaries<float>();
                    break;
                case Types::VECTOR_DOUBLE:
                    checkVectorBoundaries<double>();
                    break;
                case Types::VECTOR_STRING:
                    checkVectorBoundaries<std::string>();
                    break;
                case Types::VECTOR_HASH:
                    checkVectorBoundaries<Hash>();
                    break;
                default:
                    // Avoid compilation warnings
                    break;
            }
        }

        void OverwriteElement::commit() {
            if (!m_node) throw KARABO_LOGIC_EXCEPTION("Please call key(..) before other methods of OverwriteElement");
            if (!m_schema->isLeaf(m_path)) return;

            checkOptions();
            checkBoundaries();
        }

        void OverwriteElement::checkIfRestrictionApplies(const Restrictions::Restriction& restriction) const {
            if (!m_node) throw KARABO_LOGIC_EXCEPTION("Please call key(..) before other methods of OverwriteElement");
            if (restriction == true) {
                const std::string& key = m_node->getKey();
                const std::string& name = restriction.name;
                const std::string& msg = "Element (" + key + ") does not allow overwriting attribute " + name + "!";
                throw KARABO_LOGIC_EXCEPTION(msg);
            }
        }

        OverwriteElement& OverwriteElement::setNewOptions(const std::string& opts, bool protect,
                                                          const std::string& sep) {
            if (protect) checkIfRestrictionApplies(m_restrictions.options); // only protect if set from outside.
            if (!m_path.empty()) m_schema->setOptions(m_path, opts, sep);
            return *this;
        }


    } // namespace data
} // namespace karabo
