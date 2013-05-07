#include "ConnectionWrap.hh"

namespace karabo {
    namespace pyexfel {
        
        void ConnectionWrap::startAsync(karabo::net::Connection& connection, const bp::object& connectionHandler) {
            karabo::util::Hash hash;
            // Check that 'connectionHandler' object is a class method
            if (hasattr(connectionHandler, "__self__")) { // class method
                const bp::object& selfObject = connectionHandler.attr("__self__");
                hash.set("_selfObject", selfObject.ptr());
                std::string funcName(bp::extract<std::string>(connectionHandler.attr("__name__")));
                hash.set("_function", funcName);
            } else { // free function
                hash.set("_function", connectionHandler.ptr());
            }
            {
                boost::mutex::scoped_lock lock(m_changedConnectionHandlersMutex);
                m_connectionHandlers[&connection] = hash;
            }
            connection.startAsync(proxyConnectionHandler);
        }

        void ConnectionWrap::proxyConnectionHandler(karabo::net::Channel::Pointer channel) {
            using namespace karabo::net;
            using namespace karabo::util;
            Connection* conn = channel->getConnection().get();
            Hash hash;
            std::map<Connection*, Hash>::iterator it;
            {
                boost::mutex::scoped_lock lock(m_changedConnectionHandlersMutex);
                it = m_connectionHandlers.find(conn);
                if (it == m_connectionHandlers.end())
                    throw KARABO_PYTHON_EXCEPTION("Logical error: connection is not registered");
                hash += it->second; // merge to hash
                m_connectionHandlers.erase(it);
            }
            if (!hash.has("_selfObject") && !hash.has("_function"))
                throw KARABO_PYTHON_EXCEPTION("Logical error: connection registration parameters are not found");

            ScopedGILAcquire gil;
            if (hash.has("_selfObject") && hash.has("_function")) {
                // Class method is handler
                PyObject* objptr = hash.get<PyObject*>("_selfObject");
                std::string funcName = hash.get<std::string>("_function");
                bp::call_method<void>(objptr, funcName.c_str(), bp::object(channel));
            } else if (hash.has("_function")) {
                // Free function is handler
                PyObject* funcptr = hash.get<PyObject*>("_function");
                bp::call<void>(funcptr, bp::object(channel));
            } else
                throw KARABO_PYTHON_EXCEPTION("Logical error: connection registration parameters do not fit!");
        }

        void ConnectionWrap::setErrorHandler(karabo::net::Connection& connection, const bp::object& errorHandler) {
            karabo::util::Hash hash;
            // Check that 'connectionHandler' object is a class method
            if (hasattr(errorHandler, "__self__")) { // class method
                const bp::object& selfObject = errorHandler.attr("__self__");
                hash.set("_selfObject", selfObject.ptr());
                std::string funcName(bp::extract<std::string>(errorHandler.attr("__name__")));
                hash.set("_function", funcName);
            } else { // free function
                hash.set("_function", errorHandler.ptr());
            }
            {
                boost::mutex::scoped_lock lock(m_changedErrorHandlersMutex);
                m_errorHandlers[&connection] = hash;
            }
            connection.setErrorHandler(proxyErrorHandler);
        }

        void ConnectionWrap::proxyErrorHandler(karabo::net::Channel::Pointer channel, const karabo::net::ErrorCode& code) {
            using namespace karabo::net;
            using namespace karabo::util;
            Connection* conn = channel->getConnection().get();
            Hash hash;
            std::map<Connection*, Hash>::iterator it;
            {
                boost::mutex::scoped_lock lock(m_changedErrorHandlersMutex);
                it = m_errorHandlers.find(conn);
                if (it == m_errorHandlers.end())
                    throw KARABO_PYTHON_EXCEPTION("Logical error in Error handling: connection is not registered");
                hash += it->second; // merge to hash
            }
            if (!hash.has("_selfObject") && !hash.has("_function"))
                throw KARABO_PYTHON_EXCEPTION("Logical error in Error handling: connection registration parameters are not found");

            ScopedGILAcquire gil;
            if (hash.has("_selfObject") && hash.has("_function")) {
                // Class method is handler
                PyObject* objptr = hash.get<PyObject*>("_selfObject");
                std::string funcName = hash.get<std::string>("_function");
                bp::call_method<void>(objptr, funcName.c_str(), bp::object(channel), bp::object(code));
            } else if (hash.has("_function")) {
                // Free function is handler
                PyObject* funcptr = hash.get<PyObject*>("_function");
                bp::call<void>(funcptr, bp::object(channel), bp::object(code));
            } else
                throw KARABO_PYTHON_EXCEPTION("Logical error in Error handling: connection registration parameters do not fit!");
        }

        boost::mutex ConnectionWrap::m_changedConnectionHandlersMutex;
        std::map<karabo::net::Connection*, karabo::util::Hash> ConnectionWrap::m_connectionHandlers;
        boost::mutex ConnectionWrap::m_changedErrorHandlersMutex;
        std::map<karabo::net::Connection*, karabo::util::Hash> ConnectionWrap::m_errorHandlers;

    }
}
