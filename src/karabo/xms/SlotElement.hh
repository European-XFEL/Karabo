/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on July 1, 2011, 2:23 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_UTIL_SLOTELEMENT_HH
#define	KARABO_UTIL_SLOTELEMENT_HH

#include <karabo/util/GenericElement.hh>
#include <karabo/util/ToLiteral.hh>

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
                
                //default value of requiredAccessLevel for Slot element: USER
                this->m_node->template setAttribute<int>(KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL, karabo::util::Schema::USER);

                m_child.set("connectedSignals", 0);
                m_child.setAttribute("connectedSignals", KARABO_SCHEMA_DISPLAYED_NAME, "Connected Signals");
                m_child.setAttribute("connectedSignals", KARABO_SCHEMA_DESCRIPTION, "Signals already connected to this slot");
                m_child.setAttribute<int>("connectedSignals", KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL, karabo::util::Schema::ADMIN);
                m_child.setAttribute<int>("connectedSignals", KARABO_SCHEMA_ASSIGNMENT, karabo::util::Schema::OPTIONAL_PARAM);
                m_child.setAttribute<int>("connectedSignals", KARABO_SCHEMA_NODE_TYPE, karabo::util::Schema::LEAF);
                m_child.setAttribute<int>("connectedSignals", KARABO_SCHEMA_LEAF_TYPE, karabo::util::Schema::COMMAND);
                m_child.setAttribute("connectedSignals", KARABO_SCHEMA_VALUE_TYPE, karabo::util::ToLiteral::to<karabo::util::Types::VECTOR_STRING>());

                connectionsAreReconfigurable();
            }

            /**
             * The <b>allowedStates</b> method serves for setting up allowed states for the element
             * @param states A string describing list of possible states.
             * @param sep A separator symbol used for parsing previous argument for list of states
             * @return reference to the Element (to allow method's chaining)
             */
            Derived& allowedStates(const std::string& states, const std::string& sep = " ,;") {
                this->m_node->setAttribute(KARABO_SCHEMA_ALLOWED_STATES, karabo::util::fromString<std::string, std::vector>(states, sep));
                return *(static_cast<Derived*> (this));
            }

            Derived& connectionAssignmentIsMandatory() {
                m_child.setAttribute<int>("connectedSignals", KARABO_SCHEMA_ASSIGNMENT, karabo::util::Schema::MANDATORY_PARAM);
                return *(static_cast<Derived*> (this));
            }

            Derived& connectionAssignmentIsOptional() {
                m_child.setAttribute<int>("connectedSignals", KARABO_SCHEMA_ASSIGNMENT, karabo::util::Schema::OPTIONAL_PARAM);
                return *(static_cast<Derived*> (this));
            }

            Derived& connectionsAreNotReconfigurable() {
                m_child.setAttribute<int>("connectedSignals", KARABO_SCHEMA_ACCESS_MODE, karabo::util::INIT);
                return *(static_cast<Derived*> (this));
            }

            Derived& connectionsAreReconfigurable() {
                m_child.setAttribute<int>("connectedSignals", KARABO_SCHEMA_ACCESS_MODE, karabo::util::WRITE);
                return *(static_cast<Derived*> (this));
            }

            virtual void beforeAddition() = 0;

        };

        class SLOT_ELEMENT : public SlotElementBase<SLOT_ELEMENT> {

        public:

            SLOT_ELEMENT(karabo::util::Schema& expected) : SlotElementBase<SLOT_ELEMENT>(expected) {
            }

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
                this->m_child.template setAttribute<int>("arg1", KARABO_SCHEMA_ASSIGNMENT, karabo::util::Schema::MANDATORY_PARAM);
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
    }
}

#endif	

