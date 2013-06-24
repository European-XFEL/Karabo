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

        static bp::object start(karabo::net::Connection& connection) {
            karabo::net::Channel::Pointer channel;
            {
                ScopedGILRelease nogil;
                channel = connection.start();
            }
            return bp::object(channel);
        }

        static void stop(karabo::net::Connection& connection) {
            ScopedGILRelease nogil;
            connection.stop();
        }

        static void setIOService(karabo::net::Connection& connection, const bp::object& obj) {
            using namespace karabo::net;
            if (bp::extract<IOService::Pointer>(obj).check()) {
                const IOService::Pointer& io = bp::extract<IOService::Pointer>(obj);
                connection.setIOService(io);
            }
            throw KARABO_PYTHON_EXCEPTION("Python object in parameters is not IOService::Pointer");
        }

        static void startAsync(karabo::net::Connection& connection, const bp::object& connectionHandler);
        static void setErrorHandler(karabo::net::Connection& connection, const bp::object& errorHandler);

    private:

        static void proxyConnectionHandler(karabo::net::Channel::Pointer channel);
        static void proxyErrorHandler(karabo::net::Channel::Pointer channel, const karabo::net::ErrorCode& code);

        // I've taken this helper function from Burkhard

        static bool hasattr(bp::object obj, const std::string& attrName) {
            return PyObject_HasAttrString(obj.ptr(), const_cast<char*> (attrName.c_str()));
        }

    private:
        static boost::mutex m_changedConnectionHandlersMutex;
        static std::map<karabo::net::Connection*, karabo::util::Hash> m_connectionHandlers;
        static boost::mutex m_changedErrorHandlersMutex;
        static std::map<karabo::net::Connection*, karabo::util::Hash> m_errorHandlers;
    };
}

#endif	/* KARATHON_CONNECTIONWRAP_HH */

