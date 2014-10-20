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


#include "Requestor.hh"
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
                m_signalSlotable->m_producerChannel->write(header, body, 9);
            } catch (...) {
                KARABO_RETHROW_AS(KARABO_NETWORK_EXCEPTION("Problems sending request"));
            }
        }

        void Requestor::receiveResponse(karabo::util::Hash::Pointer& header, karabo::util::Hash::Pointer& body) {
            
            //if (m_isReceived) throw KARABO_SIGNALSLOT_EXCEPTION("You have to send a request before you can receive a response");
            int currentWaitingTime = 0;
            while (!m_signalSlotable->hasReceivedReply(m_replyId) && currentWaitingTime < m_timeout) {
                boost::this_thread::sleep(boost::posix_time::milliseconds(2));
                currentWaitingTime += 2;
            }
            if (currentWaitingTime >= m_timeout) {
                throw KARABO_TIMEOUT_EXCEPTION("Reply timed out");
            }
            
            m_signalSlotable->popReceivedReply(m_replyId, header, body);
            
            m_isReceived = true;
            m_isRequested = false;
        }        
    }
}

