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
                       const std::string& signalInstanceId, const std::string& signalFunction)
            : m_signalSlotable(const_cast<SignalSlotable*>(signalSlotable)),
              m_channel(channel),
              m_signalInstanceId(signalInstanceId),
              m_signalFunction(signalFunction),
              m_argsType(typeid(karabo::data::Types::NONE)) {}


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


        void Signal::doEmit(const karabo::data::Hash::Pointer& message) {
            using namespace karabo::data;
            try {
                SlotMap registeredSlots;
                {
                    std::lock_guard<std::mutex> lock(m_registeredSlotsMutex);
                    registeredSlots = m_registeredSlots;
                }
                Hash::Pointer header = prepareHeader(registeredSlots);

                // Two communication paths:
                // - Those that registered are local instances and should be addressed via in-process shortcut
                // - Then we send to broker - usually someone is subscribed.

                // Try all registered slots whether we could send in-process
                for (auto it = registeredSlots.cbegin(); it != registeredSlots.cend(); ++it) {
                    for (const std::string& slot : it->second) {
                        const std::string& devId = it->first;
                        if (!m_signalSlotable->tryToCallDirectly(devId, slot, header, message)) {
                            KARABO_LOGGING_WARN(
                                  "A registered slot instance is not available for direct call (anymore?): ", devId);
                        }
                    }
                }

                // publish via broker
                m_channel->sendSignal(m_signalFunction, header, message);

            } catch (const karabo::data::Exception& e) {
                KARABO_RETHROW_AS(KARABO_SIGNALSLOT_EXCEPTION("Problem sending a signal"))
            }
        }


        karabo::data::Hash::Pointer Signal::prepareHeader(const SlotMap& slots) const {
            if (m_signalInstanceId.empty() && m_signalSlotable) {
                // Hack to fix empty id if signal created before SignalSlotable::init (which defines the id) was called.
                // Happens currently (2.10.0) for signals registered in constructors of devices
                *const_cast<std::string*>(&m_signalInstanceId) = m_signalSlotable->getInstanceId();
            }

            karabo::data::Hash::Pointer header(std::make_shared<karabo::data::Hash>());
            header->set("signalInstanceId", m_signalInstanceId);
            // Needed here since Signal is by-passing m_signalSlotable->doSendMessage(..).
            header->set("MQTimestamp", m_signalSlotable->getEpochMillis());
            return header;
        }

    } // namespace xms
} // namespace karabo
