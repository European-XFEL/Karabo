/*
 * File:   utils.hh
 * Author: gero.flucke@xfel.eu
 *
 * Created on February 11, 2016, 10:30 AM
 *
 *  * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */

#ifndef KARABO_NET_UTILS_HH
#define KARABO_NET_UTILS_HH

#include <boost/asio/io_service.hpp>
#include <memory>
#include <string>
#include <tuple>


namespace karabo {
    namespace net {

        using AsyncHandler = std::function<void(const boost::system::error_code)>;

        /**
         * Return the bare host name after stripping domain (exflxxx12345.desy.de => exflxxx12345)
         * @return
         */
        std::string bareHostName();

        /**
         * Wrapper around boost::asio::io_service::run that catches exceptions,
         * logs them as errors and continues after some delay.
         * @param service shared pointer to the io_service
         * @param category the category used for logging
         * @param errorMessage will be part of the logged error
         * @param delayInMilliSec is the delay after each catch
         */
        void runProtected(std::shared_ptr<boost::asio::io_service> service, const std::string& category,
                          const std::string& errorMessage, unsigned int delayInMilliSec = 100);

        /**
         * Parses a URL and returns a tuple.
         *
         * The URL must of format: <scheme>:<scheme-dependent-part>
         *
         * @param url A well formed URL
         * @return tuple containing scheme and scheme dependent part
         */
        std::tuple<std::string, std::string> parseGenericUrl(const std::string& url);

        /**
         * Parses a HTTP-like URL and returns a tuple.
         *
         * The URL must of format: <scheme>://<domain>:<port>/<path>?<query>
         *
         * @param url A well formed URL
         * @return tuple containing scheme, domain, port, path and query
         */
        std::tuple<std::string, std::string, std::string, std::string, std::string> parseUrl(const std::string& url);

        std::string urlencode(const std::string& value);

        /**
         * Returns an IP string from a Classless Inter-Domain Routing specification
         *
         * e.g. the string 192.168.0.0/24 represents the IP range between 192.168.0.0 and 192.168.0.255.
         *
         * The function will ignore loopback interface and interfaces that are down.
         * Only IP4 specifications are implemented.
         *
         * @return an IP address matching the input range or the input string
         *         if the input string does not specify a network range or
         *         if it does not match any external active interface
         */
        std::string getIpFromCIDRNotation(const std::string& addressRange);

        enum class AsyncStatus {

            PENDING = 0,
            FAILED = -1,
            DONE = 1
        };

        enum class ConnectionStatus {

            DISCONNECTED = 0,
            CONNECTING,
            CONNECTED,
            DISCONNECTING // needed?
        };
    } // namespace net
} // namespace karabo
#endif /* KARABO_NET_UTILS_HH */
