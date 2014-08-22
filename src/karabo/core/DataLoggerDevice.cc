/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include <karabo/io/Input.hh>
#include "DataLoggerDevice.hh"
#include "karabo/io/FileTools.hh"

namespace karabo {
    namespace core {

        using namespace krb_log4cpp;
        using namespace std;
        using namespace karabo::util;
        using namespace karabo::io;


        KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<OkErrorFsm>, DataLoggerDevice)

        void DataLoggerDevice::expectedParameters(Schema& expected) {

            INT32_ELEMENT(expected).key("flushInterval")
                    .displayedName("Flush interval")
                    .description("The interval after which the memory accumulated data is made persistent")
                    .unit(Unit::SECOND)
                    .assignmentOptional().defaultValue(40)
                    .reconfigurable()
                    .commit();
            
            OVERWRITE_ELEMENT(expected).key("visibility")
                    .setNewDefaultValue(5)
                    .commit();
        }


        DataLoggerDevice::DataLoggerDevice(const Hash& input) : Device<OkErrorFsm>(input) {
            setupSlots();
            // Initialize the memory data structure (currently only devices are supported)
            m_systemHistory.set("device", Hash());
        }


        DataLoggerDevice::~DataLoggerDevice() {
            m_persistData = false;
            m_persistDataThread.join();
        }


        void DataLoggerDevice::setupSlots() {
            SLOT2(slotChanged, Hash /*changedConfig*/, string /*deviceId*/);
        }


        void DataLoggerDevice::okStateOnEntry() {
            // Register handlers
            remote().registerInstanceNewMonitor(boost::bind(&karabo::core::DataLoggerDevice::instanceNewHandler, this, _1));
            //remote().registerInstanceUpdatedMonitor(boost::bind(&karabo::core::DataLoggerDevice::instanceUpdatedHandler, this, _1));
            remote().registerInstanceGoneMonitor(boost::bind(&karabo::core::DataLoggerDevice::instanceGoneHandler, this, _1));
            // Follow changes
            const Hash& systemTopology = remote().getSystemTopology(); // Get all current instances in the system
            boost::optional<const Hash::Node&> node = systemTopology.find("device");
            if (node) {
                const Hash& devices = node->getValue<Hash>();
                for (Hash::const_iterator it = devices.begin(); it != devices.end(); ++it) {
                    if (it->hasAttribute("archive") && (it->getAttribute<bool>("archive") == false)) continue;
                    const std::string& deviceId = it->getKey();
                    if (deviceId == m_instanceId) continue;
                    createDeviceEntry(deviceId);
                    connectT(deviceId, "signalChanged", "", "slotChanged");
                }
            }
            // Prepare backend to persist data
            if (!boost::filesystem::exists("karaboHistory")) {
                boost::filesystem::create_directory("karaboHistory");
            }
            // Start persisting
            m_persistData = true;
            m_persistDataThread = boost::thread(boost::bind(&karabo::core::DataLoggerDevice::persistDataThread, this));
        }


        void DataLoggerDevice::instanceNewHandler(const karabo::util::Hash& topologyEntry) {
            boost::mutex::scoped_lock lock(m_systemHistoryMutex);
             KARABO_LOG_DEBUG << "instanceNewHandler";
            const std::string& type = topologyEntry.begin()->getKey();
            if (type == "device") {
                const Hash& entry = topologyEntry.begin()->getValue<Hash>();
                const string& deviceId = entry.begin()->getKey();
                if (entry.hasAttribute(deviceId, "archive") && (entry.getAttribute<bool>(deviceId, "archive") == false)) return;
                
                if (m_systemHistory.has("device." + deviceId)) {
                    // Handle dirty shutdown -> flag last existing entry to be last
                    connectT(deviceId, "signalChanged", "", "slotChanged");
                } else {
                    KARABO_LOG_DEBUG << "Registered new device \"" << deviceId << "\" for archiving";
                    createDeviceEntry(deviceId);
                    connectT(deviceId, "signalChanged", "", "slotChanged");
                }
            }
        }


        void DataLoggerDevice::createDeviceEntry(const std::string& deviceId) {
            Schema schema;
            this->fetchSchema(deviceId, schema);
            Hash hash;
            this->fetchConfiguration(deviceId, hash);

            Hash configuration;
            for (Hash::const_iterator it = hash.begin(); it != hash.end(); ++it) {
                Hash val("v", it->getValueAsAny());
                val.setAttributes("v", it->getAttributes());
                configuration.set<vector<Hash> >(it->getKey(), vector<Hash>(1, val));
            }
            Hash tmp("schema", vector<Hash>(1, Hash("v", schema)), "configuration", configuration);
            Timestamp().toHashAttributes(tmp.getAttributes("schema"));
            Timestamp().toHashAttributes(tmp.getAttributes("configuration"));
            m_systemHistory.set("device." + deviceId, tmp);
        }


        void DataLoggerDevice::fetchConfiguration(const std::string& deviceId, karabo::util::Hash& configuration) const {
            try {
                request(deviceId, "slotGetConfiguration").timeout(2000).receive(configuration);
            } catch (const TimeoutException&) {
                KARABO_LOG_FRAMEWORK_ERROR << "Configuration request for device \"" << deviceId << "\" timed out";
                Exception::clearTrace();
            }
        }


        void DataLoggerDevice::fetchSchema(const std::string& deviceId, karabo::util::Schema& schema) const {
            try {
                request(deviceId, "slotGetSchema", false).timeout(2000).receive(schema); // Retrieves full schema
            } catch (const TimeoutException&) {
                KARABO_LOG_FRAMEWORK_ERROR << "Schema request for device \"" << deviceId << "\" timed out";
                Exception::clearTrace();
            }
        }


        void DataLoggerDevice::instanceGoneHandler(const std::string& instanceId) {
            boost::mutex::scoped_lock lock(m_systemHistoryMutex);
            string path("device." + instanceId + ".configuration");
            if (m_systemHistory.has(path)) {
                KARABO_LOG_DEBUG << "Tagging device \"" << instanceId << "\" for being discontinued...";
                Hash& deviceEntry = m_systemHistory.get<Hash>(path);
                if (deviceEntry.empty()) { // Need to fetch from file
                    boost::filesystem::path filePath("karaboHistory/" + instanceId + ".xml");
                    if (boost::filesystem::exists(filePath)) {
                        KARABO_LOG_DEBUG << "Fetching back from file";
                        Hash deviceHistory;
                        loadFromFile(deviceHistory, filePath.string());
                        Hash& tmp = deviceHistory.get<Hash>("configuration");
                        createLastValidConfiguration(tmp);
                        saveToFile(deviceHistory, filePath.string(), Hash("format.Xml.indentation", 1));
                    }
                } else { // Still in memory
                    KARABO_LOG_DEBUG << "Data still resides in memory";
                    Hash& tmp = m_systemHistory.get<Hash>(path);
                    createLastValidConfiguration(tmp);
                }
            }
        }


        void DataLoggerDevice::createLastValidConfiguration(karabo::util::Hash& tmp) {
            for (Hash::iterator it = tmp.begin(); it != tmp.end(); ++it) {
                vector<Hash>& keyHistory = it->getValue<vector<Hash> >();
                Hash lastEntry = keyHistory.back();
                karabo::util::Timestamp().toHashAttributes(lastEntry.getAttributes("v"));
                lastEntry.setAttribute("v", "isLast", true);
                keyHistory.push_back(lastEntry);
            }
        }


        void DataLoggerDevice::slotChanged(const karabo::util::Hash& changedConfig, const std::string& deviceId) {
            boost::mutex::scoped_lock lock(m_systemHistoryMutex);
            string path("device." + deviceId + ".configuration");
            if (m_systemHistory.has(path)) {
                
                // Get schema for this device
                Schema schema = remote().getDeviceSchema(deviceId);
                Hash& tmp = m_systemHistory.get<Hash>(path);
                for (Hash::const_iterator it = changedConfig.begin(); it != changedConfig.end(); ++it) {
                    if (schema.hasArchivePolicy(it->getKey()) && (schema.getArchivePolicy(it->getKey()) == Schema::NO_ARCHIVING)) continue;
                    Hash val("v", it->getValueAsAny());
                    val.setAttributes("v", it->getAttributes());
                    boost::optional<Hash::Node&> node = tmp.find(it->getKey());
                    //KARABO_LOG_FRAMEWORK_DEBUG << val;
                    if (node) node->getValue<vector<Hash> >().push_back(val);
                    else tmp.set(it->getKey(), std::vector<Hash>(1, val));
                }
            } else {
                KARABO_LOG_WARN << "Could not find: " << path << " in " << m_systemHistory;
            }
        }


        void DataLoggerDevice::persistDataThread() { //

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
                            saveToFile(hist, filePath.string(), Hash("format.Xml.indentation", -1));
                        } else {
                            // Write
                            saveToFile(it->getValue<Hash>(), filePath.string());
                        }
                        // Release memory
                        it->setValue(Hash("schema", vector<Hash>(), "configuration", Hash()));
                    }
                }
                m_systemHistoryMutex.unlock();
                boost::this_thread::sleep(boost::posix_time::seconds(this->get<int>("flushInterval")));
            }
        }
    }
}
