/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on February 7, 2011, 10:49 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_XMS_SIGNAL_HH
#define	KARABO_XMS_SIGNAL_HH

#include <boost/asio.hpp>
#include <karabo/util/Factory.hh>
#include <karabo/net/BrokerChannel.hh>

/**
 * The main Karabo namespace
 */
namespace karabo {

    /**
     * Namespace for package xms
     */
    namespace xms {

        #define KARABO_SYS_PRIO 4
        #define KARABO_SYS_TTL 600000
 
        #define KARABO_PUB_PRIO 3
        #define KARABO_PUB_TTL 600000
 
        // Forward SignalSlotable
        class SignalSlotable;

        class Signal {

        public:

            KARABO_CLASSINFO(Signal, "Signal", "1.0")

            Signal(const SignalSlotable* signalSlotable, const karabo::net::BrokerChannel::Pointer& channel,
                   const std::string& signalInstanceId, const std::string& signalFunction,
                   const int priority = KARABO_SYS_PRIO, const int messageTimeToLive = KARABO_SYS_TTL);

            virtual ~Signal() {
            }

            size_t nRegisteredSlots() const;

            void registerSlot(const std::string& slotInstanceId, const std::string& slotFunction);

            /**
             * Undo registration of a slot
             * @param slotInstanceId instance id of the slot to be removed
             * @param slotFunction the slot - if empty string, remove all registered slots of slotInstanceId
             * @return bool whether slot registration could be undone, i.e. false if slot was not registered
             */
            bool unregisterSlot(const std::string& slotInstanceId, const std::string& slotFunction = "");

            void emit0() {
                send(karabo::util::Hash::Pointer(new karabo::util::Hash));
            }

            template <class A1>
            void emit1(const A1& a1) {
                karabo::util::Hash::Pointer message(new karabo::util::Hash("a1", a1));
                send(message);
            }

            template <class A1, class A2>
            void emit2(const A1& a1, const A2& a2) {
                karabo::util::Hash::Pointer message(new karabo::util::Hash("a1", a1, "a2", a2));
                send(message);
            }

            template <class A1, class A2, class A3>
            void emit3(const A1& a1, const A2& a2, const A3& a3) {
                karabo::util::Hash::Pointer message(new karabo::util::Hash("a1", a1, "a2", a2, "a3", a3));
                send(message);
            }

            template <class A1, class A2, class A3, class A4>
            void emit4(const A1& a1, const A2& a2, const A3& a3, const A4& a4) {
                karabo::util::Hash::Pointer message(new karabo::util::Hash("a1", a1, "a2", a2, "a3", a3, "a4", a4));
                send(message);
            }
            
            //private:
        protected:

            void updateConnectedSlotsString();

            void send(const karabo::util::Hash::Pointer& message);

            karabo::util::Hash::Pointer prepareHeader() const;

        protected:

            SignalSlotable* m_signalSlotable;
            const karabo::net::BrokerChannel::Pointer& m_channel;
            std::string m_signalInstanceId;
            std::string m_signalFunction;
            std::string m_registeredSlotsString;
            std::string m_registeredSlotInstanceIdsString;
            size_t m_nRegisteredSlots;
            std::map<std::string, std::set<std::string> > m_registeredSlots;
            int m_priority;
            int m_messageTimeToLive;
        };

    } // namespace xms
} // namespace karabo

#endif
