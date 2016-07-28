/*
 * File:   utils.hh
 * Author: gero.flucke@xfel.eu
 *
 * Created on February 11, 2016, 10:30 AM
 *
 *  * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_NET_UTILS_HH
#define	KARABO_NET_UTILS_HH

#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>

namespace boost {
    namespace asio {
        class io_service;
    }
}

namespace karabo {
    namespace net {
        /// Bare host name after stripping domain (exflxxx12345.desy.de => exflxxx12345)
        std::string bareHostName();

        /// Wrapper around boost::asio::io_service::run that catches exceptions,
        /// logs them as errors and continues after some delay.
        /// @param service shared pointer to the io_service
        /// @param category the category used for logging
        /// @param errorMessage will be part of the logged error
        /// @param delayInMilliSec is the delay after each catch
        void runProtected(boost::shared_ptr<boost::asio::io_service> service, const std::string& category,
                          const std::string& errorMessage, unsigned int delayInMilliSec = 100);

        /**
         * Parses a URL and returns a tuple.
         * @param url
         * @return tuple containing scheme, domain, port, path and query
         */
        boost::tuple<std::string, std::string, std::string, std::string, std::string> parseUrl(const std::string& url);
    }
}

#endif	/* KARABO_NET_UTILS_HH */

