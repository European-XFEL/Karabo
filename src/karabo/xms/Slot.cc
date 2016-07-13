/*
 * $Id: Slot.cc 2810 2011-01-07 10:36:58Z wrona $
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on August 19, 2010, 4:49 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
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


        void Slot::extractSenderInformation(const karabo::util::Hash& header) {
            boost::optional<const Hash::Node&> node = header.find("userId");
            if (node) m_userIdOfSender = node->getValue<string>();
            node = header.find("accessLevel");
            if (node) m_accessLevelOfSender = node->getValue<string>();
            node = header.find("signalInstanceId");
            if (node) m_instanceIdOfSender = node->getValue<string>();
            node = header.find("sessionToken");
            if (node) m_sessionTokenOfSender = node->getValue<string>();
        }


        void Slot::invalidateSenderInformation() {
            m_userIdOfSender = "";
            m_accessLevelOfSender = "";
            m_instanceIdOfSender = "";
            m_sessionTokenOfSender = "";
        }


        void Slot::callRegisteredSlotFunctions(const Hash &header, const Hash &body) {
            try {
                boost::mutex::scoped_lock lock(m_callRegisteredSlotFunctionsMutex);
                extractSenderInformation(header);
                doCallRegisteredSlotFunctions(body);
                invalidateSenderInformation();
            } catch (const karabo::util::CastException& e) {
                invalidateSenderInformation();
                KARABO_RETHROW_AS(KARABO_SIGNALSLOT_EXCEPTION("Received incompatible argument (see above) for slot \"" + m_slotFunction + "\". Check your connection!"));
            } catch (const karabo::util::Exception& e) {
                invalidateSenderInformation();
                KARABO_RETHROW_AS(KARABO_SIGNALSLOT_EXCEPTION("An exception was thrown in slot \"" + m_slotFunction + "\""));
            } catch (const std::exception& e) {
                invalidateSenderInformation();
                std::string msg(e.what());
                KARABO_RETHROW_AS(KARABO_SIGNALSLOT_EXCEPTION(((msg += " was thrown in slot \"") += m_slotFunction) += "\""));
            } catch (...) {
                invalidateSenderInformation();
                KARABO_RETHROW;
            }
        }
    }
}
