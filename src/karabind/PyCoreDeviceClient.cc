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

#include <karabo/core/DeviceClient.hh>

#include "HandlerWrap.hh"
#include "PyCoreLockWrap.hh"
#include "Wrapper.hh"


namespace py = pybind11;


namespace karabind {

    class DeviceClientWrap : public karabo::core::DeviceClient {
       public:
        typedef boost::shared_ptr<DeviceClientWrap> Pointer;

        DeviceClientWrap(const std::string& instanceId = std::string()) : karabo::core::DeviceClient(instanceId) {}

        DeviceClientWrap(boost::shared_ptr<karabo::xms::SignalSlotable>& o) : karabo::core::DeviceClient(o) {}

        ~DeviceClientWrap() {}

        bool registerPropertyMonitor(const std::string& instanceId, const std::string& key, const py::object& handler,
                                     const py::object& userData) {
            karabo::util::Schema schema;
            {
                py::gil_scoped_release release;
                schema = getDeviceSchema(instanceId);
            }
            if (!schema.has(key)) return false;
            {
                py::gil_scoped_release release;
                cacheAndGetConfiguration(instanceId);
            }
            {
                boost::mutex::scoped_lock lock(m_propertyChangedHandlersMutex);
                if (py::hasattr(handler, "__self__")) {
                    const py::object& oself = handler.attr("__self__");
                    std::string funcName = handler.attr("__name__").cast<std::string>();
                    m_propertyChangedHandlers.set(instanceId + "." + key + "._function", funcName);
                    m_propertyChangedHandlers.set(instanceId + "." + key + "._selfObject", oself.ptr());
                } else {
                    m_propertyChangedHandlers.set(instanceId + "." + key + "._function", handler.ptr());
                }
                if (!userData.is_none()) {
                    m_propertyChangedHandlers.set(instanceId + "." + key + "._userData", userData);
                }
            }
            immortalize(instanceId);
            return true;
        }

        void instantiateNoWait(const std::string& serverId, const std::string& classId,
                               const karabo::util::Hash& config) {
            py::gil_scoped_release release;
            this->karabo::core::DeviceClient::instantiateNoWait(serverId, classId, config);
        }

        void instantiateNoWait(const std::string& serverId, const karabo::util::Hash& config) {
            py::gil_scoped_release release;
            this->karabo::core::DeviceClient::instantiateNoWait(serverId, config);
        }

        void executeNoWait(std::string instanceId, const std::string& functionName) {
            py::gil_scoped_release release;
            m_signalSlotable.lock()->call(instanceId, functionName);
        }

        void execute(std::string instanceId, const std::string& functionName, int timeout) {
            py::gil_scoped_release release;
            DeviceClient::execute(instanceId, functionName, timeout);
        }

        void killDeviceNoWait(const std::string& deviceId) {
            py::gil_scoped_release release;
            this->karabo::core::DeviceClient::killDeviceNoWait(deviceId);
        }

        void killServerNoWait(const std::string& serverId) {
            py::gil_scoped_release release;
            this->karabo::core::DeviceClient::killServerNoWait(serverId);
        }

        boost::shared_ptr<LockWrap> lockPy(const std::string& deviceId, bool recursive, int timeout) {
            // non waiting request for lock

            if (timeout == 0) {
                return boost::make_shared<LockWrap>(
                      boost::make_shared<karabo::core::Lock>(m_signalSlotable, deviceId, recursive));
            }

            // timeout was given
            const int waitTime = 1; // second
            int nTries = 0;
            while (true) {
                try {
                    return boost::make_shared<LockWrap>(
                          boost::make_shared<karabo::core::Lock>(m_signalSlotable, deviceId, recursive));
                } catch (const karabo::util::LockException& e) {
                    if (nTries++ > timeout / waitTime && timeout != -1) {
                        KARABO_RETHROW;
                    }
                    // otherwise pass through and try again
                    boost::this_thread::sleep(boost::posix_time::seconds(waitTime));
                }
            }
        }
    };
} // namespace karabind


using namespace karabo::xms;
using namespace karabo::util;
using namespace karabo::core;
using namespace karabind;
using namespace std;


void exportPyCoreDeviceClient(py::module_& m) {
    // py::class_<DeviceClient>("DeviceClientBase").def(bp::init<const string&>());

    py::class_<DeviceClientWrap, boost::shared_ptr<DeviceClientWrap>>(m, "DeviceClient")

          .def(py::init<const string&>())

          .def(py::init<boost::shared_ptr<SignalSlotable>&>())

          .def("getInstanceId", &DeviceClientWrap::getInstanceId)

          .def("login", &DeviceClientWrap::login)

          .def("logout", &DeviceClientWrap::logout)

          .def("setInternalTimeout", &DeviceClientWrap::setInternalTimeout, py::arg("internalTimeout"))

          .def("getInternalTimeout", &DeviceClientWrap::getInternalTimeout)

          .def(
                "exists",
                [](const DeviceClientWrap::Pointer& self, const std::string& instanceId) {
                    std::pair<bool, std::string> p;
                    {
                        py::gil_scoped_release release;
                        p = self->exists(instanceId);
                    }
                    return py::make_tuple(p.first, p.second);
                },
                py::arg("instanceId"))

          .def(
                "enableInstanceTracking",
                [](const DeviceClientWrap::Pointer& self) {
                    py::gil_scoped_release release;
                    self->enableInstanceTracking();
                },
                "Enables tracking of new and departing device instances\n\n"
                "The handlers registered with registerInstance[New|Gone|Updated]Monitor\n"
                "will be called accordingly. If the handler for instanceNew is registered before\n"
                "calling this method, it will be called for each device currently in the system.\n\n"
                "NOTE: Use wisely!\n"
                "There is a performance cost to tracking all devices since it means\n"
                "subscribing to the heartbeats of all servers and devices in the system.")

          .def("getSystemInformation",
               [](const DeviceClientWrap::Pointer& self) {
                   Hash info;
                   {
                       py::gil_scoped_release release;
                       info = self->getSystemInformation();
                   }
                   return py::cast(std::move(info));
               })

          .def("getSystemTopology",
               [](const DeviceClientWrap::Pointer& self) {
                   Hash topology;
                   {
                       py::gil_scoped_release release;
                       topology = self->getSystemTopology();
                   }
                   return py::cast(std::move(topology));
               })

          .def("getServers",
               [](const DeviceClientWrap::Pointer& self) -> py::list {
                   std::vector<std::string> servers;
                   {
                       py::gil_scoped_release release;
                       servers = self->getServers();
                   }
                   return py::cast(std::move(servers));
               })

          .def(
                "getClasses",
                [](const DeviceClientWrap::Pointer& self, const std::string& deviceServer) -> py::list {
                    std::vector<std::string> classes;
                    {
                        py::gil_scoped_release release;
                        classes = self->getClasses(deviceServer);
                    }
                    return py::cast(std::move(classes));
                },
                py::arg("serverId"))

          .def(
                "getDevices",
                [](const DeviceClientWrap::Pointer& self, const std::string& serverId) -> py::list {
                    std::vector<std::string> devices;
                    {
                        py::gil_scoped_release release;
                        if (serverId.empty()) {
                            devices = self->getDevices();
                        } else {
                            devices = self->getDevices(serverId);
                        }
                    }
                    return py::cast(std::move(devices));
                },
                py::arg("serverId") = "")

          .def(
                "getDeviceSchema",
                [](const DeviceClientWrap::Pointer& self, const std::string& instanceId) {
                    Schema schema;
                    {
                        py::gil_scoped_release release;
                        schema = self->getDeviceSchema(instanceId);
                    }
                    return py::cast(std::move(schema));
                },
                py::arg("instanceId"))

          .def(
                "getDeviceSchemaNoWait",
                [](const DeviceClientWrap::Pointer& self, const std::string& instanceId) {
                    Schema schema;
                    {
                        py::gil_scoped_release release;
                        schema = self->getDeviceSchemaNoWait(instanceId);
                    }
                    return py::cast(std::move(schema));
                },
                py::arg("instanceId"))

          .def(
                "getActiveSchema",
                [](const DeviceClientWrap::Pointer& self, const std::string& instanceId) {
                    Schema schema;
                    {
                        py::gil_scoped_release release;
                        schema = self->getActiveSchema(instanceId);
                    }
                    return py::cast(std::move(schema));
                },
                py::arg("instanceId"))

          .def(
                "getClassSchema",
                [](const DeviceClientWrap::Pointer& self, const std::string& serverId, const std::string& classId) {
                    Schema schema;
                    {
                        py::gil_scoped_release release;
                        schema = self->getClassSchema(serverId, classId);
                    }
                    return py::cast(std::move(schema));
                },
                py::arg("serverId"), py::arg("classId"))

          .def(
                "getProperties",
                [](const DeviceClientWrap::Pointer& self, const std::string& deviceId) -> py::list {
                    std::vector<std::string> props;
                    {
                        py::gil_scoped_release release;
                        props = self->getProperties(deviceId);
                    }
                    return py::cast(std::move(props));
                },
                py::arg("deviceId"))

          .def(
                "getClassProperties",
                [](const DeviceClientWrap::Pointer& self, const std::string& serverId,
                   const std::string& classId) -> py::list {
                    std::vector<std::string> props;
                    {
                        py::gil_scoped_release release;
                        props = self->getClassProperties(serverId, classId);
                    }
                    return py::cast(std::move(props));
                },
                py::arg("deviceId"), py::arg("classId"))

          .def(
                "getCurrentlySettableProperties",
                [](const DeviceClientWrap::Pointer& self, const std::string& instanceId) -> py::list {
                    std::vector<std::string> props;
                    {
                        py::gil_scoped_release release;
                        props = self->getCurrentlySettableProperties(instanceId);
                    }
                    return py::cast(std::move(props));
                },
                py::arg("instanceId"))

          .def(
                "getCurrentlyExecutableCommands",
                [](const DeviceClientWrap::Pointer& self, const std::string& instanceId) -> py::list {
                    std::vector<std::string> props;
                    {
                        py::gil_scoped_release release;
                        props = self->getCurrentlyExecutableCommands(instanceId);
                    }
                    return py::cast(std::move(props));
                },
                py::arg("instanceId"))

          .def(
                "instantiate",
                [](const DeviceClientWrap::Pointer& self, const std::string& serverId, const std::string& classId,
                   const Hash& configuration, int timeoutInSeconds) -> py::tuple {
                    std::pair<bool, std::string> p;
                    {
                        py::gil_scoped_release release;
                        p = self->instantiate(serverId, classId, configuration, timeoutInSeconds);
                    }
                    return py::make_tuple(p.first, p.second);
                },
                py::arg("serverId"), py::arg("classId"), py::arg("configuration"), py::arg("timeoutInSecs"))

          .def(
                "instantiate",
                [](const DeviceClientWrap::Pointer& self, const std::string& serverId, const Hash& configuration,
                   int timeoutInSeconds) -> py::tuple {
                    std::pair<bool, std::string> p;
                    {
                        py::gil_scoped_release release;
                        p = self->instantiate(serverId, configuration, timeoutInSeconds);
                    }
                    return py::make_tuple(p.first, p.second);
                },
                py::arg("serverId"), py::arg("configuration"), py::arg("timeoutInSecs") = -1)

          .def("instantiateNoWait",
               (void(DeviceClientWrap::*)(const std::string&, const std::string&, const Hash&)) &
                     DeviceClientWrap::instantiateNoWait,
               py::arg("serverId"), py::arg("classId"), py::arg("configuration"))

          .def("instantiateNoWait",
               (void(DeviceClientWrap::*)(const std::string&, const Hash&)) & DeviceClientWrap::instantiateNoWait,
               py::arg("serverId"), py::arg("configuration"))

          .def(
                "killDevice",
                [](const DeviceClientWrap::Pointer& self, const std::string& deviceId, int timeoutInSecs) {
                    std::pair<bool, std::string> p;
                    {
                        py::gil_scoped_release release;
                        self->killDevice(deviceId, timeoutInSecs);
                    }
                    return py::make_tuple(p.first, p.second);
                },
                py::arg("deviceId"), py::arg("timeoutInSeconds") = -1)

          .def("killDeviceNoWait", &DeviceClientWrap::killDeviceNoWait, py::arg("deviceId"))

          .def(
                "killServer",
                [](const DeviceClientWrap::Pointer& self, const std::string& serverId, int timeoutInSecs) {
                    std::pair<bool, std::string> p;
                    {
                        py::gil_scoped_release release;
                        p = self->killServer(serverId, timeoutInSecs);
                    }
                    return py::make_tuple(p.first, p.second);
                },
                py::arg("serverId"), py::arg("timeoutInSeconds") = -1)

          .def("killServerNoWait", &DeviceClientWrap::killServerNoWait, py::arg("serverId"))

          .def(
                "get",
                [](const DeviceClientWrap::Pointer& self, const std::string& instanceId, const std::string& key,
                   const std::string& keysep) {
                    boost::any value;
                    try {
                        py::gil_scoped_release release;
                        if (key.empty()) {
                            value = self->get(instanceId); // value is a Hash object
                        } else {
                            value = self->getAsAny(instanceId, key, keysep.at(0));
                        }
                    } catch (...) {
                        KARABO_RETHROW_AS(KARABO_PARAMETER_EXCEPTION("The key \"" + key +
                                                                     "\" is not found in "
                                                                     "the device \"" +
                                                                     instanceId + "\""));
                    }
                    return wrapper::castAnyToPy(value);
                },
                py::arg("instanceId"), py::arg("key") = "", py::arg("keySep") = ".")

          .def(
                "getPropertyHistory", // "getFromPast"
                [](const DeviceClientWrap::Pointer& self, const std::string& deviceId, const std::string& key,
                   const std::string& from, const std::string& to, unsigned int maxNumData) {
                    std::vector<Hash> history;
                    {
                        py::gil_scoped_release release;
                        history = self->getPropertyHistory(deviceId, key, from, to, maxNumData);
                    }
                    return py::cast(std::move(history));
                },
                py::arg("deviceId"), py::arg("key"), py::arg("from"), py::arg("to") = "", py::arg("maxNumData") = 0)

          .def(
                "getConfigurationFromPast",
                [](const DeviceClientWrap::Pointer& self, const std::string& deviceId, const std::string& timePoint) {
                    std::pair<Hash, Schema> p;
                    {
                        py::gil_scoped_release release;
                        p = self->getConfigurationFromPast(deviceId, timePoint);
                    }
                    return py::make_tuple(std::move(p.first), std::move(p.second));
                },
                py::arg("deviceId"), py::arg("timePoint"))

          .def(
                "listConfigurationFromName",
                [](const DeviceClientWrap::Pointer& self, const std::string& deviceId, const std::string& namePart) {
                    py::gil_scoped_release release;
                    return py::cast(self->listConfigurationFromName(deviceId, namePart));
                },
                py::arg("deviceId"), py::arg("namePart") = "",
                "listConfigurationFromName(deviceId, namePart): Returns the device configurations saved under names "
                "that "
                "contain a given name part.\n"
                "If an empty name part is given, all the configurations stored for the device will be returned.\n"
                "The function return is a Hash with the following keys:\n",
                "\"success\" a boolean indicating whether the operation was successful.\n"
                "\"reason\" a string describing the failure condition - empty on success.\n"
                "\"configs\" a vector of Hashes (HashList) with each hash containing information about a saved "
                "configuration.")

          .def(
                "getConfigurationFromName",
                [](const DeviceClientWrap::Pointer& self, const std::string& deviceId, const std::string& name) {
                    py::gil_scoped_release release;
                    return py::cast(self->getConfigurationFromName(deviceId, name));
                },
                py::arg("deviceId"), py::arg("name"),
                "getConfigurationFromName(deviceId, name): Returns the device configuration saved under a given name.\n"
                "May return an empty result if there's no configuration stored for the device under the given name.\n"
                "The function return is a Hash with the following keys:\n",
                "\"success\" a boolean indicating whether the operation was successful.\n"
                "\"reason\" a string describing the failure condition - empty on success.\n"
                "\"config\" a hash with data about the named device configuration.")

          .def(
                "getLastConfiguration",
                [](const DeviceClientWrap::Pointer& self, const std::string& deviceId, int priority) {
                    py::gil_scoped_release release;
                    return py::cast(self->getLastConfiguration(deviceId, priority));
                },
                py::arg("deviceId"), py::arg("priority") = 1,
                "getLastConfiguration(deviceId, priority): Returns the most recently saved device"
                "configuration with a "
                "given priority.\n"
                "May return an empty result if there's no"
                "configuration stored for the device with the given priority.\n"
                "The function return is a"
                "Hash with the following keys:\n",
                "\"success\" a boolean indicating whether the operation was successful.\n"
                "\"reason\" a string describing the failure condition - empty on success.\n"
                "\"config\" a hash with data about the most recent device configuration with the given"
                "priority.")

          .def(
                "saveConfigurationFromName",
                [](const DeviceClientWrap::Pointer& self, const std::string& name, const py::object& deviceIds,
                   const std::string& description, int priority, const std::string& user) {
                    std::vector<std::string> devices = wrapper::fromPySequenceToVectorString(deviceIds);
                    std::pair<bool, std::string> p;
                    {
                        py::gil_scoped_release release;
                        p = self->saveConfigurationFromName(name, devices, description, priority, user);
                    }
                    return py::make_tuple(p.first, p.second);
                },
                py::arg("name"), py::arg("deviceIds"), py::arg("description") = "", py::arg("priority") = 1,
                py::arg("user") = ".",
                "saveConfigurationFromName(name, deviceIds, description, priority, user):\n"
                "Saves the current device configurations (and the corresponding schemas) for a list of "
                "deviceIds\nin the Configuration Database under a common name, user, priority and "
                "description.\nThe function return is a pair (tuple) with a boolean value indicating the "
                "operation success as\nthe first value and a string detailing the failure cause when the "
                "operation fails.")

          .def(
                "registerInstanceNewMonitor",
                [](const DeviceClientWrap::Pointer& self, const py::object& handler) {
                    self->registerInstanceNewMonitor(HandlerWrap<const Hash&>(handler, "instanceNew monitor"));
                },
                py::arg("callbackFunction"),
                "registerInstanceNewMonitor(handler): Register callback handler \"handler\" to be called "
                "when new instances come online\nThe handler function should have the signature "
                "handler(topologyEntry) where \"topologyEntry\" is a Hash")

          .def(
                "registerInstanceUpdatedMonitor",
                [](const DeviceClientWrap::Pointer& self, const py::object& handler) {
                    self->registerInstanceUpdatedMonitor(HandlerWrap<const Hash&>(handler, "instanceUpdate monitor"));
                },
                py::arg("callbackFunction"),
                "registerInstanceUpdatedMonitor(handler): Register callback handler \"handler\" to be called "
                "when instances update their instanceInfo\nThe handler function should have the "
                "signature handler(topologyEntry) where \"topologyEntry\" is a Hash")

          .def(
                "registerInstanceGoneMonitor",
                [](const DeviceClientWrap::Pointer& self, const py::object& handler) {
                    self->registerInstanceGoneMonitor(
                          HandlerWrap<const std::string&, const Hash&>(handler, "instanceGone monitor"));
                },
                py::arg("callbackFunction"),
                "registerInstanceGoneMonitor(handler): Register callback handler \"handler\" to be called "
                "when instances go offline\nThe handler function should have the signature handler(instanceId,"
                " instanceInfo) where:\n\"instanceId\" is a string containing name of the device which went "
                "offline\n\"instanceInfo\" is a Hash")

          .def(
                "registerSchemaUpdatedMonitor",
                [](const DeviceClientWrap::Pointer& self, const py::object& handler) {
                    self->registerSchemaUpdatedMonitor(
                          HandlerWrap<const std::string&, const Schema&>(handler, "schemaUpdate monitor"));
                },
                py::arg("callbackFunction"),
                "registerSchemaUpdatedMonitor(handler): Register a callback handler \"handler\" to be "
                "triggered if an instance receives a schema update from the distributed system\nThe "
                "handler function should have the signature handler(instanceId, schema) where:\n"
                "\"instanceId\" is a string containing name of the device which updated schema\n"
                "\"schema\" is the updated schema\n\nNote: Currently, registering only a schema update "
                "monitor with an instance\nof a DeviceClient is not enough to have the registered callback "
                "activated.\nA workaround for this is to also register a property monitor with the\n"
                "same instance of DeviceClient that has been used to register the schema\nupdate monitor.\n\n")

          .def("registerPropertyMonitor", &DeviceClientWrap::registerPropertyMonitor, py::arg("instanceId"),
               py::arg("key"), py::arg("callbackFunction"), py::arg("userData") = py::none())

          .def(
                "registerDeviceMonitor",
                [](const DeviceClientWrap::Pointer& self, const std::string& instanceId, const py::object& handler,
                   const py::object& userData) {
                    if (userData.is_none()) {
                        self->registerDeviceMonitor(
                              instanceId, HandlerWrap<const std::string&, const Hash&>(handler, "device monitor"));
                    } else {
                        self->registerDeviceMonitor(instanceId,
                                                    HandlerWrap<const std::string&, const Hash&, const boost::any&>(
                                                          handler, "device monitor"),
                                                    userData);
                    }
                },
                py::arg("instanceId"), py::arg("callbackFunction"), py::arg("userData") = py::none())

          .def(
                "setDeviceMonitorInterval",
                [](const DeviceClientWrap::Pointer& self, long int milliseconds) {
                    py::gil_scoped_release release;
                    self->setDeviceMonitorInterval(milliseconds);
                },
                py::arg("milliseconds"))

          .def("unregisterPropertyMonitor", &DeviceClientWrap::unregisterPropertyMonitor, py::arg("instanceId"),
               py::arg("key"))

          .def("unregisterDeviceMonitor", &DeviceClientWrap::unregisterDeviceMonitor, py::arg("instanceId"))

          .def(
                "registerChannelMonitor",
                [](const DeviceClientWrap::Pointer& self, const std::string& channelName, const py::object& dataHandler,
                   const Hash& inputChannelConfig, const py::object& eosHandler, const py::object& inputHandler,
                   const py::object& statusTracker) {
                    DeviceClient::InputChannelHandlers handlers;
                    if (!dataHandler.is_none()) {
                        handlers.dataHandler = InputChannelDataHandler(dataHandler, "data");
                    }
                    if (!eosHandler.is_none()) {
                        handlers.eosHandler = HandlerWrap<const InputChannel::Pointer&>(eosHandler, "EOS");
                    }
                    if (!inputHandler.is_none()) {
                        handlers.inputHandler = HandlerWrap<const InputChannel::Pointer&>(inputHandler, "input");
                    }
                    if (!statusTracker.is_none()) {
                        handlers.statusTracker =
                              HandlerWrap<karabo::net::ConnectionStatus>(statusTracker, "channelStatusTracker");
                    }
                    py::gil_scoped_release release;

                    return self->registerChannelMonitor(channelName, handlers, inputChannelConfig);
                },
                py::arg("channelName"), py::arg("dataHandler") = py::none(), py::arg("inputChannelCfg") = Hash(),
                py::arg("eosHandler") = py::none(), py::arg("inputHandler") = py::none(),
                py::arg("statusTracker") = py::none(),
                "Register a handler to monitor defined output channel.\n\n"
                "Internally, an InputChannel is created and configured.\n"
                "@param channelName identifies the channel as a concatenation of the id of\n"
                "                    its device, a colon (:) and the name of the output\n"
                "                    channel (e.g. A/COOL/DEVICE:output)\n"
                "@param dataHandler called when data arrives, arguments are data (Hash) and\n"
                "                   meta data (Hash/MetaData)\n"
                "@param inputChannelCfg configures the channel via InputChannel.create(..)\n"
                "                 use default except you know what your are doing\n"
                "                 for experts: \"connectedOutputChannels\" will be overwritten\n"
                "@param eosHandler called on end of stream, argument is the InputChannel\n\n"
                "@param inputHandler called when data arrives, argument is the InputChannel\n\n"
                "@param statusTracker called with a 'ConnectionStatus' as argument when the "
                "connection\n                     status of the underlying InputChannel changes\n"
                "@return False if channel is already registered")

          .def(
                "unregisterChannelMonitor",
                [](const DeviceClientWrap::Pointer& self, const std::string& channelName) {
                    self->unregisterChannelMonitor(channelName);
                },
                py::arg("channelName"))

          .def(
                "set",
                [](const DeviceClientWrap::Pointer& self, const std::string& instanceId, const std::string& key,
                   const py::object& value, const std::string& keySep, int timeout) {
                    Hash tmp;
                    hashwrap::set(tmp, key, value, keySep);
                    {
                        py::gil_scoped_release release;
                        self->set(instanceId, tmp, timeout);
                    }
                },
                py::arg("deviceId"), py::arg("key"), py::arg("value"), py::arg("keySep") = ".",
                py::arg("timeoutInSeconds") = -1)

          .def(
                "set",
                [](const DeviceClientWrap::Pointer& self, const std::string& instanceId, const Hash& hash,
                   int timeout) {
                    py::gil_scoped_release release;
                    self->set(instanceId, hash, timeout);
                },
                py::arg("deviceId"), py::arg("hash"), py::arg("timeoutInSeconds") = -1)

          .def(
                "setAttribute",
                [](const DeviceClientWrap::Pointer& self, const std::string& instanceId, const std::string& key,
                   const std::string& attributeKey, const py::object& attributeValue, int timeoutInSeconds) {
                    boost::any attrValueAsAny;
                    wrapper::castPyToAny(attributeValue, attrValueAsAny);
                    py::gil_scoped_release release;
                    self->setAttribute(instanceId, key, attributeKey, attrValueAsAny, timeoutInSeconds);
                },
                py::arg("deviceId"), py::arg("key"), py::arg("attributeKey"), py::arg("attributeValue"),
                py::arg("timeoutInSeconds") = -1)

          .def(
                "setNoWait",
                [](const DeviceClientWrap::Pointer& self, const std::string& instanceId, const std::string& key,
                   const py::object& value, const std::string& keySep = ".") {
                    Hash tmp;
                    hashwrap::set(tmp, key, value, keySep);
                    py::gil_scoped_release release;
                    self->setNoWait(instanceId, tmp);
                },
                py::arg("instanceId"), py::arg("key"), py::arg("value"), py::arg("keySep") = ".")

          .def("execute", &DeviceClientWrap::execute, py::arg("instanceId"), py::arg("functionName"),
               py::arg("timeoutInSeconds") = -1)

          .def("executeNoWait", &DeviceClientWrap::executeNoWait, py::arg("instanceId"), py::arg("functionName"))

          .def(
                "getOutputChannelSchema",
                [](const DeviceClientWrap::Pointer& self, const std::string& deviceId,
                   const std::string& outputChannelName) {
                    Hash result;
                    {
                        py::gil_scoped_release release;
                        result = self->getOutputChannelSchema(deviceId, outputChannelName);
                    }
                    return py::cast(std::move(result));
                },
                py::arg("deviceId"), py::arg("outputChannelName"))

          .def(
                "getOutputChannelNames",
                [](const DeviceClientWrap::Pointer& self, const std::string& deviceId) {
                    std::vector<std::string> names;
                    {
                        py::gil_scoped_release release;
                        names = self->getOutputChannelNames(deviceId);
                    }
                    return py::cast(std::move(names));
                },
                py::arg("deviceId"))

          .def(
                "getDataSourceSchemaAsHash",
                [](const DeviceClientWrap::Pointer& self, const std::string& dataSourceId, int access) {
                    Hash::Pointer properties(new Hash());
                    {
                        py::gil_scoped_release release;
                        self->getDataSourceSchemaAsHash(dataSourceId, *properties, access);
                    }
                    return py::cast(properties);
                },
                py::arg("dataSourceId"),
                py::arg("accessType") = AccessType::INIT | AccessType::READ | AccessType::WRITE)

          .def("lock", &DeviceClientWrap::lockPy, py::arg("deviceId"), py::arg("recursive") = false,
               py::arg("timeout") = -1);
}
