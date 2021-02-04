/*
 * File:   ComWrap.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on July 11, 2012, 11:22 AM
 */

#ifndef KARATHON_DEVCOM_HH
#define	KARATHON_DEVCOM_HH

#include <iostream>
#include <boost/python.hpp>
#include <boost/function.hpp>
#include <karabo/core/DeviceClient.hh>

#include "SignalSlotableWrap.hh"
#include "HashWrap.hh"
#include "ScopedGILRelease.hh"
#include "ScopedGILAcquire.hh"
#include "PyCoreLockWrap.hh"
#include "Wrapper.hh"

namespace bp = boost::python;
using namespace karabo::xms;

namespace karathon {

    class DeviceClientWrap : public karabo::core::DeviceClient {

    public:

        DeviceClientWrap(const std::string& instanceId = std::string())
            : DeviceClient(instanceId)
            , m_isVerbose(true) {
            boost::shared_ptr<karabo::xms::SignalSlotable> p = m_signalSlotable.lock();
            if (!p) {
                throw KARABO_PARAMETER_EXCEPTION("Broker connection is not valid.");
            }
            p->updateInstanceInfo(karabo::util::Hash("lang", "bound"));
            m_signalSlotableWrap = boost::static_pointer_cast<SignalSlotableWrap > (p);
        }

        DeviceClientWrap(boost::shared_ptr<SignalSlotableWrap>& o)
            : DeviceClient(boost::static_pointer_cast<karabo::xms::SignalSlotable>(o))
            , m_isVerbose(true) {
            boost::shared_ptr<karabo::xms::SignalSlotable> p = m_signalSlotable.lock();
            if (!p) {
                throw KARABO_PARAMETER_EXCEPTION("Broker connection is not valid.");
            }
            m_signalSlotableWrap = boost::static_pointer_cast<SignalSlotableWrap > (p);
        }

        ~DeviceClientWrap() {
        }

        bp::tuple instantiatePy(const std::string& serverId, const karabo::util::Hash& configuration, int timeoutInSeconds) {
            ScopedGILRelease nogil;
            return Wrapper::fromStdPairToPyTuple(this->instantiate(serverId, configuration, timeoutInSeconds));
        }

        bp::tuple instantiatePy(const std::string& serverId, const std::string& classId, const karabo::util::Hash& configuration, int timeoutInSeconds) {
            ScopedGILRelease nogil;
            return Wrapper::fromStdPairToPyTuple(this->instantiate(serverId, classId, configuration, timeoutInSeconds));
        }

        bp::tuple killDevicePy(const std::string& deviceId, int timeoutInSeconds = -1) {
            ScopedGILRelease nogil;
            return Wrapper::fromStdPairToPyTuple(this->killDevice(deviceId, timeoutInSeconds));
        }

        bp::tuple killServerPy(const std::string& serverId, int timeoutInSeconds = -1) {
            ScopedGILRelease nogil;
            return Wrapper::fromStdPairToPyTuple(this->killServer(serverId, timeoutInSeconds));
        }

        bp::tuple existsPy(const std::string& instanceId) {
            ScopedGILRelease nogil;
            return Wrapper::fromStdPairToPyTuple(this->exists(instanceId));
        }

        void enableInstanceTrackingPy() {
            ScopedGILRelease nogil;
            enableInstanceTracking();
        }

        bp::object getSystemInformationPy() {
            ScopedGILRelease nogil;
            return bp::object(getSystemInformation());
        }

        bp::object getSystemTopologyPy() {
            ScopedGILRelease nogil;
            return bp::object(getSystemTopology());
        }

        bp::object getServersPy() {
            ScopedGILRelease nogil;
            return Wrapper::fromStdVectorToPyList(this->getServers());
        }

        bp::object getClassesPy(const std::string& deviceServer) {
            ScopedGILRelease nogil;
            return Wrapper::fromStdVectorToPyList(this->getClasses(deviceServer));
        }

        //            bp::object getDevicesPy(const std::string& deviceServer) {
        //                return Wrapper::fromStdVectorToPyList(this->getDevices(deviceServer));
        //            }

        bp::object getDevicesPy() {
            ScopedGILRelease nogil;
            return Wrapper::fromStdVectorToPyList(this->getDevices());
        }

        bp::object getDevicesPy(const std::string& serverId) {
            ScopedGILRelease nogil;
            return Wrapper::fromStdVectorToPyList(this->getDevices(serverId));
        }

        bp::object getPropertiesPy(const std::string& deviceId) {
            ScopedGILRelease nogil;
            return Wrapper::fromStdVectorToPyList(this->getProperties(deviceId));
        }

        bp::object getClassPropertiesPy(const std::string& serverId, const std::string& classId) {
            ScopedGILRelease nogil;
            return Wrapper::fromStdVectorToPyList(this->getClassProperties(serverId, classId));
        }

        bp::object getCurrentlySettablePropertiesPy(const std::string& instanceId) {
            ScopedGILRelease nogil;
            return Wrapper::fromStdVectorToPyList(this->getCurrentlySettableProperties(instanceId));
        }

        bp::object getCurrentlyExecutableCommandsPy(const std::string& instanceId) {
            ScopedGILRelease nogil;
            return Wrapper::fromStdVectorToPyList(this->getCurrentlyExecutableCommands(instanceId));
        }

        bp::object getPy(const std::string& instanceId, const std::string& key, const std::string& keySep = ".") {
            boost::any value;
            try {
                ScopedGILRelease nogil;
                value = this->DeviceClient::getAsAny(instanceId, key, keySep.at(0));
            } catch (...) {
                KARABO_RETHROW_AS(KARABO_PARAMETER_EXCEPTION("The key \"" + key + "\" is not found in the device \"" + instanceId + "\""));
            }
            return Wrapper::toObject(value, HashWrap::isDefault(PyTypes::PYTHON_DEFAULT));
        }

        bp::object getConfigurationPy(const std::string& instanceId) {
            ScopedGILRelease nogil;
            return bp::object(this->DeviceClient::get(instanceId));
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
            // boost::bind is safe here because the handler is dispatched
            // directly and not via the event loop
            this->DeviceClient::registerInstanceNewMonitor(
                boost::bind(&DeviceClientWrap::proxyPythonCallbackHash, this, handler, _1));
        }

        void registerInstanceUpdatedMonitor(const bp::object& handler) {
            // boost::bind is safe here because the handler is dispatched
            // directly and not via the event loop
            this->DeviceClient::registerInstanceUpdatedMonitor(
                boost::bind(&DeviceClientWrap::proxyPythonCallbackHash, this, handler, _1));
        }

        void registerInstanceGoneMonitor(const bp::object& handler) {
            // boost::bind is safe here because the handler is dispatched
            // directly and not via the event loop
            this->DeviceClient::registerInstanceGoneMonitor(
                boost::bind(&DeviceClientWrap::proxyPythonCallbackStringHash, this, handler, _1, _2));
        }

        void registerSchemaUpdatedMonitor(const bp::object& handler) {
            // boost::bind is safe here because the handler is dispatched
            // directly and not via the event loop
            this->DeviceClient::registerSchemaUpdatedMonitor(
                boost::bind(&DeviceClientWrap::proxyPythonCallbackStringSchema, this, handler, _1, _2));
        }

        void registerDeviceMonitor(const std::string& instanceId, const bp::object& handler, const bp::object& userData = bp::object()) {
            if (userData.is_none()) {
                this->DeviceClient::registerDeviceMonitor(instanceId,
                    boost::bind(&DeviceClientWrap::proxyPythonCallbackStringHash, this, handler, _1, _2));
            } else {
                this->DeviceClient::registerDeviceMonitor(instanceId,
                    boost::bind(&DeviceClientWrap::proxyPythonCallbackStringHashAny, this, handler, _1, _2, _3), userData);
            }
        }

        bool registerPropertyMonitor(const std::string& instanceId, const std::string& key, const bp::object& callbackFunction, const bp::object& userData = bp::object()) {
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
                    boost::mutex::scoped_lock lock(m_propertyChangedHandlersMutex);
                    if (Wrapper::hasattr(callbackFunction, "__self__")) {
                        const bp::object & selfObject(callbackFunction.attr("__self__"));
                        std::string funcName(bp::extract<std::string > (callbackFunction.attr("__name__")));
                        m_propertyChangedHandlers.set(instanceId + "." + key + "._function", funcName);
                        m_propertyChangedHandlers.set(instanceId + "." + key + "._selfObject", selfObject.ptr());
                    } else {
                        m_propertyChangedHandlers.set(instanceId + "." + key + "._function", callbackFunction.ptr());
                    }
                    if (!userData.is_none()) m_propertyChangedHandlers.set(instanceId + "." + key + "._userData", userData);
                }

                immortalize(instanceId);
                return true;
            } else {
                return false;
            }
        }

        static void proxyChannelStatusTracker(const bp::object& handler, karabo::net::ConnectionStatus status) {
            Wrapper::proxyHandler(handler, "channelStatusTracker", status);
        }

        bool registerChannelMonitorPy(const std::string& channelName,
                                      const bp::object& dataHandler = bp::object(),
                                      const karabo::util::Hash& inputChannelCfg = karabo::util::Hash(),
                                      const bp::object& eosHandler = bp::object(),
                                      const bp::object& inputHandler = bp::object(),
                                      const bp::object& statusTracker = bp::object()) {

            InputChannelHandlers handlers;

            if (!dataHandler.is_none()) {
                handlers.dataHandler = boost::bind(&InputChannelWrap::proxyDataHandler, dataHandler, _1, _2);
            }
            if (!eosHandler.is_none()) {
                handlers.eosHandler = boost::bind(&InputChannelWrap::proxyEndOfStreamEventHandler, eosHandler, _1);
            }
            if (!inputHandler.is_none()) {
                handlers.inputHandler = boost::bind(&InputChannelWrap::proxyInputHandler, inputHandler, _1);
            }
            if (!statusTracker.is_none()) {
                handlers.statusTracker = boost::bind(&proxyChannelStatusTracker, statusTracker, _1);
            }

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

        void setPy(const std::string& instanceId, const std::string& key, const bp::object& value, const std::string& keySep = ".", int timeout = -1) {
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

        void setNoWaitPy(const std::string& instanceId, const std::string& key, const bp::object& value, const std::string& keySep = ".") {
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

        bp::object getFromPastPy(const std::string& deviceId, const std::string& key, const std::string& from, std::string to = "", unsigned int maxNumData = 0) {
            return Wrapper::fromStdVectorToPyHashList(this->getFromPast(deviceId, key, from, to, maxNumData));
        }

        bp::object getPropertyHistoryPy(const std::string& deviceId, const std::string& key, const std::string& from, std::string to = "", unsigned int maxNumData = 0) {
            return Wrapper::fromStdVectorToPyHashList(this->getFromPast(deviceId, key, from, to, maxNumData));
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

        bp::tuple saveConfigurationFromNamePy(const std::string& name,
                                              const bp::object& deviceIds,
                                              const std::string& description,
                                              int priority,
                                              const std::string& user) {
            ScopedGILRelease nogil;

            return Wrapper::fromStdPairToPyTuple(
                this->DeviceClient::saveConfigurationFromName(
                    name,
                    karathon::Wrapper::fromPyListToStdVector<std::string>(deviceIds),
                    description, priority, user
                )
            );
        }

        bp::object getConfigurationFromPastPy(const std::string& deviceId, const std::string& timePoint) {
            return Wrapper::fromStdPairToPyTuple<karabo::util::Hash, karabo::util::Schema>(this->getConfigurationFromPast(deviceId, timePoint));
        }

        karabo::util::Hash getOutputChannelSchema(const std::string & deviceId, const std::string& outputChannelName) {
            ScopedGILRelease nogil;
            return this->DeviceClient::getOutputChannelSchema(deviceId, outputChannelName);
        }

        std::vector<std::string> getOutputChannelNames(const std::string & deviceId) {
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
            //non waiting request for lock

            if (timeout == 0) {
                return boost::make_shared<karathon::LockWrap>(boost::make_shared<karabo::core::Lock>(m_signalSlotable,
                                                                                                     deviceId, recursive));
            }

            //timeout was given
            const int waitTime = 1; //second
            int nTries = 0;
            while (true) {
                try {
                    return boost::make_shared<karathon::LockWrap>(boost::make_shared<karabo::core::Lock>(m_signalSlotable,
                                                                                                         deviceId, recursive));

                } catch (const karabo::util::LockException& e) {
                    if (nTries++ > timeout / waitTime && timeout != -1) {
                        //rethrow
                        throw KARABO_LOCK_EXCEPTION(e.userFriendlyMsg());
                    }
                    //otherwise pass through and try again
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
                    registeredMonitors = m_propertyChangedHandlers.get<karabo::util::Hash > (instanceId);
                }
            }
            if (!registeredMonitors.empty()) {

                this->callMonitor(instanceId, registeredMonitors, hash);
            }
        }

        void callMonitor(const std::string& instanceId, const karabo::util::Hash& registered, const karabo::util::Hash& current, std::string path = "") {
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
                    const karabo::util::Hash& entry = registered.get<karabo::util::Hash > (currentPath);
                    boost::optional<const karabo::util::Hash::Node&> nodeFunc = entry.find("_function");
                    boost::optional<const karabo::util::Hash::Node&> nodeSelfObject = entry.find("_selfObject");
                    boost::optional<const karabo::util::Hash::Node&> nodeData = entry.find("_userData");
                    {

                        try {
                            if (nodeSelfObject) {
                                if (nodeData) {
                                    ScopedGILAcquire gil;
                                    bp::call_method<void>(nodeSelfObject->getValue<PyObject*>(), nodeFunc->getValue<std::string>().c_str(), instanceId, currentPath, Wrapper::toObject(it->getValueAsAny()), t, nodeData->getValue<bp::object >());
                                } else {
                                    ScopedGILAcquire gil;
                                    bp::call_method<void>(nodeSelfObject->getValue<PyObject*>(), nodeFunc->getValue<std::string>().c_str(), instanceId, currentPath, Wrapper::toObject(it->getValueAsAny()), t);
                                }
                            } else {
                                if (nodeData) {
                                    ScopedGILAcquire gil;
                                    bp::call<void>(nodeFunc->getValue<PyObject*>(), instanceId, currentPath, Wrapper::toObject(it->getValueAsAny()), t, nodeData->getValue<bp::object >());
                                } else {
                                    ScopedGILAcquire gil;
                                    bp::call<void>(nodeFunc->getValue<PyObject*>(), instanceId, currentPath, Wrapper::toObject(it->getValueAsAny()), t);
                                }
                            }

                        } catch (const karabo::util::Exception& e) {
                            std::cout << e.userFriendlyMsg();
                        }
                    }
                }
                if (it->is<karabo::util::Hash>()) callMonitor(instanceId, registered, it->getValue<karabo::util::Hash>(), currentPath);
            }
        }

        void proxyPythonCallbackHash(const bp::object& handler, const karabo::util::Hash& arg1) {
            Wrapper::proxyHandler(handler, "ProxyCallbackHash type", arg1);
        }

        void proxyPythonCallbackStringHash(const bp::object& handler, const std::string& arg1, const karabo::util::Hash& arg2) {
            Wrapper::proxyHandler(handler, "ProxyCallbackStringHash type", arg1, arg2);
        }

        void proxyPythonCallbackStringSchema(const bp::object& handler, const std::string& arg1, const karabo::util::Schema& arg2) {
            Wrapper::proxyHandler(handler, "ProxyCallbackStringSchema type", arg1, arg2);
        }

        void proxyPythonCallbackStringHashAny(const bp::object& handler, const std::string& arg1, const karabo::util::Hash& arg2, const boost::any& arg3) {
            Wrapper::proxyHandler(handler, "ProxyCallbackStringHashAny type", arg1, arg2, arg3);
        }

    private: // members

        bool m_isVerbose;

        boost::shared_ptr<SignalSlotableWrap> m_signalSlotableWrap;

    };
}

#endif /* KARATHON_DEVCOM_HH */
