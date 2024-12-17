/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on April 29, 2010, 10:10 AM
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

#include "Signal.hh"

#include "SignalSlotable.hh"

namespace karabo {
    namespace xms {


        Signal::Signal(const SignalSlotable* signalSlotable, const karabo::net::Broker::Pointer& channel,
                       const std::string& signalInstanceId, const std::string& signalFunction, const int priority,
                       const int messageTimeToLive)
            : m_signalSlotable(const_cast<SignalSlotable*>(signalSlotable)),
              m_channel(channel),
              m_signalInstanceId(signalInstanceId),
              m_signalFunction(signalFunction),
              m_priority(priority),
              m_messageTimeToLive(messageTimeToLive),
              m_topic(signalSlotable->m_topic),
              m_argsType(typeid(karabo::util::Types::NONE)) {}


        void Signal::setSlotStrings(const SlotMap& slots, karabo::util::Hash& header) const {
            std::ostringstream registeredSlotsString;
            std::ostringstream registeredSlotInstanceIdsString;
            if (slots.empty()) {
                registeredSlotsString << "__none__";
                registeredSlotInstanceIdsString << "__none__";
            } else {
                for (auto it = slots.cbegin(); it != slots.cend(); ++it) {
                    registeredSlotInstanceIdsString << '|' << it->first << '|';
                    registeredSlotsString << '|' << it->first << ':';
                    // it->second is a set<string> - optimize here compared to simple '<< toString(it->second)'
                    bool first = true;
                    for (const std::string& slot : it->second) {
                        if (first) {
                            first = false;
                        } else {
                            registeredSlotsString << ',';
                        }
                        registeredSlotsString << slot;
                    } // end optimization
                    registeredSlotsString << '|';
                }
            }
            // To avoid a copy of the (temporary!) string returned by ostringstream::str(), use bindReference and assign
            // (Hash::set has no rvalue reference argument overload, but string has move assignment...)
            header.bindReference<std::string>("slotInstanceIds") = registeredSlotInstanceIdsString.str();
            header.bindReference<std::string>("slotFunctions") = registeredSlotsString.str();
        }


        bool Signal::registerSlot(const std::string& slotInstanceId, const std::string& slotFunction) {
            std::lock_guard<std::mutex> lock(m_registeredSlotsMutex);
            // Note: m_registeredSlots[slotInstanceId] is a set<std::string> and set<..>::insert(arg) returns an
            //       std::pair where second tells whether arg was already in the set or is newly added.
            return m_registeredSlots[slotInstanceId].insert(slotFunction).second;
        }


        bool Signal::unregisterSlot(const std::string& slotInstanceId, const std::string& slotFunction) {
            std::lock_guard<std::mutex> lock(m_registeredSlotsMutex);
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
                SlotMap registeredSlots;
                {
                    std::lock_guard<std::mutex> lock(m_registeredSlotsMutex);
                    registeredSlots = m_registeredSlots;
                }
                Hash::Pointer header = prepareHeader(registeredSlots);

                // Two ways to emit: 1) In-process 2) Broker
                // TODO Improve the code here, to be a bit more disentangled and speedy

                // Not connected to any slot
                if (registeredSlots.empty()) {
                    // Heartbeats are an exception, must always be sent
                    if (m_signalFunction == "signalHeartbeat") {
                        m_channel->write(m_topic, header, message, m_priority, m_messageTimeToLive);
                        return;
                    } else {
                        // Do not even produce traffic on the way to the broker, as no one cares for this message
                        return;
                    }
                }

                // Try all registered slots whether we could send in-process
                const size_t fullNumRegisteredSlots = registeredSlots.size();
                for (auto it = registeredSlots.cbegin(); it != registeredSlots.cend();) {
                    if (m_signalSlotable->tryToCallDirectly(it->first, header, message)) {
                        registeredSlots.erase(it++);
                    } else {
                        ++it;
                    }
                }

                // publish leftovers via broker
                if (registeredSlots.size() > 0) {
                    if (registeredSlots.size() != fullNumRegisteredSlots) {
                        // overwrite destinations to erase those that received locally, to avoid duplicates
                        setSlotStrings(registeredSlots, *header);
                    }
                    m_channel->write(m_topic, header, message, m_priority, m_messageTimeToLive);
                }

            } catch (const karabo::util::Exception& e) {
                KARABO_RETHROW_AS(KARABO_SIGNALSLOT_EXCEPTION("Problem sending a signal"))
            }
        }


        karabo::util::Hash::Pointer Signal::prepareHeader(const SlotMap& slots) const {
            if (m_signalInstanceId.empty() && m_signalSlotable) {
                // Hack to fix empty id if signal created before SignalSlotable::init (which defines the id) was called.
                // Happens currently (2.10.0) for signals registered in constructors of devices
                *const_cast<std::string*>(&m_signalInstanceId) = m_signalSlotable->getInstanceId();
            }

            karabo::util::Hash::Pointer header(std::make_shared<karabo::util::Hash>());
            header->set("signalInstanceId", m_signalInstanceId);
            header->set("signalFunction", m_signalFunction);
            setSlotStrings(slots, *header);
            header->set("hostName", boost::asio::ip::host_name());
            header->set("userName", m_signalSlotable->getUserName());
            // Timestamp added to be able to measure latencies even if broker is by-passed (or non-JMS)
            // Needed here since Signal is by-passing m_signalSlotable->doSendMessage(..).
            header->set("MQTimestamp", m_signalSlotable->getEpochMillis());
            return header;
        }


        void Signal::setTopic(const std::string& topic) {
            m_topic = topic;
        }
    } // namespace xms
} // namespace karabo
