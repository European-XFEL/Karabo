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
#include "ConnectionWrap.hh"

#include "Wrapper.hh"

using namespace std;
using namespace karabo::net;
using namespace karabo::util;
using namespace boost::placeholders;

namespace bp = boost::python;


namespace karathon {


    int ConnectionWrap::startAsync(const karabo::net::Connection::Pointer& connection,
                                   const bp::object& connectionHandler) {
        if (!PyCallable_Check(connectionHandler.ptr()))
            throw KARABO_PYTHON_EXCEPTION("Registered object is not a function object.");
        int port = 0;
        try {
            ScopedGILRelease nogil;
            port = connection->startAsync(boost::bind(proxyConnectionHandler, _1, connectionHandler, _2));
        } catch (...) {
            KARABO_RETHROW
        }
        return port;
    }


    void ConnectionWrap::proxyConnectionHandler(const karabo::net::ErrorCode& code, const bp::object& connectionHandler,
                                                const karabo::net::Channel::Pointer& channel) {
        Wrapper::proxyHandler(connectionHandler, "Connection type", code, channel);
    }
} // namespace karathon
