/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on September 23, 2011, 11:12 AM
 *
 * Major re-design on February 1, 2013, 1:00 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_UTIL_OVERWRITEELEMENT_HH
#define	KARABO_UTIL_OVERWRITEELEMENT_HH

#include "Schema.hh"


namespace karabo {
    namespace util {

        class OverwriteElement {

            Schema* m_schema;
            Hash::Node* m_node;

        public:

            OverwriteElement(Schema& expected) : m_schema(&expected) {
            }

            /**
             * Specify the key to be overwritten
             * @param name unique key name
             * @return  reference to the Element
             */
            OverwriteElement& key(std::string const& name) {
                boost::optional<Hash::Node&> node = m_schema->getParameterHash().find(name);
                if (node) { // exists
                    m_node = node.get_ptr();
                } else {
                    // Could be, the parameter is assembled under different rules, we should silently ignore this then.
                    m_node = 0;
                    //throw KARABO_PARAMETER_EXCEPTION("Key \"" + name + "\" was not set before, thus can not be overwritten.");
                }
                return *this;
            }

            OverwriteElement& setNewDisplayedName(const std::string& name) {
                if (m_node) m_node->setAttribute(KARABO_SCHEMA_DISPLAYED_NAME, name);
                return *this;
            }

            OverwriteElement& setNewDescription(const std::string& description) {
                if (m_node) m_node->setAttribute(KARABO_SCHEMA_DESCRIPTION, description);
                return *this;
            }

            template <class AliasType>
            OverwriteElement& setNewAlias(const AliasType& alias) {
                if (m_node) m_node->setAttribute<AliasType > (KARABO_SCHEMA_ALIAS, alias);
                return *this;
            }

            template <class TagType>
            OverwriteElement& setNewTag(const TagType& tag) {
                if (m_node) m_node->setAttribute<TagType > ("tag", tag);
                return *this;
            }

            OverwriteElement& setNewAssignmentMandatory() {

                if (m_node) m_node->setAttribute<int>(KARABO_SCHEMA_ASSIGNMENT, Schema::MANDATORY_PARAM);
                return *this;
            }

            OverwriteElement& setNewAssignmentOptional() {

                if (m_node) m_node->setAttribute<int>(KARABO_SCHEMA_ASSIGNMENT, Schema::OPTIONAL_PARAM);
                return *this;
            }

            OverwriteElement& setNewAssignmentInternal() {

                if (m_node) m_node->setAttribute<int>(KARABO_SCHEMA_ASSIGNMENT, Schema::INTERNAL_PARAM);
                return *this;
            }

            OverwriteElement& setNowInit() {

                if (m_node) m_node->setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, INIT);
                return *this;
            }

            OverwriteElement& setNowReconfigurable() {

                if (m_node) m_node->setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, WRITE);
                return *this;
            }

            OverwriteElement& setNowReadOnly() {

                if (m_node) m_node->setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, READ);
                return *this;
            }

            template <class ValueType>
            OverwriteElement& setNewDefaultValue(const ValueType& value) {
                if (m_node) m_node->setAttribute(KARABO_SCHEMA_DEFAULT_VALUE, value);
                return *this;
            }

            template <class ValueType>
            OverwriteElement& setNewMinInc(const ValueType& value) {

                if (m_node) m_node->setAttribute(KARABO_SCHEMA_MIN_INC, value);
                return *this;
            }

            template <class ValueType>
            OverwriteElement& setNewMaxInc(const ValueType& value) {

                if (m_node) m_node->setAttribute(KARABO_SCHEMA_MAX_INC, value);
                return *this;
            }

            template <class ValueType>
            OverwriteElement& setNewMinExc(const ValueType& value) {

                if (m_node) m_node->setAttribute(KARABO_SCHEMA_MIN_EXC, value);
                return *this;
            }

            template <class ValueType>
            OverwriteElement& setNewMaxExc(const ValueType& value) {

                if (m_node) m_node->setAttribute(KARABO_SCHEMA_MAX_EXC, value);
                return *this;
            }

            OverwriteElement& setNewOptions(const std::string& opts, const std::string& sep = " ,;") {

                if (m_node) m_node->setAttribute(KARABO_SCHEMA_OPTIONS, karabo::util::fromString<std::string, std::vector > (opts, sep));
                return *this;
            }

            OverwriteElement& setNewOptions(const std::vector<karabo::util::State>& opts) {
                return setNewOptions(toString(opts), ",");
            }

            OverwriteElement& setNewOptions(const karabo::util::State& s1) {
                const karabo::util::State arr[] = {s1};
                return setNewOptions(std::vector<karabo::util::State>(arr, arr + 1));
            }

            OverwriteElement& setNewOptions(const karabo::util::State& s1, const karabo::util::State& s2) {
                const karabo::util::State arr[] = {s1, s2};
                return setNewOptions(std::vector<karabo::util::State>(arr, arr + 2));
            }

            OverwriteElement& setNewOptions(const karabo::util::State& s1, const karabo::util::State& s2, const karabo::util::State& s3) {
                const karabo::util::State arr[] = {s1, s2, s3};
                return setNewOptions(std::vector<karabo::util::State>(arr, arr + 3));
            }

            OverwriteElement& setNewOptions(const karabo::util::State& s1, const karabo::util::State& s2, const karabo::util::State& s3, const karabo::util::State& s4) {
                const karabo::util::State arr[] = {s1, s2, s3, s4};
                return setNewOptions(std::vector<karabo::util::State>(arr, arr + 4));
            }

            OverwriteElement& setNewOptions(const karabo::util::State& s1, const karabo::util::State& s2, const karabo::util::State& s3, const karabo::util::State& s4, const karabo::util::State& s5) {
                const karabo::util::State arr[] = {s1, s2, s3, s4, s5};
                return setNewOptions(std::vector<karabo::util::State>(arr, arr + 5));
            }

            OverwriteElement& setNewOptions(const karabo::util::State& s1, const karabo::util::State& s2, const karabo::util::State& s3, const karabo::util::State& s4, const karabo::util::State& s5, const karabo::util::State& s6) {
                const karabo::util::State arr[] = {s1, s2, s3, s4, s5, s6};
                return setNewOptions(std::vector<karabo::util::State>(arr, arr + 6));
            }

            OverwriteElement& setNewOptions(const karabo::util::State& s1, const karabo::util::State& s2, const karabo::util::State& s3, const karabo::util::State& s4, const karabo::util::State& s5, const karabo::util::State& s6, const karabo::util::State& s7) {
                const karabo::util::State arr[] = {s1, s2, s3, s4, s5, s6, s7};
                return setNewOptions(std::vector<karabo::util::State>(arr, arr + 7));
            }

            OverwriteElement& setNewOptions(const karabo::util::State& s1, const karabo::util::State& s2, const karabo::util::State& s3, const karabo::util::State& s4, const karabo::util::State& s5, const karabo::util::State& s6, const karabo::util::State& s7, const karabo::util::State& s8) {
                const karabo::util::State arr[] = {s1, s2, s3, s4, s5, s6, s7, s8};
                return setNewOptions(std::vector<karabo::util::State>(arr, arr + 8));
            }

            OverwriteElement& setNewOptions(const std::vector<std::string>& opts) {

                if (m_node) m_node->setAttribute(KARABO_SCHEMA_OPTIONS, opts);
                return *this;
            }

            OverwriteElement& setNewAllowedStates(const std::vector<karabo::util::State>& states) {

                const std::string stateList = karabo::util::toString(states);
                if (m_node) m_node->setAttribute(KARABO_SCHEMA_ALLOWED_STATES, karabo::util::fromString<std::string, std::vector > (stateList, ","));

                return *this;
            }

            //overloads for up to six states

            OverwriteElement& setNewAllowedStates(const karabo::util::State& s1) {
                const karabo::util::State arr[] = {s1};
                return setNewAllowedStates(std::vector<karabo::util::State>(arr, arr + 1));
            }

            OverwriteElement& setNewAllowedStates(const karabo::util::State& s1, const karabo::util::State& s2) {
                const karabo::util::State arr[] = {s1, s2};
                return setNewAllowedStates(std::vector<karabo::util::State>(arr, arr + 2));
            }

            OverwriteElement& setNewAllowedStates(const karabo::util::State& s1, const karabo::util::State& s2, const karabo::util::State& s3) {
                const karabo::util::State arr[] = {s1, s2, s3};
                return setNewAllowedStates(std::vector<karabo::util::State>(arr, arr + 3));
            }

            OverwriteElement& setNewAllowedStates(const karabo::util::State& s1, const karabo::util::State& s2, const karabo::util::State& s3, const karabo::util::State& s4) {
                const karabo::util::State arr[] = {s1, s2, s3, s4};
                return setNewAllowedStates(std::vector<karabo::util::State>(arr, arr + 4));
            }

            OverwriteElement& setNewAllowedStates(const karabo::util::State& s1, const karabo::util::State& s2, const karabo::util::State& s3, const karabo::util::State& s4, const karabo::util::State& s5) {
                const karabo::util::State arr[] = {s1, s2, s3, s4, s5};
                return setNewAllowedStates(std::vector<karabo::util::State>(arr, arr + 5));
            }

            OverwriteElement& setNewAllowedStates(const karabo::util::State& s1, const karabo::util::State& s2, const karabo::util::State& s3, const karabo::util::State& s4, const karabo::util::State& s5, const karabo::util::State& s6) {
                const karabo::util::State arr[] = {s1, s2, s3, s4, s5, s6};
                return setNewAllowedStates(std::vector<karabo::util::State>(arr, arr + 6));
            }

            OverwriteElement& setNowObserverAccess() {

                if (m_node) m_node->setAttribute<int>(KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL, Schema::OBSERVER);
                return *this;
            }

            OverwriteElement& setNowUserAccess() {

                if (m_node) m_node->setAttribute<int>(KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL, Schema::USER);
                return *this;
            }

            OverwriteElement& setNowOperatorAccess() {

                if (m_node) m_node->setAttribute<int>(KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL, Schema::OPERATOR);
                return *this;
            }

            OverwriteElement& setNowExpertAccess() {

                if (m_node) m_node->setAttribute<int>(KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL, Schema::EXPERT);
                return *this;
            }

            OverwriteElement& setNowAdminAccess() {

                if (m_node) m_node->setAttribute<int>(KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL, Schema::ADMIN);
                return *this;
            }

            OverwriteElement& setNewUnit(const UnitType& unit) {

                if (m_node) m_node->setAttribute<int>(KARABO_SCHEMA_UNIT_ENUM, unit);
                std::pair<std::string, std::string> names = karabo::util::getUnit(unit);
                if (m_node) m_node->setAttribute(KARABO_SCHEMA_UNIT_NAME, names.first);
                if (m_node) m_node->setAttribute(KARABO_SCHEMA_UNIT_SYMBOL, names.second);
                return *this;
            }

            OverwriteElement& setNewMetricPrefix(const MetricPrefixType& metricPrefix) {

                if (m_node) m_node->setAttribute<int>(KARABO_SCHEMA_METRIC_PREFIX_ENUM, metricPrefix);
                std::pair<std::string, std::string> names = karabo::util::getMetricPrefix(metricPrefix);
                if (m_node) m_node->setAttribute(KARABO_SCHEMA_METRIC_PREFIX_NAME, names.first);
                if (m_node) m_node->setAttribute(KARABO_SCHEMA_METRIC_PREFIX_SYMBOL, names.second);
                return *this;
            }

            /**
             * The <b>commit</b> method injects the element to the expected parameters list. If not called
             * the element is not usable. This must be called after the element is fully defined.
             * @return reference to the GenericElement
             */
            void commit() {
                // Does nothing, changes happened on existing node
            }



        };

        template <>
        inline OverwriteElement& OverwriteElement::setNewDefaultValue<karabo::util::State>(const karabo::util::State& value) {
            return setNewDefaultValue(toString(value));
        }


        typedef OverwriteElement OVERWRITE_ELEMENT;
    }
}
#endif
