/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on February 7, 2011, 10:49 AM
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

#ifndef KARABO_XMS_SIGNAL_HH
#define KARABO_XMS_SIGNAL_HH

#include <boost/asio.hpp>
#include <karabo/net/Broker.hh>
#include <karabo/util/Factory.hh>
#include <tuple>
#include <typeindex>
#include <typeinfo>
#include <vector>

/**
 * The main Karabo namespace
 */
namespace karabo {

    /**
     * Namespace for package xms
     */
    namespace xms {

        // Reasoning for 2 minutes lifetime used below:
        // Under test conditions, we can read small messages at about 2 kHz speed.
        // If, for whatever reasons, this reading is blocked completely, the local openmqc queue accumulates
        // 240,000 messages within these 2 minutes (if messages cannot be dropped). But this also adds to the
        // broker backlog, since the broker is awaiting acknowledgement. This is already a quarter of the normal
        // maximum broker backlog we allow at XFEL. If this maximum is reached, communication is practically dead.
        // So we should stop increasing this backlog by starting to drop messages as expired.

        // priority and lifetime of messages that cannot be dropped - except if they expire (after 2 minutes)
#define KARABO_SYS_PRIO 4
#define KARABO_SYS_TTL 120000

        // priority and lifetime of messages that can be dropped and, after 2 minutes, expire
#define KARABO_PUB_PRIO 3
#define KARABO_PUB_TTL 120000

        // Forward SignalSlotable
        class SignalSlotable;

        class Signal {
            typedef std::map<std::string, std::set<std::string> > SlotMap;

           public:
            KARABO_CLASSINFO(Signal, "Signal", "1.0")

            Signal(const SignalSlotable* signalSlotable, const karabo::net::Broker::Pointer& channel,
                   const std::string& signalInstanceId, const std::string& signalFunction, const int priority,
                   const int messageTimeToLive);

            virtual ~Signal() {}

            /**
             * Use like setSignature<int, util::Hash, std::string>() to ensure that any emitted signal
             * has to take arguments of these three types in that order.
             */
            template <typename... Args>
            void setSignature() {
                m_argsType = std::type_index(typeid(std::tuple<Args...>));
            }

            /**
             * Register a slot to receive an emitted signal
             * @param slotInstanceId id of the instance of the slot
             * @param slotFunction name of the slot
             * @return bool whether freshly registered (false means: was already registered)
             */
            bool registerSlot(const std::string& slotInstanceId, const std::string& slotFunction);

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

            template <typename... Args>
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

            /**
             * This function allows to use a specific topic to which all messages are emitted
             * If the setter is not called, the topic of SignalSlotable will be used
             * NOTE: The idea is to keep a door open for a later change where each emit will use a topic
             * identical to the signal name. In that case the setter can just be removed.
             * @param topic The topic name
             */
            void setTopic(const std::string& topic);

           protected:
            void setSlotStrings(const SlotMap& slots, karabo::util::Hash& header) const;

            karabo::util::Hash::Pointer prepareHeader(const SlotMap& slots) const;

           private:
            void doEmit(const karabo::util::Hash::Pointer& message);

           protected:
            SignalSlotable* m_signalSlotable;
            const karabo::net::Broker::Pointer& m_channel;
            const std::string m_signalInstanceId;
            const std::string m_signalFunction;
            boost::mutex m_registeredSlotsMutex;
            SlotMap m_registeredSlots;
            int m_priority;
            int m_messageTimeToLive;
            std::string m_topic;

           private:
            std::type_index m_argsType;
        };
    } // namespace xms
} // namespace karabo

#endif
