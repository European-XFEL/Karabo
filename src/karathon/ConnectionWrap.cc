#include "ConnectionWrap.hh"

namespace karathon {

    void ConnectionWrap::startAsync(const karabo::net::Connection::Pointer& connection, const bp::object& connectionHandler) {
        {
            boost::mutex::scoped_lock lock(m_changedConnectionHandlersMutex);
            m_connectionHandlers[connection.get()] = karabo::util::Hash("_function", connectionHandler);
        }
        ScopedGILRelease nogil;
        connection->startAsync(proxyConnectionHandler);
    }

    void ConnectionWrap::proxyConnectionHandler(karabo::net::Channel::Pointer channel) {
        using namespace karabo::net;
        using namespace karabo::util;
        Hash hash;
        ScopedGILAcquire gil;
        {
            Connection::Pointer conn = channel->getConnection();
            boost::mutex::scoped_lock lock(m_changedConnectionHandlersMutex);
            std::map<Connection*, Hash>::iterator it = m_connectionHandlers.find(conn.get());
            if (it == m_connectionHandlers.end())
                throw KARABO_PYTHON_EXCEPTION("Logical error: connection is not registered");
            hash += it->second; // merge to hash
            m_connectionHandlers.erase(it);
        }
        if (!hash.has("_function"))
            throw KARABO_PYTHON_EXCEPTION("Logical error: connection registration parameters are not found");

        bp::object func = hash.get<bp::object>("_function");
        if (!PyObject_HasAttrString(func.ptr(), "func_name"))
            throw KARABO_PYTHON_EXCEPTION("Registered object is not a function object.");
        func(bp::object(channel));
    }

    void ConnectionWrap::setErrorHandler(const karabo::net::Connection::Pointer& connection, const bp::object& errorHandler) {
        {
            boost::mutex::scoped_lock lock(m_changedErrorHandlersMutex);
            m_errorHandlers[connection.get()] = karabo::util::Hash("_function", errorHandler);
        }
        ScopedGILRelease nogil;
        connection->setErrorHandler(proxyErrorHandler);
    }

    void ConnectionWrap::proxyErrorHandler(karabo::net::Channel::Pointer channel, const karabo::net::ErrorCode& code) {
        using namespace karabo::net;
        using namespace karabo::util;
        Hash hash;
        ScopedGILAcquire gil;
        {
            Connection* conn = channel->getConnection().get();
            boost::mutex::scoped_lock lock(m_changedErrorHandlersMutex);
            std::map<Connection*, Hash>::iterator it = m_errorHandlers.find(conn);
            if (it == m_errorHandlers.end())
                throw KARABO_PYTHON_EXCEPTION("Logical error in Error handling: connection is not registered");
            hash += it->second; // merge to hash
        }
        if (!hash.has("_function"))
            throw KARABO_PYTHON_EXCEPTION("Logical error in Error handling: connection registration parameters are not found");

        // Free function is handler
        bp::object func = hash.get<bp::object>("_function");
        if (!PyObject_HasAttrString(func.ptr(), "func_name"))
            throw KARABO_PYTHON_EXCEPTION("Registered object is not a function object.");
        func(bp::object(channel), bp::object(code));
    }

    boost::mutex ConnectionWrap::m_changedConnectionHandlersMutex;
    std::map<karabo::net::Connection*, karabo::util::Hash> ConnectionWrap::m_connectionHandlers;
    boost::mutex ConnectionWrap::m_changedErrorHandlersMutex;
    std::map<karabo::net::Connection*, karabo::util::Hash> ConnectionWrap::m_errorHandlers;

}
