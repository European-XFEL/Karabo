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
 * File:   NetworkInterface.hh
 * Author: <jose.vazquez@xfel.eu>
 *
 */

#ifndef KARABO_UTIL_NETWORK_INTERFACE_HH
#define KARABO_UTIL_NETWORK_INTERFACE_HH

#include <arpa/inet.h>
#include <ifaddrs.h>

#include <cstdint>
#include <fstream>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace karabo::net {

    class NetworkInterface {
       public:
        /**
         * Construct a NetworkInterface object from an interface or IP address.
         */
        NetworkInterface(const std::string& name_or_ip, bool exclude_loopback = true);

        ~NetworkInterface() = default;

        NetworkInterface(const NetworkInterface&) = default;
        NetworkInterface(NetworkInterface&&) = default;
        NetworkInterface& operator=(const NetworkInterface&) = default;
        NetworkInterface& operator=(NetworkInterface&&) = default;

        /**
         * Return the interface name for the object (for instance, 'lo'
         * or 'enp4s0')
         */
        inline const std::string& name() const noexcept {
            return m_name;
        }

        /**
         * Return the presentation address for the object.
         *
         * The presentation address is the IP address (four numbers between
         * 0 and 255, separated with '.')
         */
        inline const std::string& presentationIP() const noexcept {
            return m_presentationAddress;
        }

       private:
        std::string m_name;
        std::string m_presentationAddress;
        std::uint32_t m_binaryAddress;

        void constructFromInterface(const struct ifaddrs*) noexcept;
    };

    // Send an address to a stream (mostly for testing purposes)
    std::ostream& operator<<(std::ostream& os, const NetworkInterface& ni);

} // namespace karabo::net

#endif
