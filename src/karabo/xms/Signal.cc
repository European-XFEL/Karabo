/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on April 29, 2010, 10:10 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "SignalSlotable.hh"
#include "Signal.hh"

namespace karabo {
    namespace xms {

        Signal::Signal(const SignalSlotable* signalSlotable, const karabo::net::BrokerChannel::Pointer& channel, const std::string& signalInstanceId, const std::string& signalFunction) :
        m_signalSlotable(signalSlotable), m_channel(channel), m_signalInstanceId(signalInstanceId), m_signalFunction(signalFunction) {
            updateConnectedSlotsString();
        }

        void Signal::updateConnectedSlotsString() {
            m_registeredSlotsString.clear();
            m_registeredSlotInstanceIdsString.clear();
            if (nRegisteredSlots() == 0) {
                m_registeredSlotsString = "__none__";
                m_registeredSlotInstanceIdsString = "__none__";
            } else {
                for (std::map<std::string, std::set<std::string> >::const_iterator it = m_registeredSlots.begin(); it != m_registeredSlots.end(); ++it) {
                    m_registeredSlotInstanceIdsString += "|" + it->first + "|";
                    m_registeredSlotsString += "|" + it->first + ":" + karabo::util::toString(it->second) + "|";
                }
            }
        }

        size_t Signal::nRegisteredSlots() const {
            return m_registeredSlots.size();
        }

        void Signal::registerSlot(const std::string& slotInstanceId, const std::string& slotFunction) {
            m_registeredSlots[slotInstanceId].insert(slotFunction);
            updateConnectedSlotsString();
        }

        void Signal::unregisterSlot(const std::string& slotInstanceId, const std::string& slotFunction) {
            std::map<std::string, std::set<std::string> >::iterator it = m_registeredSlots.find(slotInstanceId);
            if (it != m_registeredSlots.end()) {
                it->second.erase(slotFunction);
                if (it->second.empty()) m_registeredSlots.erase(slotInstanceId);
                updateConnectedSlotsString();
            }
        }

        void Signal::send(const karabo::util::Hash& message) {
            try {
                // TODO Do not send if no slots are connected
                //if (m_connectedSlotsString == "__none__") return;
                karabo::util::Hash header = prepareHeader();
                m_channel->write(header, message);
            } catch (const karabo::util::Exception& e) {
                KARABO_RETHROW_AS(KARABO_SIGNALSLOT_EXCEPTION("Problem sending a signal"))
            }
        }

        karabo::util::Hash Signal::prepareHeader() const {
            karabo::util::Hash header;            
            header.set("signalInstanceId", m_signalInstanceId);
            header.set("signalFunction", m_signalFunction);
            header.set("slotInstanceIds", m_registeredSlotInstanceIdsString);
            header.set("slotFunctions", m_registeredSlotsString);
            header.set("hostName", boost::asio::ip::host_name());
            header.set("userName", m_signalSlotable->getUserName());
            return header;
        }


    }
}