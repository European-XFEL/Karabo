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
 * File:   ConnectionWrap.hh
 * Author: Sergey Esenov <serguei.essenov@xfel.eu>
 *
 * Created on April 5, 2013, 10:10 AM
 */

#ifndef KARATHON_CONNECTIONWRAP_HH
#define KARATHON_CONNECTIONWRAP_HH

#include <boost/python.hpp>
#include <map>

#include "ScopedGILAcquire.hh"
#include "ScopedGILRelease.hh"
#include "karabo/net/Channel.hh"
#include "karabo/net/Connection.hh"
#include "karabo/util/Hash.hh"


namespace bp = boost::python;

namespace karathon {

    class ConnectionWrap {
       public:
        static bp::object start(const karabo::net::Connection::Pointer& connection) {
            karabo::net::Channel::Pointer channel;
            {
                ScopedGILRelease nogil;
                channel = connection->start();
            }
            return bp::object(channel);
        }

        static void stop(const karabo::net::Connection::Pointer& connection) {
            ScopedGILRelease nogil;
            connection->stop();
        }

        static int startAsync(const karabo::net::Connection::Pointer& connection, const bp::object& connectionHandler);

       private:
        static void proxyConnectionHandler(const karabo::net::ErrorCode& code, const bp::object& connectionHandler,
                                           const karabo::net::Channel::Pointer& channel);
    };
} // namespace karathon

#endif /* KARATHON_CONNECTIONWRAP_HH */
