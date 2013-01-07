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

            .def("getAvailableInstances", &DeviceClientWrap::getAvailableInstancesPy)
            .def("getSchema", (const Schema & (DeviceClient::*)(const string&, const string&, const string&))(&DeviceClient::getSchema), (bp::arg("instanceId"),bp::arg("key") = "", bp::arg("keySep") = "" ) , bp::return_value_policy<bp::copy_const_reference > ())
            
            .def("setDefaultTimeout", (void (DeviceClient::*)(const unsigned int))(&DeviceClient::setDefaultTimeout), bp::arg("defaultTimeout"))
            .def("getDefaultTimeout", (int (DeviceClient::*)( ) const)(&DeviceClient::getDefaultTimeout))
            
            .def("setDefaultKeySeparator", (void (DeviceClient::*)(const string&))(&DeviceClient::setDefaultKeySeparator), bp::arg("defaultKeySep"))
            .def("getDefaultKeySeparator", (const string& (DeviceClient::*)( ) const)(&DeviceClient::getDefaultKeySeparator), bp::return_value_policy<bp::copy_const_reference > ())
            
            .def("getDeviceServers", &DeviceClientWrap::getDeviceServersPy)
            .def("getDeviceClasses", &DeviceClientWrap::getDeviceClassesPy)
            .def("getDevices", &DeviceClientWrap::getDevicesPy)
            .def("getDeviceParameters", &DeviceClientWrap::getDeviceParametersPy, (bp::arg("instanceId"), bp::arg("key") = "", bp::arg("keySep") = "" ))
            .def("getDeviceParametersFlat", &DeviceClientWrap::getDeviceParametersFlatPy, (bp::arg("instanceId"), bp::arg("keySep") = "" ))
            .def("getCurrentlySettableProperties", &DeviceClientWrap::getCurrentlySettablePropertiesPy, (bp::arg("instanceId"), bp::arg("keySep") = "" ))
            .def("getCurrentlyExecutableCommands", &DeviceClientWrap::getCurrentlyExecutableCommandsPy, (bp::arg("instanceId"), bp::arg("keySep") = "" ))
            
            
            .def("isCommand", (bool (DeviceClient::*)(const string&, const string&, const string&))(&DeviceClient::isCommand), (bp::arg("instanceId"), bp::arg("key"), bp::arg("keySep") = "" ))
            .def("isProperty", (bool (DeviceClient::*)(const string&, const string&, const string&))(&DeviceClient::isProperty), (bp::arg("instanceId"), bp::arg("key"), bp::arg("keySep") = "" ))
            .def("isChoiceOfNodes", (bool (DeviceClient::*)(const string&, const string&, const string&))(&DeviceClient::isChoiceOfNodes), (bp::arg("instanceId"), bp::arg("key"), bp::arg("keySep") = "" ))
            .def("isListOfNodes", (bool (DeviceClient::*)(const string&, const string&, const string&))(&DeviceClient::isListOfNodes), (bp::arg("instanceId"), bp::arg("key"), bp::arg("keySep") = "" ))
            .def("isNode", (bool (DeviceClient::*)(const string&, const string&, const string&))(&DeviceClient::isNode), (bp::arg("instanceId"), bp::arg("key"), bp::arg("keySep") = "" ))
            .def("isLeaf", (bool (DeviceClient::*)(const string&, const string&, const string&))(&DeviceClient::isLeaf), (bp::arg("instanceId"), bp::arg("key"), bp::arg("keySep") = "" ))
            .def("isAccessInitOnly", (bool (DeviceClient::*)(const string&, const string&, const string&))(&DeviceClient::isAccessInitOnly), (bp::arg("instanceId"), bp::arg("key"), bp::arg("keySep") = "" ))
            .def("isAccessReconfigurable", (bool (DeviceClient::*)(const string&, const string&, const string&))(&DeviceClient::isAccessReconfigurable), (bp::arg("instanceId"), bp::arg("key"), bp::arg("keySep") = "" ))
            .def("isAccessReadOnly", (bool (DeviceClient::*)(const string&, const string&, const string&))(&DeviceClient::isAccessReadOnly), (bp::arg("instanceId"), bp::arg("key"), bp::arg("keySep") = "" ))
            .def("isAssignmentOptional", (bool (DeviceClient::*)(const string&, const string&, const string&))(&DeviceClient::isAssignmentOptional), (bp::arg("instanceId"), bp::arg("key"), bp::arg("keySep") = "" ))
            .def("isAssignmentMandatory", (bool (DeviceClient::*)(const string&, const string&, const string&))(&DeviceClient::isAssignmentMandatory), (bp::arg("instanceId"), bp::arg("key"), bp::arg("keySep") = "" ))
            
            .def("getValueTypeAsString", (string (DeviceClient::*)(const string&, const string&, const string&))(&DeviceClient::getValueTypeAsString), (bp::arg("instanceId"), bp::arg("key"), bp::arg("keySep") = "" ))
            .def("getDescription", (string (DeviceClient::*)(const string&, const string&, const string&))(&DeviceClient::getDescription), (bp::arg("instanceId"), bp::arg("key"), bp::arg("keySep") = "" ))
            .def("getDisplayedName", (string (DeviceClient::*)(const string&, const string&, const string&))(&DeviceClient::getDisplayedName), (bp::arg("instanceId"), bp::arg("key"), bp::arg("keySep") = "" ))
            .def("getDisplayType", (string (DeviceClient::*)(const string&, const string&, const string&))(&DeviceClient::getDisplayType), (bp::arg("instanceId"), bp::arg("key"), bp::arg("keySep") = "" ))
            .def("getUnitName", (string (DeviceClient::*)(const string&, const string&, const string&))(&DeviceClient::getUnitName), (bp::arg("instanceId"), bp::arg("key"), bp::arg("keySep") = "" ))
            .def("getUnitSymbol", (string (DeviceClient::*)(const string&, const string&, const string&))(&DeviceClient::getUnitSymbol), (bp::arg("instanceId"), bp::arg("key"), bp::arg("keySep") = "" ))
            
            .def("getAllowedStates", &DeviceClientWrap::getAllowedStatesPy, (bp::arg("instanceId"), bp::arg("key"), bp::arg("keySep") = "" ))
            .def("getValueOptions", &DeviceClientWrap::getValueOptionsPy, (bp::arg("instanceId"), bp::arg("key"), bp::arg("keySep") = "" ))
            
            .def("instantiateNoWait", (void (DeviceClient::*)(const string&, const string&, const Hash&))(&DeviceClient::instantiateNoWait), (bp::arg("serverInstanceId"), bp::arg("classId"), bp::arg("configuration") = karabo::util::Hash() ))
            .def("instantiateNoWait", (void (DeviceClient::*)(const string&, const Hash&))(&DeviceClient::instantiateNoWait), (bp::arg("serverInstanceId"), bp::arg("configuration")))
            .def("instantiateWait", (std::pair<bool, std::string> (DeviceClient::*)(const string&, const string&, const Hash&, int))(&DeviceClient::instantiateWait), (bp::arg("serverInstanceId"), bp::arg("classId"), bp::arg("configuration") = karabo::util::Hash(), bp::arg("timeout") = -1))
            .def("instantiateWait", (std::pair<bool, std::string> (DeviceClient::*)(const string&, const Hash&, int))(&DeviceClient::instantiateWait), (bp::arg("serverInstanceId"), bp::arg("configuration"), bp::arg("timeout") = -1))
            .def("kill", (void (DeviceClient::*)(const string&))(&DeviceClient::kill), bp::arg("instanceId"))
            
            .def("get", &DeviceClientWrap::getPy, (bp::arg("instanceId"), bp::arg("key"), bp::arg("keySep") = "."))
            .def("get", (const Hash& (DeviceClient::*)(const string&))(&DeviceClient::get), bp::arg("instanceId"), bp::return_value_policy<bp::copy_const_reference > ())
            
            .def("registerPropertyMonitor", (bool (DeviceClientWrap::*)(const string&, const string&, const bp::object&, const bp::object&))(&DeviceClientWrap::registerMonitor), (bp::arg("instanceId"), bp::arg("key"), bp::arg("callbackFunction"), bp::arg("userData") = bp::object()))
            .def("registerDeviceMonitor", (void (DeviceClientWrap::*)(const string&, const bp::object&, const bp::object&))(&DeviceClientWrap::registerMonitor), (bp::arg("instanceId"), bp::arg("callbackFunction"), bp::arg("userData") = bp::object()))
            
            .def("unregisterPropertyMonitor", (void (DeviceClient::*)(const string&, const string&))(&DeviceClient::unregisterMonitor), (bp::arg("instanceId"), bp::arg("key")))
            .def("unregisterDeviceMonitor", (void (DeviceClient::*)(const string&))(&DeviceClient::unregisterMonitor), bp::arg("instanceId"))
            
            
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

