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
        

        Signal::Signal(const SignalSlotable* signalSlotable, const karabo::net::BrokerChannel::Pointer& channel,
                       const std::string& signalInstanceId, const std::string& signalFunction,
                       const int priority, const int messageTimeToLive)
        : m_signalSlotable(const_cast<SignalSlotable*>(signalSlotable))
        , m_channel(channel)
        , m_signalInstanceId(signalInstanceId)
        , m_signalFunction(signalFunction)
        , m_priority(priority)
        , m_messageTimeToLive(messageTimeToLive) {
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
                if (slotFunction.empty()) {
                    m_registeredSlots.erase(slotInstanceId);
                } else {
                    it->second.erase(slotFunction);
                    if (it->second.empty()) m_registeredSlots.erase(slotInstanceId);
                }
                updateConnectedSlotsString();
            }
        }

        void Signal::send(const karabo::util::Hash& message) {
            using namespace karabo::util;
            try {
                karabo::util::Hash header = prepareHeader();
                // In case we are connected to a single local instance we shortcut the broker
                if ((m_registeredSlots.size() == 1) && 
                        (m_registeredSlots.find(m_signalSlotable->m_instanceId) != m_registeredSlots.end())) {
                    m_signalSlotable->injectEvent(m_channel, Hash::Pointer(new Hash(header)), Hash::Pointer(new Hash(message)));
                } else {
                    // Do not send if no slots are connected except heartbeats.
                    // Heartbeats always have slotInstanceId == __none__ 
                    if (m_signalFunction != "signalHeartbeat" && m_registeredSlotInstanceIdsString == "__none__") return;
                    m_channel->write(header, message, m_priority, m_messageTimeToLive);
                }
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
