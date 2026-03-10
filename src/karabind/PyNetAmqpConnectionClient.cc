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
#include "karabo/net/AmqpConnection.hh"
#include "karabo/net/AmqpHashClient.hh"
#include "karabo/net/EventLoop.hh"
#include "karabo/net/utils.hh"

namespace py = pybind11;

using namespace std;
using namespace karabo::net;
using namespace karabo::data;
using namespace karabind;


static AMQP::Table convertFromHashToTable(const Hash& queueFlags) {
    // The input Hash should be one-level: no dots in keys
    AMQP::Table table;
    for (Hash::const_iterator it = queueFlags.begin(); it != queueFlags.end(); ++it) {
        const std::string& key = it->getKey();
        const std::any& operand = it->getValueAsAny();
        if (operand.type() == typeid(bool)) table.set(key, std::any_cast<bool>(operand));
        else if (operand.type() == typeid(int8_t)) table.set(key, std::any_cast<int8_t>(operand));
        else if (operand.type() == typeid(uint8_t)) table.set(key, std::any_cast<uint8_t>(operand));
        else if (operand.type() == typeid(int16_t)) table.set(key, std::any_cast<int16_t>(operand));
        else if (operand.type() == typeid(uint16_t)) table.set(key, std::any_cast<uint16_t>(operand));
        else if (operand.type() == typeid(int32_t)) table.set(key, std::any_cast<int32_t>(operand));
        else if (operand.type() == typeid(uint32_t)) table.set(key, std::any_cast<uint32_t>(operand));
        else if (operand.type() == typeid(int64_t)) table.set(key, std::any_cast<int64_t>(operand));
        else if (operand.type() == typeid(uint64_t)) table.set(key, std::any_cast<uint64_t>(operand));
        else if (operand.type() == typeid(const std::string&))
            table.set(key, std::any_cast<const std::string&>(operand));
        else if (operand.type() == typeid(const char*)) table.set(key, std::any_cast<const char*>(operand));
        else {
            throw KARABO_PYTHON_EXCEPTION("The type value of key '" + key +
                                          "' does not supported as value in AMQP::Table");
        }
    }
    return table;
}


void exportPyNetAmqpConnectionClient(py::module_& m) {
    {
        py::class_<AmqpConnection, AmqpConnection::Pointer>(m, "AmqpConnection")
              .def(py::init<std::vector<std::string>>(), py::arg("brokerUrls"))
              .def(
                    "getCurrentUrl",
                    [](const AmqpConnection::Pointer& self) -> py::str {
                        std::string url = "";
                        {
                            py::gil_scoped_release release;
                            url = self->getCurrentUrl();
                        }
                        return url;
                    },
                    "Reports the url of the currently connected to broker")
              .def(
                    "isConnected",
                    [](const AmqpConnection::Pointer& self) -> py::bool_ {
                        bool connected = false;
                        {
                            py::gil_scoped_release release;
                            connected = self->isConnected();
                        }
                        return connected;
                    },
                    "Whether connection established")
              .def(
                    "connectionInfo",
                    [](const AmqpConnection::Pointer& self) -> py::str {
                        std::string info = "";
                        {
                            py::gil_scoped_release release;
                            info = self->connectionInfo();
                        }
                        return info;
                    },
                    R"pbdoc(
                    Various info about internal connection (for debug logs)
                    )pbdoc")
              .def(
                    "asyncConnect",
                    [](const AmqpConnection::Pointer& self, const py::object& handler) {
                        if (handler.is_none()) throw KARABO_PYTHON_EXCEPTION("asyncConnect handler is None");
                        HandlerWrap<const boost::system::error_code> wrappedHandler(handler, "asyncConnect");
                        py::gil_scoped_release release;
                        self->asyncConnect(wrappedHandler);
                    },
                    py::arg("handler"), R"pbdoc(
                    Connect to AMQP broker asynchronously. The handler is called after operation is completed.
                    The handler signature:
                        def handler(ec: ErrorCode):
                            if ec.value() != 0:
                                raise RuntimeError(f"Connect failure: {ec.message()}")
                            ...
                    )pbdoc");
    }

    {
        py::class_<AmqpHashClient, AmqpHashClient::Pointer>(m, "AmqpHashClient")
              .def_static(
                    "create",
                    [](AmqpConnection::Pointer connection, std::string instanceId, const Hash& queueArgs,
                       const py::object& readHandler, const py::object& errorHandler) {
                        if (readHandler.is_none())
                            throw KARABO_PYTHON_EXCEPTION("AmqpHashClient.create readHandler is None");
                        if (errorHandler.is_none())
                            throw KARABO_PYTHON_EXCEPTION("AmqpHashClient.create errorHandler is None");
                        HandlerWrap<Hash::Pointer, Hash::Pointer, std::string, std::string> wrappedReadHandler(
                              readHandler, "create");
                        HandlerWrap<std::string> wrappedErrorHandler(errorHandler, "create");
                        py::gil_scoped_release release;
                        return AmqpHashClient::create(connection, instanceId, convertFromHashToTable(queueArgs),
                                                      std::move(wrappedReadHandler), std::move(wrappedErrorHandler));
                    },
                    py::arg("connection"), py::arg("instanceId"), py::arg("queueArgs"), py::arg("readHandler"),
                    py::arg("errorHandler"), R"pbdoc(
                    Create client with message interface based on two Hashes (header and body).

                    AmqpHashClient.create(connection, instanceId, queueArgs, readHandler, errorHandler)
                    where
                        connection   - AmqpConnection instance
                        instanceId   - unique string identifier
                        queueArgs    - Hash with key/value options for creating AMQP Queue
                        readHandler  - handler like...
                                         def read_handler(header: Hash, body: Hash, exchange: str, routingKey: str)
                        errorHandler - handler like...
                                         def error_handler(error_message: str)
                    )pbdoc")
              .def(
                    "asyncSubscribe",
                    [](const AmqpHashClient::Pointer& self, const std::string& exchange, const std::string& routingKey,
                       const py::object& handler) {
                        if (handler.is_none()) throw KARABO_PYTHON_EXCEPTION("asyncSubscribe handler is None");
                        HandlerWrap<boost::system::error_code> wrappedHandler(handler, "asyncSubscribe");
                        py::gil_scoped_release release;
                        self->asyncSubscribe(exchange, routingKey, wrappedHandler);
                    },
                    py::arg("exchange"), py::arg("routingKey"), py::arg("handler"),
                    "Subscribe to the topic exchange with the routing key. The handler is called when subscription is "
                    "done.")
              .def(
                    "asyncUnsubscribe",
                    [](const AmqpHashClient::Pointer& self, const std::string& exchange, const std::string& routingKey,
                       const py::object& handler) {
                        if (handler.is_none()) throw KARABO_PYTHON_EXCEPTION("asyncUnsubscribe handler is None");
                        HandlerWrap<boost::system::error_code> wrappedHandler(handler, "asyncUnsubscribe");
                        py::gil_scoped_release release;
                        self->asyncUnsubscribe(exchange, routingKey, wrappedHandler);
                    },
                    py::arg("exchange"), py::arg("routingKey"), py::arg("handler"),
                    "Unsubscribe from the exchange and routing key. The handler is called when operation is done.")
              .def(
                    "asyncUnsubscribeAll",
                    [](const AmqpHashClient::Pointer& self, const py::object& handler) {
                        if (handler.is_none()) throw KARABO_PYTHON_EXCEPTION("asyncUnsubscribeAll handler is None");
                        HandlerWrap<boost::system::error_code> wrappedHandler(handler, "asyncUnsubscribeAll");
                        py::gil_scoped_release release;
                        self->asyncUnsubscribeAll(wrappedHandler);
                    },
                    py::arg("handler"), "Send to broker special message to disable all client's subscriptions")
              .def(
                    "asyncPublish",
                    [](const AmqpHashClient::Pointer& self, const std::string& exchange, const std::string& routingKey,
                       const Hash::Pointer& header, const Hash::Pointer& body, const py::object& handler) {
                        if (handler.is_none()) throw KARABO_PYTHON_EXCEPTION("asyncPublish handler is None");
                        HandlerWrap<boost::system::error_code> wrappedHandler(handler, "asyncPublish");
                        py::gil_scoped_release release;
                        self->asyncPublish(exchange, routingKey, header, body, wrappedHandler);
                    },
                    py::arg("exchange"), py::arg("routingKey"), py::arg("header"), py::arg("body"), py::arg("handler"),
                    "Publish header/body on exchange/routingKey asynchronous. When done, the jandler is called.")

              .attr("__karabo_cpp_classid__") = "AmqpHashClient";
    }
}
