/*
 * $Id: Slot.hh 2810 2011-01-07 10:36:58Z wrona $
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on August 19, 2010, 4:49 PM
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

#ifndef KARABO_XMS_SLOT_HH
#define KARABO_XMS_SLOT_HH

#include "karabo/util/Hash.hh"
#include "karabo/util/PackParameters.hh"

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
            Slot(const std::string& slotFunction) : m_slotFunction(slotFunction) {}

            virtual ~Slot() {}

            std::string m_slotFunction;

            std::mutex m_registeredSlotFunctionsMutex;

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

        template <typename Ret, typename... Args>
        class SlotN : public Slot {
           public:
            typedef typename std::function<Ret(const Args&...)> SlotHandler;

            void registerSlotFunction(const SlotHandler& slotHandler) {
                std::lock_guard<std::mutex> lock(m_registeredSlotFunctionsMutex);
                m_slotHandlers.push_back(slotHandler);
            }

            SlotN(const std::string& slotFunction) : Slot(slotFunction) {}

            virtual ~SlotN() {}

            /**
             * To be called under protection of m_registeredSlotFunctionsMutex
             * @param body a Hash with up to four keys "a1" to "a4" with the expected types behind
             */
            virtual void doCallRegisteredSlotFunctions(const karabo::util::Hash& body) {
                for (const auto& handler : m_slotHandlers) {
                    karabo::util::call(handler, karabo::util::unpack<Args...>(body));
                }
            }

           private:
            typename std::vector<SlotHandler> m_slotHandlers;
        };
    } // namespace xms
} // namespace karabo

#endif
