/*
 * $Id: Connection.cc 3392 2011-04-28 12:49:18Z heisenb@DESY.DE $
 *
 * File:   Connection.cc
 * Author: WP76 <wp76@xfel.eu>
 *
 * Created on November 2, 2010, 9:47 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "Connection.hh"
#include "Channel.hh"
#include <karabo/util/SimpleElement.hh>

namespace karabo {
    namespace net {

        using namespace karabo::util;

        void Connection::expectedParameters(Schema& expected) {

            STRING_ELEMENT(expected)
                    .key("serializationType")
                    .displayedName("Serialization Type")
                    .description("Decides whether the serialization type for objects will be binary or text")
                    .options("text binary")
                    .assignmentOptional().defaultValue("text")
                    .init()
                    .commit();
        }

        Connection::Connection(const karabo::util::Hash& input) {

            input.get("serializationType", m_serializationType);

            // Always create an IOService object
            m_service = IOService::Pointer(new IOService);
        }

        IOService::Pointer Connection::getIOService() const {
            return m_service;
        }

        void Connection::setIOService(const IOService::Pointer& ioService) {
            m_service = ioService;
        }

        void Connection::setIOServiceType(const std::string& serviceType) {
            m_service->setService(serviceType);
        }

    }
}

