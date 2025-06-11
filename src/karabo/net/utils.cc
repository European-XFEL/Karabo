/*
 * $Id$
 *
 * Author: gero.flucke@xfel.eu>
 *
 * Created on February 11, 2016
 *
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

#include "karabo/net/utils.hh"

#include <arpa/inet.h>
#include <ifaddrs.h>

#include <boost/asio.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/network_v4.hpp>
#include <iostream>
#include <regex>

#include "karabo/data/types/Exception.hh"
#include "karabo/data/types/StringTools.hh"
#include "karabo/log/Logger.hh"

using namespace std;
using namespace boost;
namespace ip = boost::asio::ip;

std::string karabo::net::bareHostName() {
    std::string hostName = ip::host_name();

    // Find first dot and erase everything after it:
    const std::string::size_type dotPos = hostName.find('.');
    if (dotPos != std::string::npos) hostName.erase(dotPos);

    return hostName;
}


void karabo::net::runProtected(std::shared_ptr<boost::asio::io_context> service, const std::string& category,
                               const std::string& errorMessage, unsigned int delayInMilliSec) {
    // See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference/io_context.html:
    // "If an exception is thrown from a handler, the exception is allowed to propagate through the throwing
    //  thread's invocation of run(), run_one(), poll() or poll_one(). No other threads that are calling any of
    //  these functions are affected. It is then the responsibility of the application to catch the exception.
    //
    //  After the exception has been caught, the run(), run_one(), poll() or poll_one() call may be restarted
    //  without the need for an intervening call to reset(). This allows the thread to rejoin the io_context
    //  object's thread pool without impacting any other threads in the pool."

    const std::string fullMessage(" when running io_context (" + errorMessage + "), continue in " +
                                        karabo::data::toString(delayInMilliSec) += " ms");
    while (true) {
        bool caught = true;
        try {
            service->run();
            caught = false; // run exited normally
            break;
        } catch (karabo::data::Exception& e) {
            KARABO_LOG_FRAMEWORK_ERROR_C(category) << "Exception" << fullMessage << ": " << e;
        } catch (std::exception& e) {
            KARABO_LOG_FRAMEWORK_ERROR_C(category) << "Standard exception" << fullMessage << ": " << e.what();
        } catch (...) {
            KARABO_LOG_FRAMEWORK_ERROR_C(category) << "Unknown exception" << fullMessage << ".";
        }
        if (caught) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delayInMilliSec));
        }
    }
}


std::tuple<std::string, std::string> karabo::net::parseGenericUrl(const std::string& url) {
    std::regex ex("([^:]+):(?://)?(.+)");
    std::cmatch what;
    string scheme;
    string schemeSpecific;
    if (regex_match(url.c_str(), what, ex)) {
        scheme = string(what[1].first, what[1].second);
        schemeSpecific = string(what[2].first, what[2].second);
    }
    return std::make_tuple(scheme, schemeSpecific);
}


std::tuple<std::string, std::string, std::string, std::string, std::string> karabo::net::parseUrl(
      const std::string& url) {
    const std::tuple<std::string, std::string> parsed_url = karabo::net::parseGenericUrl(url);
    const string& scheme = std::get<0>(parsed_url);
    const string& schemeDependent = std::get<1>(parsed_url);

    std::regex ex("([^/ :]+):?([^/ ]*)(/?[^ #?]*)\\x3f?([^ #]*)#?([^ ]*)");
    std::cmatch what;
    string domain;
    string port;
    string path;
    string query;

    if (scheme.size() > 0 && schemeDependent.size() && regex_match(schemeDependent.c_str(), what, ex)) {
        domain = string(what[1].first, what[1].second);
        port = string(what[2].first, what[2].second);
        path = string(what[3].first, what[3].second);
        query = string(what[4].first, what[4].second);
    }
    return std::make_tuple(scheme, domain, port, path, query);
}


string karabo::net::urlencode(const std::string& value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (std::string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
        std::string::value_type c = (*i);

        // Keep alphanumeric and other accepted characters intact
        if (c == ' ') {
            escaped << '+';
            continue;
        } else if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        }

        // Any other characters are percent-encoded
        escaped << std::uppercase;
        escaped << '%' << std::setw(2) << int(static_cast<unsigned char>(c));
        escaped << std::nouppercase;
    }

    return escaped.str();
}

std::string karabo::net::getIpFromCIDRNotation(const std::string& input) {
    boost::system::error_code ec;
    ip::network_v4 subnet = ip::make_network_v4(input, ec);
    std::string res = input;
    if (ec) {
        return res;
    }
    ip::address_v4_range range = subnet.hosts();

    // adapted from https://stackoverflow.com/a/62303963 by tstenner CC BY-SA 4.0
    ifaddrs* ifs;
    if (getifaddrs(&ifs)) {
        return res;
    }
    for (auto ifaddr = ifs; ifaddr != nullptr; ifaddr = ifaddr->ifa_next) {
        // No address? Skip.
        if (ifaddr->ifa_addr == nullptr) continue;

        // Interface isn't active? Skip.
        if (!(ifaddr->ifa_flags & IFF_UP)) continue;

        if (ifaddr->ifa_addr->sa_family == AF_INET) {
            ip::address_v4 addr =
                  ip::address_v4(htonl(reinterpret_cast<sockaddr_in*>(ifaddr->ifa_addr)->sin_addr.s_addr));
            // not a valid address
            if (addr.is_unspecified() || addr.is_loopback()) {
                continue;
            }
            // found the address
            if (range.find(addr) != range.end()) {
                res = addr.to_string();
                break;
            }
        }
    }
    freeifaddrs(ifs);
    return res;
}
