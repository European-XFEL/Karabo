/*
 * $Id: Slot.hh 2810 2011-01-07 10:36:58Z wrona $
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on August 19, 2010, 4:49 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_XMS_SLOT_HH
#define	KARABO_XMS_SLOT_HH

#include <boost/type_traits.hpp>

#include <karabo/util/Factory.hh>
#include <karabo/net/BrokerChannel.hh>
/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package io
     */
    namespace xms {

        // Forward SignalSlotable
        class SignalSlotable;

        class Slot {

            std::string m_instanceIdOfSender;
            std::string m_userIdOfSender;
            std::string m_roleOfSender;
            std::string m_sessionTokenOfSender;
            
        public:

            const std::string& getInstanceIdOfSender() const;

            const std::string& getUserIdOfSender() const;

            const std::string& getRoleIdOfSender() const;
            
            const std::string& getSessionTokenOfSender() const;

        protected:

            Slot(SignalSlotable* signalSlotable, const karabo::net::BrokerChannel::Pointer& channel, const std::string& slotInstanceId, const std::string& slotFunction)
            : m_signalSlotable(signalSlotable), m_channel(channel), m_slotInstanceId(slotInstanceId + "|"), m_slotFunction(slotFunction + "|") {
                std::string filterCondition;
                filterCondition = "slotInstanceId LIKE '%" + m_slotInstanceId + "%' AND slotFunction LIKE '%" + m_slotFunction + "%'";
                m_channel->setFilter(filterCondition);
            }

            virtual ~Slot() {
            }

            void handlePossibleReply(const karabo::util::Hash& header);

            void extractSenderInformation(const karabo::util::Hash& header);

            void invalidateSenderInformation();

            //void startSlotProcessing();

            //void stopSlotProcessing();

            template <class T>
            const T& getAndCast(const std::string& key, const karabo::util::Hash& hash) const {
                return hash.get<T > (key);
            }

            SignalSlotable* m_signalSlotable;
            karabo::net::BrokerChannel::Pointer m_channel;
            std::string m_slotInstanceId;
            std::string m_slotFunction;

        };

        // This one does appropriate conversions for a boolean target type
        template <>
        const bool& Slot::getAndCast(const std::string& key, const karabo::util::Hash& hash) const;

        class Slot0 : public Slot {

            typedef boost::function<void () > SlotHandler;

        public:

            KARABO_CLASSINFO(Slot0, "Slot0", "1.0")

            Slot0(SignalSlotable* signalSlotable, const karabo::net::BrokerChannel::Pointer& channel, const std::string& slotInstanceId, const std::string& slotFunction)
            : Slot(signalSlotable, channel, slotInstanceId, slotFunction) {
                m_channel->readAsyncHashHash(boost::bind(&Slot0::callRegisteredSlotFunctions, this, _1, _2, _3));
            }

            virtual ~Slot0() {
            }

            void registerSlotFunction(const SlotHandler& slotHandler) {
                m_slotHandlers.push_back(slotHandler);
            }

        private:

            void callRegisteredSlotFunctions(karabo::net::BrokerChannel::Pointer /*channel*/, const karabo::util::Hash& body, const karabo::util::Hash& header) {
                extractSenderInformation(header);
                for (size_t i = 0; i < m_slotHandlers.size(); ++i) {
                    m_slotHandlers[i]();
                    handlePossibleReply(header);
                }
                invalidateSenderInformation();
            }

            std::vector<SlotHandler> m_slotHandlers;
        };

        template <class A1>
        class Slot1 : public Slot {

            typedef boost::function<void (const A1&) > SlotHandler;

        public:

            KARABO_CLASSINFO(Slot1, "Slot1", "1.0")

            Slot1(SignalSlotable* signalSlotable, const karabo::net::BrokerChannel::Pointer& channel, const std::string& slotInstanceId, const std::string& slotFunction)
            : Slot(signalSlotable, channel, slotInstanceId, slotFunction) {
                m_channel->readAsyncHashHash(boost::bind(&Slot1::callRegisteredSlotFunctions, this, _1, _2, _3));
            }

            virtual ~Slot1() {
            }

            void registerSlotFunction(const SlotHandler& slotHandler) {
                m_slotHandlers.push_back(slotHandler);
            }

        private:

            void callRegisteredSlotFunctions(karabo::net::BrokerChannel::Pointer /*channel*/, const karabo::util::Hash& body, const karabo::util::Hash& header) {
                try {
                    extractSenderInformation(header);
                    const A1& a1 = getAndCast<A1>("a1", body);
                    for (size_t i = 0; i < m_slotHandlers.size(); ++i) {
                        m_slotHandlers[i](a1);
                        handlePossibleReply(header);
                    }
                    invalidateSenderInformation();
                } catch (const karabo::util::CastException& e) {
                    karabo::util::Exception::addToTrace(e);
                    KARABO_LOG_FRAMEWORK_ERROR << KARABO_SIGNALSLOT_EXCEPTION("Received incompatible argument (see above) for slot \"" + m_slotFunction + "\". Check your connection!");
                    invalidateSenderInformation();
                }
            }

            std::vector<SlotHandler> m_slotHandlers;
        };

        template <class A1, class A2>
        class Slot2 : public Slot {

            typedef boost::function<void (const A1&, const A2&) > SlotHandler;

        public:

            KARABO_CLASSINFO(Slot2, "Slot2", "1.0")

            Slot2(SignalSlotable* signalSlotable, const karabo::net::BrokerChannel::Pointer& channel, const std::string& slotInstanceId, const std::string& slotFunction)
            : Slot(signalSlotable, channel, slotInstanceId, slotFunction) {
                m_channel->readAsyncHashHash(boost::bind(&Slot2::callRegisteredSlotFunctions, this, _1, _2, _3));
            }

            virtual ~Slot2() {
            }

            void registerSlotFunction(const SlotHandler& slotHandler) {
                m_slotHandlers.push_back(slotHandler);
            }

        private:

            void callRegisteredSlotFunctions(karabo::net::BrokerChannel::Pointer /*channel*/, const karabo::util::Hash& body, const karabo::util::Hash& header) {
                try {
                    extractSenderInformation(header);
                    const A1& a1 = getAndCast<A1>("a1", body);
                    const A2& a2 = getAndCast<A2>("a2", body);
                    for (size_t i = 0; i < m_slotHandlers.size(); ++i) {
                        m_slotHandlers[i](a1, a2);
                        handlePossibleReply(header);
                    }
                    invalidateSenderInformation();
                } catch (const karabo::util::CastException& e) {
                    karabo::util::Exception::addToTrace(e);
                    std::cout << KARABO_SIGNALSLOT_EXCEPTION("Received incompatible arguments (see above) for slot \"" + m_slotFunction + "\". Check your connection!") << std::endl;
                    invalidateSenderInformation();
                }
            }

            std::vector<SlotHandler> m_slotHandlers;
        };

        template <class A1, class A2, class A3>
        class Slot3 : public Slot {

            typedef boost::function<void (const A1&, const A2&, const A3&) > SlotHandler;

        public:

            KARABO_CLASSINFO(Slot3, "Slot3", "1.0")

            Slot3(SignalSlotable* signalSlotable, const karabo::net::BrokerChannel::Pointer& channel, const std::string& slotInstanceId, const std::string& slotFunction)
            : Slot(signalSlotable, channel, slotInstanceId, slotFunction) {
                m_channel->readAsyncHashHash(boost::bind(&Slot3::callRegisteredSlotFunctions, this, _1, _2, _3));
            }

            virtual ~Slot3() {
            }

            void registerSlotFunction(const SlotHandler& slotHandler) {
                m_slotHandlers.push_back(slotHandler);
            }

        private:

            void callRegisteredSlotFunctions(karabo::net::BrokerChannel::Pointer /*channel*/, const karabo::util::Hash& body, const karabo::util::Hash& header) {
                try {
                    extractSenderInformation(header);
                    const A1& a1 = getAndCast<A1 > ("a1", body);
                    const A2& a2 = getAndCast<A2 > ("a2", body);
                    const A3& a3 = getAndCast<A3 > ("a3", body);
                    for (size_t i = 0; i < m_slotHandlers.size(); ++i) {
                        m_slotHandlers[i](a1, a2, a3);
                        handlePossibleReply(header);
                    }
                    invalidateSenderInformation();
                } catch (const karabo::util::CastException& e) {
                    karabo::util::Exception::addToTrace(e);
                    std::cout << KARABO_SIGNALSLOT_EXCEPTION("Received incompatible arguments (see above) for slot \"" + m_slotFunction + "\". Check your connection!") << std::endl;
                    invalidateSenderInformation();
                }
            }

            std::vector<SlotHandler> m_slotHandlers;
        };

        template <class A1, class A2, class A3, class A4>
        class Slot4 : public Slot {

            typedef boost::function<void (const A1&, const A2&, const A3&, const A4&) > SlotHandler;

        public:

            KARABO_CLASSINFO(Slot4, "Slot4", "1.0")

            Slot4(SignalSlotable* signalSlotable, const karabo::net::BrokerChannel::Pointer& channel, const std::string& slotInstanceId, const std::string& slotFunction)
            : Slot(signalSlotable, channel, slotInstanceId, slotFunction) {
                m_channel->readAsyncHashHash(boost::bind(&Slot4::callRegisteredSlotFunctions, this, _1, _2, _3));
            }

            virtual ~Slot4() {
            }

            void registerSlotFunction(const SlotHandler& slotHandler) {
                m_slotHandlers.push_back(slotHandler);
            }

        private:

            void callRegisteredSlotFunctions(karabo::net::BrokerChannel::Pointer /*channel*/, const karabo::util::Hash& body, const karabo::util::Hash& header) {
                try {
                    extractSenderInformation(header);
                    const A1& a1 = getAndCast<A1 > ("a1", body);
                    const A2& a2 = getAndCast<A2 > ("a2", body);
                    const A3& a3 = getAndCast<A3 > ("a3", body);
                    const A4& a4 = getAndCast<A4 > ("a4", body);
                    for (size_t i = 0; i < m_slotHandlers.size(); ++i) {
                        m_slotHandlers[i](a1, a2, a3, a4);
                        handlePossibleReply(header);
                    }
                    invalidateSenderInformation();
                } catch (const karabo::util::CastException& e) {
                    karabo::util::Exception::addToTrace(e);
                    std::cout << KARABO_SIGNALSLOT_EXCEPTION("Received incompatible arguments (see above) for slot \"" + m_slotFunction + "\". Check your connection!") << std::endl;
                    invalidateSenderInformation();
                }
            }
            std::vector<SlotHandler> m_slotHandlers;
        };

    } // namespace xms
} // namespace karabo

#endif
