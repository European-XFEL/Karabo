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


        Signal::Signal(const SignalSlotable* signalSlotable, const karabo::net::Broker::Pointer& channel,
                       const std::string& signalInstanceId, const std::string& signalFunction,
                       const int priority, const int messageTimeToLive) :
            m_signalSlotable(const_cast<SignalSlotable*> (signalSlotable)),
            m_channel(channel),
            m_signalInstanceId(signalInstanceId),
            m_signalFunction(signalFunction),
            m_priority(priority),
            m_messageTimeToLive(messageTimeToLive),
            m_topic(signalSlotable->m_topic),
            m_argsType(typeid (karabo::util::Types::NONE)) {
        }


        std::pair<std::string, std::string> Signal::generateSlotStrings(const SlotMap& slots) const {
            std::string registeredSlotInstanceIdsString;
            std::string registeredSlotsString;
            if (slots.empty()) {
                registeredSlotsString = "__none__";
                registeredSlotInstanceIdsString = "__none__";
            } else {
                for (auto it = slots.cbegin(); it != slots.cend(); ++it) {
                    registeredSlotInstanceIdsString += "|" + it->first + "|";
                    registeredSlotsString += "|" + it->first + ":" + karabo::util::toString(it->second) + "|";
                }
            }
            return std::make_pair(registeredSlotInstanceIdsString, registeredSlotsString);
        }


        void Signal::registerSlot(const std::string& slotInstanceId, const std::string& slotFunction) {
            m_registeredSlots[slotInstanceId].insert(slotFunction);
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
            }
            return didErase;
        }


        void Signal::doEmit(const karabo::util::Hash::Pointer& message) {
            using namespace karabo::util;
            try {
                // prepareHeader should be called once per 'emit'!
                Hash::Pointer header = prepareHeader(m_registeredSlots);

                // Two ways to emit: 1) In-process 2) Broker
                // TODO Improve the code here, to be a bit more disentangled and speedy

                // Not connected to any slot
                if (m_registeredSlots.empty()) {
                    // Heartbeats are an exception, must always be sent
                    if (m_signalFunction == "signalHeartbeat") {
                        m_channel->write(m_topic, header, message, m_priority, m_messageTimeToLive);
                        return;
                    } else {
                        // Do not even produce traffic on the way to the broker, as no one cares for this message
                        return;
                    }
                }

                // Copy the registered slots
                SlotMap registeredSlots = m_registeredSlots;

                // Try all registered slots whether we could send in-process
                for (auto it = registeredSlots.cbegin(); it != registeredSlots.cend();) {
                    if (m_signalSlotable->tryToCallDirectly(it->first, header, message)) {
                        registeredSlots.erase(it++);
                    } else {
                        ++it;
                    }
                }

                // publish leftovers via broker
                if (registeredSlots.size() > 0) {
                    // header contains updated slot leftovers
                    m_channel->write(m_topic, header, message, m_priority, m_messageTimeToLive);
                }

            } catch (const karabo::util::Exception& e) {
                KARABO_RETHROW_AS(KARABO_SIGNALSLOT_EXCEPTION("Problem sending a signal"))
            }
        }


        karabo::util::Hash::Pointer Signal::prepareHeader(const SlotMap& slots) const {
            karabo::util::Hash::Pointer header(new karabo::util::Hash);
            std::pair<std::string, std::string> slotStrings = generateSlotStrings(slots);
            header->set("signalInstanceId", m_signalInstanceId);
            header->set("signalFunction", m_signalFunction);
            header->set("slotInstanceIds", slotStrings.first);
            header->set("slotFunctions", slotStrings.second);
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
