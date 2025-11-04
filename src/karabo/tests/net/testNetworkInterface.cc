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
 * File:   NetworkInterface_Test.hh
 * Author: <jose.vazquez@xfel.eu>
 *
 */

#include <gtest/gtest.h>
#include <netinet/in.h>

#include "karabo/data/types/Exception.hh"
#include "karabo/net/NetworkInterface.hh"

using namespace std;
using namespace karabo::net;


static std::string findHostInterface() {
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


TEST(TestNetworkInterface, testRightInAddrT) {
    EXPECT_TRUE(typeid(uint32_t) == typeid(in_addr_t));
}


TEST(TestNetworkInterface, testConstructor) {
    std::unique_ptr<NetworkInterface> interface;
    EXPECT_NO_THROW(interface.reset(new NetworkInterface{"127.0.0.1", false}));
    EXPECT_EQ("127.0.0.1", interface->presentationIP());
    EXPECT_EQ("lo", interface->name());

    EXPECT_NO_THROW(interface.reset(new NetworkInterface{"l*", false}));
    EXPECT_EQ("127.0.0.1", interface->presentationIP());
    EXPECT_EQ("lo", interface->name());

    EXPECT_THROW(interface.reset(new NetworkInterface{"adkdf;aosidj"}), karabo::data::LogicException);
    EXPECT_THROW(interface.reset(new NetworkInterface{"127.0.0.1"}), karabo::data::LogicException);
    EXPECT_THROW(interface.reset(new NetworkInterface{"lo"}), karabo::data::LogicException);
    EXPECT_THROW(interface.reset(new NetworkInterface{"l?"}), karabo::data::LogicException);

    std::string ip = ::findHostInterface();
    if (ip.length() == 0) return;

    EXPECT_NO_THROW(interface.reset(new NetworkInterface{ip}));
    EXPECT_EQ(ip, interface->presentationIP());

    string copy{ip};
    auto dot = copy.rfind('.');
    copy.erase(dot);

    EXPECT_NO_THROW(interface.reset(new NetworkInterface{copy + ".0/24"}));
    EXPECT_EQ(ip, interface->presentationIP());

    EXPECT_THROW(interface.reset(new NetworkInterface{copy + ".0/8"}), karabo::data::LogicException);
}
