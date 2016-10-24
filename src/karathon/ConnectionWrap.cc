#include "ConnectionWrap.hh"

using namespace std;
using namespace karabo::net;
using namespace karabo::util;

namespace bp = boost::python;


namespace karathon {


    int ConnectionWrap::startAsync(const karabo::net::Connection::Pointer& connection, const bp::object& connectionHandler) {
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


    void ConnectionWrap::proxyConnectionHandler(const karabo::net::ErrorCode& code, const bp::object& connectionHandler, karabo::net::Channel::Pointer channel) {
        ScopedGILAcquire gil;
        try {
            connectionHandler(bp::object(code), bp::object(channel));
        } catch (const bp::error_already_set& e) {
            if (PyErr_Occurred()) {
                PyErr_Print();
            }
            throw KARABO_PYTHON_EXCEPTION("ConnectionHandler has thrown an exception. See above.");
        } catch (...) {
            KARABO_RETHROW_AS(KARABO_PYTHON_EXCEPTION("Un-handled or forwarded exception happened in python handler"));
        }
    }
}
