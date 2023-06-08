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
/*
 * File:   ComWrap.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on July 11, 2012, 11:22 AM
 */

#ifndef KARATHON_DEVCOM_HH
#define KARATHON_DEVCOM_HH

#include <boost/function.hpp>
#include <boost/python.hpp>
#include <iostream>
#include <karabo/core/DeviceClient.hh>

#include "HashWrap.hh"
#include "PyCoreLockWrap.hh"
#include "ScopedGILAcquire.hh"
#include "ScopedGILRelease.hh"
#include "SignalSlotableWrap.hh"
#include "Wrapper.hh"

namespace bp = boost::python;
using namespace karabo::xms;

namespace karathon {

    class DeviceClientWrap : public karabo::core::DeviceClient {
       public:
        DeviceClientWrap(const std::string& instanceId = std::string()) : DeviceClient(instanceId), m_isVerbose(true) {
            boost::shared_ptr<karabo::xms::SignalSlotable> p = m_signalSlotable.lock();
            if (!p) {
                throw KARABO_PARAMETER_EXCEPTION("Broker connection is not valid.");
            }
            p->updateInstanceInfo(karabo::util::Hash("lang", "bound"));
            m_signalSlotableWrap = boost::static_pointer_cast<SignalSlotableWrap>(p);
        }

        DeviceClientWrap(boost::shared_ptr<SignalSlotableWrap>& o)
            : DeviceClient(boost::static_pointer_cast<karabo::xms::SignalSlotable>(o)), m_isVerbose(true) {
            boost::shared_ptr<karabo::xms::SignalSlotable> p = m_signalSlotable.lock();
            if (!p) {
                throw KARABO_PARAMETER_EXCEPTION("Broker connection is not valid.");
            }
            m_signalSlotableWrap = boost::static_pointer_cast<SignalSlotableWrap>(p);
        }

        ~DeviceClientWrap() {}

        std::string getInstanceIdPy() {
            return this->getInstanceId();
        }

        bp::tuple instantiatePy(const std::string& serverId, const karabo::util::Hash& configuration,
                                int timeoutInSeconds) {
            std::pair<bool, std::string> instantiateReply;
            {
                ScopedGILRelease nogil;
                instantiateReply = this->instantiate(serverId, configuration, timeoutInSeconds);
            }
            return Wrapper::fromStdPairToPyTuple(instantiateReply);
        }

        bp::tuple instantiatePy(const std::string& serverId, const std::string& classId,
                                const karabo::util::Hash& configuration, int timeoutInSeconds) {
            std::pair<bool, std::string> instantiateReply;
            {
                ScopedGILRelease nogil;
                instantiateReply = this->instantiate(serverId, classId, configuration, timeoutInSeconds);
            }
            return Wrapper::fromStdPairToPyTuple(instantiateReply);
        }

        bp::tuple killDevicePy(const std::string& deviceId, int timeoutInSeconds = -1) {
            std::pair<bool, std::string> killReply;
            {
                ScopedGILRelease nogil;
                killReply = this->killDevice(deviceId, timeoutInSeconds);
            }
            return Wrapper::fromStdPairToPyTuple(killReply);
        }

        bp::tuple killServerPy(const std::string& serverId, int timeoutInSeconds = -1) {
            std::pair<bool, std::string> killReply;
            {
                ScopedGILRelease nogil;
                killReply = this->killServer(serverId, timeoutInSeconds);
            }
            return Wrapper::fromStdPairToPyTuple(killReply);
        }

        bp::tuple existsPy(const std::string& instanceId) {
            std::pair<bool, std::string> existsReply;
            {
                ScopedGILRelease nogil;
                existsReply = this->exists(instanceId);
            }
            return Wrapper::fromStdPairToPyTuple(existsReply);
        }

        void enableInstanceTrackingPy() {
            ScopedGILRelease nogil;
            enableInstanceTracking();
        }

        karabo::util::Hash getSystemInformationPy() {
            ScopedGILRelease nogil;
            return getSystemInformation();
        }

        karabo::util::Hash getSystemTopologyPy() {
            ScopedGILRelease nogil;
            return getSystemTopology();
        }

        bp::list getServersPy() {
            std::vector<std::string> servers;
            {
                ScopedGILRelease nogil;
                servers = this->getServers();
            }
            return Wrapper::fromStdVectorToPyList(servers);
        }

        bp::list getClassesPy(const std::string& deviceServer) {
            std::vector<std::string> devClasses;
            {
                ScopedGILRelease nogil;
                devClasses = this->getClasses(deviceServer);
            }
            return Wrapper::fromStdVectorToPyList(devClasses);
        }

        bp::list getDevicesPy() {
            std::vector<std::string> devices;
            {
                ScopedGILRelease nogil;
                devices = this->getDevices();
            }
            return Wrapper::fromStdVectorToPyList(devices);
        }

        bp::list getDevicesByServerPy(const std::string& serverId) {
            std::vector<std::string> devices;
            {
                ScopedGILRelease nogil;
                devices = this->getDevices(serverId);
            }
            return Wrapper::fromStdVectorToPyList(devices);
        }

        bp::list getPropertiesPy(const std::string& deviceId) {
            std::vector<std::string> props;
            {
                ScopedGILRelease nogil;
                props = this->getProperties(deviceId);
            }
            return Wrapper::fromStdVectorToPyList(props);
        }

        bp::list getClassPropertiesPy(const std::string& serverId, const std::string& classId) {
            std::vector<std::string> classProps;
            {
                ScopedGILRelease nogil;
                classProps = this->getClassProperties(serverId, classId);
            }
            return Wrapper::fromStdVectorToPyList(classProps);
        }

        bp::list getCurrentlySettablePropertiesPy(const std::string& instanceId) {
            std::vector<std::string> props;
            {
                ScopedGILRelease nogil;
                props = this->getCurrentlySettableProperties(instanceId);
            }
            return Wrapper::fromStdVectorToPyList(props);
        }

        bp::list getCurrentlyExecutableCommandsPy(const std::string& instanceId) {
            std::vector<std::string> cmds;
            {
                ScopedGILRelease nogil;
                cmds = this->getCurrentlyExecutableCommands(instanceId);
            }
            return Wrapper::fromStdVectorToPyList(cmds);
        }

        bp::object getPy(const std::string& instanceId, const std::string& key, const std::string& keySep = ".") {
            boost::any value;
            try {
                ScopedGILRelease nogil;
                value = this->DeviceClient::getAsAny(instanceId, key, keySep.at(0));
            } catch (...) {
                KARABO_RETHROW_AS(KARABO_PARAMETER_EXCEPTION("The key \"" + key + "\" is not found in the device \"" +
                                                             instanceId + "\""));
            }
            return Wrapper::toObject(value, HashWrap::isDefault(PyTypes::PYTHON_DEFAULT));
        }

        karabo::util::Hash getConfigurationPy(const std::string& instanceId) {
            ScopedGILRelease nogil;
            return this->DeviceClient::get(instanceId);
        }

        karabo::util::Schema getDeviceSchema(const std::string& instanceId) {
            ScopedGILRelease nogil;
            return this->DeviceClient::getDeviceSchema(instanceId);
        }

        karabo::util::Schema getDeviceSchemaNoWait(const std::string& instanceId) {
            ScopedGILRelease nogil;
            return this->DeviceClient::getDeviceSchemaNoWait(instanceId);
        }

        karabo::util::Schema getActiveSchema(const std::string& instanceId) {
            ScopedGILRelease nogil;
            return this->DeviceClient::getActiveSchema(instanceId);
        }

        karabo::util::Schema getClassSchema(const std::string& serverId, const std::string& classId) {
            ScopedGILRelease nogil;
            return this->DeviceClient::getClassSchema(serverId, classId);
        }

        void registerInstanceNewMonitor(const bp::object& handler) {
            this->DeviceClient::registerInstanceNewMonitor(
                  HandlerWrap<const karabo::util::Hash&>(handler, "instanceNew monitor"));
        }

        void registerInstanceUpdatedMonitor(const bp::object& handler) {
            this->DeviceClient::registerInstanceUpdatedMonitor(
                  HandlerWrap<const karabo::util::Hash&>(handler, "instanceUpdated monitor"));
        }

        void registerInstanceGoneMonitor(const bp::object& handler) {
            this->DeviceClient::registerInstanceGoneMonitor(
                  HandlerWrap<const std::string&, const karabo::util::Hash&>(handler, "instanceGone monitor"));
        }

        void registerSchemaUpdatedMonitor(const bp::object& handler) {
            this->DeviceClient::registerSchemaUpdatedMonitor(
                  HandlerWrap<const std::string&, const karabo::util::Schema&>(handler, "schemaUpdate monitor"));
        }

        void registerDeviceMonitor(const std::string& instanceId, const bp::object& handler,
                                   const bp::object& userData = bp::object()) {
            if (userData.is_none()) {
                this->DeviceClient::registerDeviceMonitor(
                      instanceId,
                      HandlerWrap<const std::string&, const karabo::util::Hash&>(handler, "device monitor"));
            } else {
                this->DeviceClient::registerDeviceMonitor(
                      instanceId,
                      HandlerWrap<const std::string&, const karabo::util::Hash&, const boost::any&>(handler,
                                                                                                    "device monitor 2"),
                      userData); // TODO: C++ stores this bp::object and might delete it without GIL protection!
            }
        }

        bool registerPropertyMonitor(const std::string& instanceId, const std::string& key,
                                     const bp::object& callbackFunction, const bp::object& userData = bp::object()) {
            karabo::util::Schema schema;
            {
                ScopedGILRelease nogil;
                schema = this->getDeviceSchema(instanceId);
            }
            if (schema.has(key)) {
                {
                    ScopedGILRelease nogil;
                    this->cacheAndGetConfiguration(instanceId);
                }

                {
                    // TODO: Handling of callback via storage of bp::object::ptr() inside C++ treated container
                    // (m_propertyChangedHandlers)
                    //       is garbage - it will at least make the Python reference counting wrong...
                    boost::mutex::scoped_lock lock(m_propertyChangedHandlersMutex);
                    if (Wrapper::hasattr(callbackFunction, "__self__")) {
                        const bp::object& selfObject(callbackFunction.attr("__self__"));
                        std::string funcName(bp::extract<std::string>(callbackFunction.attr("__name__")));
                        m_propertyChangedHandlers.set(instanceId + "." + key + "._function", funcName);
                        m_propertyChangedHandlers.set(instanceId + "." + key + "._selfObject", selfObject.ptr());
                    } else {
                        m_propertyChangedHandlers.set(instanceId + "." + key + "._function", callbackFunction.ptr());
                    }
                    if (!userData.is_none())
                        m_propertyChangedHandlers.set(instanceId + "." + key + "._userData", userData);
                }

                immortalize(instanceId);
                return true;
            } else {
                return false;
            }
        }

        bool registerChannelMonitorPy(const std::string& channelName, const bp::object& dataHandler = bp::object(),
                                      const karabo::util::Hash& inputChannelCfg = karabo::util::Hash(),
                                      const bp::object& eosHandler = bp::object(),
                                      const bp::object& inputHandler = bp::object(),
                                      const bp::object& statusTracker = bp::object()) {
            InputChannelHandlers handlers;

            if (!dataHandler.is_none()) {
                handlers.dataHandler = InputChannelWrap::DataHandlerWrap(dataHandler, "data");
            }
            if (!eosHandler.is_none()) {
                handlers.eosHandler = HandlerWrap<const karabo::xms::InputChannel::Pointer&>(eosHandler, "EOS");
            }
            if (!inputHandler.is_none()) {
                handlers.inputHandler = HandlerWrap<const karabo::xms::InputChannel::Pointer&>(inputHandler, "input");
            }
            if (!statusTracker.is_none()) {
                handlers.statusTracker =
                      HandlerWrap<karabo::net::ConnectionStatus>(statusTracker, "channelStatusTracker");
            }

            ScopedGILRelease nogil;
            return this->DeviceClient::registerChannelMonitor(channelName, handlers, inputChannelCfg);
        }

        bool unregisterChannelMonitorPy(const std::string& channelName) {
            // Need this wrapper since DeviceClient::unregisterChannelMonitor is overloaded.
            // Otherwise we could us that one directly.
            return this->DeviceClient::unregisterChannelMonitor(channelName);
        }


        void setDeviceMonitorIntervalPy(long int milliseconds) {
            ScopedGILRelease nogil;
            setDeviceMonitorInterval(milliseconds);
        }

        void setPy(const std::string& instanceId, const std::string& key, const bp::object& value,
                   const std::string& keySep = ".", int timeout = -1) {
            karabo::util::Hash tmp;
            HashWrap::set(tmp, key, value, keySep);
            {
                ScopedGILRelease nogil;
                this->set(instanceId, tmp, timeout);
            }
        }

        void setPy(const std::string& instanceId, const karabo::util::Hash& value, int timeout = -1) {
            ScopedGILRelease nogil;
            this->set(instanceId, value, timeout);
        }

        void setNoWaitPy(const std::string& instanceId, const std::string& key, const bp::object& value,
                         const std::string& keySep = ".") {
            karabo::util::Hash tmp;
            HashWrap::set(tmp, key, value, keySep);
            ScopedGILRelease nogil;
            this->setNoWait(instanceId, tmp);
        }

        void setAttributePy(const std::string& instanceId, const std::string& key, const std::string& attributeKey,
                            const bp::object& attributeValue, int timeoutInSeconds = -1) {
            // HashWrap::set does special treatment of Hash, PyArray (i.e. numpy array), ImageData and PyDict -
            // but we do not want these as attributes anyway...
            boost::any attrValueAsAny;
            Wrapper::toAny(attributeValue, attrValueAsAny);

            ScopedGILRelease nogil;
            this->setAttribute(instanceId, key, attributeKey, attrValueAsAny, timeoutInSeconds);
        }

        void executeNoWaitPy(std::string instanceId, const std::string& functionName) {
            ScopedGILRelease nogil;
            m_signalSlotableWrap->call(instanceId, functionName);
        }

        void executePy(std::string instanceId, const std::string& functionName, int timeout = -1) {
            ScopedGILRelease nogil;
            execute(instanceId, functionName, timeout);
        }

        bp::object getPropertyHistoryPy(const std::string& deviceId, const std::string& key, const std::string& from,
                                        std::string to = "", unsigned int maxNumData = 0) {
            std::vector<karabo::util::Hash> propHist;
            {
                ScopedGILRelease nogil;
                propHist = this->getPropertyHistory(deviceId, key, from, to, maxNumData);
            }
            return Wrapper::fromStdVectorToPyHashList(propHist);
        }

        karabo::util::Hash listConfigurationFromName(const std::string& deviceId, const std::string& namePart) {
            ScopedGILRelease nogil;
            return this->DeviceClient::listConfigurationFromName(deviceId, namePart);
        }

        karabo::util::Hash getConfigurationFromName(const std::string& deviceId, const std::string& name) {
            ScopedGILRelease nogil;
            return this->DeviceClient::getConfigurationFromName(deviceId, name);
        }

        karabo::util::Hash getLastConfiguration(const std::string& deviceId, int priority) {
            ScopedGILRelease nogil;
            return this->DeviceClient::getLastConfiguration(deviceId, priority);
        }

        bp::tuple saveConfigurationFromNamePy(const std::string& name, const bp::object& deviceIds,
                                              const std::string& description, int priority, const std::string& user) {
            ScopedGILRelease nogil;

            return Wrapper::fromStdPairToPyTuple(this->DeviceClient::saveConfigurationFromName(
                  name, karathon::Wrapper::fromPyListToStdVector<std::string>(deviceIds), description, priority, user));
        }

        bp::object getConfigurationFromPastPy(const std::string& deviceId, const std::string& timePoint) {
            return Wrapper::fromStdPairToPyTuple<karabo::util::Hash, karabo::util::Schema>(
                  this->getConfigurationFromPast(deviceId, timePoint));
        }

        karabo::util::Hash getOutputChannelSchema(const std::string& deviceId, const std::string& outputChannelName) {
            ScopedGILRelease nogil;
            return this->DeviceClient::getOutputChannelSchema(deviceId, outputChannelName);
        }

        std::vector<std::string> getOutputChannelNames(const std::string& deviceId) {
            ScopedGILRelease nogil;
            return this->DeviceClient::getOutputChannelNames(deviceId);
        }

        karabo::util::Hash::Pointer getDataSourceSchemaAsHashPy(const std::string& dataSourceId, int access) {
            ScopedGILRelease nogil;
            karabo::util::Hash::Pointer properties(new karabo::util::Hash());
            getDataSourceSchemaAsHash(dataSourceId, *properties, access);
            return properties;
        }

        boost::shared_ptr<karathon::LockWrap> lockPy(const std::string& deviceId, bool recursive, int timeout) {
            // non waiting request for lock

            if (timeout == 0) {
                return boost::make_shared<karathon::LockWrap>(
                      boost::make_shared<karabo::core::Lock>(m_signalSlotable, deviceId, recursive));
            }

            // timeout was given
            const int waitTime = 1; // second
            int nTries = 0;
            while (true) {
                try {
                    return boost::make_shared<karathon::LockWrap>(
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


       private:
        void notifyPropertyChangedMonitors(const karabo::util::Hash& hash, const std::string& instanceId) {
            karabo::util::Hash registeredMonitors;
            {
                boost::mutex::scoped_lock lock(m_propertyChangedHandlersMutex);
                if (m_propertyChangedHandlers.has(instanceId)) {
                    registeredMonitors = m_propertyChangedHandlers.get<karabo::util::Hash>(instanceId);
                }
            }
            if (!registeredMonitors.empty()) {
                this->callMonitor(instanceId, registeredMonitors, hash);
            }
        }

        void callMonitor(const std::string& instanceId, const karabo::util::Hash& registered,
                         const karabo::util::Hash& current, std::string path = "") {
            for (karabo::util::Hash::const_iterator it = current.begin(); it != current.end(); ++it) {
                std::string currentPath = it->getKey();
                if (!path.empty()) currentPath = path + "." + it->getKey();
                if (registered.has(currentPath + "._function")) {
                    karabo::util::Timestamp t;
                    try {
                        t = karabo::util::Timestamp::fromHashAttributes(it->getAttributes());
                    } catch (...) {
                        KARABO_LOG_FRAMEWORK_WARN << "No timestamp information given on \"" << it->getKey() << "/";
                    }
                    const karabo::util::Hash& entry = registered.get<karabo::util::Hash>(currentPath);
                    boost::optional<const karabo::util::Hash::Node&> nodeFunc = entry.find("_function");
                    boost::optional<const karabo::util::Hash::Node&> nodeSelfObject = entry.find("_selfObject");
                    boost::optional<const karabo::util::Hash::Node&> nodeData = entry.find("_userData");
                    {
                        try {
                            if (nodeSelfObject) {
                                if (nodeData) {
                                    ScopedGILAcquire gil;
                                    bp::call_method<void>(nodeSelfObject->getValue<PyObject*>(),
                                                          nodeFunc->getValue<std::string>().c_str(), instanceId,
                                                          currentPath, Wrapper::toObject(it->getValueAsAny()), t,
                                                          nodeData->getValue<bp::object>());
                                } else {
                                    ScopedGILAcquire gil;
                                    bp::call_method<void>(nodeSelfObject->getValue<PyObject*>(),
                                                          nodeFunc->getValue<std::string>().c_str(), instanceId,
                                                          currentPath, Wrapper::toObject(it->getValueAsAny()), t);
                                }
                            } else {
                                if (nodeData) {
                                    ScopedGILAcquire gil;
                                    bp::call<void>(nodeFunc->getValue<PyObject*>(), instanceId, currentPath,
                                                   Wrapper::toObject(it->getValueAsAny()), t,
                                                   nodeData->getValue<bp::object>());
                                } else {
                                    ScopedGILAcquire gil;
                                    bp::call<void>(nodeFunc->getValue<PyObject*>(), instanceId, currentPath,
                                                   Wrapper::toObject(it->getValueAsAny()), t);
                                }
                            }

                        } catch (const karabo::util::Exception& e) {
                            std::cout << e.detailedMsg();
                        }
                    }
                }
                if (it->is<karabo::util::Hash>())
                    callMonitor(instanceId, registered, it->getValue<karabo::util::Hash>(), currentPath);
            }
        }

       private: // members
        bool m_isVerbose;

        boost::shared_ptr<SignalSlotableWrap> m_signalSlotableWrap;
    };
} // namespace karathon

#endif /* KARATHON_DEVCOM_HH */
