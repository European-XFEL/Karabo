/*
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */
/*
 * File:   NetworkInterface_Test.cc
 * Author: <jose.vazquez@xfel.eu>
 *
 */

#include "NetworkInterface_Test.hh"

#include <cppunit/TestAssert.h>
#include <netinet/in.h>

// #include <karabo/util.hpp>
//  #include <karabo/net.hpp>
#include <karabo/net/NetworkInterface.hh>

#include "karabo/data/types/Exception.hh"

using namespace std;
using namespace karabo::net;

CPPUNIT_TEST_SUITE_REGISTRATION(NetworkInterface_Test);

std::string findHostInterface() {
    struct ifaddrs* ifap{nullptr};

    if (getifaddrs(&ifap) == -1) {
        throw std::runtime_error{std::string{"Error fetching host addresses: "} + strerror(errno)};
    }

    for (auto ifa = ifap; ifa; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr->sa_family != AF_INET) {
            continue;
        }

        char presentation_ip[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET, &(reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr)->sin_addr), presentation_ip,
                  INET6_ADDRSTRLEN);

        if (std::string{presentation_ip}.find("127.") != 0) {
            freeifaddrs(ifap);
            return presentation_ip;
        }
    }

    freeifaddrs(ifap);
    return {};
}

NetworkInterface_Test::NetworkInterface_Test() {}


NetworkInterface_Test::~NetworkInterface_Test() {}

void NetworkInterface_Test::setUp() {}
void NetworkInterface_Test::tearDown() {}


void NetworkInterface_Test::testRightInAddrT() {
    CPPUNIT_ASSERT(typeid(uint32_t) == typeid(in_addr_t));
}


void NetworkInterface_Test::testConstructor() {
    std::unique_ptr<NetworkInterface> interface;
    CPPUNIT_ASSERT_NO_THROW(interface.reset(new NetworkInterface{"127.0.0.1", false}));
    CPPUNIT_ASSERT_EQUAL(std::string{"127.0.0.1"}, interface->presentationIP());
    CPPUNIT_ASSERT_EQUAL(std::string{"lo"}, interface->name());

    CPPUNIT_ASSERT_NO_THROW(interface.reset(new NetworkInterface{"l*", false}));
    CPPUNIT_ASSERT_EQUAL(std::string{"127.0.0.1"}, interface->presentationIP());
    CPPUNIT_ASSERT_EQUAL(std::string{"lo"}, interface->name());

    CPPUNIT_ASSERT_THROW(interface.reset(new NetworkInterface{"adkdf;aosidj"}), karabo::data::LogicException);
    CPPUNIT_ASSERT_THROW(interface.reset(new NetworkInterface{"127.0.0.1"}), karabo::data::LogicException);
    CPPUNIT_ASSERT_THROW(interface.reset(new NetworkInterface{"lo"}), karabo::data::LogicException);
    CPPUNIT_ASSERT_THROW(interface.reset(new NetworkInterface{"l?"}), karabo::data::LogicException);

    std::string ip = findHostInterface();
    if (ip.length() == 0) return;

    CPPUNIT_ASSERT_NO_THROW(interface.reset(new NetworkInterface{ip}));
    CPPUNIT_ASSERT_EQUAL(ip, interface->presentationIP());

    string copy{ip};
    auto dot = copy.rfind('.');
    copy.erase(dot);

    CPPUNIT_ASSERT_NO_THROW(interface.reset(new NetworkInterface{copy + ".0/24"}));
    CPPUNIT_ASSERT_EQUAL(ip, interface->presentationIP());

    CPPUNIT_ASSERT_THROW(interface.reset(new NetworkInterface{copy + ".0/8"}), karabo::data::LogicException);
}
