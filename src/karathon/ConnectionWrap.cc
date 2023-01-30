#include "ConnectionWrap.hh"

#include "Wrapper.hh"

using namespace std;
using namespace karabo::net;
using namespace karabo::util;
using namespace boost::placeholders;

namespace bp = boost::python;


namespace karathon {


    int ConnectionWrap::startAsync(const karabo::net::Connection::Pointer& connection,
                                   const bp::object& connectionHandler) {
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


    void ConnectionWrap::proxyConnectionHandler(const karabo::net::ErrorCode& code, const bp::object& connectionHandler,
                                                const karabo::net::Channel::Pointer& channel) {
        Wrapper::proxyHandler(connectionHandler, "Connection type", code, channel);
    }
} // namespace karathon
