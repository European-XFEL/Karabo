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
using namespace karabo::pyexfel;
using namespace std;
namespace bp = boost::python;

void exportPyCoreDeviceClient() {
    
    bp::class_<DeviceClient, boost::noncopyable > ("DeviceClientBase")
            .def(bp::init<const string&, const Hash&>())
            ;

    bp::class_<DeviceClientWrap, bp::bases<DeviceClient>, boost::noncopyable > ("DeviceClient")

            .def(bp::init<const string&, const Hash&>())
            .def(bp::init<boost::shared_ptr<SignalSlotableWrap>& >())
            
            .def("setDefaultTimeout", (void (DeviceClient::*)(const unsigned int))(&DeviceClient::setDefaultTimeout), bp::arg("defaultTimeout"))
            .def("getDefaultTimeout", (int (DeviceClient::*)( ) const)(&DeviceClient::getDefaultTimeout))
            .def("exists", &DeviceClientWrap::existsPy, (bp::arg("instanceId")))
            .def("getServers", &DeviceClientWrap::getServersPy)
            .def("getClasses", &DeviceClientWrap::getClassesPy,(bp::arg("instanceId")))
            .def("getDevices", &DeviceClientWrap::getDevicesPy)
            .def("getFullSchema",(karabo::util::Schema (DeviceClient::*)(const string&))(&DeviceClient::getFullSchema), (bp::arg("instanceId")))
            .def("getActiveSchema",(Schema (DeviceClient::*)(const string&))(&DeviceClient::getActiveSchema), (bp::arg("instanceId")))
            .def("getClassSchema",(Schema (DeviceClient::*)(const string&))(&DeviceClient::getClassSchema), (bp::arg("instanceId")))
            .def("getCurrentlySettableProperties", &DeviceClientWrap::getCurrentlySettablePropertiesPy, (bp::arg("instanceId")))
            .def("getCurrentlyExecutableCommands", &DeviceClientWrap::getCurrentlyExecutableCommandsPy, (bp::arg("instanceId")))
            
            .def("instantiateNoWait", (void (DeviceClient::*)(const string&, const string&, const Hash&))(&DeviceClient::instantiateNoWait), (bp::arg("serverInstanceId"), bp::arg("classId"), bp::arg("configuration") = karabo::util::Hash() ))
            .def("instantiateNoWait", (void (DeviceClient::*)(const string&, const Hash&))(&DeviceClient::instantiateNoWait), (bp::arg("serverInstanceId"), bp::arg("configuration")))
            //.def("instantiateWait", (std::pair<bool, std::string> (DeviceClient::*)(const string&, const string&, const Hash&, int))(&DeviceClient::instantiateWait), (bp::arg("serverInstanceId"), bp::arg("classId"), bp::arg("configuration") = karabo::util::Hash(), bp::arg("timeout") = -1))
            //.def("instantiateWait", (std::pair<bool, std::string> (DeviceClient::*)(const string&, const Hash&, int))(&DeviceClient::instantiateWait), (bp::arg("serverInstanceId"), bp::arg("configuration"), bp::arg("timeout") = -1))
            .def("killNoWait", (void (DeviceClient::*)(const string&))(&DeviceClient::killNoWait), bp::arg("instanceId"))
            
            .def("get", &DeviceClientWrap::getPy, (bp::arg("instanceId"), bp::arg("key"), bp::arg("keySep") = "."))
            .def("get", (Hash (DeviceClient::*)(const string&))(&DeviceClient::get), bp::arg("instanceId"))
            
            .def("registerPropertyMonitor", (bool (DeviceClientWrap::*)(const string&, const string&, const bp::object&, const bp::object&))(&DeviceClientWrap::registerPropertyMonitor), (bp::arg("instanceId"), bp::arg("key"), bp::arg("callbackFunction"), bp::arg("userData") = bp::object()))
            .def("registerDeviceMonitor", (void (DeviceClientWrap::*)(const string&, const bp::object&, const bp::object&))(&DeviceClientWrap::registerDeviceMonitor), (bp::arg("instanceId"), bp::arg("callbackFunction"), bp::arg("userData") = bp::object()))
            
            .def("unregisterPropertyMonitor", (void (DeviceClient::*)(const string&, const string&))(&DeviceClient::unregisterPropertyMonitor), (bp::arg("instanceId"), bp::arg("key")))
            .def("unregisterDeviceMonitor", (void (DeviceClient::*)(const string&))(&DeviceClient::unregisterDeviceMonitor), bp::arg("instanceId"))
            
            
            .def("setWait", &DeviceClientWrap::setWaitPy, (bp::arg("instanceId"), bp::arg("key"),bp::arg("value"),bp::arg("keySep")=".",bp::arg("timeout")=-1))
            .def("setNoWait", &DeviceClientWrap::setNoWaitPy, (bp::arg("instanceId"), bp::arg("key"),bp::arg("value"),bp::arg("keySep")="."))
            
            .def("executeWait", &DeviceClientWrap::executeWaitPy0,(bp::arg("instanceId"), bp::arg("functionName"), bp::arg("timeout")=-1))
            .def("executeWait", &DeviceClientWrap::executeWaitPy1,(bp::arg("instanceId"), bp::arg("functionName"), bp::arg("a1"), bp::arg("timeout")=-1))
            .def("executeWait", &DeviceClientWrap::executeWaitPy2,(bp::arg("instanceId"), bp::arg("functionName"), bp::arg("a1"), bp::arg("a2"), bp::arg("timeout")=-1))
            .def("executeWait", &DeviceClientWrap::executeWaitPy3,(bp::arg("instanceId"), bp::arg("functionName"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3"), bp::arg("timeout")=-1))
            .def("executeWait", &DeviceClientWrap::executeWaitPy4,(bp::arg("instanceId"), bp::arg("functionName"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3"), bp::arg("a4"), bp::arg("timeout")=-1))
            
            .def("executeNoWait", &DeviceClientWrap::executeNoWaitPy0,(bp::arg("instanceId"), bp::arg("functionName")))
            .def("executeNoWait", &DeviceClientWrap::executeNoWaitPy1,(bp::arg("instanceId"), bp::arg("functionName"), bp::arg("a1")))
            .def("executeNoWait", &DeviceClientWrap::executeNoWaitPy2,(bp::arg("instanceId"), bp::arg("functionName"), bp::arg("a1"), bp::arg("a2")))
            .def("executeNoWait", &DeviceClientWrap::executeNoWaitPy3,(bp::arg("instanceId"), bp::arg("functionName"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3")))
            .def("executeNoWait", &DeviceClientWrap::executeNoWaitPy4,(bp::arg("instanceId"), bp::arg("functionName"), bp::arg("a1"), bp::arg("a2"), bp::arg("a3"), bp::arg("a4")))
            ;
}

