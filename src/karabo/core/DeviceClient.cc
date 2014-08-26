/*
 * $Id: Com.cc 6624 2012-06-27 12:57:09Z heisenb $
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (c) 2010-2012 European XFEL GmbH Hamburg. All rights reserved.
 */

#include <karabo/log/Logger.hh>
#include <karabo/io/FileTools.hh>
#include <karabo/webAuth/Authenticator.hh>

#include "DeviceClient.hh"

using namespace std;
using namespace karabo::util;
using namespace karabo::xms;
using namespace karabo::webAuth;


namespace karabo {
    namespace core {


        DeviceClient::DeviceClient(const std::string& brokerType, const karabo::util::Hash& brokerConfiguration) :
        m_isShared(false), m_internalTimeout(2000), m_isAdvancedMode(false), m_getOlder(true) {

            std::string ownInstanceId = generateOwnInstanceId();
            m_signalSlotable = boost::shared_ptr<SignalSlotable > (new SignalSlotable(ownInstanceId, brokerType, brokerConfiguration));
            
            
            karabo::util::Hash instanceInfo;
            instanceInfo.set("type", "client");
            instanceInfo.set("compatibility", DeviceClient::classInfo().getVersion());
            instanceInfo.set("host", boost::asio::ip::host_name());
                        
            m_eventThread = boost::thread(boost::bind(&karabo::xms::SignalSlotable::runEventLoop, m_signalSlotable, 60, instanceInfo, 2));

            // TODO Comment in to activate aging
            m_ageingThread = boost::thread(boost::bind(&karabo::core::DeviceClient::age, this));

            this->setupSlots();
            this->checkMaster();
            this->cacheAvailableInstances();

        }


        DeviceClient::DeviceClient(const boost::shared_ptr<SignalSlotable>& signalSlotable) :
        m_signalSlotable(signalSlotable), m_isShared(true), m_internalTimeout(2000), m_isAdvancedMode(false), m_getOlder(true) {

            // TODO Comment in to activate aging
            m_ageingThread = boost::thread(boost::bind(&karabo::core::DeviceClient::age, this));

            this->setupSlots();
            this->checkMaster();
            this->cacheAvailableInstances();

        }


        void DeviceClient::setupSlots() {
            m_signalSlotable->registerSlot<Hash, string > (boost::bind(&karabo::core::DeviceClient::slotChanged, this, _1, _2), "slotChanged");
            m_signalSlotable->registerSlot<string, Hash > (boost::bind(&karabo::core::DeviceClient::slotInstanceNew, this, _1, _2), "slotInstanceNew", SignalSlotable::GLOBAL);
            m_signalSlotable->registerSlot<string, Hash > (boost::bind(&karabo::core::DeviceClient::slotInstanceUpdated, this, _1, _2), "slotInstanceUpdated", SignalSlotable::GLOBAL);
            m_signalSlotable->registerSlot<string, Hash > (boost::bind(&karabo::core::DeviceClient::slotInstanceGone, this, _1, _2), "slotInstanceGone", SignalSlotable::GLOBAL);
            m_signalSlotable->registerInstanceNotAvailableHandler(boost::bind(&karabo::core::DeviceClient::onInstanceNotAvailable, this, _1, _2));
            m_signalSlotable->registerInstanceAvailableAgainHandler(boost::bind(&karabo::core::DeviceClient::onInstanceAvailableAgain, this, _1, _2));
        }


        void DeviceClient::checkMaster() {

            try {
                m_signalSlotable->request("*", "slotMasterPing").timeout(100).receive(m_masterDeviceId);
            } catch (karabo::util::TimeoutException&) {
                karabo::util::Exception::clearTrace();
                // Allow for any Karabo_Master to become master
                if (m_signalSlotable->getInstanceId().substr(0, 13) == "Karabo_Master") {
                    KARABO_LOG_FRAMEWORK_INFO << "Instance \"" << m_signalSlotable->getInstanceId() << "\" is becoming master";
                    m_signalSlotable->registerSlot<string > (boost::bind(&karabo::core::DeviceClient::slotMasterPing, this), "slotMasterPing", SignalSlotable::GLOBAL);
                    m_masterMode = IS_MASTER;
                } else { // stand-alone mode
                    KARABO_LOG_FRAMEWORK_DEBUG << "No master instance found. Running in stand-alone mode";
                    m_masterMode = NO_MASTER;
                }
                return;
            }
            KARABO_LOG_FRAMEWORK_DEBUG << "Master instance found (\"" << m_masterDeviceId << "\")";
            m_masterMode = HAS_MASTER;
        }


        void DeviceClient::cacheAvailableInstances() {
            vector<pair<string, Hash> > instances;
            if (m_masterMode == HAS_MASTER) {
                instances = m_signalSlotable->getAvailableInstances(false); // Without tracking
            } else {
                instances = m_signalSlotable->getAvailableInstances(true); // With tracking
            }
            for (size_t i = 0; i < instances.size(); ++i) {
                const string& instanceId = instances[i].first;
                const Hash& instanceInfo = instances[i].second;

                mergeIntoRuntimeSystemDescription(prepareTopologyEntry(instanceId, instanceInfo));

            }
            KARABO_LOG_FRAMEWORK_DEBUG << "cacheAvailableInstances() was called";
        }


        karabo::util::Hash DeviceClient::prepareTopologyEntry(const std::string& instanceId, const karabo::util::Hash& instanceInfo) {
            Hash entry;
            string path(prepareTopologyPath(instanceId, instanceInfo));
            Hash::Node & entryNode = entry.set(path, Hash());
            for (Hash::const_iterator it = instanceInfo.begin(); it != instanceInfo.end(); ++it) {
                entryNode.setAttribute(it->getKey(), it->getValueAsAny());
            }
            return entry;
        }


        std::string DeviceClient::prepareTopologyPath(const std::string& instanceId, const karabo::util::Hash& instanceInfo) {
            boost::optional<const Hash::Node&> node = instanceInfo.find("type");
            string type = "unknown";
            if (node) type = node->getValue<string>();
            return string(type + "." + instanceId);
        }


        void DeviceClient::mergeIntoRuntimeSystemDescription(const karabo::util::Hash& entry) {
            boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
            m_runtimeSystemDescription.merge(entry);
        }


        bool DeviceClient::existsInRuntimeSystemDescription(const std::string& path) const {
            boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
            return m_runtimeSystemDescription.has(path);
        }


        void DeviceClient::slotInstanceNew(const std::string& instanceId, const karabo::util::Hash& instanceInfo) {
            KARABO_LOG_FRAMEWORK_DEBUG << "slotInstanceNew was called";

            string path(prepareTopologyPath(instanceId, instanceInfo));

            // If this is true, the instance silently died and was restarted so quickly we couldn't recognize
            if (existsInRuntimeSystemDescription(path)) {

                KARABO_LOG_FRAMEWORK_INFO << "Detected unrecognized dirty shutdown of instance \"" << instanceId
                        << "\", which got quickly restarted and is now available again";

                eraseFromRuntimeSystemDescription(path);
                eraseFromInstanceUsage(instanceId);

            }

            Hash entry = prepareTopologyEntry(instanceId, instanceInfo);
            mergeIntoRuntimeSystemDescription(entry);

            // Track the instance if we are running without master
            if (m_masterMode != HAS_MASTER) m_signalSlotable->trackExistenceOfInstance(instanceId);

            if (m_instanceNewHandler) m_instanceNewHandler(entry);
        }


        void DeviceClient::onInstanceAvailableAgain(const std::string& instanceId, const Hash& instanceInfo) {
            KARABO_LOG_FRAMEWORK_DEBUG << "onInstanceAvailableAgain was called";

            try {
                string path = prepareTopologyPath(instanceId, instanceInfo);
                bool hasInstance = existsInRuntimeSystemDescription(path);

                if (hasInstance) {
                    // Should never happen
                    KARABO_LOG_FRAMEWORK_ERROR << "Instance \"" << instanceId << "\" silently appeared, which never died before.";
                } else {
                    KARABO_LOG_FRAMEWORK_INFO << "Previously lost instance \"" << instanceId << "\" silently came back";
                    Hash entry = prepareTopologyEntry(instanceId, instanceInfo);
                    mergeIntoRuntimeSystemDescription(entry);
                    if (m_instanceNewHandler) m_instanceNewHandler(entry);
                }
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onInstanceAvailableAgain(): " << e;
            } catch (...) {
                KARABO_LOG_FRAMEWORK_ERROR << "Unknown exception thrown in onInstanceAvailableAgain()";
            }
        }


        void DeviceClient::eraseFromRuntimeSystemDescription(const std::string& path) {
            try {
                boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
                m_runtimeSystemDescription.erase(path);
            } catch (...) {
                KARABO_LOG_FRAMEWORK_ERROR << "Could not erase path \"" << path << " from device-client cache";
            }
        }


        void DeviceClient::removeFromSystemTopology(const std::string& instanceId) {
            boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
            for (Hash::iterator it = m_runtimeSystemDescription.begin(); it != m_runtimeSystemDescription.end(); ++it) {
                Hash& tmp = it->getValue<Hash>();
                boost::optional<Hash::Node&> node = tmp.find(instanceId);
                if (node) {
                    tmp.erase(instanceId);
                    break;
                }
            }
        }


        void DeviceClient::slotInstanceUpdated(const std::string& instanceId, const karabo::util::Hash& instanceInfo) {

            Hash entry = prepareTopologyEntry(instanceId, instanceInfo);
            mergeIntoRuntimeSystemDescription(entry);

            if (m_instanceUpdatedHandler) m_instanceUpdatedHandler(entry);

            KARABO_LOG_FRAMEWORK_DEBUG << "slotInstanceUpdated was called";
        }


        void DeviceClient::onInstanceNotAvailable(const std::string& instanceId, const karabo::util::Hash& instanceInfo) {
            try {
                // TODO We have now instanceInfo information, could send it on
                KARABO_LOG_FRAMEWORK_DEBUG << "Instance \"" << instanceId << "\" silently disappeared " << instanceInfo;

                string path = prepareTopologyPath(instanceId, instanceInfo);
                eraseFromRuntimeSystemDescription(path);
                eraseFromInstanceUsage(instanceId);
                if (m_instanceGoneHandler) m_instanceGoneHandler(instanceId, instanceInfo);

            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onInstanceNotAvailable(): " << e;
            } catch (...) {
                KARABO_LOG_FRAMEWORK_ERROR << "Unknown exception thrown in onInstanceNotAvailable()";
            }
        }


        void DeviceClient::slotInstanceGone(const std::string& instanceId, const karabo::util::Hash& instanceInfo) {
            KARABO_LOG_FRAMEWORK_DEBUG << "slotInstanceGone was called";
            // Ignore the signal if it comes from master
            if (m_masterMode == IS_MASTER) {
                if (m_signalSlotable->getSenderInfo("slotInstanceGone")->getInstanceIdOfSender() == m_signalSlotable->getInstanceId()) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "Ignoring information from myself to remove silently disappeared: " << instanceId;
                    return;
                }
            }
            string path = prepareTopologyPath(instanceId, instanceInfo);
            eraseFromRuntimeSystemDescription(path);
            eraseFromInstanceUsage(instanceId);
            if (m_masterMode != HAS_MASTER) m_signalSlotable->stopTrackingExistenceOfInstance(instanceId);
            if (m_instanceGoneHandler) m_instanceGoneHandler(instanceId, instanceInfo);
        }


        DeviceClient::~DeviceClient() {
            setAgeing(false); // Joins the thread

            if (!m_isShared) {
                m_signalSlotable->stopEventLoop();
                m_eventThread.join();
            }
        }


        void DeviceClient::setInternalTimeout(const unsigned int internalTimeout) {
            m_internalTimeout = internalTimeout;
        }


        int DeviceClient::getInternalTimeout() const {
            return m_internalTimeout;
        }


        void DeviceClient::enableAdvancedMode() {
            m_isAdvancedMode = true;
        }


        void DeviceClient::disableAdvancedMode() {
            m_isAdvancedMode = false;
        }


        void DeviceClient::setAgeing(bool on) {
            if (on && !m_getOlder) {
                m_getOlder = true;
                m_ageingThread = boost::thread(boost::bind(&karabo::core::DeviceClient::age, this));
            } else if (!on && m_getOlder) {
                m_getOlder = false;
                m_ageingThread.join();
            }
        }


        std::pair<bool, std::string> DeviceClient::exists(const std::string& instanceId) {
            return m_signalSlotable->exists(instanceId);
        }


        Hash DeviceClient::getSystemInformation() {
            return m_runtimeSystemDescription;
        }


        Hash DeviceClient::getSystemTopology() {
            boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
            Hash topology;
            for (Hash::const_iterator it = m_runtimeSystemDescription.begin(); it != m_runtimeSystemDescription.end(); ++it) {
                const std::string& categoryName = it->getKey();
                const Hash& category = it->getValue<Hash>();
                Hash& entry = topology.bindReference<Hash>(categoryName);
                for (Hash::const_iterator jt = category.begin(); jt != category.end(); ++jt) {
                    Hash::Node& node = entry.set(jt->getKey(), Hash());
                    node.setAttributes(jt->getAttributes());
                }
            }
            return topology;
        }


        std::vector<std::string> DeviceClient::getServers() {
            boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
            if (m_runtimeSystemDescription.has("server")) {
                const Hash& tmp = m_runtimeSystemDescription.get<Hash>("server");
                vector<string> deviceServers;
                deviceServers.reserve(tmp.size());
                for (Hash::const_iterator it = tmp.begin(); it != tmp.end(); ++it) {
                    if (it->hasAttribute("visibility")) {
                        if (m_signalSlotable->getAccessLevel(it->getKey()) < it->getAttribute<int>("visibility")) continue;
                    }
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
                if (m_runtimeSystemDescription.hasAttribute("server." + deviceServer, "deviceClasses")) {
                    return m_runtimeSystemDescription.getAttribute<vector<string> >("server." + deviceServer, "deviceClasses");
                } else {
                    return vector<string>();
                }
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
                        if (m_signalSlotable->getAccessLevel(it->getKey()) < it->getAttribute<int>("visibility")) continue;
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
                            if (m_signalSlotable->getAccessLevel(it->getKey()) < it->getAttribute<int>("visibility")) continue;
                        }
                        devices.push_back(it->getKey());
                    }
                }
                return devices;
            }
        }


        karabo::util::Schema DeviceClient::getDeviceSchema(const std::string& instanceId) {
            return cacheAndGetDeviceSchema(instanceId);
        }


        karabo::util::Schema DeviceClient::cacheAndGetDeviceSchema(const std::string & instanceId) {
            boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
            std::string path("device." + instanceId + ".fullSchema");
            boost::optional<Hash::Node&> node = m_runtimeSystemDescription.find(path);
            if (!node) { // Not found, request and cache it
                // Request schema
                Schema schema;
                try {
                    m_signalSlotable->request(instanceId, "slotGetSchema", false).timeout(m_internalTimeout).receive(schema); // Retrieves full schema
                } catch (const TimeoutException&) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Schema request for instance \"" << instanceId << "\" timed out";
                    Exception::clearTrace();
                    return Schema();
                }
                return m_runtimeSystemDescription.set(path, schema).getValue<Schema>();
            }
            return node->getValue<Schema>();
        }


        karabo::util::Schema DeviceClient::getDeviceSchemaNoWait(const std::string& instanceId) {
            boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
            std::string path("device." + instanceId + ".fullSchema");
            const boost::optional<Hash::Node&> node = m_runtimeSystemDescription.find(path);
            if (!node || node->getValue<Schema>().empty()) { // Not found, request it
                m_signalSlotable->call(instanceId, "slotGetSchema", false);
                return Schema();
            }
            return node->getValue<Schema>();
        }


        void DeviceClient::slotSchemaUpdated(const karabo::util::Schema& schema, const std::string& deviceId) {
            {
                boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
                KARABO_LOG_FRAMEWORK_DEBUG << "slotSchemaUpdated";
                string path("device." + deviceId + ".fullSchema");
                m_runtimeSystemDescription.set(path, schema);

                path = "device." + deviceId + ".activeSchema";
                if (m_runtimeSystemDescription.has(path)) m_runtimeSystemDescription.erase(path);
            }
            if (m_schemaUpdatedHandler) m_schemaUpdatedHandler(deviceId, schema);
        }


        karabo::util::Schema DeviceClient::getActiveSchema(const std::string& instanceId) {
            return cacheAndGetActiveSchema(instanceId);
        }


        karabo::util::Schema DeviceClient::cacheAndGetActiveSchema(const std::string& instanceId) {
            std::string state = this->get<std::string > (instanceId, "state");
            std::string path("device." + instanceId + ".activeSchema." + state);
            {
                boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
                boost::optional<Hash::Node&> node = m_runtimeSystemDescription.find(path);
                if (!node) { // Not found, request and cache it
                    // Request schema
                    Schema schema;
                    try {
                        m_signalSlotable->request(instanceId, "slotGetSchema", true).timeout(m_internalTimeout).receive(schema); // Retrieves active schema
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
        }


        karabo::util::Schema DeviceClient::getClassSchema(const std::string& serverId, const std::string& classId) {
            return cacheAndGetClassSchema(serverId, classId);
        }


        karabo::util::Schema DeviceClient::cacheAndGetClassSchema(const std::string& serverId, const std::string& classId) {
            boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
            std::string path("server." + serverId + ".classes." + classId + ".description");
            boost::optional<Hash::Node&> node = m_runtimeSystemDescription.find(path);
            if (!node) { // Not found, request and cache it
                // Request schema
                Schema schema;
                try {
                    m_signalSlotable->request(serverId, "slotGetClassSchema", classId).timeout(m_internalTimeout).receive(schema); // Retrieves full schema
                } catch (const TimeoutException&) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Schema request for server \"" << serverId << "\" timed out";
                    Exception::clearTrace();
                    return Schema();
                }
                return m_runtimeSystemDescription.set(path, schema).getValue<Schema>();
            }
            return node->getValue<Schema>();
        }


        karabo::util::Schema DeviceClient::getClassSchemaNoWait(const std::string& serverId, const std::string& classId) {
            boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
            std::string path("server." + serverId + ".classes." + classId + ".description");
            boost::optional<Hash::Node&> node = m_runtimeSystemDescription.find(path);
            if (!node || node->getValue<Schema>().empty()) { // Not found, request and cache it
                // Request schema                
                m_signalSlotable->call(serverId, "slotGetClassSchema", classId);
                return Schema();
            }
            return node->getValue<Schema>();
        }


        void DeviceClient::slotClassSchema(const karabo::util::Schema& schema, const std::string& classId, const std::string& serverId) {
            {
                boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
                KARABO_LOG_FRAMEWORK_DEBUG << "slotSchemaUpdated";
                std::string path("server." + serverId + ".classes." + classId + ".description");
                m_runtimeSystemDescription.set(path, schema);
            }
            if (m_classSchemaHandler) m_classSchemaHandler(serverId, classId, schema);
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


        std::vector<std::string> DeviceClient::getCurrentlySettableProperties(const std::string& deviceId) {
            Schema schema = cacheAndGetActiveSchema(deviceId);
            int accessLevel = m_signalSlotable->getAccessLevel(deviceId);
            return filterProperties(schema, accessLevel);
        }


        std::vector<std::string> DeviceClient::getProperties(const std::string& deviceId) {
            Schema schema = cacheAndGetDeviceSchema(deviceId);
            int accessLevel = m_signalSlotable->getAccessLevel(deviceId);
            return filterProperties(schema, accessLevel);
        }


        std::vector<std::string> DeviceClient::getClassProperties(const std::string& serverId, const std::string& classId) {
            Schema schema = cacheAndGetClassSchema(serverId, classId);
            int accessLevel = m_signalSlotable->getAccessLevel(classId);
            return filterProperties(schema, accessLevel);
        }


        std::vector<std::string> DeviceClient::filterProperties(const karabo::util::Schema& schema, const int accessLevel) {
            vector<string> paths = schema.getPaths();
            std::vector<std::string> properties;


            BOOST_FOREACH(std::string path, paths) {
                if (schema.isProperty(path)) {
                    if (accessLevel < schema.getRequiredAccessLevel(path)) {
                        continue; // Not allowed
                    }
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


        std::pair<bool, std::string > DeviceClient::instantiate(const std::string& serverInstanceId, const std::string& classId, const karabo::util::Hash& configuration, int timeoutInSeconds) {
            Hash tmp(classId, configuration);
            return this->instantiate(serverInstanceId, tmp, timeoutInSeconds);
        }


        std::pair<bool, std::string > DeviceClient::instantiate(const std::string& serverInstanceId, const karabo::util::Hash& configuration, int timeoutInSeconds) {
            if (timeoutInSeconds == -1) timeoutInSeconds = 5;
            bool ok = true;
            std::string reply = "";
            try {
                m_signalSlotable->request(serverInstanceId, "slotStartDevice", configuration).timeout(timeoutInSeconds * 1000).receive(ok, reply);
            } catch (const karabo::util::Exception& e) {
                reply = e.userFriendlyMsg();
                ok = false;
                return std::make_pair(ok, reply);
            }
            if (ok) {
                // Wait until this device says hello
                bool isThere;
                int nTrials = 0;
                do {
                    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
                    nTrials++;
                    m_runtimeSystemDescriptionMutex.lock();
                    isThere = m_runtimeSystemDescription.has("device." + reply);
                    m_runtimeSystemDescriptionMutex.unlock();
                } while (!isThere && (nTrials < 20));

                if (nTrials == 20) {
                    string errorText("Device \"" + reply + "\" got started but is not accessible anymore... ZOMBIE TIME !!!!");
                    return std::make_pair(false, errorText);
                }
            }
            return std::make_pair(ok, reply);
        }


        void DeviceClient::killDeviceNoWait(const std::string & deviceId) {
            m_signalSlotable->call(deviceId, "slotKillDevice");
        }


        std::pair<bool, std::string> DeviceClient::killDevice(const std::string& deviceId, int timeoutInSeconds) {
            // TODO Do not hard code the default timeout
            if (timeoutInSeconds == -1) timeoutInSeconds = 30;
            // Give it a kill signal
            m_signalSlotable->call(deviceId, "slotKillDevice");
            // Wait until this device is gone
            bool isThere;
            int nTrials = 0;
            do {
                boost::this_thread::sleep(boost::posix_time::seconds(1));
                nTrials++;
                m_runtimeSystemDescriptionMutex.lock();
                isThere = m_runtimeSystemDescription.has("device." + deviceId);
                m_runtimeSystemDescriptionMutex.unlock();
            } while (isThere && (nTrials < timeoutInSeconds));

            if (nTrials == timeoutInSeconds) {
                string errorText("Device \"" + deviceId + "\" does not want to die in time. Try to kill it with a hammer.");
                return std::make_pair(false, errorText);
            }
            return std::make_pair(true, deviceId);
        }


        std::pair<bool, std::string> DeviceClient::killServer(const std::string& serverId, int timeoutInSeconds) {
            bool ok = true;
            string reply;
            // TODO Do not hard code the default timeout
            if (timeoutInSeconds == -1) timeoutInSeconds = 30;
            try {
                // TODO Add error text to response
                m_signalSlotable->request(serverId, "slotKillServer").timeout(timeoutInSeconds * 1000).receive(reply);
            } catch (const karabo::util::Exception& e) {
                reply = e.userFriendlyMsg();
                ok = false;
            }
            // Wait until this server is gone
            bool isThere;
            int nTrials = 0;
            do {
                boost::this_thread::sleep(boost::posix_time::seconds(1));
                nTrials++;
                m_runtimeSystemDescriptionMutex.lock();
                isThere = m_runtimeSystemDescription.has("server." + serverId);
                m_runtimeSystemDescriptionMutex.unlock();
            } while (isThere && (nTrials < timeoutInSeconds));

            if (nTrials == timeoutInSeconds) {
                string errorText("Server \"" + serverId + "\" does not want to die in time. Try to kill it with a hammer.");
                return std::make_pair(false, errorText);
            }
            return std::make_pair(ok, reply);
        }


        void DeviceClient::killServerNoWait(const std::string& serverId) {
            m_signalSlotable->call(serverId, "slotKillServer");
        }


        karabo::util::Hash DeviceClient::get(const std::string & instanceId) {
            return cacheAndGetConfiguration(instanceId);
        }


        karabo::util::Hash DeviceClient::cacheAndGetConfiguration(const std::string& deviceId) {
            boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
            std::string path("device." + deviceId + ".configuration");
            boost::optional<Hash::Node&> node = m_runtimeSystemDescription.find(path);
            if (!node) { // Not found, request and cache
                // Request configuration
                Hash hash;
                try {
                    m_signalSlotable->request(deviceId, "slotGetConfiguration").timeout(m_internalTimeout).receive(hash);
                } catch (const TimeoutException&) {
                    KARABO_RETHROW_AS(KARABO_TIMEOUT_EXCEPTION("Configuration request for device \"" + deviceId + "\" timed out"));
                    return Hash();
                }
                stayConnected(deviceId);
                return m_runtimeSystemDescription.set(path, hash).getValue<Hash>();
            } else {
                stayConnected(deviceId);
                return node->getValue<Hash>();
            }
        }


        void DeviceClient::get(const std::string& instanceId, karabo::util::Hash& hash) {
            hash = cacheAndGetConfiguration(instanceId);
        }


        karabo::util::Hash DeviceClient::getConfigurationNoWait(const std::string& deviceId) {
            boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
            std::string path("device." + deviceId + ".configuration");
            boost::optional<Hash::Node&> node = m_runtimeSystemDescription.find(path);
            stayConnected(deviceId);
            if (!node || node->getValue<Hash>().empty()) { // Not found, request
                m_signalSlotable->call(deviceId, "slotGetConfiguration");
                return Hash();
            }
            stayConnected(deviceId);
            return node->getValue<Hash>();
        }


        std::vector<karabo::util::Hash> DeviceClient::getFromPast(const std::string& deviceId, const std::string& key, const std::string& from, std::string to, int maxNumData) {
            return getPropertyHistory(deviceId, key, from, to, maxNumData);
        }


        karabo::util::vector<karabo::util::Hash> DeviceClient::getPropertyHistory(const std::string& deviceId, const std::string& key, const std::string& from, std::string to, int maxNumData) {
            if (to.empty()) to = karabo::util::Epochstamp().toIso8601();
            vector<Hash> result;
            // TODO Make this a global slot later
            Hash args("from", from, "to", to, "maxNumData", maxNumData);
            m_signalSlotable->request("Karabo_FileDataLogger_0", "slotGetPropertyHistory", deviceId, key, args).timeout(60000).receive(result);
            return result;
        }


        std::pair<karabo::util::Hash, karabo::util::Schema> DeviceClient::getConfigurationFromPast(const std::string& deviceId, const std::string& timepoint) {
            Hash hash;
            Schema schema;
            m_signalSlotable->request("Karabo_FileDataLogger_0", "slotGetConfigurationFromPast", deviceId, timepoint).timeout(60000).receive(hash, schema);
            return make_pair(hash, schema);
        }


        void DeviceClient::registerInstanceNewMonitor(const InstanceNewHandler& callBackFunction) {
            m_instanceNewHandler = callBackFunction;
        }


        void DeviceClient::registerInstanceUpdatedMonitor(const InstanceUpdatedHandler& callBackFunction) {
            m_instanceUpdatedHandler = callBackFunction;
        }


        void DeviceClient::registerInstanceGoneMonitor(const InstanceGoneHandler& callBackFunction) {
            m_instanceGoneHandler = callBackFunction;
        }


        void DeviceClient::registerSchemaUpdatedMonitor(const SchemaUpdatedHandler& callBackFunction) {
            m_signalSlotable->registerSlot<Schema, string > (boost::bind(&karabo::core::DeviceClient::slotSchemaUpdated, this, _1, _2), "slotSchemaUpdated");
            m_schemaUpdatedHandler = callBackFunction;
        }


        void DeviceClient::registerClassSchemaMonitor(const ClassSchemaHandler& callBackFunction) {
            m_signalSlotable->registerSlot<Schema, string, string > (boost::bind(&karabo::core::DeviceClient::slotClassSchema, this, _1, _2, _3), "slotClassSchema");
            m_classSchemaHandler = callBackFunction;
        }


        void DeviceClient::registerDeviceMonitor(const std::string& deviceId, const boost::function<void (const std::string& /*deviceId*/, const karabo::util::Hash& /*config*/)> & callbackFunction) {
            boost::mutex::scoped_lock lock(m_deviceChangedHandlersMutex);
            m_signalSlotable->call(deviceId, "slotGetConfiguration");
            stayConnected(deviceId);
            m_deviceChangedHandlers.set(deviceId + "._function", callbackFunction);
            immortalize(deviceId);
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
                mortalize(instanceId);
            }
        }


        void DeviceClient::unregisterDeviceMonitor(const std::string& instanceId) {
            boost::mutex::scoped_lock lock(m_deviceChangedHandlersMutex);
            if (m_deviceChangedHandlers.has(instanceId)) m_deviceChangedHandlers.erase(instanceId);
            mortalize(instanceId);
        }


        std::pair<bool, std::string > DeviceClient::set(const std::string& instanceId, const karabo::util::Hash& values, int timeoutInSeconds) {

            stayConnected(instanceId);

            // TODO Do not hardcode the default timeout
            if (timeoutInSeconds == -1) timeoutInSeconds = 3;
            bool ok = true;
            std::string errorText = "";

            try {
                // TODO Add error text to response
                m_signalSlotable->request(instanceId, "slotReconfigure", values).timeout(timeoutInSeconds * 1000).receive(ok, errorText);
            } catch (const karabo::util::Exception& e) {
                errorText = e.userFriendlyMsg();
                ok = false;
            }

            return std::make_pair(ok, errorText);
        }


        void DeviceClient::setNoWait(const std::string& instanceId, const karabo::util::Hash& values) {
            stayConnected(instanceId);
            m_signalSlotable->call(instanceId, "slotReconfigure", values);
        }


        std::string DeviceClient::generateOwnInstanceId() {
            std::string hostname(boost::asio::ip::host_name());
            std::vector<std::string> tokens;
            boost::split(tokens, hostname, boost::is_any_of("."));
            return std::string(tokens[0] + "_DeviceClient_" + karabo::util::toString(getpid()));
        }


        void DeviceClient::stayConnected(const std::string & instanceId) {
            boost::mutex::scoped_lock lock(m_instanceUsageMutex);
            if (m_instanceUsage.find(instanceId) == m_instanceUsage.end()) { // Not there yet
                m_signalSlotable->connectT(instanceId, "signalChanged", "", "slotChanged");
                m_signalSlotable->connectT(instanceId, "signalSchemaUpdated", "", "slotSchemaUpdated");
            } else if (m_instanceUsage[instanceId] >= CONNECTION_KEEP_ALIVE) { // Died before
                m_signalSlotable->connectT(instanceId, "signalChanged", "", "slotChanged");
                m_signalSlotable->connectT(instanceId, "signalSchemaUpdated", "", "slotSchemaUpdated");
            }
            m_instanceUsage[instanceId] = 0;
        }


        void DeviceClient::eraseFromInstanceUsage(const std::string& instanceId) {
            boost::mutex::scoped_lock lock(m_instanceUsageMutex);
            m_instanceUsage.erase(instanceId);
        }


        void DeviceClient::slotChanged(const karabo::util::Hash& hash, const std::string & instanceId) {
            m_runtimeSystemDescriptionMutex.lock();
            // TODO Optimize speed
            if (m_runtimeSystemDescription.has("device." + instanceId + ".configuration")) {
                Hash& tmp = m_runtimeSystemDescription.get<Hash>("device." + instanceId + ".configuration");
                tmp.merge(hash);
                m_runtimeSystemDescriptionMutex.unlock();
                // NOTE: This will block us here, i.e. we are deaf for other changes...
                // NOTE: Monitors could be implemented as additional slots or in separate threads, too.
                notifyDeviceChangedMonitors(hash, instanceId);
                notifyPropertyChangedMonitors(hash, instanceId);
            } else {
                m_runtimeSystemDescription.set("device." + instanceId + ".configuration", hash);
                m_runtimeSystemDescriptionMutex.unlock();
                notifyDeviceChangedMonitors(hash, instanceId);
                notifyPropertyChangedMonitors(hash, instanceId);
            }
        }


        void DeviceClient::notifyDeviceChangedMonitors(const karabo::util::Hash& hash, const std::string & instanceId) {

            Hash entry;

            {
                boost::mutex::scoped_lock lock(m_deviceChangedHandlersMutex);
                boost::optional<Hash::Node&> node = m_deviceChangedHandlers.find(instanceId);
                if (node) {
                    entry = node->getValue<Hash >();
                }
            }

            if (!entry.empty()) {
                boost::optional<Hash::Node&> nodeFunc = entry.find("_function");
                boost::optional<Hash::Node&> nodeData = entry.find("_userData");
                if (nodeData) {
                    boost::any_cast < boost::function<void (const std::string&, const karabo::util::Hash&, const boost::any&)> >(nodeFunc->getValueAsAny())(instanceId, hash, nodeData->getValueAsAny());
                } else {
                    boost::any_cast < boost::function<void (const std::string&, const karabo::util::Hash&)> >(nodeFunc->getValueAsAny())(instanceId, hash);
                }
            }
        }


        void DeviceClient::notifyPropertyChangedMonitors(const karabo::util::Hash& hash, const std::string & instanceId) {

            Hash registered;

            {
                boost::mutex::scoped_lock lock(m_propertyChangedHandlersMutex);
                if (m_propertyChangedHandlers.has(instanceId)) {
                    registered = m_propertyChangedHandlers.get<Hash > (instanceId);
                }
            }

            if (!registered.empty()) {
                castAndCall(instanceId, registered, hash);
            }
        }


        void DeviceClient::castAndCall(const std::string& instanceId, const Hash& registered, const Hash& current, std::string path) const {

            #define KARABO_REGISTER_CALLBACK(valueType) \
if (nodeData) {\
    boost::any_cast < boost::function<void (const std::string&, const std::string&, const valueType&, const karabo::util::Timestamp&, const boost::any&) > >(nodeFunc->getValueAsAny())(instanceId, currentPath, it->getValue<valueType >(), t, nodeData->getValueAsAny());\
} else {\
    boost::any_cast < boost::function<void (const std::string&, const std::string&, const valueType&, const karabo::util::Timestamp&) > >(nodeFunc->getValueAsAny())(instanceId, currentPath, it->getValue<valueType >(), t);\
}

            for (karabo::util::Hash::const_iterator it = current.begin(); it != current.end(); ++it) {
                std::string currentPath = it->getKey();
                if (!path.empty()) currentPath = path + "." + it->getKey();
                if (registered.has(currentPath)) {
                    Timestamp t;
                    try {
                        t = Timestamp::fromHashAttributes(it->getAttributes());
                    } catch (...) {
                        KARABO_LOG_FRAMEWORK_WARN << "No timestamp information given on \"" << it->getKey() << "/";
                    }

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
                        throw KARABO_LOGIC_EXCEPTION("Failed to call registered monitor (datatype problems)");
                    }
                }


                if (it->is<karabo::util::Hash > ()) castAndCall(instanceId, registered, it->getValue<Hash >(), currentPath);
            }
        }


        void DeviceClient::slotMasterPing() {
            m_signalSlotable->reply(m_signalSlotable->getInstanceId());
        }


        void DeviceClient::slotProvideSystemTopology() {
            m_signalSlotable->reply(getSystemTopology());
        }


        void DeviceClient::age() {
            try {
                while (m_getOlder) { // Loop forever
                    for (InstanceUsage::iterator it = m_instanceUsage.begin(); it != m_instanceUsage.end(); ++it) { // Loop connected instances

                        if (isImmortal(it->first)) continue; // Immortal, registered monitors will have this status

                        it->second++; // Others just age
                        if (it->second == CONNECTION_KEEP_ALIVE) { // Too old
                            //cout << "Instance " << it->first << " got too old. It will die a natural death." << endl;
                            m_signalSlotable->disconnect(it->first, "signalChanged", "", "slotChanged", false);
                            m_signalSlotable->disconnect(it->first, "signalSchemaUpdated", "", "slotSchemaUpdated", false);
                            std::string path("device." + it->first + ".configuration");
                            boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
                            if (m_runtimeSystemDescription.has(path)) m_runtimeSystemDescription.erase(path);
                        }
                    }
                    boost::this_thread::sleep(boost::posix_time::seconds(1));
                }
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Aging thread encountered an exception: " << e;
            } catch (...) {
                KARABO_LOG_FRAMEWORK_ERROR << "Unknown exception encountered in aging thread";
            }
        }


        void DeviceClient::immortalize(const std::string& deviceId) {
            boost::mutex::scoped_lock lock(m_immortalsMutex);
            m_immortals.insert(deviceId);
        }


        void DeviceClient::mortalize(const std::string& deviceId) {
            boost::mutex::scoped_lock lock(m_immortalsMutex);
            m_immortals.erase(deviceId);
        }


        bool DeviceClient::isImmortal(const std::string& deviceId) const {
            boost::mutex::scoped_lock lock(m_immortalsMutex);
            return m_immortals.find(deviceId) != m_immortals.end();
        }


        bool DeviceClient::login(const std::string& username, const std::string& password, const std::string& provider) {
            return m_signalSlotable->login(username, password, provider);
        }


        bool DeviceClient::logout() {
            return m_signalSlotable->logout();
        }


        std::string DeviceClient::getInstanceType(const karabo::util::Hash& instanceInfo) const {
            boost::optional<const Hash::Node&> node = instanceInfo.find("type");
            string type("unknown");
            if (node) type = node->getValue<string>();
            return type;
        }



    }
}
