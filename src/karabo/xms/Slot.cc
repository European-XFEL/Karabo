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

namespace karabo {
    namespace xms {

        void Slot::handlePossibleReply(const karabo::util::Hash& header) {
            std::pair<bool, karabo::util::Hash> possibleReply = m_signalSlotable->digestPotentialReply();
            if (possibleReply.first) {
                if (header.has("replyTo")) {
                    karabo::util::Hash replyHeader("replyFrom", header.get<std::string > ("replyTo"));
                    m_channel->write(possibleReply.second, replyHeader);
                }
            }
        }
        
        void Slot::startSlotProcessing() {
            m_signalSlotable->setSlotProcessingFlag(true);
        }
        
        void Slot::stopSlotProcessing() {
            m_signalSlotable->setSlotProcessingFlag(false);
        }
        
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
