/*
 * $Id: Slot.cc 2810 2011-01-07 10:36:58Z wrona $
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on August 19, 2010, 4:49 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "SignalSlotable.hh"
#include "Slot.hh"

using namespace karabo::util;

namespace karabo {

    namespace xms {


        const std::string& Slot::getInstanceIdOfSender() const {
            return m_instanceIdOfSender;
        }


        const std::string& Slot::getRoleIdOfSender() const {
            return m_roleOfSender;
        }


        const std::string& Slot::getUserIdOfSender() const {
            return m_userIdOfSender;
        }


        const std::string& Slot::getSessionTokenOfSender() const {
            return m_sessionTokenOfSender;
        }


        void Slot::handlePossibleReply(const karabo::util::Hash& header) {
            std::pair<bool, karabo::util::Hash> possibleReply = m_signalSlotable->digestPotentialReply();
            if (possibleReply.first) {
                if (header.has("replyTo")) {
                    karabo::util::Hash replyHeader("replyFrom", header.get<std::string > ("replyTo"));
                    m_channel->write(possibleReply.second, replyHeader);
                }
            }
        }


        void Slot::extractSenderInformation(const karabo::util::Hash& header) {
            boost::optional<const Hash::Node&> node = header.find("userId");
            if (node) m_userIdOfSender = node->getValue<string>();
            node = header.find("role");
            if (node) m_roleOfSender = node->getValue<string>();
            node = header.find("signalInstanceId");
            if (node) m_instanceIdOfSender = node->getValue<string>();
            node = header.find("sessionToken");
            if (node) m_sessionTokenOfSender = node->getValue<string>();
        }


        void Slot::invalidateSenderInformation() {
            m_userIdOfSender = "";
            m_roleOfSender = "";
            m_instanceIdOfSender = "";
            m_sessionTokenOfSender = "";
        }

        //        void Slot::startSlotProcessing() {
        //            m_signalSlotable->setSlotProcessingFlag(true);
        //        }

        //        void Slot::stopSlotProcessing() {
        //            m_signalSlotable->setSlotProcessingFlag(false);
        //        }


        template <>
        const bool& Slot::getAndCast(const std::string& key, const karabo::util::Hash& hash) const {
            //std::cout << "int -> boolean caster in action..." << std::endl;
            karabo::util::Hash::Node node;
            try {
                node = hash.getNode(key);
            } catch (...) {
                KARABO_RETHROW_AS(KARABO_PARAMETER_EXCEPTION("Slot was called with to less arguments"));
            }
            if (node.is<int>()) {
                // TODO Check portability
                const bool& b = *(reinterpret_cast<const bool*> (&(node.getValue<int>())));
                //std::cout << "casting integer to boolean, value: " << b << std::endl;
                return b;
            } else {
                return node.getValue<bool>();
            }
        }
    }
}
