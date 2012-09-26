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

#include <karabo/io/Format.hh>

namespace karabo {
    namespace net {

        using namespace karabo::util;

        void BrokerConnection::expectedParameters(Schema& expected) {

            CHOICE_ELEMENT<karabo::io::Format<karabo::util::Hash> > (expected)
                    .key("hashSerialization")
                    .displayedName("Hash Serialization")
                    .description("Decides which protocol should be used in order to transfer a Hash object")
                    .assignmentOptional().defaultValue("Bin")
                    .init()
                    .commit();

            INTERNAL_ANY_ELEMENT(expected)
                    .key("IOService")
                    .description("IO service object")
                    .commit();

            UINT32_ELEMENT(expected)
                    .key("sizeofLength")
                    .displayedName("Size of Message Length")
                    .description("The size of messageLength field in communication protocol")
                    .assignmentOptional().defaultValue(4)
                    .init()
                    .commit();
        }

        void BrokerConnection::configure(const karabo::util::Hash& input) {

            m_hashFormat = HashFormat::createChoice("hashSerialization", input);

            if (input.has("IOService")) {
                m_service.reset();
                input.get("IOService", m_service);
            } else {
                m_service = BrokerIOService::Pointer(new BrokerIOService);
            }
            m_sizeofLength = 4;
            if (input.has("sizeofLength")) {
                input.get("sizeofLength", m_sizeofLength);
            }
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

        void BrokerConnection::hashToString(const karabo::util::Hash& hash, std::string& serializedHash) {
            std::stringstream out;
            try {
                m_hashFormat->convert(hash, out);
            } catch (const karabo::util::Exception& e) {
                std::cout << e << std::endl;
            }
            
            serializedHash.assign(out.str());
        }

        void BrokerConnection::stringToHash(const std::string& serializedHash, karabo::util::Hash& hash) {
            std::stringstream in(serializedHash);
            m_hashFormat->convert(in, hash);
        }

    } // namespace net
} // namespace karabo

