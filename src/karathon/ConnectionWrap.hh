/* 
 * File:   ConnectionWrap.hh
 * Author: Sergey Esenov <serguei.essenov@xfel.eu>
 *
 * Created on April 5, 2013, 10:10 AM
 */

#ifndef KARATHON_CONNECTIONWRAP_HH
#define	KARATHON_CONNECTIONWRAP_HH

#include "ScopedGILRelease.hh"
#include "ScopedGILAcquire.hh"

#include "karabo/util/Hash.hh"
#include "karabo/net/Connection.hh"
#include "karabo/net/Channel.hh"

#include <boost/python.hpp>
#include <map>



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

        static void proxyConnectionHandler(const karabo::net::ErrorCode& code, const bp::object& connectionHandler, const karabo::net::Channel::Pointer& channel);

    };
}

#endif	/* KARATHON_CONNECTIONWRAP_HH */

