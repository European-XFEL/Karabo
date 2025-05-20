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

#include <karabo/net/utils.hh>
#include <karabo/xms/InputChannel.hh>
#include <karabo/xms/Memory.hh>
#include <karabo/xms/OutputChannel.hh>

#include "HandlerWrap.hh"
#include "PyUtilSchemaElement.hh"
#include "Wrapper.hh"

namespace py = pybind11;


namespace karabind {

    // Trampoline class to override virtual function(s) in OutputChannel
    class PyOutputChannel : public karabo::xms::OutputChannel {
       public:
        using karabo::xms::OutputChannel::OutputChannel;

        karabo::data::ClassInfo getClassInfo() const override {
            PYBIND11_OVERRIDE(karabo::data::ClassInfo, karabo::xms::OutputChannel, getClassInfo);
        }
    };

    // Trampoline class to override virtual function(s) in OutputChannel
    class PyInputChannel : public karabo::xms::InputChannel {
       public:
        using karabo::xms::InputChannel::InputChannel;

        karabo::data::ClassInfo getClassInfo() const override {
            PYBIND11_OVERRIDE(karabo::data::ClassInfo, karabo::xms::InputChannel, getClassInfo);
        }
    };

    class ChannelMetaData : public karabo::xms::Memory::MetaData {
       public:
        ChannelMetaData(const py::object& src, const py::object& ts)
            : karabo::xms::Memory::MetaData(src.cast<std::string>(), ts.cast<karabo::data::Timestamp>()) {}

        void setSource(const py::object& src) {
            karabo::xms::Memory::MetaData::setSource(src.cast<std::string>());
        }

        py::object getSource() {
            return py::cast(karabo::xms::Memory::MetaData::getSource());
        }

        void setTimestamp(const py::object& ts) {
            karabo::xms::Memory::MetaData::setTimestamp(ts.cast<karabo::data::Timestamp>());
        }

        py::object getTimestamp() {
            return py::cast(karabo::xms::Memory::MetaData::getTimestamp());
        }
    };
} // namespace karabind

using namespace karabo::data;
using namespace karabo::net;
using namespace karabo::xms;
using namespace karabind;
using namespace std;


void exportPyXmsInputOutputChannel(py::module_& m) {
    {
        py::enum_<ConnectionStatus>(m, "ConnectionStatus")
              .value("DISCONNECTED", ConnectionStatus::DISCONNECTED)
              .value("CONNECTING", ConnectionStatus::CONNECTING)
              .value("CONNECTED", ConnectionStatus::CONNECTED)
              .value("DISCONNECTING", ConnectionStatus::DISCONNECTING);
    }

    py::class_<ChannelMetaData>(m, "ChannelMetaData")

          .def(py::init([](const py::object& src, const py::object& ts) { return ChannelMetaData(src, ts); }),
               py::arg("src"), py::arg("timestamp"))

          .def("setSource", &ChannelMetaData::setSource, py::arg("source"))

          .def("getSource", &ChannelMetaData::getSource)

          .def("setTimestamp", &ChannelMetaData::setTimestamp, py::arg("timestamp"))

          .def("getTimestamp", &ChannelMetaData::getTimestamp)

          .def(
                "__getitem__",
                [](ChannelMetaData& self, const std::string& key) {
                    if (key == "source") return self.getSource();
                    else if (key == "timestamp") return self.getTimestamp();
                    else throw KARABO_PARAMETER_EXCEPTION("Unknown key");
                },
                py::arg("key"))

          .def(
                "__setitem__",
                [](ChannelMetaData& self, const std::string& key, py::object value) {
                    if (key == "source") {
                        self.setSource(value);
                    } else if (key == "timestamp") {
                        self.setTimestamp(value);
                    } else {
                        throw KARABO_PARAMETER_EXCEPTION("Unknown key");
                    }
                },
                py::arg("key"), py::arg("value"));

    {
        py::class_<OutputChannel, PyOutputChannel, std::shared_ptr<OutputChannel>>(m, "OutputChannel")

              .def("setInstanceIdAndName", &OutputChannel::setInstanceIdAndName, py::arg("instanceId"), py::arg("name"))

              .def("getInstanceId", &OutputChannel::getInstanceId, py::return_value_policy::reference_internal)

              .def("getInstanceIdName", &OutputChannel::getInstanceIdName)

              .def("getInitialConfiguration", &OutputChannel::getInitialConfiguration)

              .def("registerIOEventHandler",
                   [](const OutputChannel::Pointer& self, const py::object& handler) {
                       if (handler.is_none()) {
                           self->registerIOEventHandler(std::function<void(const OutputChannel::Pointer&)>());
                       } else {
                           using Wrap = HandlerWrap<OutputChannel::Pointer>;
                           self->registerIOEventHandler(Wrap(handler, "IOEvent"));
                       }
                   })

              .def("getInformation", &OutputChannel::getInformation)

              .def("hasRegisteredCopyInputChannel", &OutputChannel::hasRegisteredCopyInputChannel, py::arg("inputId"))

              .def("hasRegisteredSharedInputChannel", &OutputChannel::hasRegisteredSharedInputChannel,
                   py::arg("inputId"))

              .def(
                    "write",
                    [](const OutputChannel::Pointer& self, const Hash& data, const ChannelMetaData& meta) {
                        self->write(data, meta);
                    },
                    py::arg("data"), py::arg("meta"),
                    "data - a Hash with the data to write\n"
                    "meta - ChannelMetaData\n"
                    "Note 1: Any NDArray/ImageData inside data must stay untouched at least\n"
                    "        until update() has been called. See also the documentation of the\n"
                    "        safeNDArray flag of the update() method.\n"
                    "Note 2: The methods 'write(..)', '[async]Update()' and '[async]SignalEndOfStream()'\n"
                    "        must not be called concurrently")

              .def(
                    "update",
                    [](const OutputChannel::Pointer& self, bool safeNDArray) {
                        py::gil_scoped_release release;
                        self->update(safeNDArray);
                    },
                    py::arg("safeNDArray") = false,
                    "Update the output channel, i.e. send all data over the wire that was\n"
                    "previously written by calling write(..).\n"
                    "This is synchronously blocking until all data is sent (or queued)\n"
                    "safeNDArray - boolean to indicate whether all ndarrays inside the Hash\n"
                    "              passed to write(..) before are 'safe', i.e. their memory\n"
                    "              will not be referred to elsewhere after update is finished.\n"
                    "              Default is 'false', 'true' can avoid safety copies of NDArray\n"
                    "              content when data is queued or sent locally.\n"
                    "Note: The methods 'write(..)', '[async]Update()' and '[async]SignalEndOfStream()' must\n"
                    "      not be called concurrently")
              .def(
                    "asyncUpdate",
                    [](const OutputChannel::Pointer& self, bool safeNDArray, const py::object& readyHandler) {
                        std::function<void()> handler;
                        if (readyHandler.is_none()) {
                            handler = []() {};
                        } else {
                            handler = HandlerWrap<>(readyHandler, "asyncUpdate");
                        }
                        py::gil_scoped_release
                              release; // This asyncUpdate is only partially async and can block, so release GIL
                        self->asyncUpdate(safeNDArray, std::move(handler));
                    },
                    py::arg("safeNDArray") = false, py::arg("readyHandler") = py::none(),
                    "Semi-asynchronously update the output channel, i.e. asynchronously send all data\n"
                    "over the wire that was previously written by calling write(...), but block as\n"
                    "long as any of the connected InputChannels requires if wait is configured.\n\n"
                    "safeNDArray - boolean to indicate whether all NDArrays inside the Hash passed to\n"
                    "              write(..) before are 'safe', i.e. their memory will not be\n"
                    "              referred to elsewhere after update is finished. Default is 'false'\n"
                    "              'true' can avoid safety copies of NDArray content when data is\n"
                    "              queued or sent locally.\n"
                    "readyHandler - callback when data (that is not queued) has been sent and thus\n"
                    "               even NDArray data inside it can be re-used again (except if\n"
                    "               safeNDArray was set to 'true' in which case its memory may still\n"
                    "               be used in a queue).\n\n"
                    "Thread safety:\n"
                    "All the 'write(..)' methods, '[async]Update(..)' and\n"
                    "'[async]SignalEndOfStream(..)' must not be called concurrently.")

              .def(
                    "signalEndOfStream",
                    [](const OutputChannel::Pointer& self) {
                        py::gil_scoped_release release;
                        self->signalEndOfStream();
                    },
                    "Send end-of-stream (EOS) notification to all connected input channels to\n"
                    "indicate a logical break in the data stream.\n"
                    "Note: The methods 'write(..)', 'update()' and 'signalEndOfStream()' must not\n"
                    "      be called concurrently")

              .def(
                    "asyncSignalEndOfStream",
                    [](const OutputChannel::Pointer& self, const py::object& readyHandler) {
                        std::function<void()> handler;
                        if (readyHandler.is_none()) {
                            handler = []() {};
                        } else {
                            handler = HandlerWrap<>(readyHandler, "asyncSignalEndOfStream");
                        }
                        py::gil_scoped_release release;
                        self->asyncSignalEndOfStream(std::move(handler));
                    },
                    py::arg("readyHandler") = py::none(),
                    "Asynchonously send end-of-stream (EOS) notification to all connected input\n"
                    "channels to indicate a logical break in the data stream.\n\n"
                    "readyHandler - callback when notification has been sent or queued\n\n"
                    "Thread safety:\n"
                    "All the 'write(..)' methods, '[async]Update(..)' and\n"
                    "'[async]SignalEndOfStream(..)' must not be called concurrently.")

              .def(
                    "registerShowConnectionsHandler",
                    [](const OutputChannel::Pointer& self, const py::object& handler) {
                        HandlerWrap<const std::vector<Hash>&> wrappedHandler(handler, "show connections");
                        py::gil_scoped_release release;
                        // Setting the handler might overwrite and thus destruct a previous handler.
                        // That one's destruction might acquire the GIL via the destructor of HandlerWrap.
                        // So better release the GIL here (although the deadlock has not been seen without releasing).
                        self->registerShowConnectionsHandler(
                              std::move(wrappedHandler)); // move for once when handler registration
                                                          // will take rvalue reference...
                    },
                    py::arg("handler"),
                    "Register a handler to be called when the 'connection' table changes.\n"
                    "Argument of the handler is a list of Hash as described by the row schema\n"
                    "of the 'connection' table")

              .def(
                    "registerShowStatisticsHandler",
                    [](const OutputChannel::Pointer& self, const py::object& handler) {
                        HandlerWrapVullVull wrappedHandler(handler, "show statistics");
                        py::gil_scoped_release release;
                        // Setting the handler might overwrite and thus destruct a previous handler.
                        // That one's destruction might acquire the GIL via the destructor of HandlerWrap.
                        self->registerShowStatisticsHandler(wrappedHandler);
                    },
                    py::arg("handler"),
                    "Register a handler to be regularly called to update written and read bytes.\n"
                    "Argument of the handler are two lists of numbers: bytes read from and written to\n"
                    "connected channels, in the same order as in the connection table.")

              .def(
                    "registerSharedInputSelector",
                    [](const OutputChannel::Pointer& self, const py::object& handler) {
                        karabo::xms::SharedInputSelector selector;
                        if (!handler.is_none()) {
                            selector = ReturnHandlerWrap<std::string, const std::vector<std::string>&>(
                                  handler, "sharedInputSelector");
                        } // else we reset the selection
                        self->registerSharedInputSelector(std::move(selector));
                    },
                    py::arg("selector"),
                    R"(Register handler that selects which of the connected input channels that have
dataDistribution = "shared" is to be served.

The handler will be called during update() with the ids of the connected
"shared" input channels (e.g. "deviceId:input") as argument. The returned
channel id will receive the data. If an empty string or an unknown id is
returned, the data will be dropped.

:param selector takes a list of str as argument and shall return str)")

              .def_static(
                    "create",
                    [](const std::string& instanceId, const std::string& channelName, const Hash& config) {
                        OutputChannel::Pointer channel =
                              Configurator<OutputChannel>::create("OutputChannel", config, 0);
                        channel->setInstanceIdAndName(instanceId, channelName);
                        channel->initialize();
                        return py::cast(channel);
                    },
                    py::arg("instanceId"), py::arg("channelName"), py::arg("config"));
    }

    {
        py::class_<InputChannel, PyInputChannel, std::shared_ptr<karabo::xms::InputChannel>>(m, "InputChannel")

              .def("reconfigure", &InputChannel::reconfigure, py::arg("configuration"), py::arg("allowMissing") = true)

              .def("setInstanceId", &InputChannel::setInstanceId, py::arg("instanceId"))

              .def("getInstanceId", &InputChannel::getInstanceId, py::return_value_policy::reference_internal)

              .def("registerDataHandler",
                   [](const InputChannel::Pointer& self, const py::object& handler) {
                       if (!handler.is_none()) {
                           self->registerDataHandler(InputChannelDataHandler(handler, "data"));
                       } else {
                           self->registerDataHandler(InputChannel::DataHandler());
                       }
                   })

              .def("registerInputHandler",
                   [](const InputChannel::Pointer& self, const py::object& handler) {
                       if (!handler.is_none()) {
                           self->registerInputHandler(HandlerWrap<const InputChannel::Pointer&>(handler, "input"));
                       } else {
                           self->registerInputHandler(InputChannel::InputHandler());
                       }
                   })


              .def("registerEndOfStreamEventHandler",
                   [](const InputChannel::Pointer& self, const py::object& handler) {
                       self->registerEndOfStreamEventHandler(HandlerWrap<const InputChannel::Pointer&>(handler, "EOS"));
                   })

              .def(
                    "registerConnectionTracker",
                    [](const InputChannel::Pointer& self, const py::object& statusTracker) {
                        using Wrap = HandlerWrap<const std::string&, karabo::net::ConnectionStatus>;
                        self->registerConnectionTracker(Wrap(statusTracker, "channelStatusTracker"));
                    },
                    "Register a handler to track the status of the connections to\nthe configured output channels.\n"
                    "The handler has two arguments:\n"
                    " * the output channel string like '<deviceId>:<outChannel>'\n"
                    " * the ConnectionStatus")

              .def("getConnectionStatus",
                   [](const InputChannel::Pointer& self) -> py::dict {
                       std::unordered_map<std::string, ConnectionStatus> map = self->getConnectionStatus();
                       py::dict d;
                       for (auto it = map.begin(); it != map.end(); ++it) {
                           d[py::cast(it->first)] = py::cast(it->second);
                       }
                       return d;
                   })

              .def("getConnectedOutputChannels",
                   [](const InputChannel::Pointer& self) -> py::dict {
                       typedef std::map<std::string, karabo::data::Hash> OutputChannels;
                       const OutputChannels& ochannels = self->getConnectedOutputChannels();
                       py::dict d;
                       for (OutputChannels::const_iterator it = ochannels.begin(); it != ochannels.end(); ++it) {
                           d[py::cast(it->first)] = py::cast(it->second);
                       }
                       return d;
                   })

              .def(
                    "read", [](const InputChannel::Pointer& self, size_t idx) { return self->read(idx); },
                    py::arg("idx"))

              .def("size", &InputChannel::size)

              .def("getMinimumNumberOfData", &InputChannel::getMinimumNumberOfData)

              .def(
                    "connectSync",
                    [](const InputChannel::Pointer& self, const Hash& outputChannelInfo) -> py::str {
                        std::string msg;
                        {
                            py::gil_scoped_release release;
                            auto promi = std::make_shared<std::promise<boost::system::error_code>>();
                            auto futur = promi->get_future();
                            self->connect(outputChannelInfo,
                                          [promi](const boost::system::error_code& e) { promi->set_value(e); });
                            auto e = futur.get();
                            if (e.value()) msg = e.message();
                        }
                        return msg;
                    },
                    py::arg("outputChannelInfo"))

              .def(
                    "connect",
                    [](const InputChannel::Pointer& self, const Hash& outputChannelInfo, const py::object& handler) {
                        auto wrappedHandler = HandlerWrap<const std::string&>(handler, "Connect");
                        py::gil_scoped_release release;
                        self->connect(outputChannelInfo, [wrappedHandler](const boost::system::error_code& e) {
                            std::string msg;
                            if (e.value()) msg = e.message();
                            wrappedHandler(msg);
                        });
                    },
                    py::arg("outputChannelInfo"), py::arg("callable"))

              .def(
                    "disconnect",
                    [](const InputChannel::Pointer& self, const Hash& outputChannelInfo) {
                        py::gil_scoped_release release;
                        self->disconnect(outputChannelInfo);
                    },
                    py::arg("outputChannelInfo"))

              .def("getMetaData",
                   [](const InputChannel::Pointer& self) {
                       auto ret = std::vector<karabo::data::Hash>();
                       {
                           py::gil_scoped_release release;
                           const std::vector<karabo::xms::InputChannel::MetaData>& md = self->getMetaData();
                           for (auto it = md.begin(); it != md.end(); ++it) {
                               // TODO: Properly wrap MetaData object - currently this will be visible in Python
                               // as hash
                               ret.push_back(*reinterpret_cast<const karabo::data::Hash*>(&*it));
                           }
                       }
                       return py::cast(ret);
                   })

              .def_static(
                    "create",
                    [](const std::string& instanceId, const std::string& channelName, const Hash& config) {
                        InputChannel::Pointer channel = Configurator<InputChannel>::create("InputChannel", config);
                        channel->setInstanceId(instanceId + ":" + channelName);
                        return py::cast(channel);
                    },
                    py::arg("instanceId"), py::arg("channelName"), py::arg("config"));
    }

    {
        py::class_<OutputChannelElement>(m, "OUTPUT_CHANNEL")

              .def(py::init<Schema&>(), py::arg("expected"))

              .def("key", &OutputChannelElement::key, py::arg("key"), py::return_value_policy::reference_internal)

              .def("displayedName", &OutputChannelElement::displayedName, py::arg("name"),
                   py::return_value_policy::reference_internal)

              .def("description", &OutputChannelElement::description, py::arg("description"),
                   py::return_value_policy::reference_internal)

              .def("dataSchema", &OutputChannelElement::dataSchema, py::arg("schema"),
                   py::return_value_policy::reference_internal)

              .def("commit", &OutputChannelElement::commit)

              ;

        // TODO: check if we need below declaration
        // py::implicitly_convertible<Schema&, OutputChannelElement>();
    }

    {
        py::class_<InputChannelElement>(m, "INPUT_CHANNEL")

              .def(py::init<karabo::data::Schema&>(), py::arg("expected"))

              .def("key", &InputChannelElement::key, py::arg("key"), py::return_value_policy::reference_internal)

              .def("displayedName", &InputChannelElement::displayedName, py::arg("name"),
                   py::return_value_policy::reference_internal)

              .def("description", &InputChannelElement::description, py::arg("description"),
                   py::return_value_policy::reference_internal)

              .def("dataSchema", &InputChannelElement::dataSchema, py::arg("schema"),
                   py::return_value_policy::reference_internal)

              .def("commit", &InputChannelElement::commit)

              ;

        // TODO: Check if we need below declaration ...
        // py::implicitly_convertible<Schema&, InputChannelElement>();
    }
}

// Register classes (at "static" time) to be able to create instances via factory functions ...
KARABO_REGISTER_FOR_CONFIGURATION(karabo::xms::OutputChannel)
// Register also the constructor with an extra int flag:
KARABO_REGISTER_FOR_CONFIGURATION_ADDON(int, karabo::xms::OutputChannel);
KARABO_REGISTER_FOR_CONFIGURATION(karabo::xms::InputChannel)
