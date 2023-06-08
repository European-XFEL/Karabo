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
 * File:   ChannelWrap.hh
 * Author: Sergey Esenov <serguei.essenov@xfel.eu>
 *
 * Created on April 4, 2013, 3:22 PM
 */

#ifndef KARABO_PYEXFEL_CHANNELWRAP_HH
#define KARABO_PYEXFEL_CHANNELWRAP_HH

#include <boost/python.hpp>
#include <karabo/net/Channel.hh>

#include "ScopedGILRelease.hh"

namespace bp = boost::python;

namespace karathon {

    class ChannelWrap {
       public:
        static bp::object getConnection(karabo::net::Channel::Pointer channel) {
            karabo::net::Connection::Pointer connection;
            {
                ScopedGILRelease nogil;
                connection = channel->getConnection();
            }
            return bp::object(connection);
        }

        static size_t readSizeInBytes(karabo::net::Channel::Pointer channel) {
            ScopedGILRelease nogil;
            return channel->readSizeInBytes();
        }

        static bp::object readStr(karabo::net::Channel::Pointer channel);
        static bp::object readHash(karabo::net::Channel::Pointer channel);
        static bp::tuple readHashStr(karabo::net::Channel::Pointer channel);
        static bp::tuple readHashHash(karabo::net::Channel::Pointer channel);
        static void write(karabo::net::Channel::Pointer channel, const bp::object& obj);
        static void write2(karabo::net::Channel::Pointer channel, const bp::object& header, const bp::object& obj);
        static void readAsyncSizeInBytes(karabo::net::Channel::Pointer channel, const bp::object& handler);
        static void readAsyncStr(karabo::net::Channel::Pointer channel, const bp::object& handler);
        static void readAsyncHash(karabo::net::Channel::Pointer channel, const bp::object& handler);
        static void readAsyncHashStr(karabo::net::Channel::Pointer channel, const bp::object& handler);
        static void readAsyncHashHash(karabo::net::Channel::Pointer channel, const bp::object& handler);
        static void writeAsyncStr(karabo::net::Channel::Pointer channel, const bp::object& data,
                                  const bp::object& handler);
        static void writeAsyncHash(karabo::net::Channel::Pointer channel, const bp::object& data,
                                   const bp::object& handler);
        static void writeAsyncHashStr(karabo::net::Channel::Pointer channel, const bp::object& hdr,
                                      const bp::object& data, const bp::object& handler);
        static void writeAsyncHashHash(karabo::net::Channel::Pointer channel, const bp::object& hdr,
                                       const bp::object& data, const bp::object& handler);
        //        static void setErrorHandler(karabo::net::Channel::Pointer channel, const bp::object& handler);

        static size_t id(karabo::net::Channel::Pointer channel) {
            return size_t(&(*channel));
        }

        static void clear() {}

       private:
        static void proxyReadSizeInBytesHandler(const bp::object& handler, karabo::net::Channel::Pointer channel,
                                                const size_t& size);
        static void proxyReadStringHandler(const karabo::net::ErrorCode& code, const bp::object& handler,
                                           karabo::net::Channel::Pointer channel, const std::string& s);
        static void proxyReadHashHandler(const karabo::net::ErrorCode& code, const bp::object& handler,
                                         karabo::net::Channel::Pointer channel, const karabo::util::Hash& hash);
        static void proxyReadHashVectorHandler(const karabo::net::ErrorCode& code, const bp::object& handler,
                                               karabo::net::Channel::Pointer channel, const karabo::util::Hash& hash,
                                               const std::vector<char>& v);
        static void proxyReadHashHashHandler(const karabo::net::ErrorCode& code, const bp::object& handler,
                                             karabo::net::Channel::Pointer channel, const karabo::util::Hash& h,
                                             const karabo::util::Hash& b);
        static void proxyWriteCompleteHandler(const karabo::net::ErrorCode& code, const bp::object& handler,
                                              karabo::net::Channel::Pointer channel);

        static bool hasattr(bp::object obj, const std::string& attrName) {
            return PyObject_HasAttrString(obj.ptr(), const_cast<char*>(attrName.c_str()));
        }
    };

} // namespace karathon
#endif /* KARABO_PYEXFEL_CHANNELWRAP_HH */
