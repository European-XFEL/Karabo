/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on July 1, 2011, 2:23 PM
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


#ifndef KARABO_UTIL_SLOTELEMENT_HH
#define KARABO_UTIL_SLOTELEMENT_HH

#include "karabo/data/schema/GenericElement.hh"
#include "karabo/data/types/Exception.hh"
#include "karabo/data/types/Schema.hh"
#include "karabo/data/types/State.hh"
#include "karabo/data/types/ToLiteral.hh"
// OK to include from 'karabo/log' (for KARABO_LOG_FRAMEWORK_WARN) as long as this file does not move to 'karabo/util':
#include "karabo/log/Logger.hh"

namespace karabo {
    namespace xms {

        template <class Derived>
        class SlotElementBase : public karabo::data::GenericElement<Derived> {
           protected:
            karabo::data::Hash m_child;

           public:
            SlotElementBase(karabo::data::Schema& expected) : karabo::data::GenericElement<Derived>(expected) {
                using namespace karabo::data;
                this->m_node->template setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, WRITE);
#if __GNUC__ >= 12
                this->m_node->template setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, static_cast<int>(Schema::NODE));
#else
                constexpr int schemaNode = static_cast<int>(Schema::NODE);
                this->m_node->template setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, schemaNode);
#endif
                this->m_node->setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, "Slot"); // Reserved displayType for commands
                this->m_node->setAttribute(KARABO_SCHEMA_CLASS_ID, "Slot");

                // default value of requiredAccessLevel for Slot element: OPERATOR
#if __GNUC__ >= 12
                this->m_node->template setAttribute<int>(KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL,
                                                         static_cast<int>(Schema::OPERATOR));
#else
                constexpr int schemaOperator = static_cast<int>(Schema::OPERATOR);
                this->m_node->template setAttribute<int>(KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL, schemaOperator);
#endif
            }

            /**
             * The <b>allowedStates</b> method serves for setting up allowed states for the element
             * @param states A string describing list of possible states.
             * @param sep A separator symbol used for parsing previous argument for list of states
             * @return reference to the Element (to allow method's chaining)
             */

            Derived& allowedStates(const std::vector<karabo::data::State>& value) {
                const std::string stateString = karabo::data::toString(value);
                this->m_node->setAttribute(KARABO_SCHEMA_ALLOWED_STATES,
                                           karabo::data::fromString<std::string, std::vector>(stateString, ","));
                return *(static_cast<Derived*>(this));
            }

            // overloads for up to six elements
            Derived& allowedStates(const karabo::data::State& s1) {
                const karabo::data::State arr[] = {s1};
                return allowedStates(std::vector<karabo::data::State>(arr, arr + 1));
            }

            Derived& allowedStates(const karabo::data::State& s1, const karabo::data::State& s2) {
                const karabo::data::State arr[] = {s1, s2};
                return allowedStates(std::vector<karabo::data::State>(arr, arr + 2));
            }

            Derived& allowedStates(const karabo::data::State& s1, const karabo::data::State& s2,
                                   const karabo::data::State& s3) {
                const karabo::data::State arr[] = {s1, s2, s3};
                return allowedStates(std::vector<karabo::data::State>(arr, arr + 3));
            }

            Derived& allowedStates(const karabo::data::State& s1, const karabo::data::State& s2,
                                   const karabo::data::State& s3, const karabo::data::State& s4) {
                const karabo::data::State arr[] = {s1, s2, s3, s4};
                return allowedStates(std::vector<karabo::data::State>(arr, arr + 4));
            }

            Derived& allowedStates(const karabo::data::State& s1, const karabo::data::State& s2,
                                   const karabo::data::State& s3, const karabo::data::State& s4,
                                   const karabo::data::State& s5) {
                const karabo::data::State arr[] = {s1, s2, s3, s4, s5};
                return allowedStates(std::vector<karabo::data::State>(arr, arr + 5));
            }

            Derived& allowedStates(const karabo::data::State& s1, const karabo::data::State& s2,
                                   const karabo::data::State& s3, const karabo::data::State& s4,
                                   const karabo::data::State& s5, const karabo::data::State& s6) {
                const karabo::data::State arr[] = {s1, s2, s3, s4, s5, s6};
                return allowedStates(std::vector<karabo::data::State>(arr, arr + 6));
            }

            virtual void beforeAddition() override = 0;

            Derived& key(const std::string& name) override {
                if (name.find('_') != std::string::npos) {
                    if (name == "clear_namespace") {
                        throw KARABO_PARAMETER_EXCEPTION(
                              "Slot 'clear_namespace' prohibited since reserved got internal usage in GUI client.");
                    }
                    KARABO_LOG_FRAMEWORK_WARN_C("SlotElementBase")
                          << "Slot '" << name
                          << "' contains a '_'. This might lead to unexpected behaviour since the `_` is internally "
                             "used for slots inside a nodes";
                }
                return karabo::data::GenericElement<Derived>::key(name);
            }
        };

        class SLOT_ELEMENT : public SlotElementBase<SLOT_ELEMENT> {
           public:
            SLOT_ELEMENT(karabo::data::Schema& expected) : SlotElementBase<SLOT_ELEMENT>(expected) {}

            void beforeAddition() {
                this->m_node->setValue(this->m_child);
            }
        };

        template <class A1>
        class SLOT_ELEMENT1 : public SlotElementBase<SLOT_ELEMENT1<A1> > {
           public:
            SLOT_ELEMENT1(karabo::data::Schema& expected) : SlotElementBase<SLOT_ELEMENT1<A1> >(expected) {
                this->m_child.set("arg1", 0);
                this->m_child.setAttribute("arg1", KARABO_SCHEMA_DISPLAYED_NAME, "Argument 1");
                this->m_child.template setAttribute<int>("arg1", KARABO_SCHEMA_ACCESS_MODE, karabo::data::WRITE);
#if __GNUC__ >= 12
                this->m_child.template setAttribute<int>("arg1", KARABO_SCHEMA_ASSIGNMENT,
                                                         static_cast<int>(karabo::data::Schema::MANDATORY_PARAM));
#else
                constexpr int schemaMandatory = static_cast<int>(karabo::data::Schema::MANDATORY_PARAM);
                this->m_child.template setAttribute<int>("arg1", KARABO_SCHEMA_ASSIGNMENT, schemaMandatory);
#endif
            }

            SLOT_ELEMENT1& arg1Description(const std::string& desc) {
                this->m_child.setAttribute("arg1", KARABO_SCHEMA_DESCRIPTION, desc);
                return *this;
            }

            SLOT_ELEMENT1& arg1DisplayName(const std::string& name) {
                this->m_child.setAttribute("arg1", KARABO_SCHEMA_DISPLAYED_NAME, name);
                return *this;
            }

            void beforeAddition() {
                this->m_node->setValue(this->m_child);
            }
        };
    } // namespace xms
} // namespace karabo

#endif
