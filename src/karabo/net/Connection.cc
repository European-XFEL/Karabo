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

#include <karabo/io/Format.hh>

#include "Connection.hh"

namespace karabo {
    namespace net {

        using namespace karabo::util;

        void Connection::expectedParameters(Schema& expected) {

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

        void Connection::configure(const karabo::util::Hash& input) {

            m_hashFormat = HashFormat::createChoice("hashSerialization", input);

            if (input.has("IOService")) {
                m_service.reset();
                input.get("IOService", m_service);
            } else {
                m_service = IOService::Pointer(new IOService);
            }
            m_sizeofLength = 4;
            if (input.has("sizeofLength")) {
                input.get("sizeofLength", m_sizeofLength);
            }
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

        void Connection::hashToString(const karabo::util::Hash& hash, std::string& serializedHash) {
            std::stringstream out;
            try {
                m_hashFormat->convert(hash, out);
            } catch (const karabo::util::Exception& e) {
                std::cout << e << std::endl;
            }
            
            serializedHash.assign(out.str());
        }

        void Connection::stringToHash(const std::string& serializedHash, karabo::util::Hash& hash) {
            std::stringstream in(serializedHash);
            m_hashFormat->convert(in, hash);
        }

    } // namespace net
} // namespace karabo

