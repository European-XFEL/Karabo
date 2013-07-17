/* 
 * File:   ComWrap.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on July 11, 2012, 11:22 AM
 */

#ifndef KARABO_PYKARABO_DEVCOM_HH
#define	KARABO_PYKARABO_DEVCOM_HH

#include <iostream>
#include <boost/python.hpp>
#include <boost/function.hpp>
#include <karabo/core/DeviceClient.hh>

#include "SignalSlotableWrap.hh"
#include "HashWrap.hh"
#include "ScopedGILRelease.hh"
#include "ScopedGILAcquire.hh"

namespace bp = boost::python;

namespace karathon {

    class DeviceClientWrap : public karabo::core::DeviceClient {

    public:

        DeviceClientWrap(const std::string& connectionType = "Jms", const karabo::util::Hash& connectionParameters = karabo::util::Hash()) :
        DeviceClient(boost::shared_ptr<karabo::xms::SignalSlotable>(new SignalSlotableWrap(DeviceClient::generateOwnInstanceId(), connectionType, connectionParameters))),
        m_isVerbose(true) {
            m_signalSlotableWrap = boost::static_pointer_cast<SignalSlotableWrap > (m_signalSlotable);
        }

        DeviceClientWrap(boost::shared_ptr<SignalSlotableWrap>& o) : DeviceClient(boost::static_pointer_cast<karabo::xms::SignalSlotable>(o)),
        m_isVerbose(true) {
            m_signalSlotableWrap = boost::static_pointer_cast<SignalSlotableWrap > (m_signalSlotable);
        }

        ~DeviceClientWrap() {
        }

        bp::tuple instantiatePy(const std::string& serverId, const karabo::util::Hash& configuration, int timeoutInSeconds = -1) {
            ScopedGILRelease nogil;
            return Wrapper::fromStdPairToPyTuple(this->instantiate(serverId, configuration, timeoutInSeconds));
        }

        bp::tuple instantiatePy(const std::string& serverId, const std::string& classId, const karabo::util::Hash& configuration, int timeoutInSeconds = -1) {
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
            return Wrapper::fromStdPairToPyTuple(this->exists(instanceId));
        }

        bp::object getServersPy() {
            return Wrapper::fromStdVectorToPyList(this->getServers());
        }

        bp::object getClassesPy(const std::string& deviceServer) {
            return Wrapper::fromStdVectorToPyList(this->getClasses(deviceServer));
        }

        //            bp::object getDevicesPy(const std::string& deviceServer) {
        //                return Wrapper::fromStdVectorToPyList(this->getDevices(deviceServer));
        //            }

        bp::object getDevicesPy() {
            return Wrapper::fromStdVectorToPyList(this->getDevices());
        }

        bp::object getDevicesPy(const std::string& serverId) {
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
            try {
                return HashWrap::get(this->cacheAndGetConfiguration(instanceId), key, keySep);
            } catch (const karabo::util::Exception& e) {
                throw KARABO_PARAMETER_EXCEPTION("Could not fetch parameter \"" + key + "\" from device \"" + instanceId + "\"");
            }
        }

        void registerDeviceMonitor(const std::string& instanceId, const bp::object& callbackFunction, const bp::object& userData = bp::object()) {
            std::cout << "DeviceClientWrap::registerDeviceMonitor on instanceId : \"" << instanceId << "\"" << std::endl;
            boost::mutex::scoped_lock lock(m_deviceChangedHandlersMutex);
            this->cacheAndGetConfiguration(instanceId);
            if (Wrapper::hasattr(callbackFunction, "__self__")) {
                const bp::object & selfObject(callbackFunction.attr("__self__"));
                std::string funcName(bp::extract<std::string > (callbackFunction.attr("__name__")));
                m_deviceChangedHandlers.set(instanceId + "._function", funcName);
                m_deviceChangedHandlers.set(instanceId + "._selfObject", selfObject.ptr());
            } else {
                m_deviceChangedHandlers.set(instanceId + "._function", callbackFunction.ptr());
            }
            if (!userData.is_none()) m_deviceChangedHandlers.set(instanceId + "._userData", userData);
        }

        bool registerPropertyMonitor(const std::string& instanceId, const std::string& key, const bp::object& callbackFunction, const bp::object& userData = bp::object()) {
            karabo::util::Schema schema = this->getDeviceSchema(instanceId);
            if (schema.has(key)) {
                boost::mutex::scoped_lock lock(m_propertyChangedHandlersMutex);
                this->cacheAndGetConfiguration(instanceId);
                if (Wrapper::hasattr(callbackFunction, "__self__")) {
                    const bp::object & selfObject(callbackFunction.attr("__self__"));
                    std::string funcName(bp::extract<std::string > (callbackFunction.attr("__name__")));
                    m_propertyChangedHandlers.set(instanceId + "." + key + "._function", funcName);
                    m_propertyChangedHandlers.set(instanceId + "." + key + "._selfObject", selfObject.ptr());
                } else {
                    m_propertyChangedHandlers.set(instanceId + "." + key + "._function", callbackFunction.ptr());
                }
                if (!userData.is_none()) m_propertyChangedHandlers.set(instanceId + "." + key + "._userData", userData);
                return true;
            } else {
                return false;
            }
        }

        bp::tuple setPy(const std::string& instanceId, const std::string& key, const bp::object& value, const std::string& keySep = ".", int timeout = -1) {
            karabo::util::Hash tmp;
            std::pair<bool, std::string> result;
            HashWrap::set(tmp, key, value, keySep);
            {
                ScopedGILRelease nogil;
                result = this->set(instanceId, tmp, timeout);
            }
            return bp::make_tuple(result.first, result.second);
        }
        
        
        bp::tuple setPy(const std::string& instanceId, const karabo::util::Hash& value, int timeout = -1) {
            ScopedGILRelease nogil;
            std::pair<bool, std::string> result = this->set(instanceId, value, timeout);
            return bp::make_tuple(result.first, result.second);
        }

        
        void setNoWaitPy(const std::string& instanceId, const std::string& key, const bp::object& value, const std::string& keySep = ".") {
            karabo::util::Hash tmp;
            HashWrap::set(tmp, key, value, keySep);
            ScopedGILRelease nogil;
            this->setNoWait(instanceId, tmp);
        }

        void executeNoWaitPy0(std::string instanceId, const std::string& functionName) {
            ScopedGILRelease nogil;
            m_signalSlotableWrap->call(instanceId, functionName);
        }

        void executeNoWaitPy1(std::string instanceId, const std::string& functionName, const bp::object& a1) const {
            ScopedGILRelease nogil;
            m_signalSlotableWrap->callPy1(instanceId, functionName, a1);
        }

        void executeNoWaitPy2(std::string instanceId, const std::string& functionName, const bp::object& a1, const bp::object& a2) const {
            m_signalSlotableWrap->callPy2(instanceId, functionName, a1, a2);
        }

        void executeNoWaitPy3(std::string instanceId, const std::string& functionName, const bp::object& a1, const bp::object& a2, const bp::object& a3) const {
            m_signalSlotableWrap->callPy3(instanceId, functionName, a1, a2, a3);
        }

        void executeNoWaitPy4(std::string instanceId, const std::string& functionName, const bp::object& a1, const bp::object& a2, const bp::object& a3, const bp::object& a4) const {
            m_signalSlotableWrap->callPy4(instanceId, functionName, a1, a2, a3, a4);
        }

        bp::tuple executePy0(std::string instanceId, const std::string& functionName, int timeout = -1) {
            if (timeout == -1) timeout = 3;

            std::pair<bool, std::string> result;
            bp::tuple tuple;
            {
                ScopedGILRelease nogil;
                result = this->execute(instanceId, functionName, timeout);
            }
            tuple = bp::make_tuple(result.first, result.second);
            return tuple;
        }

        bp::tuple executePy1(std::string instanceId, const std::string& functionName, const bp::object& a1, int timeout = -1) const {
            if (timeout == -1) timeout = 3;

            bp::tuple result;

            try {
                result = m_signalSlotableWrap->requestPy1(instanceId, functionName, a1).waitForReply(timeout * 1000);
            } catch (const karabo::util::Exception& e) {
                return bp::make_tuple(false, e.userFriendlyMsg());
            }
            return bp::make_tuple(true, result[0]);
        }

        bp::tuple executePy2(std::string instanceId, const std::string& functionName, const bp::object& a1, const bp::object& a2, int timeout = -1) const {
            if (timeout == -1) timeout = 3;

            bp::tuple result;

            try {
                result = m_signalSlotableWrap->requestPy2(instanceId, functionName, a1, a2).waitForReply(timeout * 1000);
            } catch (const karabo::util::Exception& e) {
                return bp::make_tuple(false, e.userFriendlyMsg());
            }
            return bp::make_tuple(true, result[0]);
        }

        bp::tuple executePy3(std::string instanceId, const std::string& functionName, const bp::object& a1, const bp::object& a2, const bp::object& a3, int timeout = -1) const {
            if (timeout == -1) timeout = 3;

            bp::tuple result;

            try {
                result = m_signalSlotableWrap->requestPy3(instanceId, functionName, a1, a2, a3).waitForReply(timeout * 1000);
            } catch (const karabo::util::Exception& e) {
                return bp::make_tuple(false, e.userFriendlyMsg());
            }
            return bp::make_tuple(true, result[0]);
        }

        bp::tuple executePy4(std::string instanceId, const std::string& functionName, const bp::object& a1, const bp::object& a2, const bp::object& a3, const bp::object& a4, int timeout = -1) const {
            if (timeout == -1) timeout = 3;

            bp::tuple result;

            try {
                result = m_signalSlotableWrap->requestPy4(instanceId, functionName, a1, a2, a3, a4).waitForReply(timeout * 1000);
            } catch (const karabo::util::Exception& e) {
                return bp::make_tuple(false, e.userFriendlyMsg());
            }
            return bp::make_tuple(true, result[0]);
        }

    private:

        void notifyDeviceChangedMonitors(const karabo::util::Hash& hash, const std::string& instanceId) {
            boost::mutex::scoped_lock lock(m_deviceChangedHandlersMutex);
            boost::optional<karabo::util::Hash::Node&> node = m_deviceChangedHandlers.find(instanceId);
            if (node) {
                const karabo::util::Hash& entry = node->getValue<karabo::util::Hash>();
                boost::optional<const karabo::util::Hash::Node&> nodeFunc = entry.find("_function");
                boost::optional<const karabo::util::Hash::Node&> nodeSelfObject = entry.find("_selfObject");
                boost::optional<const karabo::util::Hash::Node&> nodeData = entry.find("_userData");

                ScopedGILAcquire gil;
                try {
                    if (nodeSelfObject) {
                        if (nodeData) {
                            bp::call_method<void>(nodeSelfObject->getValue<PyObject*>(), nodeFunc->getValue<std::string>().c_str(), instanceId, hash, nodeData->getValue<bp::object >());
                        } else {
                            bp::call_method<void>(nodeSelfObject->getValue<PyObject*>(), nodeFunc->getValue<std::string>().c_str(), instanceId, hash);
                        }
                    } else {
                        if (nodeData) {
                            bp::call<void>(nodeFunc->getValue<PyObject*>(), instanceId, hash, nodeData->getValue<bp::object >());
                        } else {
                            bp::call<void>(nodeFunc->getValue<PyObject*>(), instanceId, hash);
                        }
                    }
                } catch (const karabo::util::Exception& e) {
                    std::cout << e.userFriendlyMsg();
                }
            }
        }

        void notifyPropertyChangedMonitors(const karabo::util::Hash& hash, const std::string& instanceId) {
            boost::mutex::scoped_lock lock(m_propertyChangedHandlersMutex);
            if (m_propertyChangedHandlers.has(instanceId)) {
                this->callMonitor(instanceId, m_propertyChangedHandlers.get<karabo::util::Hash > (instanceId), hash);
            }
        }

        void callMonitor(const std::string& instanceId, const karabo::util::Hash& registered, const karabo::util::Hash& current, std::string path = "") {
            for (karabo::util::Hash::const_iterator it = current.begin(); it != current.end(); ++it) {
                std::string currentPath = it->getKey();
                if (!path.empty()) currentPath = path + "." + it->getKey();
                if (registered.has(currentPath + "._function")) {
                    karabo::util::Timestamp t;
                    try {
                        t =  karabo::util::Timestamp::fromHashAttributes(it->getAttributes());
                    } catch (...) {
                        KARABO_LOG_FRAMEWORK_WARN << "No timestamp information given on \"" << it->getKey() << "/";
                    }
                    const karabo::util::Hash& entry = registered.get<karabo::util::Hash > (currentPath);
                    boost::optional<const karabo::util::Hash::Node&> nodeFunc = entry.find("_function");
                    boost::optional<const karabo::util::Hash::Node&> nodeSelfObject = entry.find("_selfObject");
                    boost::optional<const karabo::util::Hash::Node&> nodeData = entry.find("_userData");
                    {
                        ScopedGILAcquire gil;
                        try {
                            if (nodeSelfObject) {
                                if (nodeData) {
                                    bp::call_method<void>(nodeSelfObject->getValue<PyObject*>(), nodeFunc->getValue<std::string>().c_str(), instanceId, currentPath, Wrapper::toObject(it->getValueAsAny()), t, nodeData->getValue<bp::object >());
                                } else {
                                    bp::call_method<void>(nodeSelfObject->getValue<PyObject*>(), nodeFunc->getValue<std::string>().c_str(), instanceId, currentPath, Wrapper::toObject(it->getValueAsAny()), t);
                                }
                            } else {
                                if (nodeData) {
                                    bp::call<void>(nodeFunc->getValue<PyObject*>(), instanceId, currentPath, Wrapper::toObject(it->getValueAsAny()), t, nodeData->getValue<bp::object >());
                                } else {
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

    private: // members

        bool m_isVerbose;

        boost::shared_ptr<SignalSlotableWrap> m_signalSlotableWrap;

    };
}

#endif

