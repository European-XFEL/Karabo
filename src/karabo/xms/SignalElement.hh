/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on July 1, 2011, 2:23 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_UTIL_SIGNALELEMENT_HH
#define	KARABO_UTIL_SIGNALELEMENT_HH

#include <karabo/util/ComplexElement.hh>

namespace karabo {
    namespace xms {

        class SignalElement {

            protected:

            karabo::util::ComplexElement m_outerElement;
            karabo::util::SimpleElement<std::vector<std::string> > m_connectedSlots;

        public:

            SignalElement(karabo::util::Schema& expected) : m_outerElement(karabo::util::ComplexElement(expected)) {
                m_outerElement.reconfigureAndRead();
                m_connectedSlots.key("connectedSlots");
                m_connectedSlots.displayedName("Connected Slots");

                // By default connections are reconfigurable and optional
                connectionAssignmentIsOptional();
                connectionsAreReconfigurable();
            }

            SignalElement& key(const std::string& name) {
                m_outerElement.key(name);
                return *this;
            }

            SignalElement& displayedName(const std::string& displayedName) {
                m_outerElement.displayedName(displayedName);
                return *this;
            }

            SignalElement& description(const std::string& desc) {
                m_outerElement.description(desc);
                return *this;
            }

            SignalElement& connectionAssignmentIsMandatory() {
                m_outerElement.assignmentMandatory();
                m_connectedSlots.assignmentMandatory();
                return *this;
            }

            SignalElement& connectionAssignmentIsOptional() {
                m_outerElement.assignmentOptional();
                m_connectedSlots.assignmentOptional().noDefaultValue();
                return *this;
            }

            SignalElement& connectionsAreNotReconfigurable() {
                m_outerElement.initAndRead();
                m_connectedSlots.init();
                return *this;
            }

            SignalElement& connectionsAreReconfigurable() {
                m_outerElement.reconfigureAndRead();
                m_connectedSlots.reconfigurable();
                return *this;
            }

            void commit() {
                karabo::util::Schema& innerElement = m_outerElement.commit();
                m_connectedSlots.commit(innerElement);
            }
        };

        typedef SignalElement SIGNAL_ELEMENT;
    }
}

#endif	

