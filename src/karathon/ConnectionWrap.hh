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

        static void setIOService(const karabo::net::Connection::Pointer& connection, const bp::object& obj) {
            using namespace karabo::net;
            if (bp::extract<IOService::Pointer>(obj).check()) {
                const IOService::Pointer& io = bp::extract<IOService::Pointer>(obj);
                ScopedGILRelease nogil;
                connection->setIOService(io);
                return;
            }
            throw KARABO_PYTHON_EXCEPTION("Python object in parameters is not IOService::Pointer");
        }
        
        static bp::object getIOService(const karabo::net::Connection::Pointer& connection) {
            ScopedGILRelease nogil;
            return bp::object(connection->getIOService());
        }

        static void startAsync(const karabo::net::Connection::Pointer& connection, const bp::object& connectionHandler);
        static void setErrorHandler(const karabo::net::Connection::Pointer& connection, const bp::object& errorHandler);

        static void clear(karabo::net::IOService::Pointer ioserv) {
            if (!ioserv) return;
            boost::mutex::scoped_lock lock(m_changedHandlersMutex);
            if (m_handlers.empty()) return;
            std::map<karabo::net::IOService*, std::map<karabo::net::Connection*, karabo::util::Hash> >::iterator it = m_handlers.find(ioserv.get());
            if (it == m_handlers.end()) return;
            std::map<karabo::net::Connection*, karabo::util::Hash>& cmap = it->second;
            cmap.clear();
            m_handlers.erase(it);
        }

    private:

        static void proxyConnectionHandler(karabo::net::Channel::Pointer channel);
        static void proxyErrorHandler(karabo::net::Connection::Pointer connection, const karabo::net::ErrorCode& code);

        // I've taken this helper function from Burkhard

        static bool hasattr(bp::object obj, const std::string& attrName) {
            return PyObject_HasAttrString(obj.ptr(), const_cast<char*> (attrName.c_str()));
        }

    private:
        static boost::mutex m_changedHandlersMutex;
        static std::map<karabo::net::IOService*, std::map<karabo::net::Connection*, karabo::util::Hash> > m_handlers;
    };
}

#endif	/* KARATHON_CONNECTIONWRAP_HH */

