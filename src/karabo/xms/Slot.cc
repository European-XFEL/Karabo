/*
 * $Id: Slot.cc 2810 2011-01-07 10:36:58Z wrona $
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

#include "Slot.hh"

using namespace karabo::util;

namespace karabo {

    namespace xms {


        const std::string& Slot::getInstanceIdOfSender() const {
            return m_instanceIdOfSender;
        }


        const std::string& Slot::getAccessLevelOfSender() const {
            return m_accessLevelOfSender;
        }


        const std::string& Slot::getUserIdOfSender() const {
            return m_userIdOfSender;
        }


        const std::string& Slot::getSessionTokenOfSender() const {
            return m_sessionTokenOfSender;
        }


        karabo::util::Hash::Pointer Slot::getHeaderOfSender() const {
            return m_headerOfSender;
        }


        void Slot::extractSenderInformation(const karabo::util::Hash& header) {
            boost::optional<const Hash::Node&> node = header.find("userId");
            if (node) m_userIdOfSender = node->getValue<std::string>();
            node = header.find("accessLevel");
            if (node) m_accessLevelOfSender = node->getValue<std::string>();
            node = header.find("signalInstanceId");
            if (node) m_instanceIdOfSender = node->getValue<std::string>();
            node = header.find("sessionToken");
            if (node) m_sessionTokenOfSender = node->getValue<std::string>();

            m_headerOfSender = boost::make_shared<karabo::util::Hash>(header);
        }


        void Slot::invalidateSenderInformation() {
            m_userIdOfSender = "";
            m_accessLevelOfSender = "";
            m_instanceIdOfSender = "";
            m_sessionTokenOfSender = "";
            m_headerOfSender.reset();
        }


        void Slot::callRegisteredSlotFunctions(const Hash& header, const Hash& body) {
            try {
                // Mutex lock prevents that the same slot of a SignalSlotable can be called concurrently.
                // SignalSlotable takes care that messages are sequentially processed per sender
                // (with one common sequence for broadcast events), but then calls to the same slot from different
                // senders can run in parallel which was guaranteed not to happen from the Karabo beginning.
                // If we give up that guarantee, the caching in extractSenderInformation(..) has to be made thread
                // safe in another way - and the protection not to add function while being called as well.
                boost::mutex::scoped_lock lock(m_registeredSlotFunctionsMutex);
                extractSenderInformation(header);
                doCallRegisteredSlotFunctions(body);
                invalidateSenderInformation();
            } catch (const karabo::util::CastException& e) {
                invalidateSenderInformation();
                KARABO_RETHROW_AS(KARABO_SIGNALSLOT_EXCEPTION("Received incompatible argument(s) for slot \"" +
                                                              m_slotFunction + "\"."));
            } catch (const std::exception& e) {
                invalidateSenderInformation();
                KARABO_RETHROW_AS(KARABO_SIGNALSLOT_EXCEPTION("Error in slot \"" + m_slotFunction + "\""));
            } catch (...) {
                invalidateSenderInformation();
                KARABO_RETHROW;
            }
        }
    } // namespace xms
} // namespace karabo
