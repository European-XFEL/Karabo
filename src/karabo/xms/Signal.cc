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
            m_registeredSlotFunctionsString.clear();
            m_registeredSlotInstanceIdsString.clear();
            if (nRegisteredSlots() == 0) {
                m_registeredSlotFunctionsString = "__none__";
                m_registeredSlotInstanceIdsString = "__none__";
            } else {
                for (std::map<std::string, size_t>::const_iterator it = m_registeredSlotFunctions.begin(); it != m_registeredSlotFunctions.end(); ++it) {
                    m_registeredSlotFunctionsString += it->first + "|";
                }
                for (std::map<std::string, size_t>::const_iterator it = m_registeredSlotInstanceIds.begin(); it != m_registeredSlotInstanceIds.end(); ++it) {
                    m_registeredSlotInstanceIdsString += it->first + "|";
                }
            }
        }


        size_t Signal::nRegisteredSlots() const {
            return m_registeredSlotFunctions.size();
        }


        void Signal::registerSlot(const std::string& slotInstanceId, const std::string& slotFunction) {
            m_registeredSlotInstanceIds[slotInstanceId]++;
            m_registeredSlotFunctions[slotFunction]++;
            updateConnectedSlotsString();
        }


        bool Signal::unregisterSlot(const std::string& slotInstanceId, const std::string& slotFunction) {
            std::map<std::string, size_t>::iterator itI = m_registeredSlotInstanceIds.find(slotInstanceId);
            std::map<std::string, size_t>::iterator itF = m_registeredSlotFunctions.find(slotFunction);
            if (itI != m_registeredSlotInstanceIds.end() && itF != m_registeredSlotFunctions.end()) {
                if (--(itI->second) == 0) m_registeredSlotInstanceIds.erase(itI);
                if (--(itF->second) == 0) m_registeredSlotFunctions.erase(itF);
                updateConnectedSlotsString();
                return true;
            } else {
                return false;
            }
        }


        void Signal::send(const karabo::util::Hash& message) {
            try {
                // TODO Do not send if no slots are connected
                //if (m_connectedSlotsString == "none") return;
                karabo::util::Hash header = prepareHeader();
                m_channel->write(message, header);
            } catch (const karabo::util::Exception& e) {
                KARABO_RETHROW_AS(KARABO_SIGNALSLOT_EXCEPTION("Problem sending a signal"))
            }
        }


        karabo::util::Hash Signal::prepareHeader() const {
            karabo::util::Hash header;
            header.set("signalFunction", m_signalFunction);
            header.set("signalInstanceId", m_signalInstanceId);
            header.set("slotFunction", m_registeredSlotFunctionsString);
            header.set("slotInstanceId", m_registeredSlotInstanceIdsString);
            header.set("hostName", boost::asio::ip::host_name());
            header.set("classId", "Signal");
            return header;
        }


    }
}