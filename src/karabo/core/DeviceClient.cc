/*
 * $Id: Com.cc 6624 2012-06-27 12:57:09Z heisenb $
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (c) 2010-2012 European XFEL GmbH Hamburg. All rights reserved.
 */

#include <karabo/log/Logger.hh>
#include <karabo/io/FileTools.hh>

#include "DeviceClient.hh"

using namespace std;
using namespace karabo::util;
using namespace karabo::xms;


namespace karabo {
    namespace core {


        DeviceClient::DeviceClient(const std::string& connectionType, const karabo::util::Hash& connectionParameters) : m_isShared(false), m_defaultTimeout(8000) {
            karabo::net::BrokerConnection::Pointer connection = karabo::net::BrokerConnection::create(connectionType, connectionParameters);
            std::string ownInstanceId = generateOwnInstanceId();
            m_signalSlotable = boost::shared_ptr<SignalSlotable > (new SignalSlotable(connection, ownInstanceId));
            m_eventThread = boost::thread(boost::bind(&karabo::xms::SignalSlotable::runEventLoop, m_signalSlotable, true, Hash()));
            this->setupSlots();
            this->cacheAvailableInstances();

        }


        DeviceClient::DeviceClient(const boost::shared_ptr<SignalSlotable>& signalSlotable) : m_signalSlotable(signalSlotable), m_isShared(true), m_defaultTimeout(8000) {
            this->setupSlots();
            this->cacheAvailableInstances();
        }


        void DeviceClient::setupSlots() {
            //karabo::log::Logger::configure(Hash("priority", "DEBUG")); // TODO REMOVE LATER
            m_signalSlotable->registerSlot<Hash, string > (boost::bind(&karabo::core::DeviceClient::slotChanged, this, _1, _2), "slotChanged");
            m_signalSlotable->registerSlot<string, Hash > (boost::bind(&karabo::core::DeviceClient::slotInstanceUpdated, this, _1, _2), "slotInstanceUpdated", SignalSlotable::GLOBAL);
            m_signalSlotable->registerSlot<string > (boost::bind(&karabo::core::DeviceClient::slotInstanceGone, this, _1), "slotInstanceGone", SignalSlotable::GLOBAL);
        }


        void DeviceClient::cacheAvailableInstances() {
            boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
            vector<pair<string, Hash> > instances = m_signalSlotable->getAvailableInstances();
            for (size_t i = 0; i < instances.size(); ++i) {
                const string& instanceId = instances[i].first;
                const Hash& instanceInfo = instances[i].second;
                boost::optional<const Hash::Node&> node = instanceInfo.find("type");
                string type = "unknown";
                if (node) type = node->getValue<string>();
                Hash entry;
                Hash::Node & entryNode = entry.set(type + "." + instanceId, Hash());
                for (Hash::const_iterator it = instanceInfo.begin(); it != instanceInfo.end(); ++it) {
                    entryNode.setAttribute(it->getKey(), it->getValueAsAny());
                }
                m_runtimeSystemDescription.merge(entry);
            }
            KARABO_LOG_FRAMEWORK_DEBUG << "cacheAvailableInstances() was called, runtimeSystemDescription looks like:";
            KARABO_LOG_FRAMEWORK_DEBUG << m_runtimeSystemDescription;
        }


        void DeviceClient::slotChanged(const karabo::util::Hash& hash, const std::string & instanceId) {
            boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
            Hash& tmp = m_runtimeSystemDescription.get<Hash>("device." + instanceId + ".configuration");
            tmp.merge(hash);
            // NOTE: This will block us here, i.e. we are deaf for other changes...
            // NOTE: Monitors could be implemented as additional slots, too.
            notifyDeviceChangedMonitors(hash, instanceId);
            notifyPropertyChangedMonitors(hash, instanceId);
        }


        void DeviceClient::slotInstanceUpdated(const std::string& instanceId, const karabo::util::Hash& instanceInfo) {
            boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
            boost::optional<const Hash::Node&> node = instanceInfo.find("type");
            string type = "unknownType";
            if (node) type = node->getValue<string>();
            Hash entry;
            Hash::Node& entryNode = entry.set(type + "." + instanceId, Hash());
            for (Hash::const_iterator it = instanceInfo.begin(); it != instanceInfo.end(); ++it) {
                entryNode.setAttribute(it->getKey(), it->getValueAsAny());
            }
            m_runtimeSystemDescription.merge(entry);
            KARABO_LOG_FRAMEWORK_DEBUG << "slotInstanceUpdated() was called, runtimeSystemDescription looks like:";
            KARABO_LOG_FRAMEWORK_DEBUG << m_runtimeSystemDescription;
        }


        void DeviceClient::slotInstanceGone(const std::string& instanceId) {
            boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
            for (Hash::iterator it = m_runtimeSystemDescription.begin(); it != m_runtimeSystemDescription.end(); ++it) {
                Hash& tmp = it->getValue<Hash>();
                boost::optional<Hash::Node&> node = tmp.find(instanceId);
                if (node) {
                    tmp.erase(instanceId);
                    KARABO_LOG_FRAMEWORK_INFO << "Instance \"" << instanceId << "\" is gone.";
                    KARABO_LOG_FRAMEWORK_DEBUG << "slotInstanceGone() was called, runtimeSystemDescription looks like:";
                    KARABO_LOG_FRAMEWORK_DEBUG << m_runtimeSystemDescription;
                    break;
                }
            }
            KARABO_LOG_FRAMEWORK_WARN << "Instance \"" << instanceId << "\" signaled death but is not known to the client...";
        }


        DeviceClient::~DeviceClient() {
            if (!m_isShared) {
                m_signalSlotable->stopEventLoop();
                m_eventThread.join();
            }
        }


        void DeviceClient::setDefaultTimeout(const unsigned int defaultTimeout) {
            m_defaultTimeout = defaultTimeout;
        }


        int DeviceClient::getDefaultTimeout() const {
            return m_defaultTimeout;
        }


        std::pair<bool, std::string> DeviceClient::exists(const std::string& instanceId) {
            string hostname;
            try {
                m_signalSlotable->request("*", "slotPing", instanceId, true).timeout(m_defaultTimeout).receive(hostname);
            } catch (karabo::util::TimeoutException) {
                return std::make_pair(false, hostname);
            }
            return std::make_pair(true, hostname);
        }


        std::vector<std::string> DeviceClient::getServers() {
            boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
            if (m_runtimeSystemDescription.has("server")) {
                const Hash& tmp = m_runtimeSystemDescription.get<Hash>("server");
                vector<string> deviceServers;
                deviceServers.reserve(tmp.size());
                for (Hash::const_iterator it = tmp.begin(); it != tmp.end(); ++it) {
                    deviceServers.push_back(it->getKey());
                }
                return deviceServers;
            } else {
                KARABO_LOG_FRAMEWORK_INFO << "No device servers found in the system";
                return vector<string>();
            }
        }


        std::vector<std::string> DeviceClient::getClasses(const std::string& deviceServer) {
            boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
            if (!m_runtimeSystemDescription.has("server." + deviceServer)) {
                KARABO_LOG_FRAMEWORK_ERROR << "Requested device server \"" << deviceServer << "\" does not exist.";
                return vector<string>();
            } else {
                return m_runtimeSystemDescription.getAttribute<vector<string> >("server." + deviceServer, "deviceClasses");
            }
        }


        std::vector<std::string> DeviceClient::getDevices() {
            boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
            if (!m_runtimeSystemDescription.has("device")) {
                return vector<string>();
            } else {
                const Hash& tmp = m_runtimeSystemDescription.get<Hash>("device");
                vector<string> devices;
                devices.reserve(tmp.size());
                for (Hash::const_iterator it = tmp.begin(); it != tmp.end(); ++it) {
                    if (it->hasAttribute("visibility")) {
                        const vector<string>& roles = it->getAttribute<vector<string> >("visibility");
                        if (!roles.empty()) {
                            if (std::find(roles.begin(), roles.end(), m_role) == roles.end()) continue;
                        }
                    }
                    devices.push_back(it->getKey());
                }
                return devices;
            }
        }


        std::vector<std::string> DeviceClient::getDevices(const std::string& deviceServer) {
            boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
            if (!m_runtimeSystemDescription.has("device")) {
                return vector<string>();
            } else {
                const Hash& tmp = m_runtimeSystemDescription.get<Hash>("device");
                vector<string> devices;
                devices.reserve(tmp.size());
                for (Hash::const_iterator it = tmp.begin(); it != tmp.end(); ++it) {
                    if (it->getAttribute<string>("serverId") == deviceServer) {
                        if (it->hasAttribute("visibility")) {
                            const vector<string>& roles = it->getAttribute<vector<string> >("visibility");
                            if (!roles.empty()) {
                                if (std::find(roles.begin(), roles.end(), m_role) == roles.end()) continue;
                            }
                        }
                        devices.push_back(it->getKey());
                    }
                }
                return devices;
            }
        }


        karabo::util::Schema DeviceClient::getFullSchema(const std::string& instanceId) {
            return cacheAndGetFullSchema(instanceId);
        }


        karabo::util::Schema DeviceClient::cacheAndGetFullSchema(const std::string & instanceId) {
            std::string path("device." + instanceId + ".fullSchema");
            boost::optional<Hash::Node&> node = m_runtimeSystemDescription.find(path);
            if (!node) { // Not found, request and cache it
                // Request schema
                Schema schema;
                try {
                    m_signalSlotable->request(instanceId, "slotGetSchema", false).timeout(m_defaultTimeout).receive(schema); // Retrieves full schema
                } catch (const TimeoutException&) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Schema request for instance \"" << instanceId << "\" timed out";
                    Exception::clearTrace();
                    return Schema();
                }
                return m_runtimeSystemDescription.set(path, schema).getValue<Schema>();
            }
            return node->getValue<Schema>();
        }


        karabo::util::Schema DeviceClient::getActiveSchema(const std::string& instanceId) {
            return cacheAndGetActiveSchema(instanceId);
        }


        karabo::util::Schema DeviceClient::cacheAndGetActiveSchema(const std::string& instanceId) {
            std::string state = this->get<std::string > (instanceId, "state");
            std::string path("device." + instanceId + ".activeSchema." + state);
            boost::optional<Hash::Node&> node = m_runtimeSystemDescription.find(path);
            if (!node) { // Not found, request and cache it
                // Request schema
                Schema schema;
                try {
                    m_signalSlotable->request(instanceId, "slotGetSchema", true).timeout(m_defaultTimeout).receive(schema); // Retrieves active schema
                } catch (const TimeoutException&) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Schema request for instance \"" << instanceId << "\" timed out";
                    Exception::clearTrace();
                    return Schema();
                }
                return m_runtimeSystemDescription.set(path, schema).getValue<Schema>();
            } else {
                return node->getValue<Schema>();
            }
        }


        karabo::util::Schema DeviceClient::getClassSchema(const std::string& deviceServer, const std::string& className) {
            // TODO Implement
            throw KARABO_NOT_IMPLEMENTED_EXCEPTION("Will be available soon");
        }


        std::vector<std::string> DeviceClient::getCurrentlyExecutableCommands(const std::string& instanceId) {
            Schema schema = cacheAndGetActiveSchema(instanceId);
            vector<string> commands;
            extractCommands(schema, "", commands);
            return commands;
        }


        void DeviceClient::extractCommands(const karabo::util::Schema& schema, const std::string& parentKey, std::vector<std::string>& commands) {
            vector<string> keys = schema.getKeys(parentKey);


            BOOST_FOREACH(std::string key, keys) {
                if (schema.isCommand(key)) {
                    commands.push_back(key);
                } else if (!schema.isLeaf(key)) {
                    extractCommands(schema, key, commands);
                }
            }
        }


        std::vector<std::string> DeviceClient::getCurrentlySettableProperties(const std::string& instanceId) {
            Schema schema = cacheAndGetActiveSchema(instanceId);
            vector<string> paths = schema.getPaths();
            std::vector<std::string> properties;


            BOOST_FOREACH(std::string path, paths) {
                if (schema.isProperty(path)) {
                    properties.push_back(path);
                }
            }
            return properties;
        }


        Hash DeviceClient::loadConfigurationFromFile(const std::string& filename) {
            Hash configuration;
            karabo::io::loadFromFile(configuration, filename);
            return configuration;
        }


        void DeviceClient::instantiateNoWait(const std::string& serverInstanceId, const std::string& classId, const karabo::util::Hash& configuration) {
            Hash tmp(classId, configuration);
            m_signalSlotable->call(serverInstanceId, "slotStartDevice", tmp);
        }


        void DeviceClient::instantiateNoWait(const std::string& serverInstanceId, const karabo::util::Hash& completeConfiguration) {
            m_signalSlotable->call(serverInstanceId, "slotStartDevice", completeConfiguration);
        }


        //        std::pair<bool, std::string > DeviceClient::instantiateWait(const std::string& serverInstanceId, const std::string& classId, const karabo::util::Hash& configuration, int timeout) {
        //            if (timeout == -1) timeout = m_defaultTimeout;
        //            Hash tmp(classId, configuration);
        //            bool ok = true;
        //            std::string errorText = "";
        //
        //            try {
        //                m_signalSlotable->request(serverInstanceId, "slotStartDevice", tmp).timeout(timeout).receive(ok, errorText);
        //            } catch (const karabo::util::Exception& e) {
        //
        //
        //                errorText = e.userFriendlyMsg();
        //                ok = false;
        //            }
        //            return std::make_pair(ok, errorText);
        //        }
        //
        //
        //        std::pair<bool, std::string > DeviceClient::instantiateWait(const std::string& serverInstanceId, const karabo::util::Hash& configuration, int timeout) {
        //            if (timeout == -1) timeout = m_defaultTimeout;
        //            bool ok = true;
        //            std::string errorText = "";
        //
        //            try {
        //                m_signalSlotable->request(serverInstanceId, "slotStartDevice", configuration).timeout(timeout).receive(ok, errorText);
        //            } catch (const karabo::util::Exception& e) {
        //
        //
        //                errorText = e.userFriendlyMsg();
        //                ok = false;
        //            }
        //            return std::make_pair(ok, errorText);
        //        }


        void DeviceClient::killNoWait(const std::string & instanceId) {
            m_signalSlotable->call(instanceId, "slotKillDeviceInstance");
        }


        std::pair<bool, std::string> DeviceClient::killWait(const std::string& instanceId) {
            // TODO implement
            throw KARABO_NOT_IMPLEMENTED_EXCEPTION("availble soon");

        }


        karabo::util::Hash DeviceClient::get(const std::string & instanceId) {
            return cacheAndGetConfiguration(instanceId);
        }


        karabo::util::Hash DeviceClient::cacheAndGetConfiguration(const std::string& instanceId) {
            boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
            std::string path("device." + instanceId + ".configuration");
            boost::optional<Hash::Node&> node = m_runtimeSystemDescription.find(path);
            if (!node) { // Not found, request and cache
                // Request configuration
                Hash hash;
                try {
                    m_signalSlotable->request(instanceId, "slotRefresh").timeout(m_defaultTimeout).receive(hash);
                } catch (const TimeoutException&) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Configuration request for instance \"" << instanceId << "\" timed out";
                    Exception::clearTrace();
                    return Hash();
                }
                // Keep up to date from now on
                m_signalSlotable->connectN(instanceId, "signalChanged", "", "slotChanged");
                return m_runtimeSystemDescription.set(path, hash).getValue<Hash>();
            } else {
                refreshInstanceUsage(instanceId); // Keep cache for longer
                return node->getValue<Hash>();
            }
        }


        void DeviceClient::get(const std::string& instanceId, karabo::util::Hash& hash) {
            hash = cacheAndGetConfiguration(instanceId);
        }


        void DeviceClient::registerDeviceMonitor(const std::string& instanceId, const boost::function<void (const karabo::util::Hash&, const std::string&)> & callbackFunction) {
            boost::mutex::scoped_lock lock(m_deviceChangedHandlersMutex);
            // Make sure we are caching this instanceId
            this->cacheAndGetConfiguration(instanceId);
            m_deviceChangedHandlers.set(instanceId + "._function", callbackFunction);
        }


        void DeviceClient::unregisterPropertyMonitor(const std::string& instanceId, const std::string& key) {
            boost::mutex::scoped_lock lock(m_propertyChangedHandlersMutex);
            boost::optional<Hash::Node&> node = m_propertyChangedHandlers.find(instanceId);
            if (node) {
                Hash& tmp = node->getValue<Hash >();
                boost::optional<Hash::Node&> tmpNode = tmp.find(key);
                if (tmpNode) {
                    tmp.erase(tmpNode->getKey());
                }
                if (tmp.empty()) {
                    m_propertyChangedHandlers.erase(node->getKey());
                }
            }
        }


        void DeviceClient::unregisterDeviceMonitor(const std::string& instanceId) {
            boost::mutex::scoped_lock lock(m_deviceChangedHandlersMutex);
            m_deviceChangedHandlers.erase(instanceId);
        }


        std::pair<bool, std::string > DeviceClient::setWait(const std::string& instanceId, const karabo::util::Hash& values, int timeout) const {
            if (timeout == -1) timeout = m_defaultTimeout;

            bool ok = true;
            std::string errorText = "";

            try {
                // TODO Add error text to response
                m_signalSlotable->request(instanceId, "slotReconfigure", values).timeout(timeout).receive(ok, errorText);
            } catch (const karabo::util::Exception& e) {
                errorText = e.userFriendlyMsg();
                ok = false;
            }
            return std::make_pair(ok, errorText);
        }


        void DeviceClient::setNoWait(const std::string& instanceId, const karabo::util::Hash& values) const {
            m_signalSlotable->call(instanceId, "slotReconfigure", values);
        }


        std::string DeviceClient::generateOwnInstanceId() {
            return std::string(boost::asio::ip::host_name() + "_DeviceClient_" + karabo::util::toString(getpid()));
        }


        void DeviceClient::refreshInstanceUsage(const std::string & instanceId) {


            boost::mutex::scoped_lock lock(m_instanceUsageMutex);
            m_instanceUsage[instanceId] = 0;
        }


        void DeviceClient::notifyDeviceChangedMonitors(const karabo::util::Hash& hash, const std::string & instanceId) {
            boost::mutex::scoped_lock lock(m_deviceChangedHandlersMutex);
            boost::optional<Hash::Node&> node = m_deviceChangedHandlers.find(instanceId);
            if (node) {
                const Hash& entry = node->getValue<Hash >();
                boost::optional<const Hash::Node&> nodeFunc = entry.find("_function");
                boost::optional<const Hash::Node&> nodeData = entry.find("_userData");
                if (nodeData) {
                    boost::any_cast < boost::function<void (const karabo::util::Hash&, const std::string&, const boost::any&)> >(nodeFunc->getValueAsAny())(hash, instanceId, nodeData->getValueAsAny());
                } else {
                    boost::any_cast < boost::function<void (const karabo::util::Hash&, const std::string&)> >(nodeFunc->getValueAsAny())(hash, instanceId);
                }
            }
        }


        void DeviceClient::notifyPropertyChangedMonitors(const karabo::util::Hash& hash, const std::string & instanceId) {
            boost::mutex::scoped_lock lock(m_propertyChangedHandlersMutex);
            if (m_propertyChangedHandlers.has(instanceId)) {


                castAndCall(instanceId, m_propertyChangedHandlers.get<karabo::util::Hash > (instanceId), hash);
            }
        }


        void DeviceClient::castAndCall(const std::string& instanceId, const Hash& registered, const Hash& current, std::string path) const {

            #define KARABO_REGISTER_CALLBACK(valueType) \
if (nodeData) {\
    boost::any_cast < boost::function<void (const valueType&, const std::string&, const boost::any&) > >(nodeFunc->getValueAsAny())(it->getValue<valueType >(), instanceId, nodeData->getValueAsAny());\
} else {\
    boost::any_cast < boost::function<void (const valueType&, const std::string&) > >(nodeFunc->getValueAsAny())(it->getValue<valueType >(), instanceId);\
}

            for (karabo::util::Hash::const_iterator it = current.begin(); it != current.end(); ++it) {
                std::string currentPath = it->getKey();
                if (!path.empty()) currentPath = path + "." + it->getKey();
                if (registered.has(currentPath)) {
                    const Hash& entry = registered.get<Hash > (currentPath);
                    boost::optional<const Hash::Node&> nodeFunc = entry.find("_function");
                    boost::optional<const Hash::Node&> nodeData = entry.find("_userData");

                    if (it->is<bool>()) {
                        KARABO_REGISTER_CALLBACK(bool);
                    } else if (it->is<char>()) {
                        KARABO_REGISTER_CALLBACK(char);
                    } else if (it->is<signed char>()) {
                        KARABO_REGISTER_CALLBACK(signed char);
                    } else if (it->is<unsigned char>()) {
                        KARABO_REGISTER_CALLBACK(unsigned char);
                    } else if (it->is<short>()) {
                        KARABO_REGISTER_CALLBACK(short);
                    } else if (it->is<unsigned short>()) {
                        KARABO_REGISTER_CALLBACK(unsigned short);
                    } else if (it->is<int>()) {
                        KARABO_REGISTER_CALLBACK(int);
                    } else if (it->is<unsigned int>()) {
                        KARABO_REGISTER_CALLBACK(unsigned int);
                    } else if (it->is<unsigned long long>()) {
                        KARABO_REGISTER_CALLBACK(unsigned long long);
                    } else if (it->is<float>()) {
                        KARABO_REGISTER_CALLBACK(float);
                    } else if (it->is<double>()) {
                        KARABO_REGISTER_CALLBACK(double);
                    } else if (it->is<std::string > ()) {
                        KARABO_REGISTER_CALLBACK(std::string);
                    } else if (it->is<boost::filesystem::path > ()) {
                        KARABO_REGISTER_CALLBACK(boost::filesystem::path);
                    } else if (it->is<karabo::util::Hash > ()) {
                        KARABO_REGISTER_CALLBACK(karabo::util::Hash);
                    } else if (it->is < std::vector<bool> >()) {
                        KARABO_REGISTER_CALLBACK(std::vector<bool>);
                    } else if (it->is<std::vector<char> >()) {
                        KARABO_REGISTER_CALLBACK(std::vector<char>);
                    } else if (it->is < std::vector<signed char> >()) {
                        KARABO_REGISTER_CALLBACK(std::vector<signed char>);
                    } else if (it->is<std::vector<unsigned char> >()) {
                        KARABO_REGISTER_CALLBACK(std::vector<unsigned char>);
                    } else if (it->is<std::vector<short> >()) {
                        KARABO_REGISTER_CALLBACK(std::vector<short>);
                    } else if (it->is<std::vector<unsigned short> >()) {
                        KARABO_REGISTER_CALLBACK(std::vector<unsigned short>);
                    } else if (it->is<std::vector<int> >()) {
                        KARABO_REGISTER_CALLBACK(std::vector<int>);
                    } else if (it->is<std::vector<unsigned int> >()) {
                        KARABO_REGISTER_CALLBACK(std::vector<unsigned int>);
                    } else if (it->is<std::vector<long long> >()) {
                        KARABO_REGISTER_CALLBACK(std::vector<long long>);
                    } else if (it->is<std::vector<unsigned long long> >()) {
                        KARABO_REGISTER_CALLBACK(std::vector<unsigned long long>);
                    } else if (it->is<std::vector<float> >()) {
                        KARABO_REGISTER_CALLBACK(std::vector<float>);
                    } else if (it->is<std::vector<double> >()) {
                        KARABO_REGISTER_CALLBACK(std::vector<double>);
                    } else if (it->is<karabo::util::Schema > ()) {
                        KARABO_REGISTER_CALLBACK(karabo::util::Schema);
                    } else if (it->is<std::vector<std::string> >()) {
                        KARABO_REGISTER_CALLBACK(std::vector<std::string>);
                    } else if (it->is<std::vector<karabo::util::Hash> >()) {
                        KARABO_REGISTER_CALLBACK(std::vector<karabo::util::Hash>);

                    } else {
                        throw KARABO_LOGIC_EXCEPTION("Failed to call registered monitored (datatype problems)");
                    }
                }


                if (it->is<karabo::util::Hash > ()) castAndCall(instanceId, registered, it->getValue<Hash >(), currentPath);
            }
        }


        void DeviceClient::clearCacheAndDisconnect(const std::string & instanceId) {
            //            boost::mutex::scoped_lock lock(m_configurationCacheMutex);
            //            m_signalSlotable->disconnect(instanceId, "signalChanged", "", "slotChanged", false);
            //            m_configurationCache.erase(instanceId);
        }


    }
}
