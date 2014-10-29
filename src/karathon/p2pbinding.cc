/* 
 * File:   p2pbinding.cc
 * Author: Sergey Esenov <serguei.essenov@xfel.eu>
 *
 * Created on April 4, 2013, 4:04 PM
 */

#include <boost/python.hpp>
#include <karabo/net/IOService.hh>
#include <karabo/net/Connection.hh>
#include <karabo/net/BrokerConnection.hh>
#include "PythonFactoryMacros.hh"
#include "IOServiceWrap.hh"
#include "ConnectionWrap.hh"
#include "ChannelWrap.hh"

namespace bp = boost::python;

using namespace std;
using namespace karabo::net;
using namespace karathon;


void exportp2p() {
    bp::docstring_options docs(true, true, false);
    
    {
        bp::class_<BrokerConnection, BrokerConnection::Pointer, boost::noncopyable>("BrokerConnection", bp::no_init)
        KARABO_PYTHON_FACTORY_CONFIGURATOR(BrokerConnection);
    }

    {
        bp::class_<ErrorCode>("ErrorCode", "This class keeps error condition: error code and error message.", bp::init<>())
                .def("value", &ErrorCode::value, "Returns error code")
                .def("message", &ErrorCode::message, "Returns error message")
                ;
    }

    {
        bp::class_<IOService, boost::shared_ptr<IOService> >("IOService", bp::no_init)
                .def("run", &IOServiceWrap().run, "This method blocks until at least one handler is registered in it.")
                .def("work", &IOServiceWrap().work, "This method blocks forever, event no handlers are registered.  Service can be stopped by 'stop' call.")
                .def("stop", &IOServiceWrap().stop, "This method stops service operations.")
                ;
    }

    {
        bp::class_<Connection, Connection::Pointer, boost::noncopyable>("Connection", bp::no_init)
                //.def("expectedParameters", &Connection::expectedParameters, (bp::arg("expected")))
                .def("start", &ConnectionWrap().start, "Starts the connection synchronously, i.e. it blocks until connection "
                     "with remote peer is established.  It returns a handle a.k.a channel used in all IO operations.")
                .def("startAsync", &ConnectionWrap().startAsync, (bp::arg("handler")), "Starts the connection asynchronously, i.e. it never blocks"
                     ", but if the connection is established, the provided handler will be called with the channel as the argument to the handler.\nRegister "
                     "new handlers in this handler using given channel if you want follow asynchronous model.")
                .def("stop", &ConnectionWrap().stop, "Stops IO service operation.")
                //.def("createChannel", &Connection::createChannel)
                .def("getIOService", &ConnectionWrap().getIOService, "Creates new IO service object for you and register connection on it.")
                .def("setIOService", &ConnectionWrap().setIOService, (bp::arg("ioserv")), "Registers connection on IO service object created externally.")
                .def("setErrorHandler", &ConnectionWrap().setErrorHandler, (bp::arg("handler")), "Sets a handler being called in case of error conditions")
                KARABO_PYTHON_FACTORY_CONFIGURATOR(Connection)
                ;
    }

    {
        bp::class_<Channel, Channel::Pointer, boost::noncopyable >("Channel",
                "Channel is a class providing IO operations.  The instance of this class can be created when connection is established.",
                bp::no_init)

                .def("getConnection", &Channel::getConnection,
                     "Returns connection object for given channel")
                .def("readSizeInBytes", &ChannelWrap().readSizeInBytes,
                     "Returns message length you can receive by 'read' call.  It blocks until message length is arrived.")
                .def("readAsyncSizeInBytes", &ChannelWrap().readAsyncSizeInBytes, (bp::arg("handler")),
                     "Register a handler that will be called when message length is arrived.  Never blocks.")
                .def("readStr", &ChannelWrap().readStr,
                     "Read message and return it as a python string.  This function will block until message is arrived.")
                .def("readHash", &ChannelWrap().readHash,
                     "Read message and return it as a Hash.  This function will block until message is arrived.")
                .def("readHashStr", &ChannelWrap().readHashStr,
                     "Read logical message that consists of two parts: header (Hash) & body (str). This function blocks until all parts are arrived.")
                .def("readHashHash", &ChannelWrap().readHashHash,
                     "Read logical message that consists of two parts: header (Hash) & body (Hash).  This function blocks until all parts are arrived.")
                .def("readAsyncStr", &ChannelWrap().readAsyncStr, (bp::arg("handler")),
                     "Register handler that will be called when the message is arrived.  Never blocks. The message will be represented as bytearray.")
                .def("readAsyncHash", &ChannelWrap().readAsyncHash, (bp::arg("handler")),
                     "Register handler that will be called when the message is arrived.  Never blocks. The message will be represented as Hash.")
                .def("readAsyncHashStr", &ChannelWrap().readAsyncHashStr, (bp::arg("handler")),
                     "Register handler that will be called when the message is arrived.  Never blocks. The message will have a header as Hash and a body as bytearray.")
                .def("readAsyncHashHash", &ChannelWrap().readAsyncHashHash, (bp::arg("handler")),
                     "Register handler that will be called when the message is arrived.  Never blocks. The message will have a header and a body as two Hashes.")
                .def("write", &ChannelWrap().write, (bp::arg("obj")),
                     "This method writes the object given in parameter list synchronously, i.e. blocks until the IO operation is completed.")
                .def("write", &ChannelWrap().write2, (bp::arg("obj"), bp::arg("body")),
                     "Helper method. It writes sequentially two messages: the header and the body. The operation is synchronous, i.e. the method blocks "
                     "until the IO operation is completed.")
                .def("writeAsyncStr", &ChannelWrap().writeAsyncStr, (bp::arg("data"), bp::arg("handler")),
                     "Register handler for write a string message.  Never blocks. The handler will be called if write operation is completed.")
                .def("writeAsyncHash", &ChannelWrap().writeAsyncHash, (bp::arg("data"), bp::arg("handler")),
                     "Register handler for write a Hash message.  Never blocks. The handler will be called if write operation is completed.")
                .def("writeAsyncHashStr", &ChannelWrap().writeAsyncHashStr, (bp::arg("hdr"), bp::arg("data"), bp::arg("handler")),
                     "Helper method. Register handler for write a Hash header and 'string' body.  Never blocks.  The handler will be called "
                     "if write operations of all message parts are completed.")
                .def("writeAsyncHashHash", &ChannelWrap().writeAsyncHashHash, (bp::arg("hdr"), bp::arg("data"), bp::arg("handler")),
                     "Helper method. Register handler for write a Hash header and Hash body.  Never blocks.  The handler will be called "
                     "if write operations of all message parts are completed.")
                .def("waitAsync", &ChannelWrap().waitAsync, (bp::arg("milliseconds"), bp::arg("handler")),
                     "The handler will be called by IO service object when the registered delay (in milliseconds) is elapsed.")
                .def("setErrorHandler", &ChannelWrap().setErrorHandler, (bp::arg("handler")),
                     "Set handler that is called if the error condition is encountered during operations on channel.")
                .def("close", &Channel::close, "Close channel session.")
                .add_property("__id__", &ChannelWrap().id, "This readonly variable keeps id that uniquely identifies channel instance.")
                KARABO_PYTHON_FACTORY_CONFIGURATOR(Channel)
                ;
    }
}
