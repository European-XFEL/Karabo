/*
 * $Id: BrokerConnection.cc 6930 2012-08-03 10:45:21Z heisenb $
 *
 * Author: <your.email@xfel.eu>
 *
 * Created on November 2, 2010, 9:47 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "BrokerConnection.hh"

#include <karabo/io/TextSerializer.hh>
#include <karabo/io/BinarySerializer.hh>
#include <karabo/util/SimpleElement.hh>

namespace karabo {
    namespace net {

        using namespace karabo::util;


        void BrokerConnection::expectedParameters(Schema& expected) {

            STRING_ELEMENT(expected)
                    .key("serializationType")
                    .displayedName("Serialization Type")
                    .description("Decides whether the serialization type for objects will be binary or text")
                    .options("text binary")
                    .assignmentOptional().defaultValue("binary")
                    .init()
                    .commit();
        }


        BrokerConnection::BrokerConnection(const karabo::util::Hash& input) {
            input.get("serializationType", m_serializationType);
            m_service = BrokerIOService::Pointer(new BrokerIOService);
        }


        BrokerIOService::Pointer BrokerConnection::getIOService() const {
            return m_service;
        }


        void BrokerConnection::setIOService(const BrokerIOService::Pointer& ioService) {
            m_service = ioService;
        }


        void BrokerConnection::setIOServiceType(const std::string& serviceType) {
            m_service->setService(serviceType);
        }

    } // namespace net
} // namespace karabo

