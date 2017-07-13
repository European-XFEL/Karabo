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

#include "karabo/util.hpp"
#include "karabo/util/PackParameters.hh"
#include "karabo/log.hpp"

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

        public:

            const std::string& getInstanceIdOfSender() const;

            const std::string& getUserIdOfSender() const;

            const std::string& getAccessLevelOfSender() const;

            const std::string& getSessionTokenOfSender() const;

            /// Return message header that triggered calling this slot
            /// Valid while callRegisteredSlotFunctions is processed.
            karabo::util::Hash::Pointer getHeaderOfSender() const;
        protected:

            Slot(const std::string& slotFunction) : m_slotFunction(slotFunction) {
            }

            virtual ~Slot() {
            }

            std::string m_slotFunction;

            mutable boost::mutex m_callRegisteredSlotFunctionsMutex;

            void extractSenderInformation(const karabo::util::Hash& header);

            void invalidateSenderInformation();

            void callRegisteredSlotFunctions(const karabo::util::Hash& header, const karabo::util::Hash& body);

            virtual void doCallRegisteredSlotFunctions(const karabo::util::Hash& body) = 0;

        private:

            std::string m_instanceIdOfSender;
            std::string m_userIdOfSender;
            std::string m_accessLevelOfSender;
            std::string m_sessionTokenOfSender;
            karabo::util::Hash::Pointer m_headerOfSender;
        };

        template <typename Ret, typename ...Args>
        class SlotN : public Slot {

        public:

            typedef typename boost::function<Ret(Args...) > SlotHandler;

            void registerSlotFunction(const SlotHandler& slotHandler) {
                m_slotHandlers.push_back(slotHandler);
            }

            SlotN(const std::string& slotFunction) : Slot(slotFunction) {
            }

            virtual ~SlotN() {
            }

            virtual void doCallRegisteredSlotFunctions(const karabo::util::Hash& body) {
                for (const auto& handler : m_slotHandlers) {
                    karabo::util::call(handler, karabo::util::unpack < Args...>(body));
                }
            }

        private:

            typename std::vector<SlotHandler> m_slotHandlers;

        };
    }
}

#endif
