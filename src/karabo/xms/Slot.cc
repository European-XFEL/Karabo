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

namespace exfel {
    namespace xms {

        void Slot::handlePossibleReply(const exfel::util::Hash& header) {
            std::pair<bool, exfel::util::Hash> possibleReply = m_signalSlotable->digestPotentialReply();
            if (possibleReply.first) {
                if (header.has("replyTo")) {
                    exfel::util::Hash replyHeader("replyFrom", header.get<std::string > ("replyTo"));
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
        const bool& Slot::getAndCast(const std::string& key, const exfel::util::Hash& hash) const {
            //std::cout << "int -> boolean caster in action..." << std::endl;
            exfel::util::Hash::const_iterator it = hash.find(key);
            if (it != hash.end()) {
                if (hash.is<int>(it)) {
                    // TODO Check portability
                    const bool& b = *(reinterpret_cast<const bool*> (&(hash.get<int>(it))));
                    //std::cout << "casting integer to boolean, value: " << b << std::endl;
                    return b;
                } else return hash.get<bool>(it);
            } else {
                throw PARAMETER_EXCEPTION("Slot was called with to less arguments");
            }
        }
    } // namespace xms
} // namespace exfel
