/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   p2pbinding.cc
 * Author: Sergey Esenov <serguei.essenov@xfel.eu>
 *
 * Created on April 4, 2013, 4:04 PM
 */

#include <boost/asio/deadline_timer.hpp>
#include <boost/python.hpp>

#include "ChannelWrap.hh"
#include "ConnectionWrap.hh"
#include "PythonFactoryMacros.hh"
#include "Wrapper.hh"
#include "karabo/net/Broker.hh"
#include "karabo/net/Connection.hh"
#include "karabo/net/EventLoop.hh"
#include "karabo/net/utils.hh"

namespace bp = boost::python;

using namespace std;
using namespace karabo::net;
using namespace karathon;


void exportp2p() {
    bp::docstring_options docs(true, true, false);

    {
        bp::enum_<ConnectionStatus>("ConnectionStatus")
              .value("DISCONNECTED", ConnectionStatus::DISCONNECTED)
              .value("CONNECTING", ConnectionStatus::CONNECTING)
              .value("CONNECTED", ConnectionStatus::CONNECTED)
              .value("DISCONNECTING", ConnectionStatus::DISCONNECTING);
    }

    {
        bp::class_<Broker, Broker::Pointer, boost::noncopyable>("Broker", bp::no_init)
              .def("expectedParameters", &Broker::expectedParameters, (bp::arg("schema")))
              .staticmethod("expectedParameters")
              .def("getBrokerUrl", &Broker::getBrokerUrl, "Reports the url of the currently connected-to broker")
              .def("brokerTypeFromEnv", &Broker::brokerTypeFromEnv)
              .staticmethod("brokerTypeFromEnv") KARABO_PYTHON_FACTORY_CONFIGURATOR(Broker);
    }

    {
        bp::class_<ErrorCode>("ErrorCode", "This class keeps error condition: error code and error message.",
                              bp::init<>())
              .def("value", &ErrorCode::value, "Returns error code")
              .def("message", (std::string(ErrorCode::*)() const) & ErrorCode::message, "Returns error message");
    }

    {
        bp::class_<Connection, Connection::Pointer, boost::noncopyable>("Connection", bp::no_init)
              //.def("expectedParameters", &Connection::expectedParameters, (bp::arg("expected")))
              .def("start", &ConnectionWrap().start,
                   "Starts the connection synchronously, i.e. it blocks until connection "
                   "with remote peer is established.  It returns a handle a.k.a channel used in all IO operations.")
              .def("startAsync", &ConnectionWrap().startAsync, (bp::arg("handler")),
                   "Starts the connection asynchronously, i.e. it never blocks"
                   ", but if the connection is established, the provided handler will be called with the channel as "
                   "the argument to the handler.\nRegister "
                   "new handlers in this handler using given channel if you want follow asynchronous model.")
              .def("stop", &ConnectionWrap().stop, "Stops IO service operation.")
              //.def("createChannel", &Connection::createChannel)
              //.def("setErrorHandler", &ConnectionWrap().setErrorHandler, (bp::arg("handler")), "Sets a handler being
              // called in case of error conditions")
              KARABO_PYTHON_FACTORY_CONFIGURATOR(Connection);
    }

    {
        bp::class_<Channel, Channel::Pointer, boost::noncopyable>(
              "Channel",
              "Channel is a class providing IO operations.  The instance of this class can be created when connection "
              "is established.",
              bp::no_init)

              .def("getConnection", &Channel::getConnection, "Returns connection object for given channel")
              .def("readSizeInBytes", &ChannelWrap().readSizeInBytes,
                   "Returns message length you can receive by 'read' call.  It blocks until message length is arrived.")
              .def("readAsyncSizeInBytes", &ChannelWrap().readAsyncSizeInBytes, (bp::arg("handler")),
                   "Register a handler that will be called when message length is arrived.  Never blocks.")
              .def("readStr", &ChannelWrap().readStr,
                   "Read message and return it as a python string.  This function will block until message is arrived.")
              .def("readHash", &ChannelWrap().readHash,
                   "Read message and return it as a Hash.  This function will block until message is arrived.")
              .def("readHashStr", &ChannelWrap().readHashStr,
                   "Read logical message that consists of two parts: header (Hash) & body (str). This function blocks "
                   "until all parts are arrived.")
              .def("readHashHash", &ChannelWrap().readHashHash,
                   "Read logical message that consists of two parts: header (Hash) & body (Hash).  This function "
                   "blocks until all parts are arrived.")
              .def("readAsyncStr", &ChannelWrap().readAsyncStr, (bp::arg("handler")),
                   "Register handler that will be called when the message is arrived.  Never blocks. The message will "
                   "be represented as bytearray.")
              .def("readAsyncHash", &ChannelWrap().readAsyncHash, (bp::arg("handler")),
                   "Register handler that will be called when the next message arrives.\n"
                   "Never blocks, but requires running EventLoop.\n"
                   "Handler will be called with three arguments:\n"
                   "  - an ErrorCode object\n"
                   "  - the channel that called readAsyncHash\n"
                   "  - the sent data deserialized to a Hash")
              .def("readAsyncHashStr", &ChannelWrap().readAsyncHashStr, (bp::arg("handler")),
                   "Register handler that will be called when the message is arrived.  Never blocks. The message will "
                   "have a header as Hash and a body as bytearray.")
              .def("readAsyncHashHash", &ChannelWrap().readAsyncHashHash, (bp::arg("handler")),
                   "Register handler that will be called when the message is arrived.  Never blocks. The message will "
                   "have a header and a body as two Hashes.")
              .def("write", &ChannelWrap().write, (bp::arg("obj")),
                   "This method writes the object given in parameter list synchronously, i.e. blocks until the IO "
                   "operation is completed.")
              .def("write", &ChannelWrap().write2, (bp::arg("obj"), bp::arg("body")),
                   "Helper method. It writes sequentially two messages: the header and the body. The operation is "
                   "synchronous, i.e. the method blocks "
                   "until the IO operation is completed.")
              .def("writeAsyncStr", &ChannelWrap().writeAsyncStr, (bp::arg("data"), bp::arg("handler")),
                   "Register handler for write a string message.  Never blocks. The handler will be called if write "
                   "operation is completed.")
              .def("writeAsyncHash", &ChannelWrap().writeAsyncHash, (bp::arg("data"), bp::arg("handler")),
                   "Register handler for write a Hash message.  Never blocks. The handler will be called if write "
                   "operation is completed.")
              .def("writeAsyncHashStr", &ChannelWrap().writeAsyncHashStr,
                   (bp::arg("hdr"), bp::arg("data"), bp::arg("handler")),
                   "Helper method. Register handler for write a Hash header and 'string' body.  Never blocks.  The "
                   "handler will be called "
                   "if write operations of all message parts are completed.")
              .def("writeAsyncHashHash", &ChannelWrap().writeAsyncHashHash,
                   (bp::arg("hdr"), bp::arg("data"), bp::arg("handler")),
                   "Helper method. Register handler for write a Hash header and Hash body.  Never blocks.  The handler "
                   "will be called "
                   "if write operations of all message parts are completed.")
              .def("close", &Channel::close, "Close channel session.")
              .add_property("__id__", &ChannelWrap().id,
                            "This readonly variable keeps id that uniquely identifies channel instance.")
                    KARABO_PYTHON_FACTORY_CONFIGURATOR(Channel);
    }

    {
        void EventLoopWorkWrap();
        void EventLoopRunWrap();
        void EventLoopPostWrap(const bp::object& callable, const bp::object& delay);
        void EventLoopSetSignalHandler(const bp::object& handler);
        bp::class_<EventLoop, boost::noncopyable>(
              "EventLoop", "EventLoop is a singleton class wrapping Boost ASIO functionality.", bp::no_init)
              .def("work", &EventLoopWorkWrap,
                   "Start the event loop and block until EventLoop::stop() is called.\n\n"
                   "The system signals SIGINT and SIGTERM will be caught and trigger the\n"
                   "following actions:\n"
                   " - a signal handler set via setSignalHandler is called,\n"
                   " - and the event loop is stopped.")
              .staticmethod("work")
              .def("run", &EventLoopRunWrap)
              .staticmethod("run")
              .def("stop", &EventLoop::stop)
              .staticmethod("stop")
              .def("addThread", (void (*)(const int)) & EventLoop::addThread, (bp::arg("nThreads") = 1))
              .staticmethod("addThread")
              .def("removeThread", (void (*)(const int)) & EventLoop::removeThread, (bp::arg("nThreads") = 1))
              .staticmethod("removeThread")
              .def("getNumberOfThreads", (size_t(*)()) & EventLoop::getNumberOfThreads)
              .staticmethod("getNumberOfThreads")
              .def("post", &EventLoopPostWrap, (bp::arg("callable"), bp::arg("delay") = bp::object()),
                   "Post 'callable' for execution in background. Needs EventLoop to run/work.\n"
                   "If 'delay' is not None, execution is postponed by given seconds.")
              .staticmethod("post")
              .def("setSignalHandler", &EventLoopSetSignalHandler, (bp::arg("handler")),
                   "Set handler to be called if a system signal is caught in 'work',\n"
                   "'handler' has to take the signal value as argument.")
              .staticmethod("setSignalHandler");
    }
}


void EventLoopWorkWrap() {
    ScopedGILRelease nogil;
    EventLoop::work();
}


void EventLoopRunWrap() {
    ScopedGILRelease nogil;
    EventLoop::run();
}

void EventLoopPostWrap(const bp::object& callable, const bp::object& delay) {
    // Wrap with GIL since callable's reference count will increase:
    HandlerWrap<> wrapped(callable, "EventLoop.post");
    unsigned int delayMillisec = 0;
    if (delay.ptr() != Py_None) {
        const double delaySeconds = bp::extract<double>(delay); // needs GIL
        delayMillisec = static_cast<unsigned int>(delaySeconds * 1.e3);
    }
    // No GIL when entering pure C++:
    ScopedGILRelease nogil;
    EventLoop::post(std::move(wrapped), delayMillisec);
}

void EventLoopSetSignalHandler(const bp::object& handler) {
    HandlerWrap<int> wrapped(handler, "EventLoop.setSignalHandler");
    // No GIL when entering pure C++:
    ScopedGILRelease nogil;
    EventLoop::setSignalHandler(wrapped);
}
