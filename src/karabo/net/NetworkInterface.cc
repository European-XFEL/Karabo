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
 * File:   NetworkInterface.cc
 * Author: <jose.vazquez@xfel.eu>
 *
 */
#include "NetworkInterface.hh"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <algorithm>
#include <cassert>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <optional>
#include <regex>
#include <stdexcept>
#include <string>
#include <vector>

#include "karabo/data/types/Exception.hh"

namespace {

    std::uint32_t presentation2network(const std::string&);
    std::string network2presentation(std::uint32_t) noexcept;
    std::regex wildcard2regex(std::string);
    std::uint32_t bitmask(int) noexcept;

    /**
     * Struct to be returned by parseCIDRFormat. (Seems nicer than
     * returning a tuple).
     */
    struct CIDRAddress {
        CIDRAddress() = delete;
        CIDRAddress(std::string a, std::uint32_t b, int m)
            : m_presentationAddress(std::move(a)), m_binaryAddress(b), m_mask(m) {}

        std::string m_presentationAddress;
        std::uint32_t m_binaryAddress;
        int m_mask;
    };

    std::optional<CIDRAddress> parseCIDRFormat(const std::string&);
    bool isLoopback(const std::string& ip);

} // namespace


namespace karabo::net {

    NetworkInterface::NetworkInterface(const std::string& name_or_ip, bool exclude_loopback) {
        // cidr is std::optional. If empty, it means a name (with or without
        // wildcards) must have been specified in 'name_or_ip'
        auto cidr = parseCIDRFormat(name_or_ip);
        auto regex_name = wildcard2regex(name_or_ip);

        if (exclude_loopback &&
            ((cidr && isLoopback(cidr->m_presentationAddress)) || std::regex_match("lo", regex_name))) {
            throw KARABO_LOGIC_EXCEPTION("loopback addresses are not allowed");
        }

        struct ifaddrs* ifap{};
        if (getifaddrs(&ifap) == -1) {
            throw KARABO_LOGIC_EXCEPTION(std::string{"Error fetching host addresses: "} + strerror(errno));
        }

        bool found{false};
        const std::uint32_t mask = cidr ? bitmask(cidr->m_mask) : 0UL;
        for (auto ifa = ifap; ifa; ifa = ifa->ifa_next) {
            // Only IPv4 addresses are supported for now
            if (ifa->ifa_addr->sa_family != AF_INET) {
                continue;
            }

            const std::uint32_t s_addr = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr)->sin_addr.s_addr;
            if ((cidr &&
                 ((cidr->m_binaryAddress & mask) == (s_addr & mask))) || // name_or_ip is an IP, and it is within range
                std::regex_match(ifa->ifa_name, regex_name)) // name_or_ip is a name of one of the host interfaces
            {
                constructFromInterface(ifa);
                found = true;
                break;
            }
        }

        freeifaddrs(ifap);

        if (!found) {
            throw KARABO_LOGIC_EXCEPTION("No interface associated to '" + name_or_ip + "' exists on the host");
        }
    }

    /**
     * Fill the object fields with the information specified in the ifaddrs object
     * passed as parameter.
     */
    void NetworkInterface::constructFromInterface(const struct ifaddrs* ifa) noexcept {
        // The assertion must be always true, since this function is called from the loop
        // in the constructor. Sanity check
        assert(ifa->ifa_addr->sa_family == AF_INET);

        m_name = ifa->ifa_name;

        auto s_addr = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr)->sin_addr.s_addr;
        m_binaryAddress = s_addr;
        m_presentationAddress = network2presentation(s_addr);
    }


    /**
     * Send a string representation of the NetworkInterface object to a stream.
     * (Mostly for debug purposes)
     */
    std::ostream& operator<<(std::ostream& os, const NetworkInterface& ni) {
        return os << ni.name() << ": " << ni.presentationIP();
    }

} // namespace karabo::net


namespace {

    /**
     * Convert a binary IP address to
     * presentation format.
     *
     * @param addr the binary address, **in network byte ordering**.
     * @return a string with the IP address in presentation format
     *         (for instance, "192.168.0.1")
     *
     */
    std::string network2presentation(std::uint32_t addr) noexcept {
        // INET6_ADDRSTRLEN is overkill, since we use only IPv4 addresses,
        // but this avoids potential bugs if in the future IP6 addresses
        // are incorporated
        char presentation_ip[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET, &addr, presentation_ip, INET6_ADDRSTRLEN);
        return presentation_ip;
    }

    /**
     * Convert an address in presentation format to binary.
     *
     * @param ip the address in presentation format (for instance,
     *           "192.168.0.1")
     *
     * @return the binary address, **in network byte ordering**.
     */
    std::uint32_t presentation2network(const std::string& ip) {
        struct in_addr address;

        if (inet_pton(AF_INET, ip.data(), &address) != 1) {
            throw KARABO_LOGIC_EXCEPTION(ip + " is not a valid address");
        }

        return address.s_addr;
    }

    /**
     * Convert a string with wildcards into a regular expression:
     *
     * - A '*' is converted into '.*'
     * - A '?' is converted into '.?'
     */
    std::regex wildcard2regex(std::string str) {
        std::string::size_type pos = 0;
        while ((pos = str.find_first_of("?*", pos)) != std::string::npos) {
            str.insert(pos, ".");
            // 2 to start looking after the last
            // found character
            pos += 2;
        }

        return std::regex{str};
    }

    /**
     * Parse a string with an address in CIDR format into its components:
     *   - Address in presentation format.
     *   - Address in binary format, **in host byte ordering**.
     *   - Mask. If a mask is not specified, the returned value is 32.
     *
     * CIDR format specifies an address as four numbers between 0 and 255
     * separated with the character '.', plus an optional mask (a number
     * between 0 and 32) separated by the character '/'. Examples:
     *
     *    127.0.0.1
     *    192.168.0.1/24
     *
     * Note that we do not interpret the mask as an acutal mask, but as an
     * indicator as to how many bits we use to specify a range of addresses.
     *
     * @return an optional that contains a struct with the result of parsing 'address',
     *         if it can be parsed, std::nullopt otherwise.
     */
    std::optional<CIDRAddress> parseCIDRFormat(const std::string& address) {
        static const std::regex cidrFormat{R"([0-9]{1,4}\.[0-9]{1,4}\.[0-9]{1,4}\.[0-9]{1,4}(/[0-9]{1,2})?)"};

        if (!std::regex_match(address.data(), cidrFormat)) {
            return std::nullopt;
        }

        // Retrieve the mask, if any.
        auto slash = address.find('/');
        int mask{32};
        if (slash != std::string_view::npos) {
            mask = std::atoi(address.substr(slash + 1).c_str());
            if (mask < 0 || mask > 32) {
                throw KARABO_LOGIC_EXCEPTION("Address mask must be between 0 and 32");
            }
        }

        auto binary_address = presentation2network(address.substr(0, slash));
        if (binary_address & ~bitmask(mask)) {
            throw KARABO_LOGIC_EXCEPTION(address + " has host bits set");
        }

        return CIDRAddress{address.substr(0, slash), binary_address, mask};
    }

    /**
     * Return a 32-bit field with its first n bits set to 1, and the
     * rest set to 0, **in network order**.
     */
    std::uint32_t bitmask(int n) noexcept {
        return htonl(~((1 << (32 - n)) - 1));
    }

    /**
     * Any address in the range 127.0.0.0 - 127.255.255.255 is reserved for the loopback.
     * (https://www.geeksforgeeks.org/how-to-find-a-loopback-address)
     */
    bool isLoopback(const std::string& ip) {
        return ip.find("127.") == 0;
    }

} // namespace
