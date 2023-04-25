/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
#include "OverwriteElement.hh"

namespace karabo {
    namespace util {
        OverwriteElement::OverwriteElement(Schema& expected) : m_schema(&expected), m_path("") {}


        OverwriteElement& OverwriteElement::key(std::string const& name) {
            m_path = name;
            boost::optional<Hash::Node&> node = m_schema->getParameterHash().find(name);
            if (node) { // exists
                m_node = node.get_ptr();
                if (node->hasAttribute(KARABO_OVERWRITE_RESTRICTIONS)) {
                    m_restrictions.assignFromAttrVector(
                          node->getAttribute<std::vector<bool> >(KARABO_OVERWRITE_RESTRICTIONS));
                }
            } else {
                // Could be, the parameter is assembled under different rules, we should silently ignore this then.
                m_node = 0;
                // throw KARABO_PARAMETER_EXCEPTION("Key \"" + name + "\" was not set before, thus can not be
                // overwritten.");
            }
            return *this;
        }

        OverwriteElement& OverwriteElement::setNewDisplayedName(const std::string& name) {
            checkIfRestrictionApplies(m_restrictions.displayedName);
            if (m_node) m_node->setAttribute(KARABO_SCHEMA_DISPLAYED_NAME, name);
            return *this;
        }

        OverwriteElement& OverwriteElement::setNewDescription(const std::string& description) {
            checkIfRestrictionApplies(m_restrictions.description);
            if (m_node) m_node->setAttribute(KARABO_SCHEMA_DESCRIPTION, description);
            return *this;
        }


        OverwriteElement& OverwriteElement::setNewAssignmentMandatory() {
            checkIfRestrictionApplies(m_restrictions.assignmentMandatory);
            if (m_node) m_node->setAttribute<int>(KARABO_SCHEMA_ASSIGNMENT, Schema::MANDATORY_PARAM);
            return *this;
        }

        OverwriteElement& OverwriteElement::setNewAssignmentOptional() {
            checkIfRestrictionApplies(m_restrictions.assignmentOptional);
            if (m_node) m_node->setAttribute<int>(KARABO_SCHEMA_ASSIGNMENT, Schema::OPTIONAL_PARAM);
            return *this;
        }

        OverwriteElement& OverwriteElement::setNewAssignmentInternal() {
            checkIfRestrictionApplies(m_restrictions.assignmentInternal);
            if (m_node) m_node->setAttribute<int>(KARABO_SCHEMA_ASSIGNMENT, Schema::INTERNAL_PARAM);
            return *this;
        }

        OverwriteElement& OverwriteElement::setNowInit() {
            checkIfRestrictionApplies(m_restrictions.init);
            if (m_node) m_node->setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, INIT);
            return *this;
        }

        OverwriteElement& OverwriteElement::setNowReconfigurable() {
            checkIfRestrictionApplies(m_restrictions.reconfigurable);
            if (m_node) m_node->setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, WRITE);
            return *this;
        }

        OverwriteElement& OverwriteElement::setNowReadOnly() {
            checkIfRestrictionApplies(m_restrictions.readOnly);
            if (m_node) m_node->setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, READ);
            return *this;
        }

        OverwriteElement& OverwriteElement::setNowValidate() {
            checkIfRestrictionApplies(m_restrictions.skipValidation);
            if (m_node) m_node->setAttribute<bool>(KARABO_SCHEMA_SKIP_VALIDATION, false);
            return *this;
        }

        OverwriteElement& OverwriteElement::setNowSkipValidation() {
            checkIfRestrictionApplies(m_restrictions.skipValidation);
            if (m_node) m_node->setAttribute<bool>(KARABO_SCHEMA_SKIP_VALIDATION, true);
            return *this;
        }

        OverwriteElement& OverwriteElement::setNewOptions(const std::string& opts, const std::string& sep) {
            return setNewOptions(opts, true, sep);
        }

        OverwriteElement& OverwriteElement::setNewOptions(const std::vector<karabo::util::State>& opts) {
            checkIfRestrictionApplies(m_restrictions.stateOptions);
            return setNewOptions(toString(opts), false, ",");
        }

        OverwriteElement& OverwriteElement::setNewOptions(const karabo::util::State& s1) {
            const karabo::util::State arr[] = {s1};
            return setNewOptions(std::vector<karabo::util::State>(arr, arr + 1));
        }

        OverwriteElement& OverwriteElement::setNewOptions(const karabo::util::State& s1,
                                                          const karabo::util::State& s2) {
            const karabo::util::State arr[] = {s1, s2};
            return setNewOptions(std::vector<karabo::util::State>(arr, arr + 2));
        }

        OverwriteElement& OverwriteElement::setNewOptions(const karabo::util::State& s1, const karabo::util::State& s2,
                                                          const karabo::util::State& s3) {
            const karabo::util::State arr[] = {s1, s2, s3};
            return setNewOptions(std::vector<karabo::util::State>(arr, arr + 3));
        }

        OverwriteElement& OverwriteElement::setNewOptions(const karabo::util::State& s1, const karabo::util::State& s2,
                                                          const karabo::util::State& s3,
                                                          const karabo::util::State& s4) {
            const karabo::util::State arr[] = {s1, s2, s3, s4};
            return setNewOptions(std::vector<karabo::util::State>(arr, arr + 4));
        }

        OverwriteElement& OverwriteElement::setNewOptions(const karabo::util::State& s1, const karabo::util::State& s2,
                                                          const karabo::util::State& s3, const karabo::util::State& s4,
                                                          const karabo::util::State& s5) {
            const karabo::util::State arr[] = {s1, s2, s3, s4, s5};
            return setNewOptions(std::vector<karabo::util::State>(arr, arr + 5));
        }

        OverwriteElement& OverwriteElement::setNewOptions(const karabo::util::State& s1, const karabo::util::State& s2,
                                                          const karabo::util::State& s3, const karabo::util::State& s4,
                                                          const karabo::util::State& s5,
                                                          const karabo::util::State& s6) {
            const karabo::util::State arr[] = {s1, s2, s3, s4, s5, s6};
            return setNewOptions(std::vector<karabo::util::State>(arr, arr + 6));
        }

        OverwriteElement& OverwriteElement::setNewOptions(const karabo::util::State& s1, const karabo::util::State& s2,
                                                          const karabo::util::State& s3, const karabo::util::State& s4,
                                                          const karabo::util::State& s5, const karabo::util::State& s6,
                                                          const karabo::util::State& s7) {
            const karabo::util::State arr[] = {s1, s2, s3, s4, s5, s6, s7};
            return setNewOptions(std::vector<karabo::util::State>(arr, arr + 7));
        }

        OverwriteElement& OverwriteElement::setNewOptions(const karabo::util::State& s1, const karabo::util::State& s2,
                                                          const karabo::util::State& s3, const karabo::util::State& s4,
                                                          const karabo::util::State& s5, const karabo::util::State& s6,
                                                          const karabo::util::State& s7,
                                                          const karabo::util::State& s8) {
            const karabo::util::State arr[] = {s1, s2, s3, s4, s5, s6, s7, s8};
            return setNewOptions(std::vector<karabo::util::State>(arr, arr + 8));
        }

        OverwriteElement& OverwriteElement::setNewOptions(const std::vector<std::string>& opts) {
            checkIfRestrictionApplies(m_restrictions.options);
            if (m_node) m_node->setAttribute(KARABO_SCHEMA_OPTIONS, opts);
            return *this;
        }

        OverwriteElement& OverwriteElement::setNewAllowedStates(const std::vector<karabo::util::State>& states) {
            checkIfRestrictionApplies(m_restrictions.allowedStates);
            const std::string stateList = karabo::util::toString(states);
            if (m_node)
                m_node->setAttribute(KARABO_SCHEMA_ALLOWED_STATES,
                                     karabo::util::fromString<std::string, std::vector>(stateList, ","));

            return *this;
        }

        // overloads for up to six states

        OverwriteElement& OverwriteElement::setNewAllowedStates(const karabo::util::State& s1) {
            const karabo::util::State arr[] = {s1};
            return setNewAllowedStates(std::vector<karabo::util::State>(arr, arr + 1));
        }

        OverwriteElement& OverwriteElement::setNewAllowedStates(const karabo::util::State& s1,
                                                                const karabo::util::State& s2) {
            const karabo::util::State arr[] = {s1, s2};
            return setNewAllowedStates(std::vector<karabo::util::State>(arr, arr + 2));
        }

        OverwriteElement& OverwriteElement::setNewAllowedStates(const karabo::util::State& s1,
                                                                const karabo::util::State& s2,
                                                                const karabo::util::State& s3) {
            const karabo::util::State arr[] = {s1, s2, s3};
            return setNewAllowedStates(std::vector<karabo::util::State>(arr, arr + 3));
        }

        OverwriteElement& OverwriteElement::setNewAllowedStates(const karabo::util::State& s1,
                                                                const karabo::util::State& s2,
                                                                const karabo::util::State& s3,
                                                                const karabo::util::State& s4) {
            const karabo::util::State arr[] = {s1, s2, s3, s4};
            return setNewAllowedStates(std::vector<karabo::util::State>(arr, arr + 4));
        }

        OverwriteElement& OverwriteElement::setNewAllowedStates(const karabo::util::State& s1,
                                                                const karabo::util::State& s2,
                                                                const karabo::util::State& s3,
                                                                const karabo::util::State& s4,
                                                                const karabo::util::State& s5) {
            const karabo::util::State arr[] = {s1, s2, s3, s4, s5};
            return setNewAllowedStates(std::vector<karabo::util::State>(arr, arr + 5));
        }

        OverwriteElement& OverwriteElement::setNewAllowedStates(
              const karabo::util::State& s1, const karabo::util::State& s2, const karabo::util::State& s3,
              const karabo::util::State& s4, const karabo::util::State& s5, const karabo::util::State& s6) {
            const karabo::util::State arr[] = {s1, s2, s3, s4, s5, s6};
            return setNewAllowedStates(std::vector<karabo::util::State>(arr, arr + 6));
        }

        OverwriteElement& OverwriteElement::setNowObserverAccess() {
            checkIfRestrictionApplies(m_restrictions.observerAccess);
            if (m_node) m_node->setAttribute<int>(KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL, Schema::OBSERVER);
            return *this;
        }

        OverwriteElement& OverwriteElement::setNowUserAccess() {
            checkIfRestrictionApplies(m_restrictions.userAccess);
            if (m_node) m_node->setAttribute<int>(KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL, Schema::USER);
            return *this;
        }

        OverwriteElement& OverwriteElement::setNowOperatorAccess() {
            checkIfRestrictionApplies(m_restrictions.operatorAccess);
            if (m_node) m_node->setAttribute<int>(KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL, Schema::OPERATOR);
            return *this;
        }

        OverwriteElement& OverwriteElement::setNowExpertAccess() {
            checkIfRestrictionApplies(m_restrictions.expertAccess);
            if (m_node) m_node->setAttribute<int>(KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL, Schema::EXPERT);
            return *this;
        }

        OverwriteElement& OverwriteElement::setNowAdminAccess() {
            checkIfRestrictionApplies(m_restrictions.adminAccess);
            if (m_node) m_node->setAttribute<int>(KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL, Schema::ADMIN);
            return *this;
        }

        OverwriteElement& OverwriteElement::setNewUnit(const UnitType& unit) {
            checkIfRestrictionApplies(m_restrictions.unit);
            if (m_node) m_node->setAttribute<int>(KARABO_SCHEMA_UNIT_ENUM, unit);
            std::pair<std::string, std::string> names = karabo::util::getUnit(unit);
            if (m_node) m_node->setAttribute(KARABO_SCHEMA_UNIT_NAME, names.first);
            if (m_node) m_node->setAttribute(KARABO_SCHEMA_UNIT_SYMBOL, names.second);
            return *this;
        }

        OverwriteElement& OverwriteElement::setNewMetricPrefix(const MetricPrefixType& metricPrefix) {
            checkIfRestrictionApplies(m_restrictions.metricPrefix);
            if (m_node) m_node->setAttribute<int>(KARABO_SCHEMA_METRIC_PREFIX_ENUM, metricPrefix);
            std::pair<std::string, std::string> names = karabo::util::getMetricPrefix(metricPrefix);
            if (m_node) m_node->setAttribute(KARABO_SCHEMA_METRIC_PREFIX_NAME, names.first);
            if (m_node) m_node->setAttribute(KARABO_SCHEMA_METRIC_PREFIX_SYMBOL, names.second);
            return *this;
        }


        OverwriteElement& OverwriteElement::setNewOverwriteRestrictions(OverwriteElement::Restrictions& restrictions) {
            checkIfRestrictionApplies(m_restrictions.overwriteRestrictions);
            if (m_node) {
                if (m_node->hasAttribute(KARABO_OVERWRITE_RESTRICTIONS)) {
                    OverwriteElement::Restrictions existing;
                    existing.assignFromAttrVector(
                          m_node->getAttribute<std::vector<bool> >(KARABO_OVERWRITE_RESTRICTIONS));
                    // now merge
                    restrictions.merge(existing);
                }
                m_node->setAttribute(KARABO_OVERWRITE_RESTRICTIONS, restrictions.toVectorAttribute());
            }
            return *this;
        }

        void OverwriteElement::commit() {
            // Does nothing, changes happened on existing node
        }


        void OverwriteElement::checkIfRestrictionApplies(const Restrictions::Restriction& restriction) const {
            if (restriction == true && m_node) {
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


    } // namespace util
} // namespace karabo
