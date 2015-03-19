#include "ConnectionWrap.hh"
#include <karabo/net/IOService.hh>

using namespace std;
using namespace karabo::net;
using namespace karabo::util;

namespace bp = boost::python;


boost::mutex karathon::ConnectionWrap::m_changedHandlersMutex;
std::map<karabo::net::IOService*, std::map<karabo::net::Connection*, karabo::util::Hash> > karathon::ConnectionWrap::m_handlers;

namespace karathon {

    void ConnectionWrap::startAsync(const karabo::net::Connection::Pointer& connection, const bp::object& connectionHandler) {
        IOService::Pointer ioserv = connection->getIOService();
        try {
            ScopedGILRelease nogil;
            connection->startAsync(boost::bind(proxyConnectionHandler, _1));
        } catch(...) {
            KARABO_RETHROW
        }
        boost::mutex::scoped_lock lock(m_changedHandlersMutex);
        map<IOService*, map<Connection*, Hash> >::iterator it = m_handlers.find(ioserv.get());
        if (it == m_handlers.end()) m_handlers[ioserv.get()] = map<Connection*, Hash>();
        map<Connection*, Hash>& cmap = m_handlers[ioserv.get()];
        map<Connection*, Hash>::iterator ii = cmap.find(connection.get());
        if (ii == cmap.end()) cmap[connection.get()] = Hash();
        Hash& hash = cmap[connection.get()];
        hash.set("_connection", connectionHandler);
    }

    void ConnectionWrap::proxyConnectionHandler(karabo::net::Channel::Pointer channel) {
        Hash hash;
        Connection::Pointer connection = channel->getConnection();
        IOService::Pointer ioserv = connection->getIOService();
        {
            boost::mutex::scoped_lock lock(m_changedHandlersMutex);
            map<IOService*, map<Connection*, Hash> >::iterator it = m_handlers.find(ioserv.get());
            if (it == m_handlers.end()) return;
            map<Connection*, Hash>& cmap = it->second; // reference to internal connections map
            map<Connection*, Hash>::iterator ii = cmap.find(connection.get());
            if (ii == cmap.end()) return;
            Hash& h = ii->second;
            if (!h.has("_connection"))
                throw KARABO_PYTHON_EXCEPTION("Logical error: connection registration parameters are not found");
            hash = h; // copy
            h.erase("_connection");
        }

        ScopedGILAcquire gil;
        bp::object onconnect = hash.get<bp::object>("_connection");
        if (!PyCallable_Check(onconnect.ptr()))
            throw KARABO_PYTHON_EXCEPTION("Registered object is not a function object.");
        onconnect(bp::object(channel));
    }

    void ConnectionWrap::setErrorHandler(const karabo::net::Connection::Pointer& connection, const bp::object& errorHandler) {
        IOService::Pointer ioserv = connection->getIOService();
        {
            boost::mutex::scoped_lock lock(m_changedHandlersMutex);
            map<IOService*, map<Connection*, Hash> >::iterator it = m_handlers.find(ioserv.get());
            if (it == m_handlers.end()) m_handlers[ioserv.get()] = map<Connection*, Hash>();
            map<Connection*, Hash>& cmap = m_handlers[ioserv.get()];
            map<Connection*, Hash>::iterator ii = cmap.find(connection.get());
            if (ii == cmap.end()) cmap[connection.get()] = Hash();
            Hash& hash = cmap[connection.get()];
            hash.set("_error", errorHandler); // register python error handler for connection
        }
        ScopedGILRelease nogil;
        connection->setErrorHandler(boost::bind(proxyErrorHandler, connection, _1));
    }

    void ConnectionWrap::proxyErrorHandler(karabo::net::Connection::Pointer connection, const karabo::net::ErrorCode& code) {
        IOService::Pointer ioserv = connection->getIOService();
        Hash hash;
        {
            boost::mutex::scoped_lock lock(m_changedHandlersMutex);
            map<IOService*, map<Connection*, Hash> >::iterator it = m_handlers.find(ioserv.get());
            if (it == m_handlers.end()) return;
            map<Connection*, Hash>& cmap = it->second; // reference to internal connections map
            map<Connection*, Hash>::iterator ii = cmap.find(connection.get());
            if (ii == cmap.end()) return;
            Hash& h = ii->second;
            if (!h.has("_error"))
                throw KARABO_PYTHON_EXCEPTION("Logical error: Error handler's registration is not found");
            hash = h; // copy
            h.erase("_error");
        }

        ScopedGILAcquire gil;
        bp::object onerror = hash.get<bp::object>("_error");
        if (!PyCallable_Check(onerror.ptr()))
            throw KARABO_PYTHON_EXCEPTION("Registered object is not a function object.");
        onerror(bp::object(connection), bp::object(code));
    }
}
