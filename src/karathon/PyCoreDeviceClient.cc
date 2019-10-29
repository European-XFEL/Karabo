/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>,
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/python.hpp>

#include <karabo/core/DeviceClient.hh>
#include "DeviceClientWrap.hh"

using namespace karabo::xms;
using namespace karabo::util;
using namespace karabo::core;
using namespace karathon;
using namespace std;
namespace bp = boost::python;


void exportPyCoreDeviceClient() {

    bp::class_<DeviceClient, boost::noncopyable > ("DeviceClientBase")
            .def(bp::init<const string&>())
            ;

    bp::class_<DeviceClientWrap, boost::shared_ptr<DeviceClientWrap>, bp::bases<DeviceClient>, boost::noncopyable > ("DeviceClient")

            .def(bp::init<const string&>())
            .def(bp::init<boost::shared_ptr<SignalSlotableWrap>& >())

            .def("login", &DeviceClient::login)
            .def("logout", &DeviceClient::logout)
            .def("setInternalTimeout", (void (DeviceClient::*)(const unsigned int))(&DeviceClient::setInternalTimeout), bp::arg("internalTimeout"))
            .def("getInternalTimeout", (int (DeviceClient::*)() const) (&DeviceClient::getInternalTimeout))
            .def("exists", &DeviceClientWrap::existsPy, (bp::arg("instanceId")))
            .def("enableInstanceTracking", &DeviceClientWrap::enableInstanceTrackingPy,
                 "Enables tracking of new and departing device instances\n\n"
                 "The handlers registered with registerInstance[New|Gone|Updated]Monitor\n"
                 "will be called accordingly. If the handler for instanceNew is registered before\n"
                 "calling this method, it will be called for each device currently in the system.\n\n"
                 "NOTE: Use wisely!\n"
                 "There is a performance cost to tracking all devices since it means\n"
                 "subscribing to the heartbeats of all servers and devices in the system.")
            .def("getSystemInformation", &DeviceClientWrap::getSystemInformationPy)
            .def("getSystemTopology", &DeviceClientWrap::getSystemTopologyPy)
            .def("getServers", &DeviceClientWrap::getServersPy)
            .def("getClasses", &DeviceClientWrap::getClassesPy, (bp::arg("instanceId")))
            .def("getDevices", (bp::object(DeviceClientWrap::*)())(&DeviceClientWrap::getDevicesPy))
            .def("getDevices", (bp::object(DeviceClientWrap::*)(const string&))(&DeviceClientWrap::getDevicesPy), (bp::arg("serverId")))
            .def("getDeviceSchema", (karabo::util::Schema(DeviceClientWrap::*)(const string&))(&DeviceClientWrap::getDeviceSchema), (bp::arg("instanceId")))
            .def("getDeviceSchemaNoWait", (karabo::util::Schema(DeviceClientWrap::*)(const string&))(&DeviceClientWrap::getDeviceSchemaNoWait), (bp::arg("instanceId")))
            .def("getActiveSchema", (Schema(DeviceClientWrap::*)(const string&))(&DeviceClientWrap::getActiveSchema), (bp::arg("instanceId")))
            .def("getClassSchema", (Schema(DeviceClientWrap::*)(const string&, const string&))(&DeviceClientWrap::getClassSchema), (bp::arg("serverId"), bp::arg("classId")))
            .def("getProperties", &DeviceClientWrap::getPropertiesPy, (bp::arg("deviceId")))
            .def("getClassProperties", &DeviceClientWrap::getClassPropertiesPy, (bp::arg("serverId"), bp::arg("classId")))
            .def("getCurrentlySettableProperties", &DeviceClientWrap::getCurrentlySettablePropertiesPy, (bp::arg("instanceId")))
            .def("getCurrentlyExecutableCommands", &DeviceClientWrap::getCurrentlyExecutableCommandsPy, (bp::arg("instanceId")))
            .def("instantiate", (bp::tuple(DeviceClientWrap::*)(const string&, const string&, const Hash&, int))(&DeviceClientWrap::instantiatePy), (bp::arg("serverId"), bp::arg("classId"), bp::arg("configuration"), bp::arg("timeoutInSeconds") = -1))
            .def("instantiate", (bp::tuple(DeviceClientWrap::*)(const string&, const Hash&, int))(&DeviceClientWrap::instantiatePy), (bp::arg("serverId"), bp::arg("configuration"), bp::arg("timeoutInSeconds") = -1))
            .def("instantiateNoWait", (void (DeviceClient::*)(const string&, const string&, const Hash&))(&DeviceClient::instantiateNoWait), (bp::arg("serverId"), bp::arg("classId"), bp::arg("configuration") = karabo::util::Hash()))
            .def("instantiateNoWait", (void (DeviceClient::*)(const string&, const Hash&))(&DeviceClient::instantiateNoWait), (bp::arg("serverId"), bp::arg("configuration")))
            .def("killDevice", (bp::tuple(DeviceClientWrap::*)(const std::string&, int))(&DeviceClientWrap::killDevicePy), (bp::arg("deviceId"), bp::arg("timeoutInSeconds") = -1))
            .def("killDeviceNoWait", (void (DeviceClient::*)(const string&))(&DeviceClient::killDeviceNoWait), bp::arg("deviceId"))
            .def("killServer", (bp::tuple(DeviceClientWrap::*)(const std::string&, int))(&DeviceClientWrap::killServerPy), (bp::arg("serverId"), bp::arg("timeoutInSeconds") = -1))
            .def("killServerNoWait", (void (DeviceClient::*)(const string&))(&DeviceClient::killServerNoWait), bp::arg("serverId"))
            .def("get", &DeviceClientWrap::getPy, (bp::arg("instanceId"), bp::arg("key"), bp::arg("keySep") = "."))
            .def("get", &DeviceClientWrap::getConfigurationPy, (bp::arg("instanceId")))
            .def("getFromPast", &DeviceClientWrap::getFromPastPy, (bp::arg("deviceId"), bp::arg("key"), bp::arg("from"), bp::arg("to") = "", bp::arg("maxNumData") = 0))
            .def("getPropertyHistory", &DeviceClientWrap::getPropertyHistoryPy, (bp::arg("deviceId"), bp::arg("key"), bp::arg("from"), bp::arg("to") = "", bp::arg("maxNumData") = 0))
            .def("getConfigurationFromPast", &DeviceClientWrap::getConfigurationFromPastPy, (bp::arg("deviceId"), bp::arg("timePoint")))
            .def("registerInstanceNewMonitor", &DeviceClientWrap::registerInstanceNewMonitor, bp::arg("callbackFunction"),
                 "registerInstanceNewMonitor(handler): Register callback handler \"handler\" to be called when new instances come online\n"
                 "The handler function should have the signature handler(topologyEntry) where \"topologyEntry\" is a Hash")
            .def("registerInstanceUpdatedMonitor", &DeviceClientWrap::registerInstanceUpdatedMonitor, bp::arg("callbackFunction"),
                 "registerInstanceUpdatedMonitor(handler): Register callback handler \"handler\" to be called when instances update their instanceInfo\n"
                 "The handler function should have the signature handler(topologyEntry) where \"topologyEntry\" is a Hash")
            .def("registerInstanceGoneMonitor", &DeviceClientWrap::registerInstanceGoneMonitor, bp::arg("callbackFunction"),
                 "registerInstanceGoneMonitor(handler): Register callback handler \"handler\" to be called when instances go offline\n"
                 "The handler function should have the signature handler(instanceId, instanceInfo) where:\n"
                 "\"instanceId\" is a string containing name of the device which went offline\n"
                 "\"instanceInfo\" is a Hash")
            .def("registerSchemaUpdatedMonitor", &DeviceClientWrap::registerSchemaUpdatedMonitor, bp::arg("callbackFunction"),
                 "registerSchemaUpdatedMonitor(handler): Register a callback handler \"handler\" to be triggered if an instance receives a schema update from the distributed system\n"
                 "The handler function should have the signature handler(instanceId, schema) where:\n"
                 "\"instanceId\" is a string containing name of the device which updated schema\n"
                 "\"schema\" is the updated schema\n\n"
                 "Note: Currently, registering only a schema update monitor with an instance\n"
                 "of a DeviceClient is not enough to have the registered call-back activated.\n"
                 "A workaround for this is to also register a property monitor with the\n"
                 "same instance of DeviceClient that has been used to register the schema\n"
                 "update monitor.\n\n")
            .def("registerPropertyMonitor", (bool (DeviceClientWrap::*)(const string&, const string&, const bp::object&, const bp::object&))(&DeviceClientWrap::registerPropertyMonitor), (bp::arg("instanceId"), bp::arg("key"), bp::arg("callbackFunction"), bp::arg("userData") = bp::object()))
            .def("registerDeviceMonitor", (void (DeviceClientWrap::*)(const string&, const bp::object&, const bp::object&))(&DeviceClientWrap::registerDeviceMonitor), (bp::arg("instanceId"), bp::arg("callbackFunction"), bp::arg("userData") = bp::object()))
            .def("setDeviceMonitorInterval", &DeviceClientWrap::setDeviceMonitorIntervalPy, bp::arg("milliseconds"))
            .def("unregisterPropertyMonitor", (void (DeviceClient::*)(const string&, const string&))(&DeviceClient::unregisterPropertyMonitor), (bp::arg("instanceId"), bp::arg("key")))
            .def("unregisterDeviceMonitor", (void (DeviceClient::*)(const string&))(&DeviceClient::unregisterDeviceMonitor), bp::arg("instanceId"))
            .def("registerChannelMonitor", &DeviceClientWrap::registerChannelMonitorPy,
                 (bp::arg("channelName"), bp::arg("dataHandler"), bp::arg("inputChannelCfg") = karabo::util::Hash(),
                  bp::arg("eosHandler") = bp::object()),
                 "Register a handler to monitor defined output channel.\n\n"
                 "Internally, an InputChannel is created and configured.\n"
                 "@param channelName identifies the channel as a concatenation of the id of\n"
                 "                    its device, a colon (:) and the name of the output\n"
                 "                    channel (e.g. A/COOL/DEVICE:output)\n"
                 "@param dataHandler called when data arrives, arguments are data (Hash) and\n"
                 "                   meta data (Hash/MetaData)\n"
                 "@param inputChannelCfg configures the channel via InputChannel.create(..)\n"
                 "                 use default except you know what your are doing\n"
                 "                 for experts: \"connectedOutputChannels\" will be overwritten,\n"
                 "                              \"onSlowness\" default is overwritten to \"drop\"\n"
                 "@param eosHandler called on end of stream, argument is the InputChannel\n\n"
                 "@return False if channel is already registered")
            .def("unregisterChannelMonitor", &DeviceClientWrap::unregisterChannelMonitorPy, bp::arg("channelName"),
                 "Unregister monitoring of output channel\n\n"
                 "@param channelName identifies the channel as a concatenation of the id of\n"
                 "                   its devices, a colon (:) and the name of the output\n"
                 "                  channel (e.g. A/COOL/DEVICE:output)\n\n"
                 "@return False if channel was not registered")
            .def("set", (void(DeviceClientWrap::*)(const string&, const string&, const bp::object&, const std::string&, int))(&DeviceClientWrap::setPy), (bp::arg("deviceId"), bp::arg("key"), bp::arg("value"), bp::arg("keySep") = ".", bp::arg("timeoutInSeconds") = -1))
            .def("set", (void(DeviceClientWrap::*)(const string&, const karabo::util::Hash&, int))(&DeviceClientWrap::setPy), (bp::arg("deviceId"), bp::arg("hash"), bp::arg("timeoutInSeconds") = -1))
            .def("setAttribute", (void(DeviceClientWrap::*)(const std::string&, const std::string&, const std::string&, const bp::object&, int))(&DeviceClientWrap::setAttributePy),
                 (bp::arg("deviceId"), bp::arg("key"), bp::arg("attributeKey"), bp::arg("attributeValue"), bp::arg("timeoutInSeconds") = -1))
            .def("setNoWait", &DeviceClientWrap::setNoWaitPy, (bp::arg("instanceId"), bp::arg("key"), bp::arg("value"), bp::arg("keySep") = "."))
            .def("execute", &DeviceClientWrap::executePy, (bp::arg("instanceId"), bp::arg("functionName"), bp::arg("timeoutInSeconds") = -1))
            .def("executeNoWait", &DeviceClientWrap::executeNoWaitPy, (bp::arg("instanceId"), bp::arg("functionName")))
            .def("getOutputChannelSchema", &DeviceClientWrap::getOutputChannelSchema, (bp::arg("deviceId"), bp::arg("outputChannelName")))
            .def("getOutputChannelNames", &DeviceClientWrap::getOutputChannelNames, (bp::arg("deviceId")))
            .def("getDataSourceSchemaAsHash", &DeviceClientWrap::getDataSourceSchemaAsHashPy, (bp::arg("dataSourceId"), bp::arg("accessType") = AccessType::INIT|AccessType::READ|AccessType::WRITE))
            .def("lock", &DeviceClientWrap::lockPy, (bp::arg("deviceId"), bp::arg("recursive") = false, bp::arg("timeout") = -1))
            ;
}

