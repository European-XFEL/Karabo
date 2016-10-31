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


        Signal::Signal(const SignalSlotable* signalSlotable, const karabo::net::JmsProducer::Pointer& channel,
                       const std::string& signalInstanceId, const std::string& signalFunction,
                       const int priority, const int messageTimeToLive) :
            m_signalSlotable(const_cast<SignalSlotable*> (signalSlotable)),
            m_channel(channel),
            m_signalInstanceId(signalInstanceId),
            m_signalFunction(signalFunction),
            m_priority(priority),
            m_messageTimeToLive(messageTimeToLive),
            m_argsType(typeid (karabo::util::Types::NONE)),
            m_topic(signalSlotable->m_topic) {
            updateConnectedSlotsString();
        }


        void Signal::updateConnectedSlotsString() {
            m_registeredSlotsString.clear();
            m_registeredSlotInstanceIdsString.clear();
            if (nRegisteredSlots() == 0) {
                m_registeredSlotsString = "__none__";
                m_registeredSlotInstanceIdsString = "__none__";
            } else {
                for (auto it = m_registeredSlots.cbegin(); it != m_registeredSlots.cend(); ++it) {
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


        bool Signal::unregisterSlot(const std::string& slotInstanceId, const std::string& slotFunction) {
            auto it = m_registeredSlots.find(slotInstanceId);
            bool didErase = false;
            if (it != m_registeredSlots.end()) {
                if (slotFunction.empty()) {
                    didErase = !it->second.empty();
                    m_registeredSlots.erase(it);
                } else {
                    didErase = (it->second.erase(slotFunction) >= 1);
                    if (it->second.empty()) m_registeredSlots.erase(it);
                }
                updateConnectedSlotsString();
            }
            return didErase;
        }


        void Signal::doEmit(const karabo::util::Hash::Pointer& message) {
            using namespace karabo::util;
            try {
                karabo::util::Hash::Pointer header = prepareHeader();

                // TODO Re-factor and enable short-cut messaging in a future MR!

                // Do not send if no slots are connected except heartbeats.
                // Heartbeats always have slotInstanceId == __none__
                // Send heartbeat signal via broker
                if (m_registeredSlotInstanceIdsString == "__none__") {
                    if (m_signalFunction == "signalHeartbeat") {
                        m_channel->write(m_topic, *header, *message, m_priority, m_messageTimeToLive);
                        return;
                    }
                }

                m_channel->write(m_topic, *header, *message, m_priority, m_messageTimeToLive);

            } catch (const karabo::util::Exception& e) {
                KARABO_RETHROW_AS(KARABO_SIGNALSLOT_EXCEPTION("Problem sending a signal"))
            }
        }


        karabo::util::Hash::Pointer Signal::prepareHeader() const {
            karabo::util::Hash::Pointer header(new karabo::util::Hash);
            header->set("signalInstanceId", m_signalInstanceId);
            header->set("signalFunction", m_signalFunction);
            header->set("slotInstanceIds", m_registeredSlotInstanceIdsString);
            header->set("slotFunctions", m_registeredSlotsString);
            header->set("hostName", boost::asio::ip::host_name());
            header->set("userName", m_signalSlotable->getUserName());
            // Timestamp added to be able to measure latencies even if broker is by-passed
            header->set("MQTimestamp", m_signalSlotable->getEpochMillis());
            return header;
        }


        void Signal::setTopic(const std::string& topic) {
            m_topic = topic;
        }
    }
}
