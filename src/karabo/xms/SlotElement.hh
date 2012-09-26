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

#include <karabo/util/ComplexElement.hh>
#include <karabo/util/SimpleElement.hh>

namespace karabo {
    namespace xms {

        template <class Derived>
        class SlotElementBase {
            
            protected:

            karabo::util::ComplexElement m_outerElement;
            karabo::util::SimpleElement<std::vector<std::string> > m_connectedSignals;

        public:

            SlotElementBase(karabo::util::Schema& expected) : m_outerElement(karabo::util::ComplexElement(expected)) {
                m_outerElement.initAndRead();
                m_outerElement.displayType("Slot");
                m_connectedSignals.key("connectedSignals");
                m_connectedSignals.displayedName("Connected Signals");
                m_connectedSignals.description("Signals already connected to this slot");
                m_connectedSignals.advanced();

                
                // By default connections are reconfigurable and optional
                connectionAssignmentIsOptional();
                connectionsAreNotReconfigurable();
            }
            
            Derived& allowedStates(const std::string& states) {
                m_outerElement.allowedStates(states);
                return *(static_cast<Derived*>(this));
            }

            Derived& key(const std::string& name) {
                m_outerElement.key(name);
                return *(static_cast<Derived*>(this));
            }
            
            template <class T>
            Derived& alias(const T& alias) {
               m_outerElement.alias(alias);
                return *(static_cast<Derived*>(this));
            }

            Derived& displayedName(const std::string& displayedName) {
                m_outerElement.displayedName(displayedName);
                return *(static_cast<Derived*>(this));
            }

            Derived& description(const std::string& desc) {
                m_outerElement.description(desc);
                return *(static_cast<Derived*>(this));
            }

            Derived& connectionAssignmentIsMandatory() {
                m_outerElement.assignmentMandatory();
                m_connectedSignals.assignmentMandatory();
                return *(static_cast<Derived*>(this));
            }

            Derived& connectionAssignmentIsOptional() {
                m_outerElement.assignmentOptional();
                m_connectedSignals.assignmentOptional().noDefaultValue();
                return *(static_cast<Derived*>(this));
            }

            Derived& connectionsAreNotReconfigurable() {
                m_outerElement.initAndRead();
                m_connectedSignals.init();
                return *(static_cast<Derived*>(this));
            }

            Derived& connectionsAreReconfigurable() {
                m_outerElement.reconfigureAndRead();
                m_connectedSignals.reconfigurable();
                return *(static_cast<Derived*>(this));
            }

            virtual void commit() = 0;
            
        };

                
        class SLOT_ELEMENT : public SlotElementBase<SLOT_ELEMENT> {
            
        public:
            SLOT_ELEMENT(karabo::util::Schema& expected) : SlotElementBase<SLOT_ELEMENT>(expected) {
            }
            
            void commit() {
                karabo::util::Schema& innerElement = this->m_outerElement.commit();
                this->m_connectedSignals.commit(innerElement);
            }
            
        };
        
        
        
        template <class A1>
        class SLOT_ELEMENT1 : public SlotElementBase<SLOT_ELEMENT1<A1> > {
            
            karabo::util::SimpleElement<A1> m_arg1;
            
        public:
            
            SLOT_ELEMENT1(karabo::util::Schema& expected) : SlotElementBase<SLOT_ELEMENT1<A1> >(expected) {
                m_arg1.key("arg1");
                m_arg1.displayedName("Argument 1");
                m_arg1.reconfigurable();
                m_arg1.assignmentMandatory();
            }
            
            SLOT_ELEMENT1& arg1Description(const std::string& desc) {
                m_arg1.description(desc);
                return *this;
            }
            
            SLOT_ELEMENT1& arg1DisplayName(const std::string& name) {
                m_arg1.displayedName(name);
                return *this;
            }
            
            void commit() {
                karabo::util::Schema& innerElement = this->m_outerElement.commit();
                this->m_connectedSignals.commit(innerElement);
                m_arg1.commit(innerElement);
            }
        };
    }
}

#endif	

