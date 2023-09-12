/*
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <pybind11/pybind11.h>

#include <boost/asio/deadline_timer.hpp>

#include "HandlerWrap.hh"
#include "PythonFactoryMacros.hh"
#include "Wrapper.hh"
#include "karabo/net/Broker.hh"
#include "karabo/net/Channel.hh"
#include "karabo/net/Connection.hh"
#include "karabo/net/EventLoop.hh"
#include "karabo/net/utils.hh"

namespace py = pybind11;

using namespace std;
using namespace karabo::net;
using namespace karabo::util;
using namespace karabind;


void exportPyNetConnectionChannel(py::module_& m) {
    {
        py::class_<Broker, Broker::Pointer>(m, "Broker")

              .def_static("expectedParameters", &Broker::expectedParameters, py::arg("schema"))

              .def("getBrokerUrl", &Broker::getBrokerUrl, "Reports the url of the currently connected-to broker")

              .def_static("brokerTypeFromEnv", &Broker::brokerTypeFromEnv)

                    KARABO_PYTHON_FACTORY_CONFIGURATOR_NOCREATE(Broker)

              .attr("__karabo_cpp_classid__") = "Broker";
    }

    {
        py::class_<ErrorCode>(m, "ErrorCode", "This class keeps error condition: error code and error message.")
              .def("value", &ErrorCode::value, "Returns error code")
              .def("message", (std::string(ErrorCode::*)() const) & ErrorCode::message, "Returns error message");
    }

    {
        py::class_<Connection, Connection::Pointer>(m, "Connection")
              //.def_static("expectedParameters", &Connection::expectedParameters, py::arg("expected"))
              .def(
                    "start",
                    [](const Connection::Pointer& self) {
                        Channel::Pointer channel;
                        {
                            py::gil_scoped_release release;
                            channel = self->start();
                        }
                        return py::cast(channel);
                    },
                    "Starts the connection synchronously, i.e. it blocks until connection "
                    "with remote peer is established.  It returns a handle a.k.a channel used in all IO operations.")
              .def(
                    "startAsync",
                    [](const Connection::Pointer& self, py::object handler) -> py::int_ {
                        if (handler.is_none()) throw KARABO_PYTHON_EXCEPTION("startAsync handler is None");
                        int port = 0;
                        {
                            py::gil_scoped_release release;
                            port = self->startAsync([handler](const ErrorCode& e, const Channel::Pointer& channel) {
                                py::gil_scoped_acquire gil;
                                try {
                                    handler(py::cast(e), py::cast(channel));
                                } catch (py::error_already_set& e) {
                                    karabind::detail::treatError_already_set(e, handler, "startAsync handler");
                                } catch (...) {
                                    KARABO_RETHROW
                                }
                            });
                        }
                        return port;
                    },
                    py::arg("handler") = py::none(),
                    "Starts the connection asynchronously, i.e. it never blocks"
                    ", but if the connection is established, the provided handler will be called with the channel as "
                    "the argument to the handler.\nRegister "
                    "new handlers in this handler using given channel if you want to handle further events in the "
                    "connection's lifetime.")
              .def(
                    "stop",
                    [](const Connection::Pointer& self) {
                        py::gil_scoped_release release;
                        self->stop();
                    },
                    "Stops IO service operation.")

                    KARABO_PYTHON_FACTORY_CONFIGURATOR(Connection)

              .attr("__karabo_cpp_classid__") = "Connection";
    }

    {
        py::class_<Channel, Channel::Pointer>(m, "Channel")
              .def("getConnection", &Channel::getConnection, "Returns connection object for given channel")
              .def(
                    "readStr",
                    [](const Channel::Pointer& self) -> py::str {
                        std::vector<char> v;
                        {
                            py::gil_scoped_release release;
                            v.resize(self->readSizeInBytes());
                            self->read(v.data(), v.size());
                        }
                        return py::str(v.data(), v.size());
                    },
                    "Read message and return it as a python string.  This function will block until the message has "
                    "arrived.")
              .def(
                    "readHash",
                    [](const Channel::Pointer& self) {
                        Hash hash;
                        {
                            py::gil_scoped_release release;
                            self->read(hash);
                        }
                        return py::cast(std::move(hash));
                    },
                    "Read message and return it as a Hash.  This function will block until the message has arrived.")
              .def(
                    "readHashStr",
                    [](const Channel::Pointer& self) -> py::tuple {
                        Hash header;
                        std::vector<char> v;
                        {
                            py::gil_scoped_release release;
                            self->read(header);
                            v.resize(self->readSizeInBytes());
                            self->read(v.data(), v.size());
                        }
                        return py::make_tuple(py::cast(std::move(header)), py::str(v.data(), v.size()));
                    },
                    "Read logical message that consists of two parts: header (Hash) & body (str). This function blocks "
                    "until all the parts have arrived.")
              .def(
                    "readHashHash",
                    [](const Channel::Pointer& self) {
                        Hash header;
                        Hash body;
                        {
                            py::gil_scoped_release release;
                            self->read(header);
                            self->read(body);
                        }
                        return py::make_tuple(py::cast(std::move(header)), py::cast(std::move(body)));
                    },
                    "Read logical message that consists of two parts: header (Hash) & body (Hash).  This function "
                    "blocks until all parts are arrived.")
              .def(
                    "readAsyncStr",
                    [](const Channel::Pointer& self, const py::object& handler) {
                        if (handler.is_none()) throw KARABO_PYTHON_EXCEPTION("readStr handler is None");
                        py::gil_scoped_release release;
                        self->readAsyncString([handler, self](const ErrorCode& e, const std::string& s) {
                            py::gil_scoped_acquire gil;
                            try {
                                handler(py::cast(e), py::cast(self), py::cast(s));
                            } catch (py::error_already_set& e) {
                                karabind::detail::treatError_already_set(e, handler, "readAsyncStr handler");
                            } catch (...) {
                                KARABO_RETHROW
                            }
                        });
                    },
                    py::arg("handler") = py::none(),
                    "Register handler that will be called when the message is arrived.  Never blocks. The message will "
                    "be represented as python string.")

              .def(
                    "readAsyncHash",
                    [](const Channel::Pointer& self, const py::object& handler) {
                        if (handler.is_none()) throw KARABO_PYTHON_EXCEPTION("readHash handler is None");
                        py::gil_scoped_release release;
                        self->readAsyncHash([handler, self](const ErrorCode& e, const Hash& h) {
                            py::gil_scoped_acquire gil;
                            try {
                                handler(py::cast(e), py::cast(self), py::cast(h));
                            } catch (py::error_already_set& e) {
                                karabind::detail::treatError_already_set(e, handler, "readAsyncHash handler");
                            } catch (...) {
                                KARABO_RETHROW
                            }
                        });
                    },
                    py::arg("handler") = py::none(),
                    "Register handler that will be called when the next message arrives.\n"
                    "Never blocks, but requires a running EventLoop.\n"
                    "Handler will be called with three arguments:\n"
                    "  - an ErrorCode object\n"
                    "  - the channel that called readAsyncHash\n"
                    "  - the sent data deserialized to a Hash")

              .def(
                    "readAsyncHashStr",
                    [](const Channel::Pointer& self, const py::object& handler) {
                        if (handler.is_none()) throw KARABO_PYTHON_EXCEPTION("readHashStr handler is None");
                        py::gil_scoped_release release;
                        self->readAsyncHashVector(
                              [handler, self](const ErrorCode& e, const Hash& h, const std::vector<char>& v) {
                                  py::gil_scoped_acquire gil;
                                  std::string s(v.begin(), v.end());
                                  try {
                                      handler(py::cast(e), py::cast(self), py::cast(h), py::cast(s));
                                  } catch (py::error_already_set& e) {
                                      karabind::detail::treatError_already_set(e, handler, "readAsyncHashStr handler");
                                  } catch (...) {
                                      KARABO_RETHROW
                                  }
                              });
                    },
                    py::arg("handler") = py::none(),
                    "Register handler that will be called when the message is arrived.  Never blocks. The message will "
                    "have a header as Hash and a body as python string.")

              .def(
                    "readAsyncHashHash",
                    [](const Channel::Pointer& self, const py::object& handler) {
                        if (handler.is_none()) throw KARABO_PYTHON_EXCEPTION("readHashHash handler is None");
                        py::gil_scoped_release release;
                        self->readAsyncHashHash([handler, self](const ErrorCode& e, const Hash& h, const Hash& b) {
                            py::gil_scoped_acquire gil;
                            try {
                                handler(py::cast(e), py::cast(self), py::cast(h), py::cast(b));
                            } catch (py::error_already_set& e) {
                                karabind::detail::treatError_already_set(e, handler, "readHashHash handler");
                            } catch (...) {
                                KARABO_RETHROW
                            }
                        });
                    },
                    py::arg("handler") = py::none(),
                    "Register handler that will be called when the message is arrived.  Never blocks. The message will "
                    "have a header and a body as two Hashes.")

              .def(
                    "write",
                    [](const Channel::Pointer& self, const py::object& o) {
                        if (py::isinstance<Hash>(o)) {
                            const Hash& msg = o.cast<Hash>();
                            py::gil_scoped_release release;
                            self->write(msg);
                        } else {
                            std::string s;
                            if (!wrapper::fromPyObjectToString(o, s)) {
                                throw KARABO_PYTHON_EXCEPTION("Not supported type");
                            }
                            py::gil_scoped_release release;
                            self->write(s.c_str(), s.size());
                        }
                    },
                    py::arg("obj"),
                    "This method writes the object given in parameter list synchronously, i.e. blocks until the IO "
                    "operation is completed. The object type can be python str, bytes or bytearray")

              .def(
                    "write",
                    [](const Channel::Pointer& self, const Hash& header, const py::object& b) {
                        if (py::isinstance<Hash>(b)) {
                            const auto& body = b.cast<Hash>();
                            py::gil_scoped_release release;
                            self->write(header, body);
                            return;
                        }
                        std::string s;
                        if (!wrapper::fromPyObjectToString(b, s)) throw KARABO_PYTHON_EXCEPTION("Not supported type");
                        py::gil_scoped_release release;
                        self->write(header, s.c_str(), s.size());
                    },
                    py::arg("header"), py::arg("body"),
                    "Helper method. It writes sequentially two messages: the header (Hash) and the body (Hash, str, "
                    "bytes, bytearray). The operation is synchronous, i.e. the method blocks until the IO operation "
                    "is completed.")

              .def(
                    "writeAsyncStr",
                    [](const Channel::Pointer& self, const py::object& o, const py::object& handler) {
                        if (handler.is_none()) throw KARABO_PYTHON_EXCEPTION("Handler is None");
                        // Does not compile with make_unique, complains about some usage of deleted function
                        auto s = std::make_shared<std::string>();
                        if (!wrapper::fromPyObjectToString(o, *s)) throw KARABO_PYTHON_EXCEPTION("Not supported type");
                        py::gil_scoped_release release;
                        const char* strPtr = s->data();
                        const size_t strSize = s->size();
                        // bind 's' to keep its memory alive and valid - need variables strXxx instead of directly
                        // passing s->func() to ensure things are not moved away before passed (argument determination
                        // order is not defined by standard, IIRC)
                        self->writeAsyncRaw(strPtr, strSize, [handler, self, s = std::move(s)](const ErrorCode& e) {
                            py::gil_scoped_acquire gil;
                            try {
                                handler(py::cast(e), py::cast(self));
                            } catch (py::error_already_set& e) {
                                karabind::detail::treatError_already_set(e, handler,
                                                                         "writeAsyncStr Completion handler");
                            } catch (...) {
                                KARABO_RETHROW
                            }
                        });
                    },
                    py::arg("data"), py::arg("handler") = py::none(),
                    "Register handler for write a string (bytes, bytearray) message.  Never blocks. The handler "
                    "will be called if write operation is completed.")

              .def(
                    "writeAsyncHash",
                    [](const Channel::Pointer& self, const Hash& data, const py::object& handler) {
                        if (handler.is_none()) throw KARABO_PYTHON_EXCEPTION("writeCompletionHandler is None");
                        py::gil_scoped_release release;
                        self->writeAsyncHash(data, [handler, self](const ErrorCode& e) {
                            py::gil_scoped_acquire gil;
                            try {
                                handler(py::cast(e), py::cast(self));
                            } catch (py::error_already_set& e) {
                                karabind::detail::treatError_already_set(e, handler, "writeCompletion");
                            } catch (...) {
                                KARABO_RETHROW
                            }
                        });
                    },
                    py::arg("data"), py::arg("handler") = py::none(),
                    "Register handler for write a Hash message.  Never blocks. The handler will be called if write "
                    "operation is completed.")

              .def(
                    "writeAsyncHashStr",
                    [](const Channel::Pointer& self, const Hash& h, const py::object& b, const py::object& handler) {
                        if (handler.is_none()) throw KARABO_PYTHON_EXCEPTION("writeCompletionHandler is None");
                        auto s = std::make_shared<std::string>();
                        if (!wrapper::fromPyObjectToString(b, *s)) throw KARABO_PYTHON_EXCEPTION("Not supported type");
                        py::gil_scoped_release release;
                        const char* strPtr = s->data();
                        const size_t strSize = s->size();
                        // See above in writeAsyncStr about strPtr, strSize and s = make_shared...
                        self->writeAsyncHashRaw(
                              h, strPtr, strSize, [handler, self, s{std::move(s)}](const ErrorCode& e) {
                                  py::gil_scoped_acquire gil;
                                  try {
                                      handler(py::cast(e), py::cast(self));
                                  } catch (py::error_already_set& e) {
                                      karabind::detail::treatError_already_set(e, handler, "writeCompletion");
                                  } catch (...) {
                                      KARABO_RETHROW
                                  }
                              });
                    },
                    py::arg("hdr"), py::arg("data"), py::arg("handler"),
                    "Helper method. Register handler for write a Hash header and 'string' ('bytes', 'bytearray') body."
                    " Never blocks.  The handler will be called if write operations of all message parts are "
                    "completed.")

              .def(
                    "writeAsyncHashHash",
                    [](const Channel::Pointer& self, const Hash& h, const Hash& b, const py::object& handler) {
                        if (handler.is_none()) throw KARABO_PYTHON_EXCEPTION("writeCompletionHandler is None");
                        py::gil_scoped_release release;
                        self->writeAsyncHashHash(h, b, [handler, self](const ErrorCode& e) {
                            py::gil_scoped_acquire gil;
                            try {
                                handler(py::cast(e), py::cast(self));
                            } catch (py::error_already_set& e) {
                                karabind::detail::treatError_already_set(e, handler, "writeCompletion");
                            } catch (...) {
                                KARABO_RETHROW
                            }
                        });
                    },
                    py::arg("hdr"), py::arg("data"), py::arg("handler") = py::none(),
                    "Helper method. Register handler for write a Hash header and Hash body.  Never blocks.  The "
                    "handler "
                    "will be called "
                    "if write operations of all message parts are completed.")

              .def("close", &Channel::close, "Close channel session.")

              .def_property_readonly(
                    "__id__", [](const Channel::Pointer& self) -> py::int_ { return std::size_t(&(*self)); },
                    "This readonly variable keeps id that uniquely identifies channel instance.");
    }
}

#include <karabo/net/AmqpBroker.hh>
#include <karabo/net/AmqpClient.hh>
#include <karabo/net/InfluxDbClient.hh>
#include <karabo/net/JmsBroker.hh>
#include <karabo/net/JmsConnection.hh>
#include <karabo/net/Strand.hh>
#include <karabo/net/TcpConnection.hh>

KARABO_REGISTER_FOR_CONFIGURATION(karabo::net::Broker, karabo::net::AmqpBroker)
KARABO_REGISTER_FOR_CONFIGURATION_ADDON(AMQP::Table, karabo::net::AmqpClient)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::net::InfluxDbClient)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::net::JmsConnection)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::net::Broker, karabo::net::JmsBroker)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::net::Strand)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::net::Connection, karabo::net::TcpConnection)
