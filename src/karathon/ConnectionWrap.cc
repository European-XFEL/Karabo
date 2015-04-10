#include "ConnectionWrap.hh"
#include <karabo/net/IOService.hh>

using namespace std;
using namespace karabo::net;
using namespace karabo::util;

namespace bp = boost::python;


namespace karathon {


    int ConnectionWrap::startAsync(const karabo::net::Connection::Pointer& connection, const bp::object& connectionHandler) {
        if (!PyCallable_Check(connectionHandler.ptr()))
            throw KARABO_PYTHON_EXCEPTION("Registered object is not a function object.");
        IOService::Pointer ioserv = connection->getIOService();
        int port = 0;
        try {
            ScopedGILRelease nogil;
            port = connection->startAsync(boost::bind(proxyConnectionHandler, connectionHandler, _1));
        } catch (...) {
            KARABO_RETHROW
        }
        return port;
    }


    void ConnectionWrap::proxyConnectionHandler(const bp::object& connectionHandler, karabo::net::Channel::Pointer channel) {
        ScopedGILAcquire gil;
        try {
            connectionHandler(bp::object(channel));
        } catch (const bp::error_already_set& e) {
            if (PyErr_Occurred()) {
                PyErr_Print();
            }
            throw KARABO_PYTHON_EXCEPTION("ConnectionHandler has thrown an exception. See above.");
        } catch (...) {
            KARABO_RETHROW_AS(KARABO_PYTHON_EXCEPTION("Un-handled or forwarded exception happened in python handler"));
        }
    }


    void ConnectionWrap::setErrorHandler(const karabo::net::Connection::Pointer& connection, const bp::object& errorHandler) {
        if (!PyCallable_Check(errorHandler.ptr()))
            throw KARABO_PYTHON_EXCEPTION("Registered object is not a function object.");
        ScopedGILRelease nogil;
        connection->setErrorHandler(boost::bind(proxyErrorHandler, errorHandler, connection, _1));
    }


    void ConnectionWrap::proxyErrorHandler(const bp::object& errorHandler, karabo::net::Connection::Pointer connection, const karabo::net::ErrorCode& code) {
        ScopedGILAcquire gil;
        try {
            errorHandler(bp::object(connection), bp::object(code));
        } catch (const bp::error_already_set& e) {
            if (PyErr_Occurred()) {
                PyErr_Print();
            }
            throw KARABO_PYTHON_EXCEPTION("ErrorHandler has thrown an exception. See above.");
        } catch (...) {
            KARABO_RETHROW_AS(KARABO_PYTHON_EXCEPTION("Un-handled or forwarded exception happened in python handler"));
        }
    }
}
