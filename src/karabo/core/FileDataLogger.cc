/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include <karabo/io/Input.hh>
#include "FileDataLogger.hh"
#include "karabo/io/FileTools.hh"

namespace karabo {
    namespace core {

        using namespace log4cpp;
        using namespace std;
        using namespace karabo::util;
        using namespace karabo::io;


        KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<OkErrorFsm>, FileDataLogger)

        void FileDataLogger::expectedParameters(Schema& expected) {

            INT32_ELEMENT(expected).key("flushInterval")
                    .displayedName("Flush interval")
                    .description("The interval after which the memory accumulated data is made persistent")
                    .unit(Unit::SECOND)
                    .assignmentOptional().defaultValue(40)
                    .reconfigurable()
                    .commit();

            STRING_ELEMENT(expected).key("fileFormat")
                    .displayedName("File format")
                    .description("The file format to use for logging")
                    .options("xml, bin, hdf5")
                    .assignmentOptional().defaultValue("bin")
                    .commit();

            INT32_ELEMENT(expected).key("maximumFileSize")
                    .displayedName("Maximum file size")
                    .description("After any archived file has reached this size it will be time-stamped and not appended anymore")
                    .unit(Unit::BYTE)
                    .metricPrefix(MetricPrefix::MEGA)
                    .reconfigurable()
                    .assignmentOptional().defaultValue(80)
                    .commit();

            OVERWRITE_ELEMENT(expected).key("visibility")
                    .setNewDefaultValue(5)
                    .commit();
        }


        FileDataLogger::FileDataLogger(const Hash& input) : Device<OkErrorFsm>(input) {

            SLOT2(slotChanged, Hash /*changedConfig*/, string /*deviceId*/);
            SLOT4(slotGetFromPast, string /*deviceId*/, string /*key*/, string /*from*/, string /*to*/);

            // Initialize the memory data structure (currently only devices are supported)
            m_systemHistory.set("device", Hash());
        }


        FileDataLogger::~FileDataLogger() {
            m_persistData = false;
            m_persistDataThread.join();
        }


        void FileDataLogger::okStateOnEntry() {
            // Register handlers
            remote().registerInstanceNewMonitor(boost::bind(&karabo::core::FileDataLogger::instanceNewHandler, this, _1));
            //remote().registerInstanceUpdatedMonitor(boost::bind(&karabo::core::FileDataLogger::instanceUpdatedHandler, this, _1));
            remote().registerInstanceGoneMonitor(boost::bind(&karabo::core::FileDataLogger::instanceGoneHandler, this, _1));
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
            m_persistDataThread = boost::thread(boost::bind(&karabo::core::FileDataLogger::persistDataThread, this));
        }


        void FileDataLogger::instanceNewHandler(const karabo::util::Hash& topologyEntry) {
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


        void FileDataLogger::createDeviceEntry(const std::string& deviceId) {
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


        void FileDataLogger::fetchConfiguration(const std::string& deviceId, karabo::util::Hash& configuration) const {
            try {
                request(deviceId, "slotGetConfiguration").timeout(5000).receive(configuration);
            } catch (const TimeoutException&) {
                KARABO_LOG_FRAMEWORK_ERROR << "Configuration request for device \"" << deviceId << "\" timed out";
                Exception::clearTrace();
            }
        }


        void FileDataLogger::fetchSchema(const std::string& deviceId, karabo::util::Schema& schema) const {
            try {
                request(deviceId, "slotGetSchema", false).timeout(5000).receive(schema); // Retrieves full schema
            } catch (const TimeoutException&) {
                KARABO_LOG_FRAMEWORK_ERROR << "Schema request for device \"" << deviceId << "\" timed out";
                Exception::clearTrace();
            }
        }


        void FileDataLogger::instanceGoneHandler(const std::string& instanceId) {
            boost::mutex::scoped_lock lock(m_systemHistoryMutex);
            string path("device." + instanceId + ".configuration");
            if (m_systemHistory.has(path)) {
                KARABO_LOG_DEBUG << "Tagging device \"" << instanceId << "\" for being discontinued...";
                Hash& deviceEntry = m_systemHistory.get<Hash>(path);
                if (deviceEntry.empty()) { // Need to fetch from file
                    boost::filesystem::path filePath("karaboHistory/" + instanceId + "." + get<string>("fileFormat"));
                    if (boost::filesystem::exists(filePath)) {
                        KARABO_LOG_DEBUG << "Fetching back from file";
                        Hash deviceHistory;
                        loadFromFile(deviceHistory, filePath.string());
                        Hash& tmp = deviceHistory.get<Hash>("configuration");
                        createLastValidConfiguration(tmp);
                        saveToFile(deviceHistory, filePath.string());
                    }
                } else { // Still in memory
                    KARABO_LOG_DEBUG << "Data still resides in memory";
                    Hash& tmp = m_systemHistory.get<Hash>(path);
                    createLastValidConfiguration(tmp);
                }
            }
        }


        void FileDataLogger::createLastValidConfiguration(karabo::util::Hash& tmp) {
            for (Hash::iterator it = tmp.begin(); it != tmp.end(); ++it) {
                vector<Hash>& keyHistory = it->getValue<vector<Hash> >();
                Hash lastEntry = keyHistory.back();
                karabo::util::Timestamp().toHashAttributes(lastEntry.getAttributes("v"));
                lastEntry.setAttribute("v", "isLast", true);
                keyHistory.push_back(lastEntry);
            }
        }


        void FileDataLogger::slotChanged(const karabo::util::Hash& changedConfig, const std::string& deviceId) {
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
                    if (node) node->getValue<vector<Hash> >().push_back(val);
                    else tmp.set(it->getKey(), std::vector<Hash>(1, val));
                }
            } else {
                KARABO_LOG_WARN << "Could not find: " << path << " in " << m_systemHistory;
            }
        }


        void FileDataLogger::persistDataThread() { //

            while (m_persistData) {

                m_systemHistoryMutex.lock();
                Hash& tmp = m_systemHistory.get<Hash>("device");
                for (Hash::iterator it = tmp.begin(); it != tmp.end(); ++it) { // Loops deviceIds
                    const string& deviceId = it->getKey();
                    if (!it->getValue<Hash>().get<Hash>("configuration").empty()) {
                        boost::filesystem::path filePath("karaboHistory/" + deviceId + "." + get<string>("fileFormat"));
                        if (boost::filesystem::exists(filePath)) {
                            // Check file size
                            if (boost::filesystem::file_size(filePath) > (get<int>("maximumFileSize") * 1E6)) {
                                int i = 0;
                                while (boost::filesystem::exists(boost::filesystem::path("karaboHistory/" + deviceId + "_" + karabo::util::toString(i) + "." + get<string>("fileFormat")))) {
                                    i++;
                                }
                                boost::filesystem::path tmp("karaboHistory/" + deviceId + "_" + karabo::util::toString(i) + "." + get<string>("fileFormat"));
                                boost::filesystem::rename(filePath, tmp);
                                saveToFile(it->getValue<Hash>(), filePath.string());
                            } else {
                                // Read - Merge - Write
                                Hash& current = it->getValue<Hash>();
                                Hash hist;
                                loadFromFile(hist, filePath.string());
                                hist.merge(current, karabo::util::Hash::MERGE_ATTRIBUTES);
                                saveToFile(hist, filePath.string());
                            }
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


        vector<Hash> FileDataLogger::slotGetFromPast(const std::string& deviceId, const std::string& key, const std::string& from, const std::string& to) {
            KARABO_LOG_FRAMEWORK_DEBUG << "slotGetFromPast()";
            vector<Hash> result;
            try {
                Epochstamp t0(from);
                Epochstamp t1(to);
                KARABO_LOG_FRAMEWORK_DEBUG << "From: " << from << " <-> " << t0.toFormattedString();
                KARABO_LOG_FRAMEWORK_DEBUG << "To:   " << to << " <-> " << t1.toFormattedString();
                boost::mutex::scoped_lock lock(m_systemHistoryMutex);

                vector<Hash> tmp = getData(deviceId, key);
                bool collect = false;
                bool done = false;
                //bool isLastFlagIsUp = false; // TODO continue here
                for (vector<Hash>::reverse_iterator rit = tmp.rbegin(); rit != tmp.rend(); ++rit) {
                    //KARABO_LOG_FRAMEWORK_DEBUG << *rit;
                    Epochstamp current;
                    try {
                       current = Epochstamp::fromHashAttributes(rit->getAttributes("v"));
                    } catch (Exception& e) {
                        // TODO Clean this
                        continue;
                    }
                    KARABO_LOG_FRAMEWORK_DEBUG << "Current:   " << current.toFormattedString();
                    if (t1 > current) collect = true;
                    if (collect) result.push_back(*rit);
                    if (t0 > current) {
                        done = true;
                        break;
                    }
                }
                if (!done) { // Puuh! Go further back!
                    // Not yet there!!
                }
                reply(result);    
                return result;
            } catch (Exception& e) {
                KARABO_LOG_ERROR << e.userFriendlyMsg();
            }
            return vector<Hash>();
        }


        vector<Hash> FileDataLogger::getData(const std::string& deviceId, const std::string& key) {
            vector<Hash> data;
            string memoryPath("device." + deviceId + ".configuration." + key);
            boost::filesystem::path filePath("karaboHistory/" + deviceId + "." + get<string>("fileFormat"));
            
            if (boost::filesystem::exists(filePath)) {
                // Read file
                Hash file;
                loadFromFile(file, filePath.string());
                if (file.has("configuration." + key)) {
                    const vector<Hash>& tmp = file.get<vector<Hash> >("configuration." + key);
                    data.insert(data.end(), tmp.begin(), tmp.end());
                }
            }
            if (m_systemHistory.has(memoryPath)) {
                const vector<Hash>& tmp = m_systemHistory.get<vector<Hash> >(memoryPath);
                data.insert(data.end(), tmp.begin(), tmp.end());
            }
            return data;
        }
    }
}
