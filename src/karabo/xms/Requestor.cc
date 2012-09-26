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

using namespace std;

namespace karabo {
    namespace xms {
        
        boost::uuids::random_generator Requestor::m_uuidGenerator;
      
            Requestor& Requestor::setResponder(const std::string& slotInstanceId, const std::string& slotFunction) {
                m_header.set("slotInstanceId", slotInstanceId + "|");
                m_header.set("slotFunction", slotFunction + "|");
                m_header.set("hostName", boost::asio::ip::host_name());
                m_header.set("replyTo", m_replyId);
                m_header.set("classId", "Requestor");
                m_channel->setFilter("replyFrom = '" + m_replyId + "'");
                return *this;
            }

    }
}

