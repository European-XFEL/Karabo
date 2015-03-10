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

#include <karabo/util.hpp>
#include <karabo/log.hpp>

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
            friend class SignalSlotable;

            std::string m_instanceIdOfSender;
            std::string m_userIdOfSender;
            std::string m_accessLevelOfSender;
            std::string m_sessionTokenOfSender;

        public:

            const std::string& getInstanceIdOfSender() const;

            const std::string& getUserIdOfSender() const;

            const std::string& getAccessLevelOfSender() const;

            const std::string& getSessionTokenOfSender() const;

        protected:

            Slot(const std::string& slotFunction) : m_slotFunction(slotFunction) {
            }

            ~Slot() {}

            std::string m_slotFunction;

            mutable boost::mutex m_callRegisteredSlotFunctionsMutex;

            void extractSenderInformation(const karabo::util::Hash& header);

            void invalidateSenderInformation();

            void callRegisteredSlotFunctions(const karabo::util::Hash& header, const karabo::util::Hash& body);
            virtual void doCallRegisteredSlotFunctions(const karabo::util::Hash& body) = 0;
        };

	template <class Handler>
	class SlotN : public Slot {
	    typedef typename boost::function<Handler> SlotHandler;

	protected:
	    SlotN(const std::string& slotFunction) : Slot(slotFunction) {
            }

            ~SlotN() {}

        public:
            void registerSlotFunction(const SlotHandler& slotHandler) {
                m_slotHandlers.push_back(slotHandler);
            }

            typename std::vector<SlotHandler> m_slotHandlers;
	};

        class Slot0 : public SlotN<void ()> {
        public:

            KARABO_CLASSINFO(Slot0, "Slot0", "1.0")

            Slot0(const std::string& slotFunction) : SlotN<void ()> (slotFunction) { }

            ~Slot0() {}
        protected:

            virtual void doCallRegisteredSlotFunctions(const karabo::util::Hash& body) {
                for (size_t i = 0; i < m_slotHandlers.size(); ++i) {
                    m_slotHandlers[i]();
                }
            }
        };

        template <class A1>
        class Slot1 : public SlotN<void (const A1&)> {
        public:

            KARABO_CLASSINFO(Slot1, "Slot1", "1.0")

            Slot1(const std::string& slotFunction) : SlotN<void (const A1&)> (slotFunction) { }

            ~Slot1() {}
        private:

            virtual void doCallRegisteredSlotFunctions(const karabo::util::Hash& body) {
                const A1& a1 = body.get<A1>("a1");
                for (size_t i = 0; i < this->m_slotHandlers.size(); ++i) {
                    this->m_slotHandlers[i](a1);
                }
            }
        };

        template <class A1, class A2>
        class Slot2 : public SlotN<void (const A1&, const A2&)> {
        public:

            KARABO_CLASSINFO(Slot2, "Slot2", "1.0")

            Slot2(const std::string& slotFunction) : SlotN<void (const A1&, const A2&)> (slotFunction) { }

            ~Slot2() {}
        private:

            virtual void doCallRegisteredSlotFunctions(const karabo::util::Hash& body) {
                const A1& a1 = body.get<A1>("a1");
                const A2& a2 = body.get<A2>("a2");
                for (size_t i = 0; i < this->m_slotHandlers.size(); ++i) {
                    this->m_slotHandlers[i](a1, a2);
                }
            }
        };

        template <class A1, class A2, class A3>
        class Slot3 : public SlotN<void (const A1&, const A2&, const A3&)> {
        public:

            KARABO_CLASSINFO(Slot3, "Slot3", "1.0")

            Slot3(const std::string& slotFunction) : SlotN<void (const A1&, const A2&, const A3&)> (slotFunction) { }
            ~Slot3() {}
        private:

            virtual void doCallRegisteredSlotFunctions(const karabo::util::Hash& body) {
                const A1& a1 = body.get<A1 > ("a1");
                const A2& a2 = body.get<A2 > ("a2");
                const A3& a3 = body.get<A3 > ("a3");
                for (size_t i = 0; i < this->m_slotHandlers.size(); ++i) {
                    this->m_slotHandlers[i](a1, a2, a3);
                }
            }
        };

        template <class A1, class A2, class A3, class A4>
        class Slot4 : public SlotN<void (const A1&, const A2&, const A3&, const A4&)> {

        public:

            KARABO_CLASSINFO(Slot4, "Slot4", "1.0")

            Slot4(const std::string& slotFunction) : SlotN<void (const A1&, const A2&, const A3&, const A4&)> (slotFunction) { }
            ~Slot4() {}
        private:

            virtual void doCallRegisteredSlotFunctions(const karabo::util::Hash& body) {
                const A1& a1 = body.get<A1 > ("a1");
                const A2& a2 = body.get<A2 > ("a2");
                const A3& a3 = body.get<A3 > ("a3");
                const A4& a4 = body.get<A4 > ("a4");
                for (size_t i = 0; i < this->m_slotHandlers.size(); ++i) {
                    this->m_slotHandlers[i](a1, a2, a3, a4);
                }
            }
        };

    }
}

#endif
