/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on July 1, 2011, 2:23 PM
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */


#ifndef KARABO_UTIL_SLOTELEMENT_HH
#define KARABO_UTIL_SLOTELEMENT_HH

#include "karabo/util/Exception.hh"
#include "karabo/util/GenericElement.hh"
#include "karabo/util/State.hh"
#include "karabo/util/ToLiteral.hh"
// OK to include from 'karabo/log' (for KARABO_LOG_FRAMEWORK_WARN) as long as this file does not move to 'karabo/util':
#include "karabo/log/Logger.hh"

namespace karabo {
    namespace xms {

        template <class Derived>
        class SlotElementBase : public karabo::util::GenericElement<Derived> {
           protected:
            karabo::util::Hash m_child;

           public:
            SlotElementBase(karabo::util::Schema& expected) : karabo::util::GenericElement<Derived>(expected) {
                this->m_node->template setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, karabo::util::WRITE);
                this->m_node->template setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, karabo::util::Schema::NODE);
                this->m_node->setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, "Slot"); // Reserved displayType for commands
                this->m_node->setAttribute(KARABO_SCHEMA_CLASS_ID, "Slot");

                // default value of requiredAccessLevel for Slot element: USER
                this->m_node->template setAttribute<int>(KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL,
                                                         karabo::util::Schema::USER);
            }

            /**
             * The <b>allowedStates</b> method serves for setting up allowed states for the element
             * @param states A string describing list of possible states.
             * @param sep A separator symbol used for parsing previous argument for list of states
             * @return reference to the Element (to allow method's chaining)
             */

            Derived& allowedStates(const std::vector<karabo::util::State>& value) {
                const std::string stateString = karabo::util::toString(value);
                this->m_node->setAttribute(KARABO_SCHEMA_ALLOWED_STATES,
                                           karabo::util::fromString<std::string, std::vector>(stateString, ","));
                return *(static_cast<Derived*>(this));
            }

            // overloads for up to six elements
            Derived& allowedStates(const karabo::util::State& s1) {
                const karabo::util::State arr[] = {s1};
                return allowedStates(std::vector<karabo::util::State>(arr, arr + 1));
            }

            Derived& allowedStates(const karabo::util::State& s1, const karabo::util::State& s2) {
                const karabo::util::State arr[] = {s1, s2};
                return allowedStates(std::vector<karabo::util::State>(arr, arr + 2));
            }

            Derived& allowedStates(const karabo::util::State& s1, const karabo::util::State& s2,
                                   const karabo::util::State& s3) {
                const karabo::util::State arr[] = {s1, s2, s3};
                return allowedStates(std::vector<karabo::util::State>(arr, arr + 3));
            }

            Derived& allowedStates(const karabo::util::State& s1, const karabo::util::State& s2,
                                   const karabo::util::State& s3, const karabo::util::State& s4) {
                const karabo::util::State arr[] = {s1, s2, s3, s4};
                return allowedStates(std::vector<karabo::util::State>(arr, arr + 4));
            }

            Derived& allowedStates(const karabo::util::State& s1, const karabo::util::State& s2,
                                   const karabo::util::State& s3, const karabo::util::State& s4,
                                   const karabo::util::State& s5) {
                const karabo::util::State arr[] = {s1, s2, s3, s4, s5};
                return allowedStates(std::vector<karabo::util::State>(arr, arr + 5));
            }

            Derived& allowedStates(const karabo::util::State& s1, const karabo::util::State& s2,
                                   const karabo::util::State& s3, const karabo::util::State& s4,
                                   const karabo::util::State& s5, const karabo::util::State& s6) {
                const karabo::util::State arr[] = {s1, s2, s3, s4, s5, s6};
                return allowedStates(std::vector<karabo::util::State>(arr, arr + 6));
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
                return karabo::util::GenericElement<Derived>::key(name);
            }
        };

        class SLOT_ELEMENT : public SlotElementBase<SLOT_ELEMENT> {
           public:
            SLOT_ELEMENT(karabo::util::Schema& expected) : SlotElementBase<SLOT_ELEMENT>(expected) {}

            void beforeAddition() {
                this->m_node->setValue(this->m_child);
            }
        };

        template <class A1>
        class SLOT_ELEMENT1 : public SlotElementBase<SLOT_ELEMENT1<A1> > {
           public:
            SLOT_ELEMENT1(karabo::util::Schema& expected) : SlotElementBase<SLOT_ELEMENT1<A1> >(expected) {
                this->m_child.set("arg1", 0);
                this->m_child.setAttribute("arg1", KARABO_SCHEMA_DISPLAYED_NAME, "Argument 1");
                this->m_child.template setAttribute<int>("arg1", KARABO_SCHEMA_ACCESS_MODE, karabo::util::WRITE);
                this->m_child.template setAttribute<int>("arg1", KARABO_SCHEMA_ASSIGNMENT,
                                                         karabo::util::Schema::MANDATORY_PARAM);
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
