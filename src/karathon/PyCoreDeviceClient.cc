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
            .def(bp::init<const string&, const Hash&>())
            ;

    bp::class_<DeviceClientWrap, bp::bases<DeviceClient>, boost::noncopyable > ("DeviceClient")

            .def(bp::init<const string&, const Hash&>())
            .def(bp::init<boost::shared_ptr<SignalSlotableWrap>& >())

            .def("login", &DeviceClient::login)
            .def("logout", &DeviceClient::logout)
            .def("setInternalTimeout", (void (DeviceClient::*)(const unsigned int))(&DeviceClient::setInternalTimeout), bp::arg("internalTimeout"))
            .def("getInternalTimeout", (int (DeviceClient::*)() const) (&DeviceClient::getInternalTimeout))
            .def("enableAdvancedMode", &DeviceClient::enableAdvancedMode)
            .def("disableAdvancedMode", &DeviceClient::disableAdvancedMode)
            .def("exists", &DeviceClientWrap::existsPy, (bp::arg("instanceId")))
            .def("getSystemInformation", (Hash(DeviceClient::*)())(&DeviceClient::getSystemInformation))
            .def("getSystemTopology", (Hash(DeviceClient::*)())(&DeviceClient::getSystemTopology))
            .def("getServers", &DeviceClientWrap::getServersPy)
            .def("getClasses", &DeviceClientWrap::getClassesPy, (bp::arg("instanceId")))
            .def("getDevices",  (bp::object (DeviceClientWrap::*)())(&DeviceClientWrap::getDevicesPy))
            .def("getDevices",  (bp::object (DeviceClientWrap::*)(const string&))(&DeviceClientWrap::getDevicesPy), (bp::arg("serverId")))
            .def("getDeviceSchema", (karabo::util::Schema(DeviceClient::*)(const string&))(&DeviceClient::getDeviceSchema), (bp::arg("instanceId")))
            .def("getActiveSchema", (Schema(DeviceClient::*)(const string&))(&DeviceClient::getActiveSchema), (bp::arg("instanceId")))
            .def("getClassSchema", (Schema(DeviceClient::*)(const string&, const string&))(&DeviceClient::getClassSchema), (bp::arg("serverId"), bp::arg("classId")))
            .def("getProperties", &DeviceClientWrap::getPropertiesPy, (bp::arg("deviceId")))
            .def("getClassProperties", &DeviceClientWrap::getClassPropertiesPy, (bp::arg("serverId"), bp::arg("classId")))
            .def("getCurrentlySettableProperties", &DeviceClientWrap::getCurrentlySettablePropertiesPy, (bp::arg("instanceId")))
            .def("getCurrentlyExecutableCommands", &DeviceClientWrap::getCurrentlyExecutableCommandsPy, (bp::arg("instanceId")))
            .def("instantiate", (bp::tuple (DeviceClientWrap::*)(const string&, const string&, const Hash&, int))(&DeviceClientWrap::instantiatePy), (bp::arg("serverId"), bp::arg("classId"), bp::arg("configuration"), bp::arg("timeoutInSeconds") = -1))
            .def("instantiate", (bp::tuple (DeviceClientWrap::*)(const string&, const Hash&, int))(&DeviceClientWrap::instantiatePy), (bp::arg("serverId"), bp::arg("configuration"), bp::arg("timeoutInSeconds") = -1))
            .def("instantiateNoWait", (void (DeviceClient::*)(const string&, const string&, const Hash&))(&DeviceClient::instantiateNoWait), (bp::arg("serverId"), bp::arg("classId"), bp::arg("configuration") = karabo::util::Hash()))
            .def("instantiateNoWait", (void (DeviceClient::*)(const string&, const Hash&))(&DeviceClient::instantiateNoWait), (bp::arg("serverId"), bp::arg("configuration")))
            .def("killDevice", (bp::tuple (DeviceClientWrap::*)(const std::string&, int))(&DeviceClientWrap::killDevicePy), (bp::arg("deviceId"), bp::arg("timeoutInSeconds") = -1))
            .def("killDeviceNoWait", (void (DeviceClient::*)(const string&))(&DeviceClient::killDeviceNoWait), bp::arg("deviceId"))
            .def("killServer", (bp::tuple (DeviceClientWrap::*)(const std::string&, int))(&DeviceClientWrap::killServerPy), (bp::arg("serverId"), bp::arg("timeoutInSeconds") = -1))
            .def("killServerNoWait", (void (DeviceClient::*)(const string&))(&DeviceClient::killServerNoWait), bp::arg("serverId"))
            .def("get", &DeviceClientWrap::getPy, (bp::arg("instanceId"), bp::arg("key"), bp::arg("keySep") = "."))
            .def("get", (Hash(DeviceClient::*)(const string&))(&DeviceClient::get), bp::arg("instanceId"))
            .def("getFromPast", &DeviceClientWrap::getFromPastPy, (bp::arg("deviceId"), bp::arg("key"), bp::arg("from"), bp::arg("to") = ""))
            .def("registerPropertyMonitor", (bool (DeviceClientWrap::*)(const string&, const string&, const bp::object&, const bp::object&))(&DeviceClientWrap::registerPropertyMonitor), (bp::arg("instanceId"), bp::arg("key"), bp::arg("callbackFunction"), bp::arg("userData") = bp::object()))
            .def("registerDeviceMonitor", (void (DeviceClientWrap::*)(const string&, const bp::object&, const bp::object&))(&DeviceClientWrap::registerDeviceMonitor), (bp::arg("instanceId"), bp::arg("callbackFunction"), bp::arg("userData") = bp::object()))
            .def("unregisterPropertyMonitor", (void (DeviceClient::*)(const string&, const string&))(&DeviceClient::unregisterPropertyMonitor), (bp::arg("instanceId"), bp::arg("key")))
            .def("unregisterDeviceMonitor", (void (DeviceClient::*)(const string&))(&DeviceClient::unregisterDeviceMonitor), bp::arg("instanceId"))
            .def("set", (bp::tuple (DeviceClientWrap::*)(const string&, const string&, const bp::object&, const std::string&, int))(&DeviceClientWrap::setPy),(bp::arg("deviceId"), bp::arg("key"), bp::arg("value"), bp::arg("keySep") = ".", bp::arg("timeoutInSeconds") = -1))
            .def("set", (bp::tuple (DeviceClientWrap::*)(const string&, const karabo::util::Hash&, int))(&DeviceClientWrap::setPy),(bp::arg("deviceId"), bp::arg("hash"), bp::arg("timeoutInSeconds") = -1))
            .def("setNoWait", &DeviceClientWrap::setNoWaitPy, (bp::arg("instanceId"), bp::arg("key"), bp::arg("value"), bp::arg("keySep") = "."))
            .def("execute", &DeviceClientWrap::executePy0, (bp::arg("instanceId"), bp::arg("functionName"), bp::arg("timeoutInSeconds") = -1))
            .def("execute", &DeviceClientWrap::executePy1, (bp::arg("instanceId"), bp::arg("functionName"), bp::arg("a1"), bp::arg("timeoutInSeconds") = -1))
            .def("execute", &DeviceClientWrap::executePy2, (bp::arg("instanceId"), bp::arg("functionName"), bp::arg("a1"), bp::arg("a2"), bp::arg("timeoutInSeconds") = -1))
            .def("execute", &DeviceClientWrap::executePy3, (bp::arg("instanceId"), bp::arg("functionName"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3"), bp::arg("timeoutInSeconds") = -1))
            .def("execute", &DeviceClientWrap::executePy4, (bp::arg("instanceId"), bp::arg("functionName"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3"), bp::arg("a4"), bp::arg("timeoutInSeconds") = -1))
            .def("executeNoWait", &DeviceClientWrap::executeNoWaitPy0, (bp::arg("instanceId"), bp::arg("functionName")))
            .def("executeNoWait", &DeviceClientWrap::executeNoWaitPy1, (bp::arg("instanceId"), bp::arg("functionName"), bp::arg("a1")))
            .def("executeNoWait", &DeviceClientWrap::executeNoWaitPy2, (bp::arg("instanceId"), bp::arg("functionName"), bp::arg("a1"), bp::arg("a2")))
            .def("executeNoWait", &DeviceClientWrap::executeNoWaitPy3, (bp::arg("instanceId"), bp::arg("functionName"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3")))
            .def("executeNoWait", &DeviceClientWrap::executeNoWaitPy4, (bp::arg("instanceId"), bp::arg("functionName"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3"), bp::arg("a4")))
            ;
}

