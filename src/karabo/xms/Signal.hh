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
#include <typeinfo>
#include <typeindex>

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
                   const int priority, const int messageTimeToLive);

            virtual ~Signal() {
            }

            /**
             * Use like setSignature<int, util::Hash, std::string>() to ensure that any emitted signal 
             * has to take arguments of these three types in that order.
             */
            template <typename ...Args>
            void setSignature() {
                m_argsType = std::type_index(typeid (std::tuple < Args...>));
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

            // This code is left until we find time to generically solve the char* to std::string problem
            /*
            template<typename ...> struct seq {

            };

            template<int N, typename S0, typename ...S> struct gens :
            gens<N - 1,
            typename std::conditional<std::is_same<S0, char const *>::value, std::string, S0>::type, S...> {

            };

            template<typename S0, typename ...S> struct gens < 0, S0, S...> {

                typedef seq < S0, S...> type;
            };

            template<typename ... S>
            bool comp_types(seq<S...>) {
                std::clog << m_argsType.name() << std::endl;
                return std::type_index(typeid (std::tuple < S...>)) == m_argsType;
            }
             */

            template <typename ...Args>
            void emit(const karabo::util::Hash::Pointer& message) {

                doEmit(message);
                // Remove above line and uncomment this once type issue is solved
                /*
                if (comp_types(typename gens<sizeof ... (Args), Args...>::type())) {
                    doEmit(message);
                } else {
                    throw KARABO_PARAMETER_EXCEPTION("Provided arguments do not fit registered signature");
                }
                 */
            }

        protected:

            void updateConnectedSlotsString();

            karabo::util::Hash::Pointer prepareHeader() const;

        private:

            void doEmit(const karabo::util::Hash::Pointer& message);

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

        private:

            std::type_index m_argsType;
        };
    } // namespace xms
} // namespace karabo

#endif
