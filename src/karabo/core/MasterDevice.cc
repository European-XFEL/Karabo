/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include <karabo/io/Input.hh>
#include "MasterDevice.hh"
#include "DeviceServer.hh"
#include "karabo/io/FileTools.hh"
#include "karabo/tests/util/Timestamp_Test.hh"

namespace karabo {
    namespace core {

        using namespace log4cpp;

        using namespace std;
        using namespace karabo::util;
        using namespace karabo::io;


        KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<OkErrorFsm>, MasterDevice)

        void MasterDevice::expectedParameters(Schema& expected) {
        }


        MasterDevice::MasterDevice(const Hash& input) : Device<OkErrorFsm>(input) {
            setupSlots();
            m_systemHistory.set("device", Hash());
        }


        MasterDevice::~MasterDevice() {
            m_persistData = false;
            m_persistDataThread.join();
        }


        void MasterDevice::setupSlots() {
            GLOBAL_SLOT3(slotValidateInstanceId, string /*hostname*/, string /*instanceType*/, string /*instanceId*/);
            GLOBAL_SLOT2(slotInstanceNew, string /*instanceId*/, Hash /*instanceInfo*/);
            GLOBAL_SLOT2(slotInstanceGone, string /*instanceId*/, Hash /*instanceInfo*/);
            SLOT2(slotChanged, Hash /*changedConfig*/, string /*deviceId*/);
        }


        void MasterDevice::slotValidateInstanceId(const std::string& hostname, const std::string& instanceType, const std::string& instanceId) {
            KARABO_LOG_INFO << "Device-server from host \"" << hostname << "\" requests device-server instanceId";
            string id = instanceId;
            if (id.empty()) { // Generate
                if (instanceType == "server") {
                    id = hostname + "_DeviceServer_" + karabo::util::toString(getNumberOfServersOnHost(hostname));
                }
            }

            string foreignHost;
            string welcomeMessage;
            Hash instanceInfo;
            try {
                request("*", "slotPing", id, true).timeout(100).receive(instanceInfo);
            } catch (const karabo::util::TimeoutException&) {
                Exception::clearTrace();
                if (m_systemNow.has("server." + id)) welcomeMessage = "Welcome back!";
                else welcomeMessage = "Your name got accepted, welcome in the team!";
                KARABO_LOG_DEBUG << "Shipping welcome message: " << welcomeMessage;
                reply(true, id, welcomeMessage); // Ok, instance does not exist
                return;
            }
            if (instanceInfo.has("host")) instanceInfo.get("host", foreignHost);
            welcomeMessage = "Another device-server with the same instance is already online (on host: " + foreignHost + ")";
            KARABO_LOG_DEBUG << "Shipping welcome message: " << welcomeMessage;
            reply(false, id, welcomeMessage); // Shit, instance exists already
        }


        size_t MasterDevice::getNumberOfServersOnHost(const std::string& hostname) {
            boost::mutex::scoped_lock lock(m_systemNowMutex);
            if (!m_systemNow.has("server")) return 0; // No server at all
            const Hash& servers = m_systemNow.get<Hash>("server");
            size_t count = 0;
            for (Hash::const_iterator it = servers.begin(); it != servers.end(); ++it) {
                if (it->getAttribute<string>("host") == hostname) count++;
            }
            return count;
        }


        void MasterDevice::okStateOnEntry() {
            if (!boost::filesystem::exists("karaboHistory")) {
                boost::filesystem::create_directory("karaboHistory");
            }
            m_persistData = true;
            m_persistDataThread = boost::thread(boost::bind(&karabo::core::MasterDevice::persistDataThread, this));
        }


        void MasterDevice::slotInstanceNew(const std::string& instanceId, const karabo::util::Hash& instanceInfo) {
            
            KARABO_LOG_DEBUG << "New instance \"" << instanceId << "\" got registered";
            
            // Skip all Karabo-intern instances
            if (instanceId.substr(0,6) == "Karabo") return;

            onInstanceNewForSystemNow(instanceId, instanceInfo);
            onInstanceNewForSystemHistory(instanceId, instanceInfo);

            // Start tracking
            trackExistenceOfInstance(instanceId);

            // Connect to changes
            connectN(instanceId, "signalChanged", "", "slotChanged");
        }


        void MasterDevice::onInstanceNewForSystemNow(const std::string& instanceId, const karabo::util::Hash& instanceInfo) {
            boost::mutex::scoped_lock lock(m_systemNowMutex);

            string type(getInstanceType(instanceInfo));
            string path(type + "." + instanceId);

            if (m_systemNow.has(path)) {
                KARABO_LOG_WARN << "Quick shutdown and restart of " << type << "\" " << instanceId << "\" detected, adapting...";
            }

            Hash entry;
            Hash::Node& entryNode = entry.set(path, Hash());
            for (Hash::const_iterator it = instanceInfo.begin(); it != instanceInfo.end(); ++it) {
                entryNode.setAttribute(it->getKey(), it->getValueAsAny());
            }

            // Fill configuration for devices
            if (type == "device") {
                Schema description;
                fetchDescription(instanceId, description);
                Hash configuration;
                fetchConfiguration(instanceId, configuration);
                entryNode.setValue(Hash("description", description, "configuration", configuration));
            }

            if (type == "server") {
                KARABO_LOG_INFO << "New server from host \"" << instanceInfo.get<string>("host") << "\" wants to register with id \"" << instanceId << "\"";

            }

            m_systemNow.merge(entry);
        }


        std::string MasterDevice::getInstanceType(const karabo::util::Hash& instanceInfo) const {
            boost::optional<const Hash::Node&> node = instanceInfo.find("type");
            string type("unknown");
            if (node) type = node->getValue<string>();
            return type;
        }


        void MasterDevice::fetchConfiguration(const std::string& deviceId, karabo::util::Hash& configuration) const {
            try {
                request(deviceId, "slotGetConfiguration").timeout(2000).receive(configuration);
            } catch (const TimeoutException&) {
                KARABO_LOG_FRAMEWORK_ERROR << "Configuration request for device \"" << deviceId << "\" timed out";
                Exception::clearTrace();
            }
        }


        void MasterDevice::fetchDescription(const std::string& deviceId, karabo::util::Schema& description) const {
            try {
                request(deviceId, "slotGetSchema", false).timeout(2000).receive(description); // Retrieves active schema
            } catch (const TimeoutException&) {
                KARABO_LOG_FRAMEWORK_ERROR << "Schema request for device \"" << deviceId << "\" timed out";
                Exception::clearTrace();
            }
        }


        void MasterDevice::onInstanceNewForSystemHistory(const std::string& instanceId, const karabo::util::Hash& instanceInfo) {
            boost::mutex::scoped_lock lock(m_systemHistoryMutex);
            if (getInstanceType(instanceInfo) == "device") {
                string path("device." + instanceId);
                if (!m_systemHistory.has(path)) {

                    const Schema& description = m_systemNow.get<Schema>(path + ".description");
                    const Hash& hash = m_systemNow.get<Hash>(path + ".configuration");
                    Hash configuration;
                    for (Hash::const_iterator it = hash.begin(); it != hash.end(); ++it) {
                        Hash val("v", it->getValueAsAny());
                        val.setAttributes("v", it->getAttributes());
                        configuration.set<vector<Hash> >(it->getKey(), vector<Hash>(1, val));
                    }
                    Hash tmp("description", vector<Hash>(1, Hash("v", description)), "configuration", configuration);
                    tmp.setAttribute("description", "t", karabo::util::Timestamp().getMsSinceEpoch());
                    tmp.setAttribute("configuration", "t", karabo::util::Timestamp().getMsSinceEpoch());

                    m_systemHistory.set(path, tmp);
                }
            }
        }


        void MasterDevice::slotInstanceGone(const std::string& instanceId, const karabo::util::Hash& instanceInfo) {
            onInstanceGoneForSystemNow(instanceId, instanceInfo);
            onInstanceGoneForSystemHistory(instanceId, instanceInfo);
        }


        void MasterDevice::instanceNotAvailable(const std::string& instanceId) {
            m_systemNowMutex.lock();
            Hash fakeInstanceInfo;
            for (Hash::const_iterator it = m_systemNow.begin(); it != m_systemNow.end(); ++it) {
                if (m_systemNow.has(it->getKey() + "." + instanceId)) {
                    fakeInstanceInfo.set("type", it->getKey());
                    m_systemNowMutex.unlock();
                    call("*", "slotInstanceGone", instanceId, fakeInstanceInfo);
                    break;
                }
            }
        }


        void MasterDevice::onInstanceGoneForSystemNow(const std::string& instanceId, const karabo::util::Hash& instanceInfo) {
            boost::mutex::scoped_lock lock(m_systemNowMutex);
            boost::optional<const Hash::Node&> node = instanceInfo.find("type");
            string type = "unknown";
            if (node) type = node->getValue<string>();
            string path(type + "." + instanceId);
            if (m_systemNow.has(path)) {
                m_systemNow.erase(path);
                KARABO_LOG_DEBUG << "Removed " << type << " \"" << instanceId << "\" from system topology";
            } else {
                KARABO_LOG_WARN << "Saw " << type << " \"" << instanceId << "\" being destroyed, which was not known before...";
            }
        }


        void MasterDevice::onInstanceGoneForSystemHistory(const std::string& instanceId, const karabo::util::Hash& instanceInfo) {
            boost::mutex::scoped_lock lock(m_systemHistoryMutex);
            if (getInstanceType(instanceInfo) == "device") {
                KARABO_LOG_DEBUG << "Tagging device \"" << instanceId << "\" for being discontinued...";
                string path("device." + instanceId + ".configuration");
                if (m_systemHistory.has(path) && !m_systemHistory.get<Hash>(path).empty()) {
                    KARABO_LOG_DEBUG << "Still in memory";
                    Hash& tmp = m_systemHistory.get<Hash>(path);
                    for (Hash::const_iterator it = tmp.begin(); it != tmp.end(); ++it) {
                        vector<Hash>& keyHistory = it->getValue<vector<Hash> >();
                        Hash lastEntry = keyHistory.back();
                        lastEntry.setAttribute("v", "t", karabo::util::Timestamp().getMsSinceEpoch());
                        lastEntry.setAttribute("v", "isLast", true);
                        keyHistory.push_back(lastEntry);
                    }
                } else { // Need to fetch from file
                    boost::filesystem::path filePath("karaboHistory/" + instanceId + ".xml");
                    if (boost::filesystem::exists(filePath)) {
                        KARABO_LOG_DEBUG << "Fetching back from file";
                        Hash deviceHistory;
                        loadFromFile(deviceHistory, filePath.string());
                        Hash& tmp = deviceHistory.get<Hash>("configuration");
                        for (Hash::const_iterator it = tmp.begin(); it != tmp.end(); ++it) {
                            vector<Hash>& keyHistory = it->getValue<vector<Hash> >();
                            Hash lastEntry = keyHistory.back();
                            lastEntry.setAttribute("v", "t", karabo::util::Timestamp().getMsSinceEpoch());
                            lastEntry.setAttribute("v", "isLast", true);
                            keyHistory.push_back(lastEntry);
                        }
                        saveToFile(deviceHistory, filePath.string(), Hash("format.Xml.indentation", 1));
                    }
                }
            }
        }


        void MasterDevice::slotChanged(const karabo::util::Hash& changedConfig, const std::string& deviceId) {
            cout << "slotChanged" << endl;
            boost::mutex::scoped_lock lock(m_systemHistoryMutex);
            string path("device." + deviceId + ".configuration");
            if (m_systemHistory.has(path)) {
                Hash& tmp = m_systemHistory.get<Hash>(path);
                for (Hash::const_iterator it = changedConfig.begin(); it != changedConfig.end(); ++it) {
                    Hash val("v", it->getValueAsAny());
                    val.setAttributes("v", it->getAttributes());
                    boost::optional<Hash::Node&> node = tmp.find(it->getKey());
                    cout << val << endl;
                    if (node) node->getValue<vector<Hash> >().push_back(val);
                    else tmp.set(it->getKey(), std::vector<Hash>(1, val));
                }
            } else {
                KARABO_LOG_WARN << "Could not find: " << path << " in " << m_systemHistory;
            }
        }


        void MasterDevice::persistDataThread() {

            while (m_persistData) {

                m_systemHistoryMutex.lock();
                Hash& tmp = m_systemHistory.get<Hash>("device");
                for (Hash::iterator it = tmp.begin(); it != tmp.end(); ++it) { // Loops deviceIds
                    const string& deviceId = it->getKey();
                    if (!it->getValue<Hash>().get<Hash>("configuration").empty()) {
                        boost::filesystem::path filePath("karaboHistory/" + deviceId + ".xml");
                        if (boost::filesystem::exists(filePath)) {
                            // Read - Merge - Write
                            Hash& current = it->getValue<Hash>();
                            Hash hist;
                            loadFromFile(hist, filePath.string());
                            hist.merge(current, karabo::util::Hash::MERGE_ATTRIBUTES);
                            saveToFile(hist, filePath.string(), Hash("format.Xml.indentation", 1));
                        } else {
                            // Write
                            saveToFile(it->getValue<Hash>(), filePath.string());
                        }
                        // Release memory
                        it->setValue(Hash("description", vector<Hash>(), "configuration", Hash()));
                    }
                }
                m_systemHistoryMutex.unlock();
                boost::this_thread::sleep(boost::posix_time::seconds(10));
            }
        }
    }
}
