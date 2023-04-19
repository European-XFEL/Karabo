/**
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */

#include "StateElement.hh"

#include "Schema.hh"
#include "State.hh"

namespace karabo {
    namespace util {


        StateElement::StateElement(Schema& expected) : GenericElement<StateElement>(expected) {
            // if no initial value is set the state will be unknown
            m_node->setAttribute(KARABO_SCHEMA_DEFAULT_VALUE, State::UNKNOWN.name());
            // set the default DAQ policy
            m_node->setAttribute<int>(KARABO_SCHEMA_DAQ_POLICY, expected.getDefaultDAQPolicy());
        }


        StateElement& StateElement::options(const karabo::util::State& s1) {
            const karabo::util::State arr[] = {s1};
            return options(std::vector<karabo::util::State>(arr, arr + 1));
        }


        StateElement& StateElement::options(const karabo::util::State& s1, const karabo::util::State& s2) {
            const karabo::util::State arr[] = {s1, s2};
            return options(std::vector<karabo::util::State>(arr, arr + 2));
        }


        StateElement& StateElement::options(const karabo::util::State& s1, const karabo::util::State& s2,
                                            const karabo::util::State& s3) {
            const karabo::util::State arr[] = {s1, s2, s3};
            return options(std::vector<karabo::util::State>(arr, arr + 3));
        }


        StateElement& StateElement::options(const karabo::util::State& s1, const karabo::util::State& s2,
                                            const karabo::util::State& s3, const karabo::util::State& s4) {
            const karabo::util::State arr[] = {s1, s2, s3, s4};
            return options(std::vector<karabo::util::State>(arr, arr + 4));
        }


        StateElement& StateElement::options(const karabo::util::State& s1, const karabo::util::State& s2,
                                            const karabo::util::State& s3, const karabo::util::State& s4,
                                            const karabo::util::State& s5) {
            const karabo::util::State arr[] = {s1, s2, s3, s4, s5};
            return options(std::vector<karabo::util::State>(arr, arr + 5));
        }


        StateElement& StateElement::options(const karabo::util::State& s1, const karabo::util::State& s2,
                                            const karabo::util::State& s3, const karabo::util::State& s4,
                                            const karabo::util::State& s5, const karabo::util::State& s6) {
            const karabo::util::State arr[] = {s1, s2, s3, s4, s5, s6};
            return options(std::vector<karabo::util::State>(arr, arr + 6));
        }


        StateElement& StateElement::options(const karabo::util::State& s1, const karabo::util::State& s2,
                                            const karabo::util::State& s3, const karabo::util::State& s4,
                                            const karabo::util::State& s5, const karabo::util::State& s6,
                                            const karabo::util::State& s7) {
            const karabo::util::State arr[] = {s1, s2, s3, s4, s5, s6, s7};
            return options(std::vector<karabo::util::State>(arr, arr + 7));
        }


        StateElement& StateElement::options(const karabo::util::State& s1, const karabo::util::State& s2,
                                            const karabo::util::State& s3, const karabo::util::State& s4,
                                            const karabo::util::State& s5, const karabo::util::State& s6,
                                            const karabo::util::State& s7, const karabo::util::State& s8) {
            const karabo::util::State arr[] = {s1, s2, s3, s4, s5, s6, s7, s8};
            return options(std::vector<karabo::util::State>(arr, arr + 8));
        }


        StateElement& StateElement::options(const std::vector<karabo::util::State>& opts) {
            return options(toString(opts), ",");
        }


        StateElement& StateElement::initialValue(const karabo::util::State& s) {
            m_node->setAttribute(KARABO_SCHEMA_DEFAULT_VALUE, s.name());
            return *this;
        }


        StateElement& StateElement::daqPolicy(const DAQPolicy& policy) {
            m_node->setAttribute<int>(KARABO_SCHEMA_DAQ_POLICY, policy);
            return *this;
        }


        void StateElement::beforeAddition() {
            m_node->setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, Schema::LEAF);
            m_node->setAttribute<int>(KARABO_SCHEMA_LEAF_TYPE, Schema::STATE);
            m_node->setAttribute<std::string>(KARABO_SCHEMA_VALUE_TYPE, ToLiteral::to<Types::STRING>());
            m_node->setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, READ);
            m_node->setAttribute<int>(KARABO_SCHEMA_ASSIGNMENT, Schema::OPTIONAL_PARAM);
            m_node->setAttribute<int>(KARABO_SCHEMA_ARCHIVE_POLICY, Schema::EVERY_EVENT);
            m_node->setAttribute<std::string>(KARABO_SCHEMA_CLASS_ID, "State");
            m_node->setAttribute<std::string>(KARABO_SCHEMA_DISPLAY_TYPE, "State");

            // finally protect setting options etc to state element via overwrite
            OverwriteElement::Restrictions restrictions;
            restrictions.options = true;
            restrictions.minInc = true;
            restrictions.minExc = true;
            restrictions.maxInc = true;
            restrictions.maxExc = true;
            restrictions.readOnly = true;
            restrictions.reconfigurable = true;
            restrictions.displayedName = true;
            restrictions.overwriteRestrictions = true;
            restrictions.stateOptions = false; // set to true by default
            m_node->setAttribute(KARABO_OVERWRITE_RESTRICTIONS, restrictions.toVectorAttribute());
        }


        StateElement& StateElement::options(const std::string& opts, const std::string& sep) {
            m_node->setAttribute(KARABO_SCHEMA_OPTIONS, karabo::util::fromString<std::string, std::vector>(opts, sep));
            return *this;
        }


        StateElement& StateElement::options(const std::vector<std::string>& opts) {
            m_node->setAttribute(KARABO_SCHEMA_OPTIONS, opts);
            return *this;
        }

    } // namespace util
} // namespace karabo