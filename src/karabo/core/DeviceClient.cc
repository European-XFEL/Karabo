/*
 * $Id: Com.cc 6624 2012-06-27 12:57:09Z heisenb $
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
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

#include "DeviceClient.hh"

#include <atomic>
#include <boost/asio/deadline_timer.hpp>
#include <boost/bind/bind.hpp>
#include <mutex>

#include "Device.hh"
#include "karabo/io/FileTools.hh"
#include "karabo/log/Logger.hh"
#include "karabo/net/EventLoop.hh"
#include "karabo/net/utils.hh"
#include "karabo/util/DataLogUtils.hh"
#include "karabo/util/FromLiteral.hh"
#include "karabo/util/MetaTools.hh"
#include "karabo/util/NDArray.hh"
#include "karabo/util/Schema.hh"

using namespace std;
using namespace karabo::util;
using namespace karabo::xms;
using namespace boost::placeholders;


namespace karabo {
    namespace core {
        const unsigned int DeviceClient::m_ageingIntervallMilliSec = 1000u;

        // Maximum number of attempts to complete the initialization of the DeviceClient by locking a weak pointer from
        //'*this'. Further details on the reasoning behind the two phase initialization of the DeviceClient instance
        // can be found https://git.xfel.eu/Karabo/Framework/merge_requests/3684.
        //
        // See also OutputChannel::initializeServerConnection(...)
        const int kMaxCompleteInitializationAttempts = 2500;

        // TODO: Move this to a constant of ConfigurationManager device once it becomes part of the Framework.
        const std::string DEFAULT_CONFIG_MANAGER_ID("KaraboConfigurationManager");

        void DeviceClient::initServiceDeviceIds(const Hash& serviceDeviceIds) {
            if (serviceDeviceIds.has("dataLoggerManagerId")) {
                m_dataLoggerManagerId = serviceDeviceIds.get<std::string>("dataLoggerManagerId");
            } else {
                m_dataLoggerManagerId = karabo::util::DATALOGMANAGER_ID;
            }
            if (serviceDeviceIds.has("configurationManagerId")) {
                m_configManagerId = serviceDeviceIds.get<std::string>("configurationManagerId");
            } else {
                m_configManagerId = DEFAULT_CONFIG_MANAGER_ID;
            }
        }

        DeviceClient::DeviceClient(const std::string& instanceId, bool implicitInit, const Hash& serviceDeviceIds)
            : m_internalSignalSlotable(),
              m_signalSlotable(),
              m_isShared(false),
              m_internalTimeout(3000),
              m_topologyInitialized(false),
              m_ageingTimer(karabo::net::EventLoop::getIOService()),
              m_getOlder(false) // Sic! To start aging in setAgeing below.
              ,
              m_signalsChangedTimer(karabo::net::EventLoop::getIOService()),
              m_runSignalsChangedTimer(false),
              m_signalsChangedInterval(-1),
              m_loggerMapCached(false),
              m_instanceChangeThrottler(nullptr) {
            initServiceDeviceIds(serviceDeviceIds);
            const std::string ownInstanceId(instanceId.empty() ? generateOwnInstanceId() : instanceId);
            Hash instanceInfo;
            instanceInfo.set("type", "client");
            instanceInfo.set("lang", "cpp");
            instanceInfo.set("visibility", 4);
            instanceInfo.set("host", net::bareHostName());
            instanceInfo.set("status", "ok");

            m_internalSignalSlotable = boost::make_shared<karabo::xms::SignalSlotable>(ownInstanceId,
                                                                                       Hash(), // default broker cfg
                                                                                       60, instanceInfo);
            m_internalSignalSlotable->start();

            m_signalSlotable = m_internalSignalSlotable;

            if (implicitInit) {
                karabo::net::EventLoop::getIOService().post(
                      boost::bind(&DeviceClient::completeInitialization, this, kMaxCompleteInitializationAttempts));
            }
        }


        DeviceClient::DeviceClient(const boost::shared_ptr<SignalSlotable>& signalSlotable, bool implicitInit,
                                   const Hash& serviceDeviceIds)
            : m_internalSignalSlotable(),
              m_signalSlotable(signalSlotable),
              m_isShared(true),
              m_internalTimeout(2000),
              m_topologyInitialized(false),
              m_ageingTimer(karabo::net::EventLoop::getIOService()),
              m_getOlder(false) // Sic! To start aging in setAgeing below.
              ,
              m_signalsChangedTimer(karabo::net::EventLoop::getIOService()),
              m_runSignalsChangedTimer(false),
              m_signalsChangedInterval(-1),
              m_loggerMapCached(false),
              m_instanceChangeThrottler(nullptr) {
            initServiceDeviceIds(serviceDeviceIds);
            if (implicitInit) {
                karabo::net::EventLoop::getIOService().post(
                      boost::bind(&DeviceClient::completeInitialization, this, kMaxCompleteInitializationAttempts));
            }
        }


        DeviceClient::DeviceClient(const std::string& instanceId, const Hash& serviceDeviceIds)
            : DeviceClient(instanceId, false, serviceDeviceIds) {}


        DeviceClient::DeviceClient(const boost::shared_ptr<karabo::xms::SignalSlotable>& signalSlotable,
                                   const Hash& serviceDeviceIds)
            : DeviceClient(signalSlotable, false, serviceDeviceIds) {}


        DeviceClient::~DeviceClient() {
            // Stop ageing pulsing timer.
            setAgeing(false);
            // Stops timer sending the collected signal(State)Changed.
            setDeviceMonitorInterval(-1);

            m_internalSignalSlotable.reset();
        }

#define KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN(...)                               \
    if (m_signalSlotable.expired()) {                                                    \
        KARABO_LOG_FRAMEWORK_ERROR << "SignalSlotable object is not valid (destroyed)."; \
        return __VA_ARGS__;                                                              \
    }


        void DeviceClient::completeInitialization(int countdown) {
            const Self::Pointer sharedSelf(weak_from_this().lock());
            if (!sharedSelf) {
                // Construction not yet completed.
                if (countdown > 0) {
                    // Post another attempt in the event loop.
                    // First rely on yield() to give constructor time to finish, but if that is not enough, sleep a bit.
                    if (2 * countdown < kMaxCompleteInitializationAttempts) {
                        boost::this_thread::sleep(boost::posix_time::milliseconds(1));
                    }
                    boost::this_thread::yield();
                    karabo::net::EventLoop::getIOService().post(
                          boost::bind(&DeviceClient::completeInitialization, this, --countdown));
                    return;
                } else {
                    const std::string msg(
                          "Maximum number of attempts reached to implicitly call DeviceClient::initialize()! ");
                    KARABO_LOG_FRAMEWORK_ERROR << msg;
                    throw KARABO_INIT_EXCEPTION(msg);
                }
            }
            initialize();

            KARABO_LOG_FRAMEWORK_DEBUG << "Implicit initialization of DeviceClient instance completed at countdown = "
                                       << countdown;
        }


        void DeviceClient::initialize() {
            this->setupSlots();
            this->setAgeing(true);
            this->kickSignalsChangedTimer();
        }


        void DeviceClient::setupSlots() {
            karabo::xms::SignalSlotable::Pointer p = m_signalSlotable.lock();
            p->registerSlot<Hash, string>(bind_weak(&karabo::core::DeviceClient::_slotChanged, this, _1, _2),
                                          "_slotChanged");
            p->registerSlot<Schema, string, string>(
                  bind_weak(&karabo::core::DeviceClient::_slotClassSchema, this, _1, _2, _3), "_slotClassSchema");
            p->registerSlot<Schema, string>(bind_weak(&karabo::core::DeviceClient::_slotSchemaUpdated, this, _1, _2),
                                            "_slotSchemaUpdated");
            p->registerSlot<string, Hash>(bind_weak(&karabo::core::DeviceClient::_slotInstanceNew, this, _1, _2),
                                          "_slotInstanceNew");
            p->registerSlot<string, Hash>(bind_weak(&karabo::core::DeviceClient::_slotInstanceGone, this, _1, _2),
                                          "_slotInstanceGone");
            p->registerSlot<string, Hash>(bind_weak(&karabo::core::DeviceClient::_slotInstanceUpdated, this, _1, _2),
                                          "_slotInstanceUpdated");
            p->registerSlot<Hash>(bind_weak(&karabo::core::DeviceClient::_slotLoggerMap, this, _1), "_slotLoggerMap");

            // No advantage from asyncConnect since connecting to one's own signal is just a call chain:
            p->connect("", "signalInstanceNew", "", "_slotInstanceNew");
            p->connect("", "signalInstanceGone", "", "_slotInstanceGone");
            p->connect("", "signalInstanceUpdated", "", "_slotInstanceUpdated");
        }


        void DeviceClient::cacheAvailableInstances() {
            m_signalSlotable.lock()->getAvailableInstances(true); // Boolean has no effect currently
            KARABO_LOG_FRAMEWORK_DEBUG << "cacheAvailableInstances() was called";
        }


        karabo::util::Hash DeviceClient::prepareTopologyEntry(const std::string& path,
                                                              const karabo::util::Hash& instanceInfo) const {
            Hash entry;
            Hash::Node& entryNode = entry.set(path, Hash());
            for (Hash::const_iterator it = instanceInfo.begin(); it != instanceInfo.end(); ++it) {
                entryNode.setAttribute(it->getKey(), it->getValueAsAny());
            }
            return entry;
        }


        std::string DeviceClient::prepareTopologyPath(const std::string& instanceId,
                                                      const karabo::util::Hash& instanceInfo) const {
            const boost::optional<const Hash::Node&> node = instanceInfo.find("type");
            // "unknown" is converted to a temporary string whose lifetime is extended to that of the const ref 'type'.
            const string& type = (node ? node->getValue<string>() : "unknown");
            return (type + ".") += instanceId;
        }


        std::string DeviceClient::findInstance(const std::string& instanceId) const {
            // NOT: boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
            //      As documented, that is callers responsibility.
            for (Hash::const_iterator it = m_runtimeSystemDescription.begin(); it != m_runtimeSystemDescription.end();
                 ++it) {
                Hash& tmp = it->getValue<Hash>();
                boost::optional<Hash::Node&> node = tmp.find(instanceId);
                if (node) {
                    return string(it->getKey() + ".") += instanceId;
                }
            }
            return string();
        }


        std::string DeviceClient::findInstanceSafe(const std::string& instanceId) const {
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

            const string path = this->prepareTopologyPath(instanceId, instanceInfo);
            const Hash entry(prepareTopologyEntry(path, instanceInfo));

            {
                boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
                if (m_runtimeSystemDescription.has(path)) {
                    // The instance was probably killed and restarted again before we noticed that the heartbeats
                    // stopped. We should properly treat its death first (especially for servers, see
                    // _slotInstanceGone).
                    KARABO_LOG_FRAMEWORK_DEBUG << instanceId
                                               << " still in runtime description - call _slotInstanceGone";
                    // Logically it is safe to lock and unlock: All _slotInstance[New|Updated|Gone] calls
                    // are serialised since they originate from calls of our instance to itself via the signals emitted
                    // by our SignalSlotable for the corresponding broadcast calls.
                    // Note a caveat:
                    // m_runtimeSystemDescription is also touched by schemas and configurations that need to be cached!
                    // Since these are not updated from broadcast calls, order is lost. Better separate cache of
                    // topology and cache of schemas and configurations?
                    lock.unlock();
                    // It is also safe to call a slot method directly here: All _slotInstanceXxx slots are called
                    // sequentially for the above reason, so this method call cannot be in parallel to real slot call.
                    this->_slotInstanceGone(instanceId, instanceInfo);
                    lock.lock();
                }
                m_runtimeSystemDescription.merge(entry);
            }
            if (isImmortal(instanceId)) { // A "zombie" that now gets alive again - connect and fill cache
                connectAndRequest(instanceId);
            }
            if (m_instanceNewHandler) m_instanceNewHandler(entry);

            if (m_instanceChangeThrottler) {
                m_instanceChangeThrottler->submitInstanceNew(instanceId, instanceInfo);
            }

            if (m_loggerMapCached && instanceId == m_dataLoggerManagerId) {
                karabo::xms::SignalSlotable::Pointer p = m_signalSlotable.lock();
                if (p) {
                    // The corresponding 'connect' is done by SignalSlotable's automatic reconnect feature.
                    // Even this request might not be needed since the logger manager emits the corresponding signal.
                    // But we cannot be 100% sure that our 'connect' has been registered in time.
                    p->requestNoWait(m_dataLoggerManagerId, "slotGetLoggerMap", "", "_slotLoggerMap");
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


        void DeviceClient::_slotInstanceUpdated(const std::string& instanceId, const karabo::util::Hash& instanceInfo) {
            KARABO_LOG_FRAMEWORK_DEBUG << "_slotInstanceUpdated was called for: " << instanceId;
            const string path = this->prepareTopologyPath(instanceId, instanceInfo);
            const Hash entry(prepareTopologyEntry(path, instanceInfo));

            {
                boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
                if (!m_runtimeSystemDescription.has(path)) {
                    // Not sure how we can get into this. But we do in the field with 2.20.2, at least if not tracking
                    // instances. Maybe some instanceGone arrives after instaceNew? Ordering should prevent that,
                    // but note that bound python instances can send two instanceGone, once from
                    // `slotKillServer/Device` and once from C++ destructor of `SignalSlotable` (if that is called).
                    // The latter, if delayed a lot somehow, may come after a new instance of the device sent its
                    // instanceNew.
                    //
                    // As a workaround, we try to clarify the situation:
                    // - Take care that we do not track the instance.
                    // - Trigger a call to our slotPingAnswer that will then trigger "signalInstanceNew" (but only
                    //   if instance is untracked).
                    if (auto sigSlot = m_signalSlotable.lock()) {
                        const bool wasTracked = sigSlot->eraseTrackedInstance(instanceId);
                        sigSlot->call(instanceId, "slotPing", sigSlot->getInstanceId(), 0, true);
                        KARABO_LOG_FRAMEWORK_WARN << getInstanceId() << ": Received instance update from '"
                                                  << instanceId << "' although it is not in runtime description"
                                                  << (wasTracked ? " (but was tracked!)" : "")
                                                  << ". Clarify by pinging it.";
                    }
                    return;
                }
                m_runtimeSystemDescription.merge(entry);
            }

            if (m_instanceUpdatedHandler) m_instanceUpdatedHandler(entry);

            if (m_instanceChangeThrottler) {
                m_instanceChangeThrottler->submitInstanceUpdate(instanceId, instanceInfo);
            }
        }


        void DeviceClient::_slotInstanceGone(const std::string& instanceId, const karabo::util::Hash& instanceInfo) {
            KARABO_LOG_FRAMEWORK_DEBUG << "_slotInstanceGone was called for: " << instanceId;
            try {
                const string path = this->prepareTopologyPath(instanceId, instanceInfo);

                std::vector<std::pair<std::string, Hash>> devicesOfServer;
                {
                    boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
                    if (!m_runtimeSystemDescription.has(path)) {
                        KARABO_LOG_FRAMEWORK_ERROR << instanceId
                                                   << " received instance gone although not in runtime description";
                        return;
                    }
                    if (path.find("server.") == 0ul) { // A server
                        devicesOfServer = findAndEraseDevicesAsGone(instanceId);
                    }
                    // clear cache
                    m_runtimeSystemDescription.erase(path);
                }
                for (const auto& pairDeviceIdAndInstanceInfo : devicesOfServer) {
                    treatInstanceAsGone(pairDeviceIdAndInstanceInfo.first, pairDeviceIdAndInstanceInfo.second);
                }
                treatInstanceAsGone(instanceId, instanceInfo);

            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in _slotInstanceGone: " << e.what();
            }
        }

        std::vector<std::pair<std::string, karabo::util::Hash>> DeviceClient::findAndEraseDevicesAsGone(
              const std::string& serverId) {
            std::vector<std::pair<std::string, karabo::util::Hash>> devicesOfServer;

            boost::optional<util::Hash::Node&> deviceNode = m_runtimeSystemDescription.find("device");
            if (deviceNode) {
                Hash& devices = deviceNode->getValue<Hash>();

                for (Hash::iterator it = devices.begin(); it != devices.end(); ++it) {
                    const Hash::Attributes& attributes = it->getAttributes();
                    if (attributes.has("serverId") && attributes.get<string>("serverId") == serverId) {
                        // OK, device belongs to the server that is gone.
                        const string& deviceId = it->getKey();
                        KARABO_LOG_FRAMEWORK_DEBUG << "Treat device '" << deviceId << "' as gone since its server '"
                                                   << serverId << "' is.";
                        // Generate instanceInfo as it would come with instanceGone:
                        Hash deviceInstanceInfo;
                        for (Hash::Attributes::const_iterator jt = attributes.begin(); jt != attributes.end(); ++jt) {
                            deviceInstanceInfo.set(jt->getKey(), jt->getValueAsAny());
                        }
                        devicesOfServer.push_back(std::make_pair(deviceId, std::move(deviceInstanceInfo)));
                    }
                }
                // Erase the found devices from m_runtimeSystemDescription ('devices' is a reference into it!):
                auto sigSlot = m_signalSlotable.lock();
                for (const auto& pairDeviceIdInstanceInfo : devicesOfServer) {
                    const std::string& deviceId = pairDeviceIdInstanceInfo.first;
                    devices.erase(deviceId);
                    // Tell signalslotable that device is gone. That ensures that we will get instanceNew even if the
                    // device recovers before signalslotable itself notices that the device might be gone.
                    if (sigSlot) {
                        sigSlot->eraseTrackedInstance(deviceId);
                    }
                }
            }
            return devicesOfServer;
        }

        void DeviceClient::treatInstanceAsGone(const std::string& instanceId, const karabo::util::Hash& instanceInfo) {
            {
                boost::mutex::scoped_lock lock(m_instanceUsageMutex);
                InstanceUsage::iterator it = m_instanceUsage.find(instanceId);
                if (it != m_instanceUsage.end()) {
                    // Should only happen for devices...
                    if (isImmortal(instanceId)) {
                        // Gone (i.e. dead), but immortal for us - a zombie!
                        // Mark it for reconnection (resurrection...) , see age(..) and connectNeeded(..)
                        it->second = -1;
                    } else {
                        m_instanceUsage.erase(it);
                    }
                    // Take care to avoid the automatic re-connect of SignalSlotable - for zombies we do it
                    // ourselves in _slotInstanceNew and take care to get the initial configuration as well.
                    disconnect(instanceId);
                }
            }
            if (m_instanceGoneHandler) m_instanceGoneHandler(instanceId, instanceInfo);

            if (m_instanceChangeThrottler) {
                m_instanceChangeThrottler->submitInstanceGone(instanceId, instanceInfo);
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
                m_ageingTimer.expires_from_now(boost::posix_time::milliseconds(m_ageingIntervallMilliSec));
                m_ageingTimer.async_wait(bind_weak(&DeviceClient::age, this, boost::asio::placeholders::error));
                KARABO_LOG_FRAMEWORK_DEBUG << "Ageing is started";
            } else if (!on && m_getOlder) {
                m_getOlder = false;
                m_ageingTimer.cancel();
                KARABO_LOG_FRAMEWORK_DEBUG << "Ageing is stopped";
            }
        }


        void DeviceClient::setDeviceMonitorInterval(long int milliseconds) {
            if (milliseconds >= 0) {
                m_signalsChangedInterval = milliseconds;
                if (!m_runSignalsChangedTimer) {
                    m_runSignalsChangedTimer = true;
                    this->kickSignalsChangedTimer();
                }
            } else if (m_runSignalsChangedTimer) {
                m_runSignalsChangedTimer = false;
                m_signalsChangedTimer.cancel();
            }
        }


        void DeviceClient::kickSignalsChangedTimer() {
            m_signalsChangedTimer.expires_from_now(
                  boost::posix_time::milliseconds(std::atomic_load(&m_signalsChangedInterval)));
            m_signalsChangedTimer.async_wait(
                  bind_weak(&DeviceClient::sendSignalsChanged, this, boost::asio::placeholders::error));
        }


        std::pair<bool, std::string> DeviceClient::exists(const std::string& instanceId) {
            if (m_signalSlotable.expired())
                return std::make_pair<bool, std::string>(false, "SignalSlotable object is not valid (destroyed).");
            return m_signalSlotable.lock()->exists(instanceId);
        }


        void DeviceClient::initTopology() {
            call_once(m_initTopologyOnce, [this]() {
                this->cacheAvailableInstances();
                this->m_topologyInitialized = true; // Atomic, only written here, inside once executed lambda.
            });
            // All but the thread that got the chance to execute the call_once lambda will
            // be busy-waiting for the topology to initialize. The thread that executed
            // the call_once will reach this point with m_topologyInitialized == true and
            // will leave immediately.
            if (!m_topologyInitialized) {
                karabo::net::EventLoop::addThread(); // to avoid any thread starvation during the sleep(s).
                while (!m_topologyInitialized) {
                    boost::this_thread::sleep_for(boost::chrono::milliseconds(50));
                }
                karabo::net::EventLoop::removeThread();
            }
        }

        void DeviceClient::enableInstanceTracking() {
            karabo::xms::SignalSlotable::Pointer p = m_signalSlotable.lock();
            if (p) {
                // Switch on the heartbeat tracking
                p->trackAllInstances();
                // First call : trigger the process of gathering the info about network presence
                initTopology();
            } else {
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
            for (Hash::const_map_iterator it = m_runtimeSystemDescription.mbegin();
                 it != m_runtimeSystemDescription.mend(); ++it) {
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
                    return m_runtimeSystemDescription.getAttribute<vector<string>>("server." + deviceServer,
                                                                                   "deviceClasses");
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
                            if (getAccessLevel(it->second.getKey()) < it->second.getAttribute<int>("visibility"))
                                continue;
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


        karabo::util::Schema DeviceClient::cacheAndGetDeviceSchema(const std::string& instanceId) {
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
            std::string dummy;
            p->request(instanceId, "slotGetSchema", false) // Retrieves full schema
                  .timeout(m_internalTimeout)
                  .receive(schema, dummy); // 2nd "return value" is deviceId

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
            // TODO: Adjust and use connectAndRequest and thus also request "slotGetConfiguration": If we are connected
            //       it is assumed that also the config cache is up to date.
            auto weakSigSlotPtr = m_signalSlotable;
            // Capturing the member variable would capture a bare 'this' - which we want to avoid and thus capture a
            // copy.
            auto successHandler = [weakSigSlotPtr, instanceId]() {
                karabo::xms::SignalSlotable::Pointer p = weakSigSlotPtr.lock();
                if (p) p->requestNoWait(instanceId, "slotGetSchema", "", "_slotSchemaUpdated", false);
            };
            auto failureHandler = [instanceId]() {
                std::string msg;
                try {
                    throw; // to get access to the original exception
                } catch (const karabo::util::TimeoutException&) {
                    msg = "timeout";
                    Exception::clearTrace();
                } catch (const std::exception& e) {
                    msg = e.what();
                }
                KARABO_LOG_FRAMEWORK_WARN << "getDeviceSchemaNoWait failed to connect to '" << instanceId
                                          << "': " << msg;
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
            std::string dummy;
            m_signalSlotable.lock()
                  ->request(instanceId, "slotGetSchema", true) // Retrieves active schema
                  .timeout(m_internalTimeout)
                  .receive(schema, dummy); // 2nd "return value" is deviceId

            boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
            return m_runtimeSystemDescription.set(path, schema).getValue<Schema>();
        }


        karabo::util::Schema DeviceClient::getClassSchema(const std::string& serverId, const std::string& classId) {
            return cacheAndGetClassSchema(serverId, classId);
        }


        karabo::util::Schema DeviceClient::cacheAndGetClassSchema(const std::string& serverId,
                                                                  const std::string& classId) {
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
            m_signalSlotable.lock()
                  ->request(serverId, "slotGetClassSchema", classId)
                  .timeout(m_internalTimeout)
                  .receive(schema); // Retrieves full schema

            boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
            return m_runtimeSystemDescription.set(path, schema).getValue<Schema>();
        }


        karabo::util::Schema DeviceClient::getClassSchemaNoWait(const std::string& serverId,
                                                                const std::string& classId) {
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


        void DeviceClient::_slotClassSchema(const karabo::util::Schema& schema, const std::string& classId,
                                            const std::string& serverId) {
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


        void DeviceClient::extractCommands(const karabo::util::Schema& schema, const std::string& parentKey,
                                           std::vector<std::string>& commands) {
            vector<string> keys = schema.getKeys(parentKey);

            for (const std::string& key : keys) {
                std::string path;
                if (!parentKey.empty()) {
                    path = parentKey + "." + key;
                } else {
                    path = key;
                }

                if (schema.isCommand(path)) {
                    commands.push_back(path);
                } else if (!schema.isLeaf(path)) {
                    extractCommands(schema, path, commands);
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


        std::vector<std::string> DeviceClient::getClassProperties(const std::string& serverId,
                                                                  const std::string& classId) {
            KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN(vector<string>());
            Schema schema = cacheAndGetClassSchema(serverId, classId);
            int accessLevel = getAccessLevel(classId);
            return filterProperties(schema, accessLevel);
        }


        std::vector<std::string> DeviceClient::filterProperties(const karabo::util::Schema& schema,
                                                                const int accessLevel) {
            vector<string> paths = schema.getPaths();
            std::vector<std::string> properties;


            for (const std::string& path : paths) {
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


        void DeviceClient::instantiateNoWait(const std::string& serverInstanceId, const std::string& classId,
                                             const karabo::util::Hash& configuration) {
            KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN();
            Hash cfgToSend = formatConfigToInstantiate(classId, configuration);
            m_signalSlotable.lock()->call(serverInstanceId, "slotStartDevice", cfgToSend);
        }


        void DeviceClient::instantiateNoWait(const std::string& serverInstanceId,
                                             const karabo::util::Hash& completeConfiguration) {
            KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN();
            m_signalSlotable.lock()->call(serverInstanceId, "slotStartDevice", completeConfiguration);
        }

        Hash DeviceClient::formatConfigToInstantiate(const std::string& classId,
                                                     const karabo::util::Hash& configuration) {
            if (configuration.has("classId")) {
                // in this case cpp device server takes the one in the configuration anyway
                // and middlelayer server is happy
                const std::string& cid = configuration.get<string>("classId");
                if (cid != classId) {
                    // this is probably not what caller wants, but we keep allowing it not to possibly break existing
                    // code
                    KARABO_LOG_FRAMEWORK_WARN << "instantiate classId parameter '" << classId
                                              << "' mismatches configuration classId '" << cid << " '.";
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

        std::pair<bool, std::string> DeviceClient::instantiate(const std::string& serverInstanceId,
                                                               const std::string& classId,
                                                               const karabo::util::Hash& configuration,
                                                               int timeoutInSeconds) {
            Hash cfgToSend = formatConfigToInstantiate(classId, configuration);
            return this->instantiate(serverInstanceId, cfgToSend, timeoutInSeconds);
        }


        std::pair<bool, std::string> DeviceClient::instantiate(const std::string& serverInstanceId,
                                                               const karabo::util::Hash& configuration,
                                                               int timeoutInSeconds) {
            if (m_signalSlotable.expired()) {
                return std::make_pair(false, "SignalSlotable object is not valid (destroyed).");
            }
            const int timeoutInMillis = (timeoutInSeconds == -1 ? 5000 : timeoutInSeconds * 1000);
            bool ok = true;
            std::string reply = "";
            try {
                m_signalSlotable.lock()
                      ->request(serverInstanceId, "slotStartDevice", configuration)
                      .timeout(timeoutInMillis)
                      .receive(ok, reply);
            } catch (const karabo::util::Exception& e) {
                reply = e.userFriendlyMsg(true);
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
                    const string errorText("Device '" + reply + "' got started but is still not accessible after " +
                                           util::toString(waitedInMillis) + " ms!");
                    return std::make_pair(false, errorText);
                }
            }
            return std::make_pair(ok, reply);
        }


        void DeviceClient::killDeviceNoWait(const std::string& deviceId) {
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
                string errorText("Device \"" + deviceId +
                                 "\" does not want to die in time. Try to kill it with a hammer.");
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
                m_signalSlotable.lock()
                      ->request(serverId, "slotKillServer")
                      .timeout(timeoutInSeconds * 1000)
                      .receive(reply);
            } catch (const karabo::util::Exception& e) {
                reply = e.userFriendlyMsg(true);
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
                string errorText("Server \"" + serverId +
                                 "\" does not want to die in time. Try to kill it with a hammer.");
                return std::make_pair(false, errorText);
            }
            return std::make_pair(ok, reply);
        }


        void DeviceClient::killServerNoWait(const std::string& serverId) {
            KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN();
            m_signalSlotable.lock()->call(serverId, "slotKillServer");
        }


        karabo::util::Hash DeviceClient::get(const std::string& instanceId) {
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
            if (result.empty()) {    // Not found, request and cache
                // Request configuration
                Hash hash;
                try {
                    std::string dummy;
                    m_signalSlotable.lock()
                          ->request(deviceId, "slotGetConfiguration")
                          .timeout(m_internalTimeout)
                          .receive(hash, dummy); // 2nd "return value" is deviceId
                } catch (const TimeoutException&) {
                    KARABO_RETHROW_AS(
                          KARABO_TIMEOUT_EXCEPTION("Configuration request for device \"" + deviceId + "\" timed out"));
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
                    if (node && !node->getValue<Hash>().empty()) return node->getValue<Hash>();
                }
            }

            // TODO: Adjust and use connectAndRequest and thus also request "slotGetSchema": If we are connected,
            //       it is assumed that also the schema cache is up to date.
            auto weakSigSlotPtr = m_signalSlotable;
            // Capturing member variable would capture a bare 'this' - which we want to avoid and thus capture a copy.
            auto successHandler = [weakSigSlotPtr, deviceId]() {
                karabo::xms::SignalSlotable::Pointer p = weakSigSlotPtr.lock();
                if (p) p->requestNoWait(deviceId, "slotGetConfiguration", "", "_slotChanged");
            };
            auto failureHandler = [deviceId]() {
                try {
                    throw;
                } catch (const std::exception& e) {
                    KARABO_LOG_FRAMEWORK_WARN << "getConfigurationNoWait failed to connect to '" << deviceId
                                              << "': " << e.what();
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
                if (p->connect(m_dataLoggerManagerId, "signalLoggerMap", "", "_slotLoggerMap")) {
                    // If we cannot connect, request makes no sense
                    Hash loggerMap;
                    try {
                        p->request(m_dataLoggerManagerId, "slotGetLoggerMap")
                              .timeout(m_internalTimeout)
                              .receive(loggerMap);
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
                if (!p->disconnect(m_dataLoggerManagerId, "signalLoggerMap", "", "_slotLoggerMap")) {
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


        std::vector<karabo::util::Hash> DeviceClient::getFromPast(const std::string& deviceId, const std::string& key,
                                                                  const std::string& from, std::string to,
                                                                  int maxNumData) {
            return getPropertyHistory(deviceId, key, from, to, maxNumData);
        }


        std::vector<karabo::util::Hash> DeviceClient::getPropertyHistory(const std::string& deviceId,
                                                                         const std::string& property,
                                                                         const std::string& from, std::string to,
                                                                         int maxNumData) {
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

            // Increasing timeout since getting history may take a while...
            p->request(dataLogReader, "slotGetPropertyHistory", deviceId, property, args)
                  .timeout(10 * m_internalTimeout)
                  .receive(dummy1, dummy2, result);
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
                        p->request(m_dataLoggerManagerId, "slotGetLoggerMap")
                              .timeout(m_internalTimeout)
                              .receive(localLogMap);
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
                ((dataLogReader += util::DATALOGREADER_PREFIX) += "0-") += dataLogServer;
            }

            return dataLogReader;
        }


        std::pair<karabo::util::Hash, karabo::util::Schema> DeviceClient::getConfigurationFromPast(
              const std::string& deviceId, const std::string& timepoint) {
            karabo::xms::SignalSlotable::Pointer p = m_signalSlotable.lock();
            if (!p) {
                KARABO_LOG_FRAMEWORK_WARN << "SignalSlotable object is not valid (destroyed).";
                return make_pair<Hash, Schema>(Hash(), Schema());
            }

            const std::string dataLogReader(this->getDataLogReader(deviceId));
            Hash hash;
            Schema schema;
            bool configAtTimepoint;
            std::string configTimepoint; // Timepoint for configuration as a string in ISO8601 format.
            p->request(dataLogReader, "slotGetConfigurationFromPast", deviceId, timepoint)
                  .timeout(10 * m_internalTimeout)
                  .receive(hash, schema, configAtTimepoint, configTimepoint);

            return make_pair(hash, schema);
        }


        karabo::util::Hash DeviceClient::listConfigurationFromName(const std::string& deviceId,
                                                                   const std::string& namePart) {
            karabo::util::Hash slotReply;
            std::vector<karabo::util::Hash> configs;
            karabo::xms::SignalSlotable::Pointer p = m_signalSlotable.lock();
            if (p) {
                try {
                    const auto& listParams = karabo::util::Hash("deviceId", deviceId, "name", namePart);

                    p->request(m_configManagerId, "slotListConfigurationFromName", listParams)
                          .timeout(10 * m_internalTimeout)
                          .receive(slotReply);

                    if (slotReply.has("items")) {
                        std::swap(configs, slotReply.get<std::vector<karabo::util::Hash>>("items"));
                    }
                    karabo::util::Hash result("success", true, "reason", "", "configs", configs);
                    return result;
                } catch (const TimeoutException&) {
                    Exception::clearTrace();
                    std::string errMsg = "Request to get configurations with namePart '" + namePart + "' for device '" +
                                         deviceId + "' timed out.";
                    KARABO_LOG_FRAMEWORK_ERROR << errMsg;
                    return karabo::util::Hash("success", false, "reason", errMsg, "configs",
                                              std::vector<karabo::util::Hash>());
                } catch (const std::exception& ex) {
                    std::string errMsg = "Request to get configurations with namePart '" + namePart + "' for device '" +
                                         deviceId + "' failed with error:" + ex.what();
                    KARABO_LOG_FRAMEWORK_ERROR << errMsg;
                    return karabo::util::Hash("success", false, "reason", errMsg, "configs",
                                              std::vector<karabo::util::Hash>());
                }
            } else {
                // Could not promote m_signalSlotable to shared pointer. This should happen only when
                // inappropriate use of DeviceClient is made.
                std::string errMsg = "Request to get configurations with namePart '" + namePart + "' for device '" +
                                     deviceId + "' failed with error: DeviceClient being destroyed; " +
                                     "could not call ConfigurationManager slot.";
                KARABO_LOG_FRAMEWORK_ERROR << errMsg;
                return karabo::util::Hash("success", false, "reason", errMsg, "configs",
                                          std::vector<karabo::util::Hash>());
            }
        }


        karabo::util::Hash DeviceClient::getConfigurationFromName(const std::string& deviceId,
                                                                  const std::string& name) {
            karabo::util::Hash slotReply;
            karabo::xms::SignalSlotable::Pointer p = m_signalSlotable.lock();
            if (p) {
                try {
                    const auto& getParams = karabo::util::Hash("name", name, "deviceId", deviceId);
                    p->request(m_configManagerId, "slotGetConfigurationFromName", getParams)
                          .timeout(10 * m_internalTimeout)
                          .receive(slotReply);
                    // The slot returns a hash with the slot arguments (input hash) under the key "input"
                    // and the retrieved config (if any) under the key "item".
                    karabo::util::Hash resultHash("success", true, "reason", "", "config", karabo::util::Hash());
                    if (slotReply.has("item")) {
                        std::swap(resultHash.get<karabo::util::Hash>("config"),
                                  slotReply.get<karabo::util::Hash>("item"));
                    }
                    return resultHash;
                } catch (const TimeoutException&) {
                    Exception::clearTrace();
                    std::string errMsg = "Request to get configuration with name '" + name + "' for device '" +
                                         deviceId + "' timed out.";
                    KARABO_LOG_FRAMEWORK_ERROR << errMsg;
                    return karabo::util::Hash("success", false, "reason", errMsg, "config", karabo::util::Hash());
                } catch (const std::exception& ex) {
                    std::string errMsg = "Request to get configuration with name '" + name + "' for device '" +
                                         deviceId + "' failed with error: " + ex.what();
                    KARABO_LOG_FRAMEWORK_ERROR << errMsg;
                    return karabo::util::Hash("success", false, "reason", errMsg, "config", karabo::util::Hash());
                }
            } else {
                // Could not promote m_signalSlotable to shared pointer. This should happen only when
                // inappropriate use of DeviceClient is made.
                std::string errMsg = "Request to get configuration with name '" + name + "' for device '" + deviceId +
                                     "' failed with error: DeviceClient being destroyed; " +
                                     "could not call ConfigurationManager slot.";
                KARABO_LOG_FRAMEWORK_ERROR << errMsg;
                return karabo::util::Hash("success", false, "reason", errMsg, "config", karabo::util::Hash());
            }
        }


        karabo::util::Hash DeviceClient::getLastConfiguration(const std::string& deviceId, int priority) {
            karabo::util::Hash slotReply;
            karabo::xms::SignalSlotable::Pointer p = m_signalSlotable.lock();
            if (p) {
                try {
                    const auto& getParams = karabo::util::Hash("deviceId", deviceId, "priority", priority);
                    p->request(m_configManagerId, "slotGetLastConfiguration", getParams)
                          .timeout(10 * m_internalTimeout)
                          .receive(slotReply);
                    // The slot returns a hash with the slot arguments (input hash) under the key "input"
                    // and the retrieved config (if any) under the key "item".
                    karabo::util::Hash resultHash("success", true, "reason", "");
                    if (slotReply.has("item")) {
                        resultHash.set("config", slotReply.get<karabo::util::Hash>("item"));
                    } else {
                        resultHash.set("config", karabo::util::Hash());
                    }
                    return resultHash;
                } catch (const TimeoutException&) {
                    Exception::clearTrace();
                    std::string errMsg = "Request to get last configuration with priority '" +
                                         karabo::util::toString(priority) + "' for device '" + deviceId +
                                         "' timed out.";
                    KARABO_LOG_FRAMEWORK_ERROR << errMsg;
                    return karabo::util::Hash("success", false, "reason", errMsg, "config", karabo::util::Hash());
                } catch (const std::exception& ex) {
                    std::string errMsg = "Request to get configuration with priority '" +
                                         karabo::util::toString(priority) + "' for device '" + deviceId +
                                         "' failed with error: " + ex.what();
                    KARABO_LOG_FRAMEWORK_ERROR << errMsg;
                    return karabo::util::Hash("success", false, "reason", errMsg, "config", karabo::util::Hash());
                }

            } else {
                // Could not promote m_signalSlotable to shared pointer. This should happen only when
                // inappropriate use of DeviceClient is made.
                std::string errMsg = "Request to get configuration with priority '" + karabo::util::toString(priority) +
                                     "' for device '" + deviceId +
                                     "' failed with error: DeviceClient being destroyed; " +
                                     "could not call ConfigurationManager slot.";
                KARABO_LOG_FRAMEWORK_ERROR << errMsg;
                return karabo::util::Hash("success", false, "reason", errMsg, "config", karabo::util::Hash());
            }
        }


        std::pair<bool, std::string> DeviceClient::saveConfigurationFromName(const std::string& name,
                                                                             const std::vector<std::string>& deviceIds,
                                                                             const std::string& description,
                                                                             int priority, const std::string& user) {
            // TODO: turn priority into an Enum class once ConfigurationManager moves into the Framework.
            if (priority < 1 || priority > 3) {
                return std::make_pair(false, "'priority' argument out of range; must be between 1 and 3.");
            }

            karabo::xms::SignalSlotable::Pointer p = m_signalSlotable.lock();
            if (p) {
                auto saveParams = karabo::util::Hash("name", name, "deviceIds", deviceIds, "priority", priority,
                                                     "description", description);
                if (user != ".") {
                    saveParams.set<std::string>("user", user);
                }

                try {
                    karabo::util::Hash slotReply;
                    p->request(m_configManagerId, "slotSaveConfigurationFromName", saveParams)
                          .timeout(10 * m_internalTimeout)
                          .receive(slotReply);
                    return make_pair(true, std::string());
                } catch (const TimeoutException&) {
                    Exception::clearTrace();
                    std::string failReason = "Request to save configuration(s) for " +
                                             karabo::util::toString(deviceIds.size()) + " device(s) under name '" +
                                             name + "' timed out.";
                    KARABO_LOG_FRAMEWORK_ERROR << failReason;
                    return make_pair(false, failReason);
                } catch (const std::exception& ex) {
                    std::string failReason = "Request to save configuration(s) for " +
                                             karabo::util::toString(deviceIds.size()) + " device(s) under name '" +
                                             name + "' failed with error: " + ex.what();
                    KARABO_LOG_FRAMEWORK_ERROR << failReason;
                    return make_pair(false, failReason);
                }
            } else { // Could not promote weak_ptr.
                return std::make_pair(false, "DeviceClient being destroyed; could not ConfigurationManager slot.");
            }
        }


        void DeviceClient::registerInstanceChangeMonitor(
              const InstanceChangeThrottler::InstanceChangeHandler& callBackFunction, unsigned int throttlerIntervalMs,
              unsigned int maxChangesPerCycle) {
            m_instanceChangeThrottler = karabo::core::InstanceChangeThrottler::createThrottler(
                  callBackFunction, throttlerIntervalMs, maxChangesPerCycle);
        }


        void DeviceClient::flushThrottledInstanceChanges() {
            if (m_instanceChangeThrottler) {
                m_instanceChangeThrottler->flush();
            }
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


        void DeviceClient::registerDeviceMonitor(
              const std::string& deviceId,
              const boost::function<void(const std::string& /*deviceId*/, const karabo::util::Hash& /*config*/)>&
                    callbackFunction) {
            // Store handler
            {
                boost::mutex::scoped_lock lock(m_deviceChangedHandlersMutex);
                m_deviceChangedHandlers.set(deviceId + "._function", callbackFunction);
            }

            registerDeviceForMonitoring(deviceId);
        }


        void DeviceClient::registerDeviceForMonitoring(const std::string& deviceId) {
            // Take care that we are connected - and asynchronously request to connect if not yet connected
            connectAndRequest(deviceId);

            // Take care that we will get updates "forever"
            immortalize(deviceId);
        }


        void DeviceClient::registerDevicesMonitor(const DevicesChangedHandler& devicesChangesHandler) {
            boost::mutex::scoped_lock lock(m_devicesChangesMutex);
            m_devicesChangesHandler = devicesChangesHandler;
        }


        void DeviceClient::connectAndRequest(const std::string& deviceId) {
            auto weakSigSlotPtr = m_signalSlotable; // Copy before capture to avoid that a bare 'this' is captured
            auto successHandler = [weakSigSlotPtr, deviceId]() {
                karabo::xms::SignalSlotable::Pointer p = weakSigSlotPtr.lock();
                if (p) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "connected to '" << deviceId << "'";
                    p->requestNoWait(deviceId, "slotGetSchema", "", "_slotSchemaUpdated", false);
                    p->requestNoWait(deviceId, "slotGetConfiguration", "", "_slotChanged");
                }
            };
            auto failureHandler = [deviceId]() { KARABO_LOG_FRAMEWORK_WARN << "failed to connect to " << deviceId; };
            stayConnected(deviceId, successHandler, failureHandler);
        }


        void DeviceClient::unregisterPropertyMonitor(const std::string& instanceId, const std::string& key) {
            bool isMortal = false;
            {
                boost::mutex::scoped_lock lock(m_propertyChangedHandlersMutex);
                boost::optional<Hash::Node&> node = m_propertyChangedHandlers.find(instanceId);
                if (node) {
                    Hash& tmp = node->getValue<Hash>();
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
            // TODO: Has to stay a live if a deviceMonitor is still there for this device, see below!
            if (isMortal) mortalize(instanceId);
        }


        void DeviceClient::unregisterDeviceMonitor(const std::string& instanceId) {
            {
                boost::mutex::scoped_lock lock(m_deviceChangedHandlersMutex);
                if (m_deviceChangedHandlers.has(instanceId)) m_deviceChangedHandlers.erase(instanceId);
                // Cache will be cleaned once age() disconnected the device.
            }
            unregisterDeviceFromMonitoring(instanceId);
        }


        void DeviceClient::unregisterDeviceFromMonitoring(const std::string& deviceId) {
            // TODO: Has to stay a live if a propertyMonitor is still there for this device, see above!
            mortalize(deviceId);
        }


        bool DeviceClient::registerChannelMonitor(const std::string& channelName,
                                                  const karabo::core::DeviceClient::InputChannelHandlers& handlers,
                                                  const karabo::util::Hash& inputChannelCfg) {
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
            // Create InputChannel with handlers (this also enables auto-reconnect):
            auto connectionTracker = [tracker = std::move(handlers.statusTracker), channelName](
                                           const std::string& outId, net::ConnectionStatus status) {
                if (tracker && channelName == outId) tracker(status);
            };
            InputChannel::Pointer input =
                  sigSlotPtr->createInputChannel(channelName, masterCfg, handlers.dataHandler, handlers.inputHandler,
                                                 handlers.eosHandler, connectionTracker);
            // Set an id for the input channel - since we do not allow to connect more than once to the same
            // output channel, our instance id is sufficient.
            const std::string myInstanceId(sigSlotPtr->getInstanceId());
            // Asynchronously connect to OutputChannel:
            auto handler = [myInstanceId, channelName](bool success) {
                if (success) {
                    KARABO_LOG_FRAMEWORK_INFO << myInstanceId << " Connected to output channel '" << channelName
                                              << "'.";
                } else {
                    try {
                        throw;
                    } catch (const std::exception& e) {
                        KARABO_LOG_FRAMEWORK_WARN
                              << myInstanceId << " Failed to connect to output channel '" << channelName
                              << "'. Automatic reconnect will be tried regularly or if destination comes up.";
                    }
                }
            };
            sigSlotPtr->asyncConnectInputChannel(input, handler);

            return true;
        }


        bool DeviceClient::registerChannelMonitor(const std::string& instanceId, const std::string& channel,
                                                  const karabo::xms::SignalSlotable::DataHandler& dataHandler,
                                                  const karabo::util::Hash& inputChannelCfg,
                                                  const karabo::xms::SignalSlotable::InputHandler& eosHandler,
                                                  const karabo::xms::SignalSlotable::InputHandler& inputHandler) {
            InputChannelHandlers handlers(dataHandler, eosHandler);
            handlers.inputHandler = inputHandler;

            return registerChannelMonitor(instanceId + ":" + channel, handlers, inputChannelCfg);
        }


        bool DeviceClient::registerChannelMonitor(const std::string& channelName,
                                                  const karabo::xms::SignalSlotable::DataHandler& dataHandler,
                                                  const karabo::util::Hash& inputChannelCfg,
                                                  const karabo::xms::SignalSlotable::InputHandler& eosHandler,
                                                  const karabo::xms::SignalSlotable::InputHandler& inputHandler) {
            InputChannelHandlers handlers(dataHandler, eosHandler);
            handlers.inputHandler = inputHandler;

            return registerChannelMonitor(channelName, handlers, inputChannelCfg);
        }


        bool DeviceClient::unregisterChannelMonitor(const std::string& instanceId, const std::string& channel) {
            return unregisterChannelMonitor(instanceId + ":" + channel);
        }


        bool DeviceClient::unregisterChannelMonitor(const std::string& channelName) {
            auto sigSlotPtr = m_signalSlotable.lock();

            return (sigSlotPtr && sigSlotPtr->removeInputChannel(channelName));
        }


        void DeviceClient::set(const std::string& instanceId, const karabo::util::Hash& values, int timeoutInSeconds) {
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
                                             /*injectTimestamps*/ false);
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


        bool DeviceClient::connectNeeded(const std::string& instanceId) {
            boost::mutex::scoped_lock lock(m_instanceUsageMutex);
            InstanceUsage::iterator it = m_instanceUsage.find(instanceId);
            if (it == m_instanceUsage.end()) {
                m_instanceUsage[instanceId] = 0;
                return true;
            }

            // < 0: Marked as a zombie in _slotInstanceGone - let's resurrect (aeh, reconnect to) it
            //      since someone registered for it. See _slotInstanceGone(..) and age(..).
            const bool result = (it->second < 0 || it->second >= CONNECTION_KEEP_ALIVE);
            it->second = 0; // reset the counter
            return result;
        }


        void DeviceClient::stayConnected(const std::string& instanceId,
                                         const boost::function<void()>& asyncSuccessHandler,
                                         const boost::function<void()>& asyncFailureHandler) {
            if (connectNeeded(instanceId)) { // Not there yet
                karabo::xms::SignalSlotable::Pointer p = m_signalSlotable.lock();
                if (!p) return;
                if (asyncSuccessHandler || asyncFailureHandler) { // async request
                    typedef SignalSlotable::SignalSlotConnection Connection;
                    const std::vector<Connection> cons{
                          Connection(instanceId, "signalChanged", "", "_slotChanged"),
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


        void DeviceClient::_slotChanged(const karabo::util::Hash& hash, const std::string& instanceId) {
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
                    // No configuration was set before for this 'instanceId'...
                    // Check if this is "initial" configuration and not simply "update"
                    if (!hash.find("_deviceId_")) return;
                    m_runtimeSystemDescription.set(path, hash);
                }
            }
            // NOTE: This will block us here, i.e. we are deaf for other changes...
            notifyPropertyChangedMonitors(hash, instanceId);
            if (m_runSignalsChangedTimer) {
                if (hash.has("doNotCompressEvents")) {
                    // magic: if the hash contains a change for the expected parameter "doNotCompressElements
                    // we are sending it directly, no throttling.
                    // Note: Since we updated m_runtimeSystemDescription above, there is no problem if a previous
                    // time: the next "flushing"  of m_runtimeSystemDescription will send the new value again.
                    boost::mutex::scoped_lock lock(m_devicesChangesMutex);
                    if (m_devicesChangesHandler) {
                        Hash deviceChanges(instanceId, hash);
                        m_devicesChangesHandler(deviceChanges);
                    }
                } else {
                    boost::mutex::scoped_lock lock(m_signalsChangedMutex);
                    // Just book keep paths here and call 'notifyDeviceChangedMonitors'
                    // later with content from m_runtimeSystemDescription.
                    // Note:
                    // If there is path "a.b" coming, should we erase possible previous changes to daughters like
                    // "a.b.c.d"? No - that should better be handled in merging into m_runtimeSystemDescription above
                    // and then the invalid path "a.b.c.d" should be ignored 'downstream' when sending.
                    hash.getPaths(m_signalsChanged[instanceId]);
                }
            } else {
                // There is a tiny (!) risk here: The last loop of the corresponding thread
                // might still be running and _later_ call 'notifyDeviceChangedMonitors'
                // with an older value...
                notifyDeviceChangedMonitors(hash, instanceId);
            }
        }


        void DeviceClient::notifyDeviceChangedMonitors(const karabo::util::Hash& hash, const std::string& instanceId) {
            Hash entry;

            {
                boost::mutex::scoped_lock lock(m_deviceChangedHandlersMutex);
                boost::optional<Hash::Node&> node = m_deviceChangedHandlers.find(instanceId);
                if (node) {
                    entry = node->getValue<Hash>();
                }
            }

            if (!entry.empty()) {
                boost::optional<Hash::Node&> nodeFunc = entry.find("_function");
                boost::optional<Hash::Node&> nodeData = entry.find("_userData");
                if (nodeData) {
                    boost::any_cast<
                          boost::function<void(const std::string&, const karabo::util::Hash&, const boost::any&)>>(
                          nodeFunc->getValueAsAny())(instanceId, hash, nodeData->getValueAsAny());
                } else {
                    boost::any_cast<boost::function<void(const std::string&, const karabo::util::Hash&)>>(
                          nodeFunc->getValueAsAny())(instanceId, hash);
                }
            }
        }


        void DeviceClient::notifyPropertyChangedMonitors(const karabo::util::Hash& hash,
                                                         const std::string& instanceId) {
            Hash registered;

            {
                boost::mutex::scoped_lock lock(m_propertyChangedHandlersMutex);
                if (m_propertyChangedHandlers.has(instanceId)) {
                    registered = m_propertyChangedHandlers.get<Hash>(instanceId);
                }
            }

            if (!registered.empty()) {
                castAndCall(instanceId, registered, hash);
            }
        }


        void DeviceClient::castAndCall(const std::string& instanceId, const Hash& registered, const Hash& current,
                                       std::string path) const {
#define KARABO_REGISTER_CALLBACK(valueType)                                                                \
    if (nodeData) {                                                                                        \
        boost::any_cast<boost::function<void(const std::string&, const std::string&, const valueType&,     \
                                             const karabo::util::Timestamp&, const boost::any&)>>(         \
              nodeFunc->getValueAsAny())(instanceId, currentPath, it->getValue<valueType>(), t,            \
                                         nodeData->getValueAsAny());                                       \
    } else {                                                                                               \
        boost::any_cast<boost::function<void(const std::string&, const std::string&, const valueType&,     \
                                             const karabo::util::Timestamp&)>>(nodeFunc->getValueAsAny())( \
              instanceId, currentPath, it->getValue<valueType>(), t);                                      \
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

                    const Hash& entry = registered.get<Hash>(currentPath);
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
                    } else if (it->is<std::string>()) {
                        KARABO_REGISTER_CALLBACK(std::string);
                    } else if (it->is<boost::filesystem::path>()) {
                        KARABO_REGISTER_CALLBACK(boost::filesystem::path);
                    } else if (it->is<karabo::util::Hash>()) {
                        KARABO_REGISTER_CALLBACK(karabo::util::Hash);
                    } else if (it->is<std::vector<bool>>()) {
                        KARABO_REGISTER_CALLBACK(std::vector<bool>);
                    } else if (it->is<std::vector<char>>()) {
                        KARABO_REGISTER_CALLBACK(std::vector<char>);
                    } else if (it->is<std::vector<signed char>>()) {
                        KARABO_REGISTER_CALLBACK(std::vector<signed char>);
                    } else if (it->is<std::vector<unsigned char>>()) {
                        KARABO_REGISTER_CALLBACK(std::vector<unsigned char>);
                    } else if (it->is<std::vector<short>>()) {
                        KARABO_REGISTER_CALLBACK(std::vector<short>);
                    } else if (it->is<std::vector<unsigned short>>()) {
                        KARABO_REGISTER_CALLBACK(std::vector<unsigned short>);
                    } else if (it->is<std::vector<int>>()) {
                        KARABO_REGISTER_CALLBACK(std::vector<int>);
                    } else if (it->is<std::vector<unsigned int>>()) {
                        KARABO_REGISTER_CALLBACK(std::vector<unsigned int>);
                    } else if (it->is<std::vector<long long>>()) {
                        KARABO_REGISTER_CALLBACK(std::vector<long long>);
                    } else if (it->is<std::vector<unsigned long long>>()) {
                        KARABO_REGISTER_CALLBACK(std::vector<unsigned long long>);
                    } else if (it->is<std::vector<float>>()) {
                        KARABO_REGISTER_CALLBACK(std::vector<float>);
                    } else if (it->is<std::vector<double>>()) {
                        KARABO_REGISTER_CALLBACK(std::vector<double>);
                    } else if (it->is<karabo::util::Schema>()) {
                        KARABO_REGISTER_CALLBACK(karabo::util::Schema);
                    } else if (it->is<std::vector<std::string>>()) {
                        KARABO_REGISTER_CALLBACK(std::vector<std::string>);
                    } else if (it->is<std::vector<karabo::util::Hash>>()) {
                        KARABO_REGISTER_CALLBACK(std::vector<karabo::util::Hash>);

                    } else {
                        throw KARABO_LOGIC_EXCEPTION("Failed to call registered monitor (datatype problems)");
                    }
                }


                if (it->is<karabo::util::Hash>())
                    castAndCall(instanceId, registered, it->getValue<Hash>(), currentPath);
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
                    // Caveat:
                    // Check for !immortal first to avoid that operator++ ages an immortal. That means that a zombie
                    // (with it->second == -1) stays a zombie. Once an immortal gets mortal, the counter starts again,
                    // i.e. it stays connected CONNECTION_KEEP_ALIVE seconds to be quickly back without
                    // disconnect/connect overhead in case immortality is re-established quickly, e.g. by a GUI client
                    // closing and opening a scene etc.
                    if (!immortal && ++(it->second) >= CONNECTION_KEEP_ALIVE) {
                        // It is mortal and too old, nobody has interest anymore:
                        this->disconnect(it->first);
                        it = m_instanceUsage.erase(it);
                    } else { // immortal or 'young'
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


        void DeviceClient::disconnect(const std::string& instanceId) {
            // Disconnect - and clean system description from respective 'areas'.
            // It does not harm to clean twice 'configuration' - if disconnected from either of
            // signal(State)Changed, the device configuration is not reliable anymore.
            karabo::xms::SignalSlotable::Pointer p = m_signalSlotable.lock();
            if (p) {
                KARABO_LOG_FRAMEWORK_DEBUG << "Prepare disconnection from '" << instanceId << "'.";
                std::vector<std::string> cleanAreas(1, "configuration");
                p->asyncDisconnect(
                      instanceId, "signalChanged", "", "_slotChanged",
                      bind_weak(&DeviceClient::disconnectHandler, this, "signalChanged", instanceId, cleanAreas));
                p->asyncDisconnect(
                      instanceId, "signalStateChanged", "", "_slotChanged",
                      bind_weak(&DeviceClient::disconnectHandler, this, "signalStateChanged", instanceId, cleanAreas));
                cleanAreas = {"fullSchema", "activeSchema"};
                p->asyncDisconnect(
                      instanceId, "signalSchemaUpdated", "", "_slotSchemaUpdated",
                      bind_weak(&DeviceClient::disconnectHandler, this, "signalSchemaUpdated", instanceId, cleanAreas));
            } else { // How happen?
                KARABO_LOG_FRAMEWORK_ERROR << "SignalSlotable invalid, cannot disconnect " << instanceId;
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


        void DeviceClient::sendSignalsChanged(const boost::system::error_code& e) {
            if (e) return;

            try {
                // Get map of all properties that changed (and clear original)
                SignalChangedMap localChanged;
                {
                    boost::mutex::scoped_lock lock(m_signalsChangedMutex);
                    m_signalsChanged.swap(localChanged);
                }
                this->doSendSignalsChanged(localChanged);
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Exception encountered in 'sendSignalsChanged': " << e.what();
            }

            if (m_runSignalsChangedTimer) {
                // As the timer is still active, kick the next pulse.
                this->kickSignalsChangedTimer();
            } else {
                // Just in case anything was added before 'm_runSignalsChangedTimer' was set to false
                // and while we processed the previous content (keep lock until done completely):
                try {
                    boost::mutex::scoped_lock lock(m_signalsChangedMutex);
                    this->doSendSignalsChanged(m_signalsChanged);
                    m_signalsChanged.clear();
                } catch (...) { // lazy to catch all levels - we are anyway done with the thread...
                    KARABO_LOG_FRAMEWORK_ERROR << "Exception encountered when leaving 'sendSignalsChanged'";
                }
            }
        }


        void DeviceClient::doSendSignalsChanged(const SignalChangedMap& localChanged) {
            Hash allUpdates;
            // Iterate on devices (i.e. keys in map)
            for (SignalChangedMap::const_iterator mapIt = localChanged.begin(), mapEnd = localChanged.end();
                 mapIt != mapEnd; ++mapIt) {
                const std::string& instanceId = mapIt->first;
                const std::set<std::string>& properties = mapIt->second;
                // Get path of instance in runtime system description and then its configuration
                const std::string path(this->findInstanceSafe(instanceId));
                const util::Hash config(this->getSectionFromRuntimeDescription(path + ".configuration"));
                if (config.empty()) { // might have failed if instance not monitored anymore
                    KARABO_LOG_FRAMEWORK_DEBUG << "Instance '" << instanceId
                                               << "' gone, cannot forward its signalChanged";
                    continue;
                }
                // Now collect all changed properties (including their attributes).
                util::Hash& instanceUpdates = allUpdates.bindReference<Hash>(instanceId);
                instanceUpdates.merge(config, Hash::REPLACE_ATTRIBUTES, properties);
                this->notifyDeviceChangedMonitors(instanceUpdates, instanceId);
            } // end loop on instances

            // Sends all updates if there's a handler defined.
            {
                boost::mutex::scoped_lock lock(m_devicesChangesMutex);
                if (m_devicesChangesHandler && !allUpdates.empty()) {
                    m_devicesChangesHandler(allUpdates);
                }
            }
        }


        void DeviceClient::immortalize(const std::string& deviceId) {
            boost::mutex::scoped_lock lock(m_immortalsMutex);
            m_immortals.insert(deviceId);
        }


        void DeviceClient::mortalize(const std::string& deviceId) {
            {
                // If we want to mortalize a zombie, it has to be dead immediately, so remove it from usage.
                // Otherwise age(..) could be fooled and resurrect the zombie to have a normal age counter. Thus a
                // request to the device within the CONNECTION_KEEP_ALIVE time will assume that it is alive and cached!
                boost::mutex::scoped_lock lock(m_instanceUsageMutex);
                InstanceUsage::iterator it = m_instanceUsage.find(deviceId);
                if (it != m_instanceUsage.end() && it->second < 0) { // a zombie
                    m_instanceUsage.erase(it);
                }
            }
            // Better erase from m_immortals after the clean-up above. Otherwise any loop on m_instanceUsage that asks
            // isImmortal(..) could be fooled.
            boost::mutex::scoped_lock lock(m_immortalsMutex);
            m_immortals.erase(deviceId);
        }


        bool DeviceClient::isImmortal(const std::string& deviceId) const {
            boost::mutex::scoped_lock lock(m_immortalsMutex);
            return m_immortals.find(deviceId) != m_immortals.end();
        }


        bool DeviceClient::login(const std::string& username, const std::string& password,
                                 const std::string& provider) {
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


        bool DeviceClient::hasAttribute(const std::string& instanceId, const std::string& key,
                                        const std::string& attribute, const char keySep) {
            return cacheAndGetConfiguration(instanceId).hasAttribute(key, attribute, keySep);
        }


        karabo::util::Hash DeviceClient::getOutputChannelSchema(const std::string& deviceId,
                                                                const std::string& outputChannelName) {
            const Schema& schema = cacheAndGetDeviceSchema(deviceId);
            const Hash& schemaHash = schema.getParameterHash();
            return schemaHash.get<Hash>(outputChannelName + ".schema");
        }


        karabo::core::Lock DeviceClient::lock(const std::string& deviceId, bool recursive, int timeout) {
            // non waiting request for lock
            if (timeout == 0) return karabo::core::Lock(m_signalSlotable, deviceId, recursive);

            // timeout was given
            const int waitTime = 1; // second
            int nTries = 0;
            while (true) {
                try {
                    return karabo::core::Lock(m_signalSlotable, deviceId, recursive);
                } catch (const karabo::util::LockException& e) {
                    if (nTries++ > timeout / waitTime && timeout != -1) {
                        KARABO_RETHROW;
                    }
                    // otherwise pass through and try again
                    boost::this_thread::sleep(boost::posix_time::seconds(waitTime));
                }
            }
        }


        int DeviceClient::getAccessLevel(const std::string& deviceId) {
            return m_accessLevel;
        }


        std::vector<std::string> DeviceClient::getOutputChannelNames(const std::string& deviceId) {
            // Request vector of names
            vector<string> names;
            KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN(names);

            karabo::xms::SignalSlotable::Pointer p = m_signalSlotable.lock();

            p->request(deviceId, "slotGetOutputChannelNames")
                  .timeout(m_internalTimeout)
                  .receive(names); // Retrieves vector of names

            return names;
        }


#undef KARABO_IF_SIGNAL_SLOTABLE_EXPIRED_THEN_RETURN

    } // namespace core
} // namespace karabo
