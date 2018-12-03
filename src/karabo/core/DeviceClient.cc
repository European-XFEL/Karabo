/*
 * $Id: Com.cc 6624 2012-06-27 12:57:09Z heisenb $
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (c) 2010-2012 European XFEL GmbH Hamburg. All rights reserved.
 */

#include "karabo/log/Logger.hh"
#include "karabo/io/FileTools.hh"
#include "karabo/webAuth/Authenticator.hh"
#include "karabo/util/DataLogUtils.hh"
#include "karabo/util/Schema.hh"

#include "DeviceClient.hh"
#include "Device.hh"
#include "karabo/net/utils.hh"
#include "karabo/net/EventLoop.hh"

using namespace std;
using namespace karabo::util;
using namespace karabo::xms;
using namespace karabo::webAuth;


namespace karabo {
    namespace core {
        const unsigned int DeviceClient::m_ageingIntervallMilliSec = 1000u;
        const unsigned int DeviceClient::m_ageingIntervallMilliSecCtr = 200u;


        DeviceClient::DeviceClient(const std::string& instanceId)
            : m_internalSignalSlotable()
            , m_signalSlotable()
            , m_isShared(false)
            , m_internalTimeout(2000)
            , m_topologyInitialized(false)
            , m_ageingTimer(karabo::net::EventLoop::getIOService())
            , m_getOlder(false) // Sic! To start aging in setAgeing below.
            , m_runSignalsChangedThread(false)
            , m_signalsChangedInterval(-1)
            , m_loggerMapCached(false) {

            const std::string ownInstanceId(instanceId.empty() ? generateOwnInstanceId() : instanceId);
            Hash instanceInfo;
            instanceInfo.set("type", "client");
            instanceInfo.set("lang", "c++");
            instanceInfo.set("visibility", 4);
            instanceInfo.set("compatibility", DeviceClient::classInfo().getVersion());
            instanceInfo.set("host", net::bareHostName());
            instanceInfo.set("status", "ok");
            m_internalSignalSlotable = karabo::xms::SignalSlotable::Pointer(new SignalSlotable(ownInstanceId,
                                                                                               "JmsConnection", Hash(),
                                                                                               60, instanceInfo));
            m_internalSignalSlotable->start();

            m_signalSlotable = m_internalSignalSlotable;

            this->setAgeing(true);
            this->setupSlots();
        }


        DeviceClient::DeviceClient(const boost::shared_ptr<SignalSlotable>& signalSlotable)
            : m_internalSignalSlotable()
            , m_signalSlotable(signalSlotable)
            , m_isShared(true)
            , m_internalTimeout(2000)
            , m_topologyInitialized(false)
            , m_ageingTimer(karabo::net::EventLoop::getIOService())
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
            // Stop thread sending the collected signal(State)Changed
            setDeviceMonitorInterval(-1);

            m_internalSignalSlotable.reset();
        }

#define KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN(...) if (m_signalSlotable.expired()) { \
                    KARABO_LOG_FRAMEWORK_ERROR << "SignalSlotable object is not valid (destroyed)."; \
                    return __VA_ARGS__; }


        void DeviceClient::setupSlots() {
            karabo::xms::SignalSlotable::Pointer p = m_signalSlotable.lock();
            // Note: Since setupSlots() is called from constructor, bind_weak is not an option...
            p->registerSlot<Hash, string > (boost::bind(&karabo::core::DeviceClient::_slotChanged, this, _1, _2), "_slotChanged");
            p->registerSlot<Schema, string, string > (boost::bind(&karabo::core::DeviceClient::_slotClassSchema, this, _1, _2, _3), "_slotClassSchema");
            p->registerSlot<Schema, string > (boost::bind(&karabo::core::DeviceClient::_slotSchemaUpdated, this, _1, _2), "_slotSchemaUpdated");
            p->registerSlot<string, Hash > (boost::bind(&karabo::core::DeviceClient::_slotInstanceNew, this, _1, _2), "_slotInstanceNew");
            p->registerSlot<string, Hash > (boost::bind(&karabo::core::DeviceClient::_slotInstanceGone, this, _1, _2), "_slotInstanceGone");
            // Note that SignalSlotable registered already a function for "slotInstanceUpdated" - both will be called
            p->registerSlot<string, Hash > (boost::bind(&karabo::core::DeviceClient::slotInstanceUpdated, this, _1, _2), "slotInstanceUpdated");
            p->registerSlot<Hash > (boost::bind(&karabo::core::DeviceClient::_slotLoggerMap, this, _1), "_slotLoggerMap");

            // No advantage from asyncConnect since connecting to one's own signal is just a call chain:
            p->connect("", "signalInstanceNew", "", "_slotInstanceNew");
            p->connect("", "signalInstanceGone", "", "_slotInstanceGone");
        }


        void DeviceClient::cacheAvailableInstances() {
            m_signalSlotable.lock()->getAvailableInstances(true); // Boolean has no effect currently
            KARABO_LOG_FRAMEWORK_DEBUG << "cacheAvailableInstances() was called";
        }


        karabo::util::Hash DeviceClient::prepareTopologyEntry(const std::string& instanceId, const karabo::util::Hash& instanceInfo) const {
            Hash entry;
            const string path(prepareTopologyPath(instanceId, instanceInfo));
            Hash::Node & entryNode = entry.set(path, Hash());
            for (Hash::const_iterator it = instanceInfo.begin(); it != instanceInfo.end(); ++it) {
                entryNode.setAttribute(it->getKey(), it->getValueAsAny());
            }
            return entry;
        }


        std::string DeviceClient::prepareTopologyPath(const std::string& instanceId, const karabo::util::Hash& instanceInfo) const {
            const boost::optional<const Hash::Node&> node = instanceInfo.find("type");
            // "unknown" is converted to a temporary string whose lifetime is extended to that of the const ref 'type'.
            const string& type = (node ? node->getValue<string>() : "unknown");
            return (type + ".") += instanceId;
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
            KARABO_LOG_FRAMEWORK_DEBUG << "_slotInstanceNew was called for: " << instanceId;

            if (this->existsInRuntimeSystemDescription(this->prepareTopologyPath(instanceId, instanceInfo))) {
                // The instance was probably killed and restarted again before we noticed that the heartbeats stopped.
                // We should properly treat its death first (especially for servers, see _slotInstanceGone).
                KARABO_LOG_FRAMEWORK_DEBUG << instanceId << " still in runtime description - call _slotInstanceGone";
                this->_slotInstanceGone(instanceId, instanceInfo);
            }

            Hash entry = prepareTopologyEntry(instanceId, instanceInfo);
            mergeIntoRuntimeSystemDescription(entry);

            if (m_instanceNewHandler) m_instanceNewHandler(entry);
            if (m_loggerMapCached && instanceId == util::DATALOGMANAGER_ID) {
                karabo::xms::SignalSlotable::Pointer p = m_signalSlotable.lock();
                if (p) {
                    // The corresponding 'connect' is done by SignalSlotable's automatic reconnect feature.
                    // Even this request might not be needed since the logger manager emits the corresponding signal.
                    // But we cannot be 100% sure that our 'connect' has been registered in time.
                    p->requestNoWait(util::DATALOGMANAGER_ID, "slotGetLoggerMap", "", "_slotLoggerMap");
                }
            }
        }


        bool DeviceClient::eraseFromRuntimeSystemDescription(const std::string& path) {
            try {
                boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
                return m_runtimeSystemDescription.erase(path);
            } catch (...) {
                KARABO_LOG_FRAMEWORK_ERROR << "Could not erase path \"" << path << " from device-client cache";
                return false;
            }
        }


        util::Hash DeviceClient::getSectionFromRuntimeDescription(const std::string& section) const {
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
            KARABO_LOG_FRAMEWORK_DEBUG << "slotInstanceUpdated was called for: " << instanceId;

            const Hash entry(prepareTopologyEntry(instanceId, instanceInfo));
            mergeIntoRuntimeSystemDescription(entry);

            if (m_instanceUpdatedHandler) m_instanceUpdatedHandler(entry);

        }


        void DeviceClient::_slotInstanceGone(const std::string& instanceId, const karabo::util::Hash& instanceInfo) {
            KARABO_LOG_FRAMEWORK_DEBUG << "_slotInstanceGone was called for: " << instanceId;
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


        void DeviceClient::setAgeing(bool on) {
            if (on && !m_getOlder) {
                m_getOlder = true;
                if (weak_from_this().lock()) {
                    m_ageingTimer.expires_from_now(boost::posix_time::milliseconds(m_ageingIntervallMilliSec));
                    m_ageingTimer.async_wait(bind_weak(&DeviceClient::age, this, boost::asio::placeholders::error));
                } else {
                    // Very likely called from constructor, so cannot use bind_weak :-(, but therefore wait only 200 ms:
                    // constructor will likely be finished, but destruction still very unlikely to have started...
                    m_ageingTimer.expires_from_now(boost::posix_time::milliseconds(m_ageingIntervallMilliSecCtr));
                    m_ageingTimer.async_wait(boost::bind(&DeviceClient::age, this, boost::asio::placeholders::error));
                }
                KARABO_LOG_FRAMEWORK_DEBUG << "Ageing is started";
            } else if (!on && m_getOlder) {
                m_getOlder = false;
                m_ageingTimer.cancel();
                KARABO_LOG_FRAMEWORK_DEBUG << "Ageing is stopped";
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

        void DeviceClient::enableInstanceTracking() {
            karabo::xms::SignalSlotable::Pointer p = m_signalSlotable.lock();
            if (p) {
                // Switch on the heartbeat tracking
                p->trackAllInstances();
                // First call : trigger the process of gathering the info about network presence
                initTopology();
            }
            else {
                KARABO_LOG_FRAMEWORK_INFO << "Instance tracking requires a valid SignalSlotable instance!";
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
                        // TODO Implement access level
                        if (getAccessLevel(it->second.getKey()) < it->second.getAttribute<int>("visibility")) continue;
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
                KARABO_LOG_FRAMEWORK_DEBUG << "Requested device server '" << deviceServer << "' does not exist.";
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
                        // TODO Re-implement access level
                        if (getAccessLevel(it->second.getKey()) < it->second.getAttribute<int>("visibility")) continue;
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
                            // TODO re-implement access level
                            if (getAccessLevel(it->second.getKey()) < it->second.getAttribute<int>("visibility")) continue;
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


            // Not found, request and cache it. Better ensure/establish connection _before_ requesting.
            // Otherwise we might miss updates in between.
            stayConnected(instanceId); // connect synchronously (if not yet connected...)
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

            // We cannot just requestNoWait 'slotGetSchema', because '_slotSchemaUpdated' will cache the Schema in
            // m_runtimeSystemDescription. But if we cache, we also have to connect for updates.
            // Disadvantage is that 'stayConnected' also connects for 'signal[State]Changed' which could be noisy.
            // But usually no-one needs just the schema without the properties as well...
            auto weakSigSlotPtr = m_signalSlotable;
            // Capturing the member variable would capture a bare 'this' - which we want to avoid and thus capture a copy.
            auto successHandler = [weakSigSlotPtr, instanceId] () {
                karabo::xms::SignalSlotable::Pointer p = weakSigSlotPtr.lock();
                if (p) p->requestNoWait(instanceId, "slotGetSchema", "", "_slotSchemaUpdated", false);
            };
            auto failureHandler = [instanceId] () {
                try {
                    throw; // to get access to the original exception
                } catch (const std::exception& e) {
                    KARABO_LOG_FRAMEWORK_WARN << "getDeviceSchemaNoWait failed to connect to '" << instanceId << "': " << e.what();
                }
            };
            stayConnected(instanceId, successHandler, failureHandler);

            return Schema();
        }


        void DeviceClient::_slotSchemaUpdated(const karabo::util::Schema& schema, const std::string& deviceId) {
            KARABO_LOG_FRAMEWORK_DEBUG << "_slotSchemaUpdated for " << deviceId;
            {
                boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
                const string path(findInstance(deviceId));
                if (path.empty()) {
                    KARABO_LOG_FRAMEWORK_WARN << "got schema for unknown instance '" << deviceId << "'.";
                    return;
                }
                m_runtimeSystemDescription.set(path + ".fullSchema", schema);
                m_runtimeSystemDescription.erase(path + ".activeSchema");
            }
            if (m_schemaUpdatedHandler) m_schemaUpdatedHandler(deviceId, schema);
        }


        karabo::util::Schema DeviceClient::getActiveSchema(const std::string& instanceId) {
            return cacheAndGetActiveSchema(instanceId);
        }


        karabo::util::Schema DeviceClient::cacheAndGetActiveSchema(const std::string& instanceId) {
            KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN(Schema());
            const std::string state(get<State>(instanceId, "state").name());
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
            int accessLevel = getAccessLevel(deviceId);
            return filterProperties(schema, accessLevel);
        }


        std::vector<std::string> DeviceClient::getProperties(const std::string& deviceId) {
            KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN(vector<string>());
            Schema schema = cacheAndGetDeviceSchema(deviceId);
            int accessLevel = getAccessLevel(deviceId);
            return filterProperties(schema, accessLevel);
        }


        std::vector<std::string> DeviceClient::getClassProperties(const std::string& serverId, const std::string& classId) {
            KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN(vector<string>());
            Schema schema = cacheAndGetClassSchema(serverId, classId);
            int accessLevel = getAccessLevel(classId);
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
            Hash cfgToSend = formatConfigToInstantiate(classId, configuration);
            m_signalSlotable.lock()->call(serverInstanceId, "slotStartDevice", cfgToSend);
        }


        void DeviceClient::instantiateNoWait(const std::string& serverInstanceId, const karabo::util::Hash& completeConfiguration) {
            KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN();
            m_signalSlotable.lock()->call(serverInstanceId, "slotStartDevice", completeConfiguration);
        }

        Hash DeviceClient::formatConfigToInstantiate(const std::string& classId, const karabo::util::Hash& configuration) {
            if (configuration.has("classId")) {
                // in this case cpp device server takes the one in the configuration anyway
                // and middlelayer server is happy
                const std::string& cid = configuration.get<string>("classId");
                if (cid != classId) {
                    // this is probably not what caller wants, but we keep allowing it not to possibly break existing code
                    KARABO_LOG_FRAMEWORK_WARN << "instantiate classId parameter '" << classId << "' mismatches configuration classId '" << cid << " '.";
                }
                return configuration;
            } else {
                Hash cfgToSend("configuration", configuration, "classId", classId);
                
                if (configuration.has("deviceId")) {
                    const std::string& did = configuration.get<string>("deviceId");
                    cfgToSend.set("deviceId", did);
                    cfgToSend.erase("configuration.deviceId");
                }    
                return cfgToSend;
            }
        }

        std::pair<bool, std::string > DeviceClient::instantiate(const std::string& serverInstanceId, const std::string& classId, const karabo::util::Hash& configuration, int timeoutInSeconds) {
                Hash cfgToSend = formatConfigToInstantiate(classId, configuration);
                return this->instantiate(serverInstanceId, cfgToSend, timeoutInSeconds);
        }


        std::pair<bool, std::string > DeviceClient::instantiate(const std::string& serverInstanceId, const karabo::util::Hash& configuration, int timeoutInSeconds) {
            if (m_signalSlotable.expired()) {
                return std::make_pair(false, "SignalSlotable object is not valid (destroyed).");
            }
            const int timeoutInMillis = (timeoutInSeconds == -1 ? 5000 : timeoutInSeconds * 1000);
            bool ok = true;
            std::string reply = "";
            try {
                m_signalSlotable.lock()->request(serverInstanceId, "slotStartDevice", configuration).timeout(timeoutInMillis).receive(ok, reply);
            } catch (const karabo::util::Exception& e) {
                reply = e.userFriendlyMsg();
                ok = false;
                return std::make_pair(ok, reply);
            }
            if (ok) {
                // Wait until this device says hello
                bool isThere = false;
                int waitedInMillis = 0;
                while (!isThere && waitedInMillis < timeoutInMillis) {
                    {
                        boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
                        isThere = m_runtimeSystemDescription.has("device." + reply);
                    }
                    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
                    waitedInMillis += 100;
                }

                if (!isThere) {
                    const string errorText("Device '" + reply + "' got started but is still not accessible after "
                                           + util::toString(waitedInMillis) + " ms!");
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
                boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
                isThere = m_runtimeSystemDescription.has("device." + deviceId);
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
                boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
                isThere = m_runtimeSystemDescription.has("server." + serverId);
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

            // Better ensure/establish connection before requesting. Otherwise we might miss updates in between.
            // If we are already connected, this is fast, but nevertheless needed to reset the ticking.
            stayConnected(deviceId); // connect synchronously (if not yet connected...)
            if (result.empty()) { // Not found, request and cache
                // Request configuration
                Hash hash;
                try {
                    m_signalSlotable.lock()->request(deviceId, "slotGetConfiguration").timeout(m_internalTimeout).receive(hash);
                } catch (const TimeoutException&) {
                    KARABO_RETHROW_AS(KARABO_TIMEOUT_EXCEPTION("Configuration request for device \"" + deviceId + "\" timed out"));
                    return result; // empty Hash
                }
                boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
                result = m_runtimeSystemDescription.set(path, hash).getValue<Hash>();
            }
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

            auto weakSigSlotPtr = m_signalSlotable;
            // Capturing member variable would capture a bare 'this' - which we want to avoid and thus capture a copy.
            auto successHandler = [weakSigSlotPtr, deviceId] () {
                karabo::xms::SignalSlotable::Pointer p = weakSigSlotPtr.lock();
                if (p) p->requestNoWait(deviceId, "slotGetConfiguration", "", "_slotChanged");
            };
            auto failureHandler = [deviceId] () {
                try {
                    throw;
                } catch (const std::exception& e) {
                    KARABO_LOG_FRAMEWORK_WARN << "getConfigurationNoWait failed to connect to '" << deviceId << "': " << e.what();
                }
            };
            stayConnected(deviceId, successHandler, failureHandler);

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
                if (p->connect(util::DATALOGMANAGER_ID, "signalLoggerMap", "", "_slotLoggerMap")) {
                    // If we cannot connect, request makes no sense
                    Hash loggerMap;
                    try {
                        p->request(util::DATALOGMANAGER_ID, "slotGetLoggerMap").timeout(m_internalTimeout).receive(loggerMap);
                        // Next 3 lines would better fit in an else block as in Python's try-except-else...
                        boost::mutex::scoped_lock lock(m_loggerMapMutex);
                        m_loggerMap = loggerMap;
                        m_loggerMapCached = true;
                        return true;
                    } catch (const TimeoutException&) {
                        Exception::clearTrace();
                        return false;
                    }
                } else {
                    KARABO_LOG_FRAMEWORK_WARN << "Failed to connect _slotLoggerMap";
                    return false;
                }
            } else {
                m_loggerMapCached = false;
                // disconnect and clear (since otherwise possibly wrong info)
                if (!p->disconnect(util::DATALOGMANAGER_ID, "signalLoggerMap", "", "_slotLoggerMap")) {
                    KARABO_LOG_FRAMEWORK_WARN << "Failed to disconnect _slotLoggerMap";
                    return false;
                }
                boost::mutex::scoped_lock lock(m_loggerMapMutex);
                m_loggerMap.clear();
                return true;
            }
        }


        void DeviceClient::_slotLoggerMap(const karabo::util::Hash& loggerMap) {
            KARABO_LOG_FRAMEWORK_DEBUG << "DeviceClient::_slotLoggerMap called";
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
                Exception::clearTrace();
                KARABO_LOG_FRAMEWORK_ERROR << "Request to DataLogReader '" << dataLogReader
                        << "' timed out for device.property '" << deviceId << "." << property << "'.";
            }
            return result;
        }


        std::string DeviceClient::getDataLogReader(const std::string& deviceId) {
            std::string dataLogReader; // the result

            // Try to get server - 1st try from map:
            std::string dataLogServer;
            const std::string loggerId(util::DATALOGGER_PREFIX + deviceId);
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
                        p->request(util::DATALOGMANAGER_ID, "slotGetLoggerMap").timeout(m_internalTimeout).receive(localLogMap);
                    } catch (const TimeoutException&) {
                        // Will fail below due to empty map...
                        Exception::clearTrace();
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
                (dataLogReader += util::DATALOGREADER_PREFIX) += toString(i++ % util::DATALOGREADERS_PER_SERVER)
                        += "-" + dataLogServer;
            }

            return dataLogReader;
        }


        std::pair<karabo::util::Hash, karabo::util::Schema> DeviceClient::getConfigurationFromPast(const std::string& deviceId, const std::string& timepoint) {
            karabo::xms::SignalSlotable::Pointer p = m_signalSlotable.lock();
            if (!p) {
                KARABO_LOG_FRAMEWORK_WARN << "SignalSlotable object is not valid (destroyed).";
                return make_pair<Hash, Schema>(Hash(), Schema());
            }

            const std::string dataLogReader(this->getDataLogReader(deviceId));
            Hash hash;
            Schema schema;
            try {
                p->request(dataLogReader, "slotGetConfigurationFromPast", deviceId, timepoint)
                        .timeout(10 * m_internalTimeout).receive(hash, schema);
            } catch (const TimeoutException&) {
                Exception::clearTrace();
                KARABO_LOG_FRAMEWORK_ERROR << "Request to DataLogReader '" << dataLogReader
                        << "' timed out for configuration at '" << timepoint << "'.";
            }

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
            // Store handler
            {
                boost::mutex::scoped_lock lock(m_deviceChangedHandlersMutex);
                m_deviceChangedHandlers.set(deviceId + "._function", callbackFunction);
            }

            // Take care that we are connected - and asynchronously request to connect if not yet connected
            auto weakSigSlotPtr = m_signalSlotable; // Copy before capture to avoid that a bare 'this' is captured
            auto successHandler = [weakSigSlotPtr, deviceId] () {
                karabo::xms::SignalSlotable::Pointer p = weakSigSlotPtr.lock();
                if (p) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "registerDeviceMonitor connected to '" << deviceId << "'";
                    p->requestNoWait(deviceId, "slotGetSchema", "", "_slotSchemaUpdated", false);
                    p->requestNoWait(deviceId, "slotGetConfiguration", "", "_slotChanged");
                }
            };
            auto failureHandler = [deviceId] () {
                KARABO_LOG_FRAMEWORK_WARN << "registerDeviceMonitor failed to connect to " << deviceId;
            };
            stayConnected(deviceId, successHandler, failureHandler);

            // Take care that we will get updates "forever"
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
                // Cache will be cleaned once age() disconnected the device.
            }
            mortalize(instanceId);
        }


        bool DeviceClient::registerChannelMonitor(const std::string& instanceId, const std::string& channel,
                                                  const karabo::xms::SignalSlotable::DataHandler& dataHandler,
                                                  const karabo::util::Hash& inputChannelCfg,
                                                  const karabo::xms::SignalSlotable::InputHandler& eosHandler) {
            return registerChannelMonitor(instanceId + ":" + channel, dataHandler, inputChannelCfg, eosHandler);
        }


        bool DeviceClient::registerChannelMonitor(const std::string& channelName,
                                                  const karabo::xms::SignalSlotable::DataHandler& dataHandler,
                                                  const karabo::util::Hash& inputChannelCfg,
                                                  const karabo::xms::SignalSlotable::InputHandler& eosHandler) {
            auto sigSlotPtr = m_signalSlotable.lock();
            // No SignalSlotable or channel already there? ==> Fail!
            if (!sigSlotPtr || sigSlotPtr->getInputChannelNoThrow(channelName)) {
                if (sigSlotPtr) {
                    KARABO_LOG_FRAMEWORK_WARN << sigSlotPtr->getInstanceId() << " cannot register channel monitor for '"
                            << channelName << "' since such an input channel already exists.";
                }
                return false;
            }

            // Prepare input configuration Hash for createInputChannel
            Hash masterCfg;
            Hash& channelCfg = masterCfg.set(channelName, inputChannelCfg).getValue<Hash>();
            channelCfg.set("connectedOutputChannels", std::vector<std::string>(1, channelName));
            if (!channelCfg.has("onSlowness")) {
                // overwrite default which is "wait" (should we tolerate "wait" at all?)
                channelCfg.set("onSlowness", "drop");
            }
            // Create InputChannel with handlers (this also enables auto-reconnect):
            InputChannel::Pointer input = sigSlotPtr->createInputChannel(channelName, masterCfg, dataHandler,
                                                                         SignalSlotable::InputHandler(), eosHandler);
            // Set an id for the input channel - since we do not allow to connect more than once to the same
            // output channel, our instance id is sufficient.
            const std::string myInstanceId(sigSlotPtr->getInstanceId());
            input->setInstanceId(myInstanceId);
            // Asynchronously connect to OutputChannel:
            auto handler = [myInstanceId, channelName](bool success) {
                if (success) {
                    KARABO_LOG_FRAMEWORK_INFO << myInstanceId << " Connected to output channel '" << channelName << "'.";
                } else {
                    try {
                        throw;
                    } catch (const std::exception& e) {
                        KARABO_LOG_FRAMEWORK_WARN << myInstanceId << " Failed to connect to output channel '"
                                << channelName << "'. Automatic reconnect will be tried if destination comes up.";
                    }
                }
            };
            sigSlotPtr->asyncConnectInputChannel(input, handler);

            return true;
        }


        bool DeviceClient::unregisterChannelMonitor(const std::string& instanceId, const std::string& channel) {
            return unregisterChannelMonitor(instanceId + ":" + channel);
        }


        bool DeviceClient::unregisterChannelMonitor(const std::string& channelName) {
            auto sigSlotPtr = m_signalSlotable.lock();

            return (sigSlotPtr && sigSlotPtr->removeInputChannel(channelName));
        }


        void DeviceClient::set(const std::string& instanceId, const karabo::util::Hash& values,
                               int timeoutInSeconds) {

            KARABO_GET_SHARED_FROM_WEAK(sp, m_signalSlotable);

            // If this is the first time we are going to talk to <instanceId>, we should get all configuration,
            // else only the answer to our set will fill the cache.
            cacheAndGetConfiguration(instanceId);

            // TODO Do not hardcode the default timeout
            if (timeoutInSeconds == -1) timeoutInSeconds = 3;

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
            if (!result.first) throw KARABO_PARAMETER_EXCEPTION(result.second);
            sp->request(instanceId, "slotReconfigure", validated).timeout(timeoutInSeconds * 1000).receive();
        }


        void DeviceClient::setNoWait(const std::string& instanceId, const karabo::util::Hash& values) {
            auto sigSlotPtr = m_signalSlotable.lock();
            if (sigSlotPtr) {
                sigSlotPtr->call(instanceId, "slotReconfigure", values);
            }
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
            it->second = 0; // reset the counter
            return result;
        }


        void DeviceClient::stayConnected(const std::string& instanceId,
                                         const boost::function<void ()>& asyncSuccessHandler,
                                         const boost::function<void ()>& asyncFailureHandler) {
            if (connectNeeded(instanceId)) { // Not there yet
                karabo::xms::SignalSlotable::Pointer p = m_signalSlotable.lock();
                if (!p) return;
                if (asyncSuccessHandler || asyncFailureHandler) { // async request
                    typedef SignalSlotable::SignalSlotConnection Connection;
                    const std::vector<Connection> cons{Connection(instanceId, "signalChanged", "", "_slotChanged"),
                                                       Connection(instanceId, "signalStateChanged", "", "_slotChanged"),
                                                       Connection(instanceId, "signalSchemaUpdated", "", "_slotSchemaUpdated")};
                    // One could 'extend' asyncFailureHandler by a wrapper that also disconnects all succeeded
                    // connections (and stop the automatic reconnect of the others). But we let that be done by the
                    // usual ageing.
                    p->asyncConnect(cons, asyncSuccessHandler, asyncFailureHandler);
                } else {
                    p->connect(instanceId, "signalChanged", "", "_slotChanged");
                    p->connect(instanceId, "signalStateChanged", "", "_slotChanged");
                    p->connect(instanceId, "signalSchemaUpdated", "", "_slotSchemaUpdated");
                }
            } else if (asyncSuccessHandler) {
                // No new connection needed, but asyncSuccessHandler should be called nevertheless.
                // TODO:
                // There is a little problem: A previous call to 'stayConnected' may have triggered a new connection.
                // Whether that has been established already or not, we end up here and directly call the
                // asyncSuccessHandler - which is (slightly) too early.
                // The same problem arises in the synchronous case where the code executed after 'stayConnected' would
                // be called too early.
                asyncSuccessHandler();
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
                    // TODO:
                    // This is at least dangerous: If some call arrives here after disconnection,
                    // we will have some config in cache - but one that will not be updated anymore!
                    // This could at least confuse killDevice(..) and killServer(..).
                    path = "device." + instanceId + ".configuration";
                    KARABO_LOG_FRAMEWORK_DEBUG << "_slotChanged created '" << path << "' for" << hash;
                } else {
                    path += ".configuration";
                }
                if (m_runtimeSystemDescription.has(path)) {
                    Hash& tmp = m_runtimeSystemDescription.get<Hash>(path);
                    // Note: 1) If hash contains empty Hash at "key", merging does not erase child "key.a".
                    //          Could be a (minor?) problem if a device injected an extension of its Schema
                    //          and later withdraws that.
                    //       2) If hash has vector<Hash> at "key", these will be appended instead that they replace
                    //          (except for table elements). To be checked that cannot be a problem.
                    //       3)  We must not send dynamic attributes with 'hash' - they would erase any previously
                    //           set attributes.
                    tmp.merge(hash);
                } else {
                    m_runtimeSystemDescription.set(path, hash);
                }
            }
            // NOTE: This will block us here, i.e. we are deaf for other changes...
            // NOTE: Monitors could be implemented as additional slots or in separate threads, too.
            notifyPropertyChangedMonitors(hash, instanceId);
            // magic: if the hash contains a change for the expected parameter "doNotCompressElements", we are sending them to the GUI no matter what
            // Don't mix doNotCompressEvents with unrelated stuff, or causality will break down
            if (m_runSignalsChangedThread && !hash.has("doNotCompressEvents")) {
                boost::mutex::scoped_lock lock(m_signalsChangedMutex);
                // Just book keep paths here and call 'notifyDeviceChangedMonitors'
                // later with content from m_runtimeSystemDescription.
                // Note:
                // If there is path "a.b" coming, should we erase possible previous changes to daughters like
                // "a.b.c.d"? No - that should better be handled in merging into m_runtimeSystemDescription above and
                // then the invalid path "a.b.c.d" should be ignored 'downstream' when sending.
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


        void DeviceClient::age(const boost::system::error_code& e) {
            if (e) return;

            try {
                boost::mutex::scoped_lock lock(m_instanceUsageMutex);
                // Loop connected instances
                for (InstanceUsage::iterator it = m_instanceUsage.begin(); it != m_instanceUsage.end(); /*NOT ++it*/) {

                    const bool immortal = this->isImmortal(it->first);
                    if (!immortal && ++(it->second) >= CONNECTION_KEEP_ALIVE) {
                        // It is mortal and too old, nobody has interest anymore:
                        // Disconnect - and clean system description from respective 'areas'.
                        // It does not harm to clean twice 'configuration' - if disconnected from either of
                        // signal(State)Changed, the device configuration is not reliable anymore.
                        const string& instanceId = it->first;
                        karabo::xms::SignalSlotable::Pointer p = m_signalSlotable.lock();
                        if (p) {
                            KARABO_LOG_FRAMEWORK_DEBUG << "Prepare disconnection from '" << instanceId << "'.";
                            std::vector<std::string> cleanAreas(1, "configuration");
                            p->asyncDisconnect(instanceId, "signalChanged", "", "_slotChanged",
                                               bind_weak(&DeviceClient::disconnectHandler, this,
                                                         "signalChanged", instanceId, cleanAreas));
                            p->asyncDisconnect(instanceId, "signalStateChanged", "", "_slotChanged",
                                               bind_weak(&DeviceClient::disconnectHandler, this,
                                                         "signalStateChanged", instanceId, cleanAreas));
                            cleanAreas = {"fullSchema", "activeSchema"};
                            p->asyncDisconnect(instanceId, "signalSchemaUpdated", "", "_slotSchemaUpdated",
                                               bind_weak(&DeviceClient::disconnectHandler, this,
                                                         "signalSchemaUpdated", instanceId, cleanAreas));
                        } else { // How happen?
                            KARABO_LOG_FRAMEWORK_ERROR << "SignalSlotable invalid in age(..), cannot disconnect "
                                    << instanceId;
                        }
                        m_instanceUsage.erase(it++); // postfix increment for erasing while iterating through map
                    } else { // immortal or 'young'
                        if (immortal) {
                            // Do not let it age. Once it gets mortal, it will stay connected for
                            // CONNECTION_KEEP_ALIVE seconds. In this way we are quickly back without the
                            // disconnect/connect overhead in case immortality is re-established quickly, e.g.
                            // by a GUI client quickly closing and opening a scene etc.
                            it->second = 0;
                        }
                        ++it;
                    }
                }
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Ageing encountered an exception: " << e.what();
            }

            if (m_getOlder) {
                m_ageingTimer.expires_from_now(boost::posix_time::milliseconds(m_ageingIntervallMilliSec));
                m_ageingTimer.async_wait(bind_weak(&DeviceClient::age, this, boost::asio::placeholders::error));
            }
        }


        void DeviceClient::disconnectHandler(const std::string& signal, const std::string& instanceId,
                                             const std::vector<std::string>& toClear) {
            KARABO_LOG_FRAMEWORK_DEBUG << "Disconnected from signal '" << signal << "' of '" << instanceId << "'.";
            const std::string path("device." + instanceId);
            for (const std::string& area : toClear) {
                const std::string fullPath(path + "." + area);
                if (!eraseFromRuntimeSystemDescription(fullPath)) {
                    // Happens e.g. for second reply from disconnecting signalState and signalStateChanged
                    // FIXME: make LOG_TRACE
                    KARABO_LOG_FRAMEWORK_DEBUG << "Failed to clear " << fullPath << " from system description "
                            << "(for signal " << signal << ").";
                }
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
                    KARABO_LOG_FRAMEWORK_DEBUG << "Instance '" << instanceId << "' gone, cannot forward its signalChanged";
                    continue;
                }
                // Now collect all changed properties (including their attributes).
                util::Hash toSend;
                toSend.merge(config, Hash::REPLACE_ATTRIBUTES, properties);
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
            // TODO: Dirty hack for now, proper authentication later
            if (username == "user") m_accessLevel = karabo::util::Schema::AccessLevel::USER;
            if (username == "operator") m_accessLevel = karabo::util::Schema::AccessLevel::OPERATOR;
            if (username == "expert") m_accessLevel = karabo::util::Schema::AccessLevel::EXPERT;
            if (username == "admin") m_accessLevel = karabo::util::Schema::AccessLevel::ADMIN;
            if (username == "god") m_accessLevel = 100;
            return true;
        }


        bool DeviceClient::logout() {
            // TODO: Dirty hack for now, proper authentication later
            return true;
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


        karabo::util::Hash DeviceClient::getOutputChannelSchema(const std::string & deviceId, const std::string& outputChannelName) {
            const Schema& schema = cacheAndGetDeviceSchema(deviceId);
            const Hash& schemaHash = schema.getParameterHash();
            return schemaHash.get<Hash>(outputChannelName+".schema");
        }


        karabo::core::Lock DeviceClient::lock(const std::string& deviceId, bool recursive, int timeout) {
            //non waiting request for lock
            if (timeout == 0) return karabo::core::Lock(m_signalSlotable, deviceId, recursive);

            //timeout was given
            const int waitTime = 1; //second
            int nTries = 0;
            while (true) {
                try {
                    return karabo::core::Lock(m_signalSlotable, deviceId, recursive);
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


        int DeviceClient::getAccessLevel(const std::string& deviceId) {
            return m_accessLevel;
        }


        std::vector<std::string> DeviceClient::getOutputChannelNames(const std::string & deviceId) {
            // Request vector of names
            vector<string> names;
            KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN(names);

            karabo::xms::SignalSlotable::Pointer p = m_signalSlotable.lock();

            try {
                p->request(deviceId, "slotGetOutputChannelNames").timeout(m_internalTimeout).receive(names); // Retrieves vector of names
            } catch (const TimeoutException&) {
                KARABO_LOG_FRAMEWORK_ERROR << "Output channel names request for instance \"" << deviceId << "\" timed out";
                Exception::clearTrace();
            }
            return names;
        }


        /**
         * Extract data source schema in form of Hash.  Data source is either a device (deviceId) or channel (deviceId:channelName):
         *     SASE1/SPB/SAMP/DATAGEN_07            is a device
         *     SASE1/SPB/SAMP/DATAGEN_07:output     is an output channel
         * @param dataSourceId
         * @return 
         */
        void DeviceClient::getDataSourceSchemaAsHash(const std::string& dataSourceId, karabo::util::Hash& properties, int accessMode) {
            properties.set(dataSourceId, Hash());

            Hash& props = properties.get<Hash>(dataSourceId);

            vector<string> vec;
            boost::split(vec, dataSourceId, boost::is_any_of(":"));

            string deviceClassId = this->get<string>(vec[0], "classId");
            string deviceVersion = this->get<string>(vec[0], "classVersion");

            if (vec.size() == 1) {
                Schema deviceSchema = this->getDeviceSchema(vec[0]);
                this->filterDataSchema(vec[0], deviceSchema, accessMode, props);
            } else if (vec.size() == 2) {
                Hash channelSchemaHash = this->getOutputChannelSchema(vec[0], vec[1]);
                this->convertSchemaHash(channelSchemaHash, accessMode, props);
            }

            properties.setAttribute(dataSourceId, "classId", deviceClassId);
            properties.setAttribute(dataSourceId, "version", deviceVersion);
        }


        void DeviceClient::filterDataSchema(const std::string& deviceId, const karabo::util::Schema& schema, int accessMode, karabo::util::Hash& hash) const {

            // Find the lastkey of the "Base class" schema
            string lastkey = "";
            {
                vector<string> baseKeys;
                Schema baseSchema;
                Device<>::expectedParameters(baseSchema);
                baseSchema.getParameterHash().getKeys(baseKeys);
                if (!baseKeys.empty()) lastkey = baseKeys[baseKeys.size() - 1];
            }

            // Filter out the "Base class" schema's entries as well as "Slots" and "Input/Output" channels
            Hash fullHash = schema.getParameterHash();
            vector<string> keys;
            fullHash.getKeys(keys);

            bool ignore = true;
            if (lastkey.empty()) ignore = false;

            for (size_t i = 0; i < keys.size(); ++i) {
                const string& key = keys[i];

                // skip up-to the lastkey inclusive
                if (ignore) {
                    if (key == lastkey) ignore = false;
                    fullHash.erase(key);
                } else if (fullHash.hasAttribute(key, KARABO_SCHEMA_DISPLAY_TYPE)) {
                    const std::string& displayType = fullHash.getAttribute<string > (key, KARABO_SCHEMA_DISPLAY_TYPE);
                    if (displayType == "Slot" || displayType == "InputChannel" || displayType == "OutputChannel") {
                        fullHash.erase(key);
                    }
                }
            }

            this->convertSchemaHash(fullHash, accessMode, hash);
        }


        void DeviceClient::convertSchemaHash(const karabo::util::Hash& schemaHash, int requestedAccessMode, karabo::util::Hash & hash) const {

            vector<string> params;
            schemaHash.getPaths(params);

            for (vector<string>::const_iterator it = params.begin(); it != params.end(); ++it) {
                const string& path = *it;
                
                // skip all parameters with DAQ policy OMIT
                if (schemaHash.hasAttribute(path, KARABO_SCHEMA_DAQ_POLICY)) {
                    const DAQPolicy& daqPolicy = static_cast<DAQPolicy>(schemaHash.getAttribute<int>(path, KARABO_SCHEMA_DAQ_POLICY));
                    if(daqPolicy == DAQPolicy::OMIT) {
                        KARABO_LOG_FRAMEWORK_DEBUG << "FILTER OUT: PATH='" << path << "', daqPolicy=" << daqPolicy;
                        continue;
                    }
                }

                // Get accessMode
                int accessMode = INIT;

                if (schemaHash.hasAttribute(path, KARABO_SCHEMA_ACCESS_MODE)) {
                    accessMode = schemaHash.getAttribute<int>(path, KARABO_SCHEMA_ACCESS_MODE);
                }

                if (!(accessMode & requestedAccessMode)) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "FILTER OUT: PATH='" << path << "', accessMode=" << accessMode << ", requestedMode=" << requestedAccessMode;
                    continue;
                }

                if (!schemaHash.hasAttribute(path, KARABO_SCHEMA_VALUE_TYPE)) continue;

                string typeAsString = schemaHash.getAttribute<string > (path, KARABO_SCHEMA_VALUE_TYPE);
                Types::ReferenceType valueType = Types::from<FromLiteral>(typeAsString);

                switch (valueType) {
                    case Types::BOOL:
                    {
                        hash.set(path, false);
                        break;
                    }
                    case Types::VECTOR_BOOL:
                    {
                        hash.set(path, vector<bool>());
                        break;
                    }
                    case Types::CHAR:
                    {
                        hash.set(path, '\0');
                        break;
                    }
                    case Types::VECTOR_CHAR:
                    {
                        hash.set(path, vector<char>());
                        break;
                    }
                    case Types::INT8:
                    {
                        hash.set<signed char>(path, 0);
                        break;
                    }
                    case Types::VECTOR_INT8:
                    {
                        hash.set(path, vector<signed char>());
                        break;
                    }
                    case Types::UINT8:
                    {
                        hash.set<unsigned char>(path, 0);
                        break;
                    }
                    case Types::VECTOR_UINT8:
                    {
                        hash.set(path, vector<unsigned char>());
                        break;
                    }

                    case Types::INT16:
                    {
                        hash.set<short>(path, 0);
                        break;
                    }
                    case Types::VECTOR_INT16:
                    {
                        hash.set(path, vector<short>());
                        break;
                    }
                    case Types::UINT16:
                    {
                        hash.set<unsigned short>(path, 0);
                        break;
                    }
                    case Types::VECTOR_UINT16:
                    {
                        hash.set(path, vector<unsigned short>());
                        break;
                    }

                    case Types::INT32:
                    {
                        hash.set<int>(path, 0);
                        break;
                    }
                    case Types::VECTOR_INT32:
                    {
                        hash.set(path, vector<int>());
                        break;
                    }
                    case Types::UINT32:
                    {
                        hash.set<unsigned int>(path, 0);
                        break;
                    }
                    case Types::VECTOR_UINT32:
                    {
                        hash.set(path, vector<unsigned int>());
                        break;
                    }

                    case Types::INT64:
                    {
                        hash.set<long long>(path, 0);
                        break;
                    }
                    case Types::VECTOR_INT64:
                    {
                        hash.set(path, vector<long long>());
                        break;
                    }
                    case Types::UINT64:
                    {
                        hash.set<unsigned long long>(path, 0);
                        break;
                    }
                    case Types::VECTOR_UINT64:
                    {
                        hash.set(path, vector<unsigned long long>());
                        break;
                    }

                    case Types::FLOAT:
                    {
                        hash.set(path, 0.0F);
                        break;
                    }
                    case Types::VECTOR_FLOAT:
                    {
                        hash.set(path, vector<float>());
                        break;
                    }

                    case Types::DOUBLE:
                    {
                        hash.set<double>(path, 0.0);
                        break;
                    }
                    case Types::VECTOR_DOUBLE:
                    {
                        hash.set(path, vector<double>());
                        break;
                    }

                    case Types::COMPLEX_FLOAT:
                    {
                        hash.set(path, complex<float>());
                        break;
                    }
                    case Types::VECTOR_COMPLEX_FLOAT:
                    {
                        hash.set(path, vector<complex<float>>());
                        break;
                    }

                    case Types::COMPLEX_DOUBLE:
                    {
                        hash.set(path, complex<double>());
                        break;
                    }
                    case Types::VECTOR_COMPLEX_DOUBLE:
                    {
                        hash.set(path, vector<complex<double>>());
                        break;
                    }

                    case Types::STRING:
                    {
                        hash.set(path, std::string());
                        break;
                    }
                    case Types::VECTOR_STRING:
                    {
                        hash.set(path, vector<string>());
                        break;
                    }
                    case Types::BYTE_ARRAY:
                    {
                        hash.set(path, std::pair<boost::shared_ptr<char>, size_t>());
                        break;
                    }


                    default:
                        KARABO_LOG_FRAMEWORK_WARN << "Unsupported property \"" << path
                                << "\" of type  \"" << Types::to<ToLiteral>(valueType) << "\".  Skip it ...";
                        //KARABO_PARAMETER_EXCEPTION("Unsupported property type : " + toString(type));
                        continue;
                }

                // Filter attributes.   We use only:
                // displayedName, description, alarm related stuff, unit, metric, pipeline, expert, user flags
                Hash::Attributes attrs = schemaHash.getAttributes(path);
                for (Hash::Attributes::const_iterator ii = attrs.begin(); ii != attrs.end(); ++ii) {
                    const string& attrKey = ii->getKey();
                    if (attrKey == KARABO_SCHEMA_CLASS_ID
                        || attrKey == KARABO_SCHEMA_ACCESS_MODE
                        || attrKey == KARABO_SCHEMA_DISPLAYED_NAME
                        || attrKey == KARABO_SCHEMA_DESCRIPTION
                        || attrKey.find("alarm") == 0
                        || attrKey == KARABO_SCHEMA_UNIT_ENUM
                        || attrKey == KARABO_SCHEMA_UNIT_NAME
                        || attrKey == KARABO_SCHEMA_UNIT_SYMBOL
                        || attrKey == KARABO_SCHEMA_METRIC_PREFIX_ENUM
                        || attrKey == KARABO_SCHEMA_METRIC_PREFIX_NAME
                        || attrKey == KARABO_SCHEMA_METRIC_PREFIX_SYMBOL
                        || attrKey == KARABO_SCHEMA_DAQ_DATA_TYPE
                        || attrKey == KARABO_HASH_CLASS_ID)
                        hash.setAttribute(path, ii->getKey(), ii->getValueAsAny());
                }
            }
            
            recursivelyAddCompoundDataTypes(schemaHash, hash);
            
        }
        
        void DeviceClient::recursivelyAddCompoundDataTypes(const karabo::util::Hash& schemaHash, karabo::util::Hash & hash) const {
            for(auto it = hash.begin(); it != hash.end(); ++it) {
                const std::string& key = it->getKey();
                if (schemaHash.hasAttribute(key, KARABO_SCHEMA_CLASS_ID)) {
                    const std::string& classId = schemaHash.getAttribute<std::string>(key, KARABO_SCHEMA_CLASS_ID);
                    it->setAttribute(KARABO_SCHEMA_CLASS_ID, classId);
                    
                    // special treatments for compounds below
                    if (classId == karabo::util::NDArray::classInfo().getClassId()){
                        Hash& h= it->getValue<Hash>();
                        if(schemaHash.hasAttribute(key+".shape", "defaultValue")){
                            h.set("shape", schemaHash.getAttributeAsAny(key+".shape", "defaultValue"));
                        }
                        if(schemaHash.hasAttribute(key+".type", "defaultValue")){
                            h.set("type", schemaHash.getAttributeAsAny(key+".type", "defaultValue"));
                        }
                    }
                }
                if(it->getType() == karabo::util::Types::HASH) recursivelyAddCompoundDataTypes(schemaHash.get<Hash>(key), it->getValue<Hash>());
            }
        }


#undef KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN

    }
}
