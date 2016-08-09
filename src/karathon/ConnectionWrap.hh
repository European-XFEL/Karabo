/* 
 * File:   ConnectionWrap.hh
 * Author: Sergey Esenov <serguei.essenov@xfel.eu>
 *
 * Created on April 5, 2013, 10:10 AM
 */

#ifndef KARATHON_CONNECTIONWRAP_HH
#define	KARATHON_CONNECTIONWRAP_HH

#include <boost/python.hpp>
#include <map>
#include <karabo/util/Hash.hh>
#include <karabo/net/IOService.hh>
#include <karabo/net/Connection.hh>
#include <karabo/net/Channel.hh>
#include "ScopedGILRelease.hh"
#include "ScopedGILAcquire.hh"

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
        static void setErrorHandler(const karabo::net::Connection::Pointer& connection, const bp::object& errorHandler);

        static void clear(karabo::net::IOService::Pointer ioserv) {
        }

    private:

        static void proxyConnectionHandler(const bp::object& connectionHandler, karabo::net::Channel::Pointer channel);
        static void proxyErrorHandler(const bp::object& errorHandler, karabo::net::Connection::Pointer connection, const karabo::net::ErrorCode& code);

        // I've taken this helper function from Burkhard

        static bool hasattr(bp::object obj, const std::string& attrName) {
            return PyObject_HasAttrString(obj.ptr(), const_cast<char*> (attrName.c_str()));
        }
    };
}

#endif	/* KARATHON_CONNECTIONWRAP_HH */

