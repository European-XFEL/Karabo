/*
 * $Id$
 *
 * File:   Requestor.cc
 * Author: <your.email@xfel.eu>
 *
 * Created on February 3, 2012, 3:39 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "SignalSlotable.hh"

using namespace std;

namespace karabo {
    namespace xms {

        boost::uuids::random_generator Requestor::m_uuidGenerator;
        
        
        Requestor::Requestor(SignalSlotable* signalSlotable) :
        m_signalSlotable(signalSlotable), m_replyId(generateUUID()), m_isRequested(false), m_isReceived(false) {
        }

        
        Requestor& Requestor::timeout(const int& milliseconds) {
            m_timeout = milliseconds;
            return *this;
        }


        karabo::util::Hash Requestor::prepareHeader(const std::string& slotInstanceId, const std::string& slotFunction) {
            karabo::util::Hash header;
            header.set("replyTo", m_replyId);
            header.set("signalInstanceId", m_signalSlotable->getInstanceId());
            header.set("signalFunction", "__request__");
            header.set("slotInstanceIds", "|" + slotInstanceId + "|");
            header.set("slotFunctions", "|" + slotInstanceId + ":" + slotFunction + "|");
            header.set("hostName", boost::asio::ip::host_name());
            header.set("userName", m_signalSlotable->getUserName());
            return header;
        }
        
        
        karabo::util::Hash Requestor::prepareHeaderNoWait(const std::string& requestSlotInstanceId, 
                const std::string& requestSlotFunction, 
                const std::string& replySlotInstanceId, 
                const std::string& replySlotFunction) {
            
            karabo::util::Hash header;
            header.set("replyInstanceIds", "|" + replySlotInstanceId + "|");
            header.set("replyFunctions", "|" + replySlotInstanceId + ":" + replySlotFunction + "|");
            header.set("signalInstanceId", m_signalSlotable->getInstanceId());
            header.set("signalFunction", "__requestNoWait__");
            header.set("slotInstanceIds", "|" + requestSlotInstanceId + "|");
            header.set("slotFunctions", "|" + requestSlotInstanceId + ":" + requestSlotFunction + "|");
            header.set("hostName", boost::asio::ip::host_name());
            header.set("userName", m_signalSlotable->getUserName());
            return header;

        }


        void Requestor::registerRequest() {
            if (m_isRequested) throw KARABO_SIGNALSLOT_EXCEPTION("You have to receive an answer before you can send a new request");
            m_isRequested = true;
            m_isReceived = false;
        }

        std::string Requestor::generateUUID() {
            return boost::uuids::to_string(m_uuidGenerator());
        }

        void Requestor::sendRequest(const karabo::util::Hash& header, const karabo::util::Hash& body) const {
            try {
                m_signalSlotable->m_producerChannel->write(header, body);
            } catch (...) {
                KARABO_RETHROW_AS(KARABO_NETWORK_EXCEPTION("Problems sending request"));
            }
        }

        void Requestor::receiveResponse(karabo::util::Hash::Pointer& header, karabo::util::Hash::Pointer& body) {
            if (!m_signalSlotable->timedWaitAndPopReceivedReply(m_replyId, header, body, m_timeout)) {
                throw KARABO_TIMEOUT_EXCEPTION("Reply timed out");
            }
            
            m_isReceived = true;
            m_isRequested = false;
        }        
    }
}

