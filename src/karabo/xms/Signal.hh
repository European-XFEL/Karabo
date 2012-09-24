/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on February 7, 2011, 10:49 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef EXFEL_XMS_SIGNAL_HH
#define	EXFEL_XMS_SIGNAL_HH

#include <boost/asio.hpp>
#include <karabo/util/Factory.hh>
#include <karabo/util/Time.hh>
#include <karabo/net/BrokerChannel.hh>

/**
 * The main European XFEL namespace
 */
namespace exfel {

    /**
     * Namespace for package io
     */
    namespace xms {

        class Signal {
        public:

            EXFEL_CLASSINFO(Signal, "Signal", "1.0")

            Signal(const exfel::net::BrokerChannel::Pointer& channel, const std::string& signalInstanceId, const std::string& signalFunction) :
            m_channel(channel), m_signalInstanceId(signalInstanceId), m_signalFunction(signalFunction) {
                updateConnectedSlotsString();
            }

            virtual ~Signal() {
            }

            size_t nRegisteredSlots() const {
                return m_registeredSlotFunctions.size();
            }

            void registerSlot(const std::string& slotInstanceId, const std::string& slotFunction) {
                m_registeredSlotInstanceIds[slotInstanceId]++;
                m_registeredSlotFunctions[slotFunction]++;
                updateConnectedSlotsString();
            }

            bool unregisterSlot(const std::string& slotInstanceId, const std::string& slotFunction) {
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

            void emit0() {
                send(exfel::util::Hash());
            }

            template <class A1>
            void emit1(const A1& a1) {
                exfel::util::Hash message("a1", a1);
                send(message);
            }

            template <class A1, class A2>
            void emit2(const A1& a1, const A2& a2) {
                exfel::util::Hash message("a1", a1, "a2", a2);
                send(message);
            }

            template <class A1, class A2, class A3>
            void emit3(const A1& a1, const A2& a2, const A3& a3) {
                exfel::util::Hash message("a1", a1, "a2", a2, "a3", a3);
                send(message);
            }

            template <class A1, class A2, class A3, class A4>
            void emit4(const A1& a1, const A2& a2, const A3& a3, const A4& a4) {
                exfel::util::Hash message("a1", a1, "a2", a2, "a3", a3, "a4", a4);
                send(message);
            }

            //private:
        protected:

            void updateConnectedSlotsString() {
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

            void send(const exfel::util::Hash& message) {
                try {
                    // Do not send if no slots are connected
                    //if (m_connectedSlotsString == "none") return;
                    exfel::util::Hash header = prepareHeader();
                    m_channel->write(message, header);
                } catch (const exfel::util::Exception& e) {
                    RETHROW_AS(SIGNALSLOT_EXCEPTION("Problem sending a signal"))
                }
            }

            exfel::util::Hash prepareHeader() const {
                exfel::util::Hash header;
                header.set("signalFunction", m_signalFunction);
                header.set("signalInstanceId", m_signalInstanceId);
                header.set("slotFunction", m_registeredSlotFunctionsString);
                header.set("slotInstanceId", m_registeredSlotInstanceIdsString);
                header.set("hostName", boost::asio::ip::host_name());
                header.set("classId", "Signal");
                return header;
            }
            
        protected:
            
            exfel::net::BrokerChannel::Pointer m_channel;
            std::string m_signalInstanceId;
            std::string m_signalFunction;
            std::string m_registeredSlotFunctionsString;
            std::string m_registeredSlotInstanceIdsString;
            size_t m_nRegisteredSlots;
            std::map<std::string, size_t> m_registeredSlotFunctions;
            std::map<std::string, size_t> m_registeredSlotInstanceIds;
        };

    } // namespace xms
} // namespace exfel

#endif
