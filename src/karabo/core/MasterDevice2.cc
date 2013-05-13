/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include <karabo/io/Input.hh>
#include "MasterDevice2.hh"
#include "DeviceServer.hh"
#include "karabo/io/FileTools.hh"
#include "karabo/tests/util/Timestamp_Test.hh"

namespace karabo {
    namespace core {

        using namespace log4cpp;

        using namespace std;
        using namespace karabo::util;
        using namespace karabo::io;


        KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<OkErrorFsm>, MasterDevice2)

        void MasterDevice2::expectedParameters(Schema& expected) {
        }


        MasterDevice2::MasterDevice2(const Hash& input) : Device<OkErrorFsm>(input) {
            setupSlots();
            m_systemArchive.set("device", Hash());
        }


        MasterDevice2::~MasterDevice2() {
            m_persistData = false;
            m_persistDataThread.join();
        }


        void MasterDevice2::setupSlots() {
            GLOBAL_SLOT2(slotInstanceUpdated, string /*instanceId*/, Hash /*instanceInfo*/);
            GLOBAL_SLOT2(slotInstanceGone, string /*instanceId*/, Hash /*instanceInfo*/);
            SLOT2(slotChanged, Hash /*changedConfig*/, string /*deviceId*/);
        }


        void MasterDevice2::okStateOnEntry() {
            if (!boost::filesystem::exists("karaboHistory")) {
                boost::filesystem::create_directory("karaboHistory");
            }
            m_persistData = true;
            m_persistDataThread = boost::thread(boost::bind(&karabo::core::MasterDevice2::persistDataThread, this));
        }


        void MasterDevice2::cacheAvailableInstances() {
            vector<pair<string, Hash> > instances = getAvailableInstances();
            for (size_t i = 0; i < instances.size(); ++i) {
                const string& instanceId = instances[i].first;
                const Hash& instanceInfo = instances[i].second;

                // Start tracking
                trackExistenceOfInstance(instanceId);

                // Update runtime cache
                handleInstanceUpdateForSystemTopology(instanceId, instanceInfo);

                if (instanceInfo.has("type")) {
                    string type(instanceInfo.get<std::string>("type"));
                    if (type == "device") {
                        handleDeviceInstanceUpdateForSystemArchive(instanceId);
                    } else if (type == "server") {
                        // TODO
                    }
                }
            }
            KARABO_LOG_DEBUG << "System archive:\n" << m_systemArchive;
        }

        void MasterDevice2::slotInstanceUpdated(const std::string& instanceId, const karabo::util::Hash& instanceInfo) {

            handleInstanceUpdateForSystemTopology(instanceId, instanceInfo);

            if (instanceInfo.has("type")) {
                if (instanceInfo.get<std::string>("type") == "device") {
                    handleDeviceInstanceUpdateForSystemArchive(instanceId);
                }
            }
            
            // Start tracking
            trackExistenceOfInstance(instanceId);
            
            // Connect to changes
            connectN(instanceId, "signalChanged", "", "slotChanged");
        }
        
         void MasterDevice2::handleInstanceUpdateForSystemTopology(const std::string& instanceId, const karabo::util::Hash& instanceInfo) {
            boost::mutex::scoped_lock lock(m_runtimeSystemTopologyMutex);
            boost::optional<const Hash::Node&> node = instanceInfo.find("type");
            string type = "unknown";
            if (node) type = node->getValue<string>();
            Hash entry;
            Hash::Node& entryNode = entry.set(type + "." + instanceId, Hash());
            for (Hash::const_iterator it = instanceInfo.begin(); it != instanceInfo.end(); ++it) {
                entryNode.setAttribute(it->getKey(), it->getValueAsAny());
            }
            m_runtimeSystemTopology.merge(entry);
        }


        void MasterDevice2::handleDeviceInstanceUpdateForSystemArchive(const std::string& deviceId) {
            boost::mutex::scoped_lock lock(m_systemArchiveMutex);
            string path("device." + deviceId);
            if (!m_systemArchive.has(path)) {

                Hash hash;
                try {
                    request(deviceId, "slotRefresh").timeout(2000).receive(hash);
                } catch (const TimeoutException&) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Configuration request for device \"" << deviceId << "\" timed out";
                    Exception::clearTrace();
                }
                Hash configuration;
                for (Hash::const_iterator it = hash.begin(); it != hash.end(); ++it) {
                    Hash val("v", it->getValueAsAny());
                    val.setAttributes("v", it->getAttributes());
                    configuration.set<vector<Hash> >(it->getKey(), vector<Hash>(1, val));
                }
                Schema schema;
                try {
                    request(deviceId, "slotGetSchema", true).timeout(2000).receive(schema); // Retrieves active schema
                } catch (const TimeoutException&) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Schema request for device \"" << deviceId << "\" timed out";
                    Exception::clearTrace();
                }
                Hash tmp("description", vector<Hash>(1, Hash("v", schema)), "configuration", configuration);
                tmp.setAttribute("configuration", "t", karabo::util::Timestamp().getMsSinceEpoch());
                tmp.setAttribute("description", "t", karabo::util::Timestamp().getMsSinceEpoch());

                m_systemArchive.set(path, tmp);
            }
        }

        void MasterDevice2::slotInstanceGone(const std::string& instanceId, const karabo::util::Hash& instanceInfo) {
            handleInstanceGoneForSystemTopology(instanceId, instanceInfo);
             if (instanceInfo.has("type")) {
                if (instanceInfo.get<std::string>("type") == "device") {
                    handleDeviceInstanceGoneForSystemArchive(instanceId);
                }
            }
        }
        
        void MasterDevice2::instanceNotAvailable(const std::string& instanceId) {
            m_runtimeSystemTopologyMutex.lock();
            Hash fakeInstanceInfo;
            for (Hash::const_iterator it = m_runtimeSystemTopology.begin(); it != m_runtimeSystemTopology.end(); ++it) {
                if (m_runtimeSystemTopology.has(it->getKey() + "." + instanceId)) {
                    fakeInstanceInfo.set("type", it->getKey());
                    m_runtimeSystemTopologyMutex.unlock();
                    call("*", "slotInstanceGone", instanceId, fakeInstanceInfo);
                    break;
                }
            }
        }
        
        void MasterDevice2::handleInstanceGoneForSystemTopology(const std::string& instanceId, const karabo::util::Hash& instanceInfo) {
            boost::mutex::scoped_lock lock(m_runtimeSystemTopologyMutex);
            boost::optional<const Hash::Node&> node = instanceInfo.find("type");
            string type = "unknown";
            if (node) type = node->getValue<string>();
            string path(type + "." + instanceId);
            if (m_runtimeSystemTopology.has(path)) {
                m_runtimeSystemTopology.erase(path);
                KARABO_LOG_DEBUG << "Removed " << type << " \"" << instanceId << "\" from system topology";
            } else {
                KARABO_LOG_WARN << "Saw " << type << " \"" << instanceId << "\" being destroyed, which was not known before...";
            }
        }
        
        void MasterDevice2::handleDeviceInstanceGoneForSystemArchive(const std::string& deviceId) {
            boost::mutex::scoped_lock lock(m_systemArchiveMutex);
            KARABO_LOG_DEBUG << "Tagging device \"" << deviceId << "\" for being discontinued...";
            string path("device." + deviceId + ".configuration");
            if (m_systemArchive.has(path) && !m_systemArchive.get<Hash>(path).empty()) {
                KARABO_LOG_DEBUG << "Still in memory";
                Hash& tmp = m_systemArchive.get<Hash>(path);
                for (Hash::const_iterator it = tmp.begin(); it != tmp.end(); ++it) {
                    vector<Hash>& keyHistory = it->getValue<vector<Hash> >();
                    Hash lastEntry = keyHistory.back();
                    lastEntry.setAttribute("v", "t", karabo::util::Timestamp().getMsSinceEpoch());
                    lastEntry.setAttribute("v", "isLast", true);
                    keyHistory.push_back(lastEntry);
                }
            } else { // Need to fetch from file
                boost::filesystem::path filePath("karaboHistory/" + deviceId + ".xml");
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


        void MasterDevice2::slotChanged(const karabo::util::Hash& changedConfig, const std::string& deviceId) {
            cout << "slotChanged" << endl;
            boost::mutex::scoped_lock lock(m_systemArchiveMutex);
            string path("device." + deviceId + ".configuration");
            if (m_systemArchive.has(path)) {
                Hash& tmp = m_systemArchive.get<Hash>(path);
                for (Hash::const_iterator it = changedConfig.begin(); it != changedConfig.end(); ++it) {
                    Hash val("v", it->getValueAsAny());
                    val.setAttributes("v", it->getAttributes());
                    boost::optional<Hash::Node&> node = tmp.find(it->getKey());
                    cout << val << endl;
                    if (node) node->getValue<vector<Hash> >().push_back(val);
                    else tmp.set(it->getKey(), std::vector<Hash>(1, val));
                }
            } else {
                KARABO_LOG_WARN << "Could not find: " << path << " in " << m_systemArchive;
            }
        }


        void MasterDevice2::persistDataThread() {
            
            while (m_persistData) {

                m_systemArchiveMutex.lock();
                Hash& tmp = m_systemArchive.get<Hash>("device");
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
                m_systemArchiveMutex.unlock();
                boost::this_thread::sleep(boost::posix_time::seconds(10));
            }
        }
    }
}
