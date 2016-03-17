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
#include <karabo/core/DataLogUtils.hh>
#include <karabo/util/Schema.hh>

#include "DeviceClient.hh"
#include "karabo/net/utils.hh"

using namespace std;
using namespace karabo::util;
using namespace karabo::xms;
using namespace karabo::webAuth;


namespace karabo {
    namespace core {


        DeviceClient::DeviceClient(const std::string& brokerType, const karabo::util::Hash& brokerConfiguration)
        : m_internalSignalSlotable()
        , m_signalSlotable()
        , m_isShared(false)
        , m_internalTimeout(2000)
        , m_isAdvancedMode(false)
        , m_topologyInitialized(false)
        , m_getOlder(false) // Sic! To start aging in setAgeing below.
        , m_runSignalsChangedThread(false)
        , m_signalsChangedInterval(-1)
        , m_loggerMapCached(false) {

            std::string ownInstanceId = generateOwnInstanceId();
            m_internalSignalSlotable = karabo::xms::SignalSlotable::Pointer(new SignalSlotable(ownInstanceId, brokerType, brokerConfiguration));
            m_internalSignalSlotable->setNumberOfThreads(2);
            m_signalSlotable = m_internalSignalSlotable;
            Hash instanceInfo;
            instanceInfo.set("type", "client");
            instanceInfo.set("lang", "c++");
            instanceInfo.set("visibility", 4);
            instanceInfo.set("compatibility", DeviceClient::classInfo().getVersion());
            instanceInfo.set("host", net::bareHostName());
            instanceInfo.set("status", "ok");

            m_eventThread = boost::thread(boost::bind(&karabo::xms::SignalSlotable::runEventLoop, m_internalSignalSlotable, 60, instanceInfo));
            boost::this_thread::sleep(boost::posix_time::milliseconds(100));
            bool ok = m_internalSignalSlotable->ensureOwnInstanceIdUnique();
            if (!ok) {
                m_eventThread.join(); // Blocks
                return;
            }

            this->setAgeing(true);
            this->setupSlots();
        }


        DeviceClient::DeviceClient(const boost::shared_ptr<SignalSlotable>& signalSlotable)
        : m_internalSignalSlotable()
        , m_signalSlotable(signalSlotable)
        , m_isShared(true)
        , m_internalTimeout(2000)
        , m_isAdvancedMode(false)
        , m_topologyInitialized(false)
        , m_getOlder(false) // Sic! To start aging in setAgeing below.
        , m_runSignalsChangedThread(false)
        , m_signalsChangedInterval(-1)
        , m_loggerMapCached(false) {

            this->setAgeing(true);
            this->setupSlots();
        }


        DeviceClient::~DeviceClient() {
            // Stop aging thread
            setAgeing(false); // Joins the thread
            KARABO_LOG_FRAMEWORK_TRACE << "DeviceClient::~DeviceClient() : age thread is joined";
            // Stop thread sending the collected signal(State)Changed
            setDeviceMonitorInterval(-1);

            if (!m_isShared) {
                if (m_internalSignalSlotable) {
                    m_internalSignalSlotable->stopEventLoop();
                    m_eventThread.join();
                }
            }
            m_internalSignalSlotable.reset();
        }


#define KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN(...) if (m_signalSlotable.expired()) { \
                    KARABO_LOG_FRAMEWORK_WARN << "SignalSlotable object is not valid (destroyed)."; \
                    return __VA_ARGS__; }


        void DeviceClient::setupSlots() {
            karabo::xms::SignalSlotable::Pointer p = m_signalSlotable.lock();
            p->registerSlot<Hash, string > (boost::bind(&karabo::core::DeviceClient::_slotChanged, this, _1, _2), "_slotChanged");
            p->registerSlot<Schema, string, string > (boost::bind(&karabo::core::DeviceClient::_slotClassSchema, this, _1, _2, _3), "_slotClassSchema");
            p->registerSlot<Schema, string > (boost::bind(&karabo::core::DeviceClient::_slotSchemaUpdated, this, _1, _2), "_slotSchemaUpdated");
            p->registerSlot<string, Hash > (boost::bind(&karabo::core::DeviceClient::_slotInstanceNew, this, _1, _2), "_slotInstanceNew");
            p->registerSlot<string, Hash > (boost::bind(&karabo::core::DeviceClient::_slotInstanceGone, this, _1, _2), "_slotInstanceGone");
            p->registerSlot<string, Hash > (boost::bind(&karabo::core::DeviceClient::slotInstanceUpdated, this, _1, _2), "slotInstanceUpdated");
            p->registerSlot<Hash > (boost::bind(&karabo::core::DeviceClient::_slotLoggerMap, this, _1), "_slotLoggerMap");
            p->connect("", "signalInstanceNew", "", "_slotInstanceNew");
            p->connect("", "signalInstanceGone", "", "_slotInstanceGone");
        }


        void DeviceClient::cacheAvailableInstances() {
            m_signalSlotable.lock()->getAvailableInstances(true); // Boolean has no effect currently
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


        std::string DeviceClient::findInstance(const std::string &instanceId) const {
            // NOT: boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
            //      As documented, that is callers responsibility.
            for (Hash::const_iterator it = m_runtimeSystemDescription.begin(); it != m_runtimeSystemDescription.end(); ++it) {
                Hash& tmp = it->getValue<Hash>();
                boost::optional<Hash::Node&> node = tmp.find(instanceId);
                if (node) {
                    return string(it->getKey() + "." + instanceId);
                }
            }
            return string();
        }


        std::string DeviceClient::findInstanceSafe(const std::string &instanceId) const {
            boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
            return this->findInstance(instanceId);
        }


        void DeviceClient::mergeIntoRuntimeSystemDescription(const karabo::util::Hash& entry) {
            boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
            m_runtimeSystemDescription.merge(entry);
        }


        bool DeviceClient::existsInRuntimeSystemDescription(const std::string& path) const {
            boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
            return m_runtimeSystemDescription.has(path);
        }


        void DeviceClient::_slotInstanceNew(const std::string& instanceId, const karabo::util::Hash& instanceInfo) {
            KARABO_LOG_FRAMEWORK_DEBUG << "slotInstanceNew was called for: " << instanceId;

            Hash entry = prepareTopologyEntry(instanceId, instanceInfo);
            mergeIntoRuntimeSystemDescription(entry);

            if (m_instanceNewHandler) m_instanceNewHandler(entry);
        }


        void DeviceClient::eraseFromRuntimeSystemDescription(const std::string& path) {
            try {
                boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
                m_runtimeSystemDescription.erase(path);
            } catch (...) {
                KARABO_LOG_FRAMEWORK_ERROR << "Could not erase path \"" << path << " from device-client cache";
            }
        }

        util::Hash DeviceClient::getSectionFromRuntimeDescription(const std::string& section) const
        {
            boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);

            boost::optional<const util::Hash::Node&> sectionNode = m_runtimeSystemDescription.find(section);
            if (sectionNode && sectionNode->is<util::Hash>()) {
                return sectionNode->getValue<util::Hash>();
            } else {
                return util::Hash();
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


        void DeviceClient::_slotInstanceGone(const std::string& instanceId, const karabo::util::Hash& instanceInfo) {
            try {

                const string path(prepareTopologyPath(instanceId, instanceInfo));
                if (!existsInRuntimeSystemDescription(path)) return;

                eraseFromRuntimeSystemDescription(path);
                eraseFromInstanceUsage(instanceId);
                if (m_instanceGoneHandler) m_instanceGoneHandler(instanceId, instanceInfo);

                if (getInstanceType(instanceInfo) != "server") return;

                // It is a server, so treat also all its devices as dead.
                const Hash deviceSection(getSectionFromRuntimeDescription("device"));

                for (Hash::const_iterator it = deviceSection.begin(); it != deviceSection.end(); ++it) {
                    const Hash::Attributes& attributes = it->getAttributes();
                    if (attributes.has("serverId") && attributes.get<string>("serverId") == instanceId) {
                        // OK, device belongs to the server that is gone.
                        const string& deviceId = it->getKey();
                        Hash deviceInstanceInfo;
                        for (Hash::Attributes::const_iterator jt = attributes.begin(); jt != attributes.end(); ++jt) {
                            deviceInstanceInfo.set(jt->getKey(), jt->getValueAsAny());
                        }
                        // Call the slot of our SignalSlotable to deregister the device.
                        // This will erase it from the tracked list and brings us back into this method.
                        xms::SignalSlotable::Pointer p(m_signalSlotable.lock());
                        if (p) p->call("", "slotInstanceGone", deviceId, deviceInstanceInfo);
                    }
                }
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in _slotInstanceGone: " << e;
            } catch (...) {
                KARABO_LOG_FRAMEWORK_ERROR << "Unknown exception thrown in _slotInstanceGone";
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
                if (m_ageingThread.joinable())
                    m_ageingThread.join();
            }
        }


        void DeviceClient::setDeviceMonitorInterval(long int milliseconds) {
            if (milliseconds >= 0) {
                m_signalsChangedInterval = boost::posix_time::milliseconds(milliseconds);
                if (!m_runSignalsChangedThread) {
                    // Extra protection: If a previous thread is not yet finished,
                    //                   wait until it is before restarting.
                    if (m_signalsChangedThread.joinable()) {
                        m_signalsChangedThread.join();
                    }
                    m_runSignalsChangedThread = true;
                    m_signalsChangedThread = boost::thread(boost::bind(&karabo::core::DeviceClient::sendSignalsChanged, this));
                }
            } else if (m_runSignalsChangedThread) {
                m_runSignalsChangedThread = false;
                if (m_signalsChangedThread.joinable()) {
                    m_signalsChangedThread.join();
                }
            }
        }


        std::pair<bool, std::string> DeviceClient::exists(const std::string& instanceId) {
            if (m_signalSlotable.expired())
                return std::make_pair<bool, std::string > (false, "SignalSlotable object is not valid (destroyed).");
            return m_signalSlotable.lock()->exists(instanceId);
        }


        void DeviceClient::initTopology() {
            if (!m_topologyInitialized) {
                this->cacheAvailableInstances();
                m_topologyInitialized = true;
            }
        }


        Hash DeviceClient::getSystemInformation() {
            KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN(Hash());
            initTopology();
            boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
            return m_runtimeSystemDescription;
        }


        Hash DeviceClient::getSystemTopology() {
            KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN(Hash());
            initTopology();
            boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
            Hash topology;
            for (Hash::const_map_iterator it = m_runtimeSystemDescription.mbegin(); it != m_runtimeSystemDescription.mend(); ++it) {
                const std::string& categoryName = it->first;
                const Hash& category = it->second.getValue<Hash>();
                Hash& entry = topology.bindReference<Hash>(categoryName);
                for (Hash::const_map_iterator jt = category.mbegin(); jt != category.mend(); ++jt) {
                    Hash::Node& node = entry.set(jt->first, Hash());
                    node.setAttributes(jt->second.getAttributes());
                }
            }
            return topology;
        }


        std::vector<std::string> DeviceClient::getServers() {
            KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN(vector<string>());
            initTopology();
            boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
            if (m_runtimeSystemDescription.has("server")) {
                const Hash& tmp = m_runtimeSystemDescription.get<Hash>("server");
                vector<string> deviceServers;
                deviceServers.reserve(tmp.size());
                karabo::xms::SignalSlotable::Pointer p = m_signalSlotable.lock();
                for (Hash::const_map_iterator it = tmp.mbegin(); it != tmp.mend(); ++it) {
                    if (it->second.hasAttribute("visibility")) {
                        if (p->getAccessLevel(it->second.getKey()) < it->second.getAttribute<int>("visibility")) continue;
                    }
                    deviceServers.push_back(it->second.getKey());
                }
                return deviceServers;
            } else {
                KARABO_LOG_FRAMEWORK_INFO << "No device servers found in the system";
                return vector<string>();
            }
        }


        std::vector<std::string> DeviceClient::getClasses(const std::string& deviceServer) {
            KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN(std::vector<std::string>());
            initTopology();
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
            KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN(vector<string>());
            initTopology();
            karabo::xms::SignalSlotable::Pointer p = m_signalSlotable.lock();

            boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
            if (!m_runtimeSystemDescription.has("device")) {
                return vector<string>();
            } else {
                const Hash& tmp = m_runtimeSystemDescription.get<Hash>("device");
                vector<string> devices;
                devices.reserve(tmp.size());
                for (Hash::const_map_iterator it = tmp.mbegin(); it != tmp.mend(); ++it) {
                    if (it->second.hasAttribute("visibility")) {
                        if (p->getAccessLevel(it->second.getKey()) < it->second.getAttribute<int>("visibility")) continue;
                    }
                    devices.push_back(it->second.getKey());
                }
                return devices;
            }
        }


        std::vector<std::string> DeviceClient::getDevices(const std::string& deviceServer) {
            KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN(vector<string>());
            initTopology();
            karabo::xms::SignalSlotable::Pointer p = m_signalSlotable.lock();

            boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
            if (!m_runtimeSystemDescription.has("device")) {
                return vector<string>();
            } else {
                const Hash& tmp = m_runtimeSystemDescription.get<Hash>("device");
                vector<string> devices;
                devices.reserve(tmp.size());
                for (Hash::const_map_iterator it = tmp.mbegin(); it != tmp.mend(); ++it) {
                    if (it->second.getAttribute<string>("serverId") == deviceServer) {
                        if (it->second.hasAttribute("visibility")) {
                            if (p->getAccessLevel(it->second.getKey()) < it->second.getAttribute<int>("visibility")) continue;
                        }
                        devices.push_back(it->second.getKey());
                    }
                }
                return devices;
            }
        }


        karabo::util::Schema DeviceClient::getDeviceSchema(const std::string& instanceId) {
            return cacheAndGetDeviceSchema(instanceId);
        }


        karabo::util::Schema DeviceClient::cacheAndGetDeviceSchema(const std::string & instanceId) {
            KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN(Schema());

            karabo::xms::SignalSlotable::Pointer p = m_signalSlotable.lock();
            
            std::string path;

            {
                boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
                path = findInstance(instanceId);
                if (path.empty()) {
                    path = "device." + instanceId + ".fullSchema";
                } else {
                    path += ".fullSchema";
                    boost::optional<Hash::Node&> node = m_runtimeSystemDescription.find(path);
                    if (node) return node->getValue<Schema>();
                }
            }
            
            // Not found, request and cache it
            // Request schema
            Schema schema;
            try {
                p->request(instanceId, "slotGetSchema", false).timeout(m_internalTimeout).receive(schema); // Retrieves full schema
            } catch (const TimeoutException&) {
                KARABO_LOG_FRAMEWORK_ERROR << "Schema request for instance \"" << instanceId << "\" timed out";
                Exception::clearTrace();
                return Schema();
            }
            boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
            return m_runtimeSystemDescription.set(path, schema).getValue<Schema>();
        }


        karabo::util::Schema DeviceClient::getDeviceSchemaNoWait(const std::string& instanceId) {
            KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN(Schema());
            {
                boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
                std::string path(findInstance(instanceId));
                if (!path.empty()) {
                    path += ".fullSchema";
                    boost::optional<Hash::Node&> node = m_runtimeSystemDescription.find(path);
                    if (node && !node->getValue<Schema>().empty()) return node->getValue<Schema>();
                }
            }

            m_signalSlotable.lock()->requestNoWait(instanceId, "slotGetSchema", "", "_slotSchemaUpdated", false);
            return Schema();
        }


        void DeviceClient::_slotSchemaUpdated(const karabo::util::Schema& schema, const std::string& deviceId) {    
            KARABO_LOG_FRAMEWORK_DEBUG << "_slotSchemaUpdated";
            {
                boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
                string path(findInstance(deviceId));
                if (path.empty()) {
                    KARABO_LOG_FRAMEWORK_WARN << "got schema for unknown instance '" << deviceId << "'.";
                    return;
                }
                m_runtimeSystemDescription.set(path + ".fullSchema", schema);

                path += ".activeSchema";
                if (m_runtimeSystemDescription.has(path)) m_runtimeSystemDescription.erase(path);
            }
            if (m_schemaUpdatedHandler) m_schemaUpdatedHandler(deviceId, schema);
        }


        karabo::util::Schema DeviceClient::getActiveSchema(const std::string& instanceId) {
            return cacheAndGetActiveSchema(instanceId);
        }


        karabo::util::Schema DeviceClient::cacheAndGetActiveSchema(const std::string& instanceId) {
            KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN(Schema());
            std::string state = this->get<std::string > (instanceId, "state");
            std::string path;
            {
                boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
                path = findInstance(instanceId);
                if (path.empty()) {
                    path = "device." + instanceId + ".activeSchema." + state;
                } else {
                    path += ".activeSchema." + state;
                    boost::optional<Hash::Node&> node = m_runtimeSystemDescription.find(path);
                    if (node) return node->getValue<Schema>();
                }
            }
            // Not found, request and cache it
            // Request schema
            Schema schema;
            try {
                m_signalSlotable.lock()->request(instanceId, "slotGetSchema", true).timeout(m_internalTimeout).receive(schema); // Retrieves active schema
            } catch (const TimeoutException&) {
                KARABO_LOG_FRAMEWORK_ERROR << "Schema request for instance \"" << instanceId << "\" timed out";
                Exception::clearTrace();
                return Schema();
            }
            boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);    
            return m_runtimeSystemDescription.set(path, schema).getValue<Schema>();
        }


        karabo::util::Schema DeviceClient::getClassSchema(const std::string& serverId, const std::string& classId) {
            return cacheAndGetClassSchema(serverId, classId);
        }


        karabo::util::Schema DeviceClient::cacheAndGetClassSchema(const std::string& serverId, const std::string& classId) {
            KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN(Schema());
            std::string path("server." + serverId + ".classes." + classId + ".description");
            {
                boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
                boost::optional<Hash::Node&> node = m_runtimeSystemDescription.find(path);
                if (node) return node->getValue<Schema>();
            }   
            // Not found, request and cache it
            // Request schema
            Schema schema;
            try {
                m_signalSlotable.lock()->request(serverId, "slotGetClassSchema", classId).timeout(m_internalTimeout).receive(schema); // Retrieves full schema
            } catch (const TimeoutException&) {
                KARABO_LOG_FRAMEWORK_ERROR << "Schema request for server \"" << serverId << "\" timed out";
                Exception::clearTrace();
                return Schema();
            }
            
            boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
            return m_runtimeSystemDescription.set(path, schema).getValue<Schema>();
        }


        karabo::util::Schema DeviceClient::getClassSchemaNoWait(const std::string& serverId, const std::string& classId) {
            KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN(Schema());
            
            {
                std::string path("server." + serverId + ".classes." + classId + ".description");
                boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
                boost::optional<Hash::Node&> node = m_runtimeSystemDescription.find(path);
                if (node && !node->getValue<Schema>().empty()) return node->getValue<Schema>();
            }
            // Not found, request and cache it
            // Request schema                
            m_signalSlotable.lock()->requestNoWait(serverId, "slotGetClassSchema", "", "_slotClassSchema", classId);
            return Schema();
        }


        void DeviceClient::_slotClassSchema(const karabo::util::Schema& schema, const std::string& classId, const std::string& serverId) {
            KARABO_LOG_FRAMEWORK_DEBUG << "_slotClassSchema";
            {
                std::string path("server." + serverId + ".classes." + classId + ".description");
                boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
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
            KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN(vector<string>());
            Schema schema = cacheAndGetActiveSchema(deviceId);
            int accessLevel = m_signalSlotable.lock()->getAccessLevel(deviceId);
            return filterProperties(schema, accessLevel);
        }


        std::vector<std::string> DeviceClient::getProperties(const std::string& deviceId) {
            KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN(vector<string>());
            Schema schema = cacheAndGetDeviceSchema(deviceId);
            int accessLevel = m_signalSlotable.lock()->getAccessLevel(deviceId);
            return filterProperties(schema, accessLevel);
        }


        std::vector<std::string> DeviceClient::getClassProperties(const std::string& serverId, const std::string& classId) {
            KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN(vector<string>());
            Schema schema = cacheAndGetClassSchema(serverId, classId);
            int accessLevel = m_signalSlotable.lock()->getAccessLevel(classId);
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
            KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN();
            Hash tmp(classId, configuration);
            m_signalSlotable.lock()->call(serverInstanceId, "slotStartDevice", tmp);
        }


        void DeviceClient::instantiateNoWait(const std::string& serverInstanceId, const karabo::util::Hash& completeConfiguration) {
            KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN();
            m_signalSlotable.lock()->call(serverInstanceId, "slotStartDevice", completeConfiguration);
        }


        std::pair<bool, std::string > DeviceClient::instantiate(const std::string& serverInstanceId, const std::string& classId, const karabo::util::Hash& configuration, int timeoutInSeconds) {
            Hash tmp(classId, configuration);
            return this->instantiate(serverInstanceId, tmp, timeoutInSeconds);
        }


        std::pair<bool, std::string > DeviceClient::instantiate(const std::string& serverInstanceId, const karabo::util::Hash& configuration, int timeoutInSeconds) {
            if (m_signalSlotable.expired()) {
                return std::make_pair(false, "SignalSlotable object is not valid (destroyed).");
            }
            if (timeoutInSeconds == -1) timeoutInSeconds = 5;
            bool ok = true;
            std::string reply = "";
            try {
                m_signalSlotable.lock()->request(serverInstanceId, "slotStartDevice", configuration).timeout(timeoutInSeconds * 1000).receive(ok, reply);
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
            KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN();
            m_signalSlotable.lock()->call(deviceId, "slotKillDevice");
        }


        std::pair<bool, std::string> DeviceClient::killDevice(const std::string& deviceId, int timeoutInSeconds) {
            if (m_signalSlotable.expired()) {
                return std::make_pair(false, "SignalSlotable object is not valid (destroyed).");
            }
            // TODO Do not hard code the default timeout
            if (timeoutInSeconds == -1) timeoutInSeconds = 30;
            // Give it a kill signal
            m_signalSlotable.lock()->call(deviceId, "slotKillDevice");
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
            if (m_signalSlotable.expired()) {
                return std::make_pair(false, "SignalSlotable object is not valid (destroyed).");
            }
            bool ok = true;
            string reply;
            // TODO Do not hard code the default timeout
            if (timeoutInSeconds == -1) timeoutInSeconds = 30;
            try {
                // TODO Add error text to response
                m_signalSlotable.lock()->request(serverId, "slotKillServer").timeout(timeoutInSeconds * 1000).receive(reply);
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
            KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN();
            m_signalSlotable.lock()->call(serverId, "slotKillServer");
        }


        karabo::util::Hash DeviceClient::get(const std::string & instanceId) {
            return cacheAndGetConfiguration(instanceId);
        }


        karabo::util::Hash DeviceClient::cacheAndGetConfiguration(const std::string& deviceId) {
            KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN(Hash());
            Hash result;
            std::string path;
            {
                boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
                path = findInstance(deviceId);
                
                if (path.empty()) {
                    path = "device." + deviceId + ".configuration";
                } else {
                    path += ".configuration";
                    boost::optional<Hash::Node&> node = m_runtimeSystemDescription.find(path);
                    if (node) result = node->getValue<Hash>();
                }
            }
                
            if (result.empty()) { // Not found, request and cache
                // Request configuration
                Hash hash;
                try {
                    m_signalSlotable.lock()->request(deviceId, "slotGetConfiguration").timeout(m_internalTimeout).receive(hash);
                } catch (const TimeoutException&) {
                    KARABO_RETHROW_AS(KARABO_TIMEOUT_EXCEPTION("Configuration request for device \"" + deviceId + "\" timed out"));
                    return result;  // empty Hash
                }
                boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
                result = m_runtimeSystemDescription.set(path, hash).getValue<Hash>();
            }
            stayConnected(deviceId);
            return result;
        }


        void DeviceClient::get(const std::string& instanceId, karabo::util::Hash& hash) {
            hash = cacheAndGetConfiguration(instanceId);
        }


        karabo::util::Hash DeviceClient::getConfigurationNoWait(const std::string& deviceId) {
            KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN(Hash());
            {
                boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
                std::string path(findInstance(deviceId));
                if (!path.empty()) {
                    path += ".configuration";
                    boost::optional<Hash::Node&> node = m_runtimeSystemDescription.find(path);
                    if (node && !node->getValue<Hash>().empty())
                        return node->getValue<Hash>();
                }
            }
            
            // "stayConnected" is expensive under the high load due to contention on its mutex
            stayConnected(deviceId);
            
            m_signalSlotable.lock()->requestNoWait(deviceId, "slotGetConfiguration", "", "_slotChanged");
            return Hash();
        }


        bool DeviceClient::cacheLoggerMap(bool toggle) {
            if (toggle == m_loggerMapCached) return true;

            karabo::xms::SignalSlotable::Pointer p = m_signalSlotable.lock();
            if (!p) {
                KARABO_LOG_FRAMEWORK_WARN << "SignalSlotable object is not valid (destroyed).";
                return false;
            } else if (toggle) {
                // connect and request a first time
                if (p->connect(core::DATALOGMANAGER_ID, "signalLoggerMap", "", "_slotLoggerMap")) {
                    // If we cannot connect, request makes no sense
                    Hash loggerMap;
                    try {
                        p->request(core::DATALOGMANAGER_ID, "slotGetLoggerMap").timeout(m_internalTimeout).receive(loggerMap);
                        // Next 3 lines would better fit in an else block as in Python's try-except-else...
                        boost::mutex::scoped_lock lock(m_loggerMapMutex);
                        m_loggerMap = loggerMap;
                        m_loggerMapCached = true;
                        return true;
                    } catch (const TimeoutException&) {
                        return false;
                    }
                } else {
                    KARABO_LOG_FRAMEWORK_WARN << "Failed to connect _slotLoggerMap";
                    return false;
                }
            } else {
                m_loggerMapCached = false;
                // disconnect and clear (since otherwise possibly wrong info)
                if (!p->disconnect(core::DATALOGMANAGER_ID, "signalLoggerMap", "", "_slotLoggerMap", false)) {
                    KARABO_LOG_FRAMEWORK_WARN << "Failed to disconnect _slotLoggerMap";
                    return false;
                }
                boost::mutex::scoped_lock lock(m_loggerMapMutex);
                m_loggerMap.clear();
                return true;
            }
        }



        void DeviceClient::_slotLoggerMap(const karabo::util::Hash& loggerMap) {
            KARABO_LOG_FRAMEWORK_INFO << "DeviceClient::_slotLoggerMap called";
            boost::mutex::scoped_lock lock(m_loggerMapMutex);
            m_loggerMap = loggerMap;
        }


        std::vector<karabo::util::Hash> DeviceClient::getFromPast(const std::string& deviceId, const std::string& key, const std::string& from, std::string to, int maxNumData) {
            return getPropertyHistory(deviceId, key, from, to, maxNumData);
        }


        std::vector<karabo::util::Hash> DeviceClient::getPropertyHistory(const std::string& deviceId, const std::string& property, const std::string& from, std::string to, int maxNumData) {
            karabo::xms::SignalSlotable::Pointer p = m_signalSlotable.lock();
            if (!p) {
                KARABO_LOG_FRAMEWORK_WARN << "SignalSlotable object is not valid (destroyed).";
                return vector<Hash>();
            }
            if (to.empty()) to = karabo::util::Epochstamp().toIso8601();

            const std::string dataLogReader(this->getDataLogReader(deviceId));
            std::vector<Hash> result;
            std::string dummy1, dummy2; // deviceId and property (as our input - relevant for receiveAsync)
            const Hash args("from", from, "to", to, "maxNumData", maxNumData);

            try {
                // Increasing timeout since getting history may take a while...
                p->request(dataLogReader, "slotGetPropertyHistory", deviceId, property, args)
                        .timeout(10 * m_internalTimeout).receive(dummy1, dummy2, result);
            } catch (const TimeoutException&) {
                KARABO_LOG_FRAMEWORK_ERROR << "Request to DataLogReader '" << dataLogReader
                        << "' timed out for device.property '" << deviceId << "." << property << "'.";
            }
            return result;
        }


        std::string DeviceClient::getDataLogReader(const std::string& deviceId) {
            std::string dataLogReader; // the result

            // Try to get server - 1st try from map:
            std::string dataLogServer;
            const std::string loggerId(core::DATALOGGER_PREFIX + deviceId);
            if (m_loggerMapCached) {
                boost::mutex::scoped_lock lock(m_loggerMapMutex);
                if (m_loggerMap.has(loggerId)) {
                    dataLogServer = m_loggerMap.get<std::string>(loggerId);
                } // else: empty loggerMap, i.e. not tracked, or non-existing/non-logged device
            }
            // 2nd try: request map from log manager:
            if (dataLogServer.empty()) {
                karabo::xms::SignalSlotable::Pointer p = m_signalSlotable.lock();
                if (p) {
                    Hash localLogMap; // to become map with key=loggerId, value=server
                    try {
                        p->request(core::DATALOGMANAGER_ID, "slotGetLoggerMap").timeout(m_internalTimeout).receive(localLogMap);
                    } catch (const TimeoutException&) {
                        // Will fail below due to empty map...
                    }
                    if (localLogMap.has(loggerId)) {
                        dataLogServer = localLogMap.get<std::string>(loggerId);
                    }
                } else {
                    KARABO_LOG_FRAMEWORK_ERROR << "SignalSlotable object is not valid (destroyed).";
                }
            }

            if (dataLogServer.empty()) {
                KARABO_LOG_FRAMEWORK_ERROR << "Failed to find data log reader for logger '" << loggerId << "'";
            } else {
                // Assemble the instanceId of a log reader
                static int i = 0; // just choose an 'arbitrary' reader
                (dataLogReader += core::DATALOGREADER_PREFIX) += toString(i++ % core::DATALOGREADERS_PER_SERVER)
                        += "-" + dataLogServer;
            }

            return dataLogReader;
        }


        std::pair<karabo::util::Hash, karabo::util::Schema> DeviceClient::getConfigurationFromPast(const std::string& deviceId, const std::string& timepoint) {
            KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN(make_pair<Hash, Schema>(Hash(), Schema()));
            Hash hash;
            Schema schema;
            m_signalSlotable.lock()->request(core::DATALOGMANAGER_ID, "slotGetConfigurationFromPast", deviceId, timepoint)
                    .timeout(60000).receive(hash, schema);
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
            KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN();

            m_schemaUpdatedHandler = callBackFunction;
        }


        void DeviceClient::registerClassSchemaMonitor(const ClassSchemaHandler& callBackFunction) {
            KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN();

            m_classSchemaHandler = callBackFunction;
        }


        void DeviceClient::registerDeviceMonitor(const std::string& deviceId, const boost::function<void (const std::string& /*deviceId*/, const karabo::util::Hash& /*config*/)> & callbackFunction) {
            KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN();
            stayConnected(deviceId);
            {
                boost::mutex::scoped_lock lock(m_deviceChangedHandlersMutex);
                m_deviceChangedHandlers.set(deviceId + "._function", callbackFunction);
            }
            m_signalSlotable.lock()->requestNoWait(deviceId, "slotGetConfiguration", "", "_slotChanged");
            immortalize(deviceId);
        }


        void DeviceClient::unregisterPropertyMonitor(const std::string& instanceId, const std::string& key) {
            bool isMortal = false;
            {
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
                        isMortal = true;
                    }
                }
            }
            if (isMortal) mortalize(instanceId);
        }


        void DeviceClient::unregisterDeviceMonitor(const std::string& instanceId) {
            {
                boost::mutex::scoped_lock lock(m_deviceChangedHandlersMutex);
                if (m_deviceChangedHandlers.has(instanceId)) m_deviceChangedHandlers.erase(instanceId);
                // What about cleaning cache
            }
            mortalize(instanceId);
        }


        std::pair<bool, std::string > DeviceClient::set(const std::string& instanceId, const karabo::util::Hash& values, int timeoutInSeconds) {
            KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN(std::make_pair<bool, string > (false, "Fail to set"));

            //stayConnected(instanceId);

            // If this is the first time we are going to talk to <instanceId>, we should get all configuration,
            // else only the answer to our set will fill the cache.            
            cacheAndGetConfiguration(instanceId);

            // TODO Do not hardcode the default timeout
            if (timeoutInSeconds == -1) timeoutInSeconds = 3;
            bool ok = true;
            std::string errorText = "";

            try {
                // Validate locally with custom validator
                Hash validated;
                Schema schema = cacheAndGetActiveSchema(instanceId);
                Validator::ValidationRules rules(/*injectDefaults=*/false,
                                                 /*allowUnrootedConfiguration=*/true,
                                                 /*allowAdditionalKeys=*/false,
                                                 /*allowMissingKeys=*/true,
                                                 /*injectTimestamps*/false);
                Validator validator(rules);
                std::pair<bool, std::string> result = validator.validate(schema, values, validated);
                if (!result.first) return result;
                // TODO Add error text to response
                m_signalSlotable.lock()->request(instanceId, "slotReconfigure", validated).timeout(timeoutInSeconds * 1000).receive(ok, errorText);
            } catch (const karabo::util::Exception& e) {
                errorText = e.userFriendlyMsg();
                ok = false;
            }

            return std::make_pair(ok, errorText);
        }


        void DeviceClient::setNoWait(const std::string& instanceId, const karabo::util::Hash& values) {
            KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN();
            //stayConnected(instanceId);
            m_signalSlotable.lock()->call(instanceId, "slotReconfigure", values);
        }


        std::string DeviceClient::generateOwnInstanceId() {
            return std::string(net::bareHostName() + "_DeviceClient_" + karabo::util::toString(getpid()));
        }


        bool DeviceClient::connectNeeded(const std::string & instanceId) {
            boost::mutex::scoped_lock lock(m_instanceUsageMutex);
            InstanceUsage::iterator it = m_instanceUsage.find(instanceId);
            if (it == m_instanceUsage.end()) {
                m_instanceUsage[instanceId] = 0;
                return true;
            }
            
            bool result = (it->second >= CONNECTION_KEEP_ALIVE);
            it->second = 0;    // reset the counter
            return result;
        }


        void DeviceClient::stayConnected(const std::string & instanceId) {
            KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN();
            karabo::xms::SignalSlotable::Pointer p = m_signalSlotable.lock();
            if (connectNeeded(instanceId)) { // Not there yet
                p->connect(instanceId, "signalChanged", "", "_slotChanged");
                p->connect(instanceId, "signalStateChanged", "", "_slotChanged");
                p->connect(instanceId, "signalSchemaUpdated", "", "_slotSchemaUpdated");
            }
        }


        void DeviceClient::eraseFromInstanceUsage(const std::string& instanceId) {
            boost::mutex::scoped_lock lock(m_instanceUsageMutex);
            m_instanceUsage.erase(instanceId);
        }


        void DeviceClient::_slotChanged(const karabo::util::Hash& hash, const std::string & instanceId) {
            {
                boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
                // TODO Optimize speed
                string path(findInstance(instanceId));
                if (path.empty()) {
                    path = "device." + instanceId + ".configuration";
                    KARABO_LOG_FRAMEWORK_DEBUG << "_slotChanged created '" << path << "' for" << hash;
                } else {
                    path += ".configuration";
                }
                if (m_runtimeSystemDescription.has(path)) {
                    Hash& tmp = m_runtimeSystemDescription.get<Hash>(path);
                    tmp.merge(hash);
                } else {
                    m_runtimeSystemDescription.set(path, hash);
                }
            }
            // NOTE: This will block us here, i.e. we are deaf for other changes...
            // NOTE: Monitors could be implemented as additional slots or in separate threads, too.
            notifyPropertyChangedMonitors(hash, instanceId);
            if (m_runSignalsChangedThread) {
                boost::mutex::scoped_lock lock(m_signalsChangedMutex);
                // Just book keep paths here and call 'notifyDeviceChangedMonitors'
                // later with content from m_runtimeSystemDescription.
                hash.getPaths(m_signalsChanged[instanceId]);
            } else {
                // There is a tiny (!) risk here: The last loop of the corresponding thread
                // might still be running and _later_ call 'notifyDeviceChangedMonitors'
                // with an older value...
                notifyDeviceChangedMonitors(hash, instanceId);
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
            m_signalSlotable.lock()->reply(m_signalSlotable.lock()->getInstanceId());
        }


        void DeviceClient::slotProvideSystemTopology() {
            karabo::xms::SignalSlotable::Pointer p = m_signalSlotable.lock();
            if (!p) {
                KARABO_LOG_FRAMEWORK_INFO << "Fail to reply because broker connection was expired.";
                return;
            }
            p->reply(getSystemTopology());
        }


        void DeviceClient::age() {
            try {
                while (m_getOlder) { // Loop forever
                    vector<string> forDisconnect;
                    {
                        boost::mutex::scoped_lock lock(m_instanceUsageMutex);
                        // Loop connected instances
                        for (InstanceUsage::iterator it = m_instanceUsage.begin(); it != m_instanceUsage.end(); /*NOT ++it*/) {

                            it->second++; // All just age (but some do not die).
                            if (!this->isImmortal(it->first) // registered monitors are immortal
                                    && it->second >= CONNECTION_KEEP_ALIVE) {
                                forDisconnect.push_back(it->first);   // we do this to reduce mutex locking time
                                m_instanceUsage.erase(it++);
                            } else {
                                ++it;
                            }
                        }
                    }

                    if (forDisconnect.size()) {
                        karabo::xms::SignalSlotable::Pointer p = m_signalSlotable.lock();
                        if (p) {
                            for (size_t i = 0; i < forDisconnect.size(); ++i) {
                                const string& instanceId = forDisconnect[i];
                                KARABO_LOG_FRAMEWORK_DEBUG << "Disconnect '" << instanceId << "'.";

                                p->disconnect(instanceId, "signalChanged", "", "_slotChanged", false);
                                p->disconnect(instanceId, "signalStateChanged", "", "_slotChanged", false);
                                p->disconnect(instanceId, "signalSchemaUpdated", "", "_slotSchemaUpdated", false);

                                const std::string path("device." + instanceId + ".configuration");
                                // Since we stopped listening, remove configuration from system description.
                                this->eraseFromRuntimeSystemDescription(path);
                            }
                        }
                    }
                    boost::this_thread::sleep(boost::posix_time::seconds(1));
                }
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Aging thread encountered an exception: " << e;
                // Aging is essential, so go on. Wait a little in case of repeating error conditions.
                boost::this_thread::sleep(boost::posix_time::seconds(5));
                this->age();
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Aging thread encountered system exception: " << e.what();
                boost::this_thread::sleep(boost::posix_time::seconds(5));
                this->age();
            } catch (...) {
                KARABO_LOG_FRAMEWORK_ERROR << "Unknown exception encountered in aging thread";
                boost::this_thread::sleep(boost::posix_time::seconds(5));
                this->age();
            }
        }

        void DeviceClient::sendSignalsChanged() {
            while (m_runSignalsChangedThread) { // Loop forever
                try {
                    // Get map of all properties that changed (and clear original)
                    SignalChangedMap localChanged;
                    {
                        boost::mutex::scoped_lock lock(m_signalsChangedMutex);
                        m_signalsChanged.swap(localChanged);
                    }
                    this->doSendSignalsChanged(localChanged);
                } catch (const Exception& e) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Exception encountered in 'sendSignalsChanged': " << e;
                } catch (const std::exception& e) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Exception encountered in 'sendSignalsChanged': " << e.what();
                } catch (...) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Unknown exception encountered in 'sendSignalsChanged'";
                }
                boost::this_thread::sleep(m_signalsChangedInterval);
            }
            // Just in case anything was added before 'm_runSignalsChangedThread' was set to false
            // and while we processed the previous content (keep lock until done completely):
            try {
                boost::mutex::scoped_lock lock(m_signalsChangedMutex);
                this->doSendSignalsChanged(m_signalsChanged);
                m_signalsChanged.clear();
            } catch (...) { // lazy to catch all levels - we are anyway done with the thread...
                KARABO_LOG_FRAMEWORK_ERROR << "Exception encountered when leaving 'sendSignalsChanged'";
            }
        }

        void DeviceClient::doSendSignalsChanged(const SignalChangedMap& localChanged) {
            // Iterate on devices (i.e. keys in map)
            for (SignalChangedMap::const_iterator mapIt = localChanged.begin(), mapEnd = localChanged.end();
                    mapIt != mapEnd; ++mapIt) {
                const std::string& instanceId = mapIt->first;
                const std::set<std::string>& properties = mapIt->second;
                // Get path of instance in runtime system description and then its configuration
                const std::string path(this->findInstanceSafe(instanceId));
                const util::Hash config(this->getSectionFromRuntimeDescription(path + ".configuration"));
                if (config.empty()) { // might have failed if instance not monitored anymore
                    KARABO_LOG_FRAMEWORK_WARN << "Instance '" << instanceId << "' gone, cannot forward its signalChanged";
                    continue;
                }
                // Now collect all changed properties (including their attributes).
                util::Hash toSend;
                for (std::set<std::string>::const_iterator it = properties.begin(), iEnd = properties.end();
                        it != iEnd; ++it) {
                    const boost::optional<const util::Hash::Node&> propertyNode = config.find(*it);
                    if (!propertyNode) {
                        KARABO_LOG_FRAMEWORK_INFO << "sendSignalsChanged: no '" << *it << "' for " << instanceId;
                        continue;
                    }
                    // Use Hash::setNode below to copy the full property with its attributes. But that uses
                    // 'propertyNode's key, not full path => find (or even create) direct mother:
                    util::Hash* motherNode = &toSend;
                    const std::string::size_type posOfDot = it->find_last_of('.'); // I'd love to get '.' from Hash!
                    if (posOfDot != std::string::npos) {
                        // So '*it' is a path with nested keys => find or create mother.
                        const std::string motherNodePath(*it, 0, posOfDot);
                        if (toSend.has(motherNodePath)) {
                            motherNode = &(toSend.get<util::Hash>(motherNodePath));
                        } else {
                            motherNode = toSend.bindPointer<util::Hash>(motherNodePath);
                        }
                    }
                    motherNode->setNode(propertyNode.get());
                } // end loop on changed properties
                this->notifyDeviceChangedMonitors(toSend, instanceId);
            } // end loop on instances
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
            KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN(false);
            return m_signalSlotable.lock()->login(username, password, provider);
        }


        bool DeviceClient::logout() {
            KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN(false);
            return m_signalSlotable.lock()->logout();
        }


        std::string DeviceClient::getInstanceType(const karabo::util::Hash& instanceInfo) const {
            boost::optional<const Hash::Node&> node = instanceInfo.find("type");
            string type("unknown");
            if (node) type = node->getValue<string>();
            return type;
        }

        bool DeviceClient::hasAttribute(const std::string& instanceId, const std::string& key, const std::string& attribute, const char keySep) {
            return cacheAndGetConfiguration(instanceId).hasAttribute(key, attribute, keySep);
        }

#undef KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN

    }
}
