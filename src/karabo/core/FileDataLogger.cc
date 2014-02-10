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
                    .assignmentOptional().defaultValue(10)
                    .commit();

            FLOAT_ELEMENT(expected).key("lastFlushDuration")
                    .displayedName("Last flush duration")
                    .description("Time needed for the last flush")
                    .unit(Unit::SECOND)
                    .readOnly()
                    .commit();

            OVERWRITE_ELEMENT(expected).key("visibility")
                    .setNewDefaultValue(5)
                    .commit();
        }


        FileDataLogger::FileDataLogger(const Hash& input) : Device<OkErrorFsm>(input) {

            // Initialize the memory data structure (currently only devices are supported)
            m_systemHistory.set("device", Hash());
        }


        FileDataLogger::~FileDataLogger() {
            m_persistData = false;
            m_persistDataThread.join();
        }


        void FileDataLogger::okStateOnEntry() {

            // Turn off ageing
            remote().setAgeing(false);

            // Register handlers
            remote().registerInstanceNewMonitor(boost::bind(&karabo::core::FileDataLogger::instanceNewHandler, this, _1));
            //remote().registerInstanceUpdatedMonitor(boost::bind(&karabo::core::FileDataLogger::instanceUpdatedHandler, this, _1));
            remote().registerInstanceGoneMonitor(boost::bind(&karabo::core::FileDataLogger::instanceGoneHandler, this, _1));

            // Prepare backend to persist data
            if (!boost::filesystem::exists("karaboHistory")) {
                boost::filesystem::create_directory("karaboHistory");
            }

            // Follow changes
            const Hash& systemTopology = remote().getSystemTopology(); // Get all current instances in the system
            boost::optional<const Hash::Node&> node = systemTopology.find("device");
            if (node) {
                const Hash& devices = node->getValue<Hash>();
                for (Hash::const_iterator it = devices.begin(); it != devices.end(); ++it) { // Loop all devices
                    if (it->hasAttribute("archive") && (it->getAttribute<bool>("archive") == false)) continue;
                    const std::string& deviceId = it->getKey();
                    if (deviceId == m_instanceId) continue; // Skip myself

                    ensureProperDeviceEntry(deviceId);
                    tagDeviceToBeDiscontinued(deviceId, false, 'l'); // 2nd arg means: device was not valid up to now, 3rd means logger
                    refreshDeviceInformation(deviceId);
                    connectT(deviceId, "signalChanged", "", "slotChanged");
                }
            }

            // Start persisting
            m_persistData = true;
            m_persistDataThread = boost::thread(boost::bind(&karabo::core::FileDataLogger::persistDataThread, this));

            // Start slots
            SLOT2(slotChanged, Hash /*changedConfig*/, string /*deviceId*/);
            SLOT2(slotSchemaUpdated, Schema /*description*/, string /*deviceId*/)
            SLOT4(slotGetFromPast, string /*deviceId*/, string /*key*/, string /*from*/, string /*to*/);
        }


        void FileDataLogger::instanceNewHandler(const karabo::util::Hash& topologyEntry) {

            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "instanceNewHandler";
                const std::string& type = topologyEntry.begin()->getKey();

                if (type == "device") { // Take out only devices for the time being
                    const Hash& entry = topologyEntry.begin()->getValue<Hash>();
                    const string& deviceId = entry.begin()->getKey();

                    // Skip devices that are marked to globally prevent archiving
                    if (entry.hasAttribute(deviceId, "archive") && (entry.getAttribute<bool>(deviceId, "archive") == false)) return;

                    // Make sure we maintain up a correct internal structure
                    ensureProperDeviceEntry(deviceId);

                    // This will flag any previous data to be discontinued
                    // (in case a device silently died and came back within the allowed heartbeat time)
                    tagDeviceToBeDiscontinued(deviceId, false, 'd'); // 2nd arg means: device was not valid up to now, 3rd means device

                    // Refresh any outdated information
                    refreshDeviceInformation(deviceId);

                    // Finally start listening to the changes
                    connectT(deviceId, "signalChanged", "", "slotChanged");

                }

            } catch (const Exception& e) {
                KARABO_LOG_ERROR << e;
            }
        }


        void FileDataLogger::ensureProperDeviceEntry(const std::string& deviceId) {
            KARABO_LOG_FRAMEWORK_DEBUG << "ensureProperDeviceEntry";
            boost::mutex::scoped_lock lock(m_systemHistoryMutex);

            if (!m_systemHistory.has("device." + deviceId)) {
                Hash tmp("schema", vector<Hash>(), "configuration", Hash());
                Timestamp().toHashAttributes(tmp.getAttributes("schema"));
                Timestamp().toHashAttributes(tmp.getAttributes("configuration"));
                m_systemHistory.set("device." + deviceId, tmp);
            }
        }


        void FileDataLogger::refreshDeviceInformation(const std::string& deviceId) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "refreshDeviceInformation " << deviceId;
                Schema schema = remote().getDeviceSchema(deviceId);
                Hash hash = remote().get(deviceId);

                // call slotSchemaUpdated updated by hand
                slotSchemaUpdated(schema, deviceId);

                // call slotChanged by hand
                slotChanged(hash, deviceId);

            } catch (...) {
                KARABO_RETHROW_AS(KARABO_INIT_EXCEPTION("Could not create new entry for " + deviceId));
            }
        }


        void FileDataLogger::instanceGoneHandler(const std::string& instanceId) {
            try {
                tagDeviceToBeDiscontinued(instanceId, true, 'd'); // 2nd arguments means: was valid up to now
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << e;
            }
        }


        void FileDataLogger::tagDeviceToBeDiscontinued(const std::string& deviceId, const bool wasValidUpToNow, const char reason) {
            try {

                boost::mutex::scoped_lock lock(m_systemHistoryMutex);

                string path("device." + deviceId);
                if (m_systemHistory.has(path)) {
                    KARABO_LOG_DEBUG << "Tagging device \"" << deviceId << "\" for being discontinued...";
                    Hash& deviceEntry = m_systemHistory.get<Hash>(path);
                    Hash& configuration = deviceEntry.get<Hash>("configuration");
                    vector<Hash>& schema = deviceEntry.get<vector<Hash> >("schema");

                    boost::filesystem::path filePath("karaboHistory/" + deviceId + "." + get<string>("fileFormat"));
                    if (boost::filesystem::exists(filePath)) {
                        KARABO_LOG_DEBUG << "Fetching back from file";

                        // Real file
                        boost::filesystem::path filePath("karaboHistory/" + deviceId + "." + get<string>("fileFormat"));
                        // Partial file for writing
                        boost::filesystem::path partPath("karaboHistory/" + deviceId + "-part." + get<string>("fileFormat"));

                        Hash deviceHistory;
                        loadFromFile(deviceHistory, filePath.string()); // READ
                        if (!configuration.empty() || !schema.empty()) deviceHistory.merge(deviceEntry); // MERGE
                        Hash& tmp = deviceHistory.get<Hash>("configuration");
                        createLastValidConfiguration(tmp, wasValidUpToNow, reason); // TAG                            
                        saveToFile(deviceHistory, partPath.string()); // WRITE .part
                        boost::filesystem::rename(partPath, filePath); // MOVE
                        deviceEntry.set("schema", vector<Hash>());
                        deviceEntry.set("configuration", Hash());

                    } else {
                        if (!configuration.empty() || !schema.empty()) {
                            KARABO_LOG_DEBUG << "Data resides only in memory";
                            createLastValidConfiguration(configuration, wasValidUpToNow, reason);
                        } else {
                            KARABO_LOG_DEBUG << "Encountered new device " << deviceId << " never seen before";
                        }
                    }
                }
            } catch (...) {
                KARABO_RETHROW_AS(KARABO_LOGIC_EXCEPTION("Problems tagging " + deviceId + " to be discontinued"));
            }
        }


        void FileDataLogger::appendDeviceConfigurationToFile(const std::string& deviceId, const karabo::util::Hash& deviceEntry) {
            try {

                const Hash& configuration = deviceEntry.get<Hash>("configuration");
                const vector<Hash>& schema = deviceEntry.get<vector<Hash> >("schema");

                if (!configuration.empty() || !schema.empty()) {

                    // Real file
                    boost::filesystem::path filePath("karaboHistory/" + deviceId + "." + get<string>("fileFormat"));
                    // Partial file for writing
                    boost::filesystem::path partPath("karaboHistory/" + deviceId + "-part." + get<string>("fileFormat"));

                    if (boost::filesystem::exists(filePath)) { // A file already exists

                        if (boost::filesystem::file_size(filePath) > (get<int>("maximumFileSize") * 1E6)) { // File is too large

                            KARABO_LOG_INFO << "File size for " << deviceId << " to too large, log-rotating...";

                            // Find the latest used file index
                            int i = 0;
                            while (boost::filesystem::exists(boost::filesystem::path("karaboHistory/" + deviceId + "_" + karabo::util::toString(i) + "." + get<string>("fileFormat")))) i++;

                            // Create a new file on the log-rotate index
                            boost::filesystem::path tmp("karaboHistory/" + deviceId + "_" + karabo::util::toString(i) + "." + get<string>("fileFormat"));

                            // Move the current to the just created log-rotated stack
                            boost::filesystem::rename(filePath, tmp);

                            // Write current data to file
                            saveToFile(deviceEntry, filePath.string());

                        } else { // File size is ok
                            // Read - Merge - Write (.part) - Move
                            Hash hist;
                            loadFromFile(hist, filePath.string()); // Read
                            hist.merge(deviceEntry, karabo::util::Hash::MERGE_ATTRIBUTES); // Merge
                            saveToFile(hist, partPath.string()); // Write part
                            boost::filesystem::rename(partPath, filePath); // Move
                        }
                    } else { // No file exists yet
                        // Write
                        saveToFile(deviceEntry, filePath.string());
                    }
                }
            } catch (...) {
                KARABO_RETHROW;
            }
        }


        void FileDataLogger::createLastValidConfiguration(karabo::util::Hash& tmp, const bool wasValidUpToNow, const char reason) {
            for (Hash::iterator it = tmp.begin(); it != tmp.end(); ++it) {
                vector<Hash>& keyHistory = it->getValue<vector<Hash> >();
                if (wasValidUpToNow) { // Create a last new entry
                    Hash lastEntry = keyHistory.back();
                    karabo::util::Timestamp().toHashAttributes(lastEntry.getAttributes("v"));
                    lastEntry.setAttribute("v", "isLast", reason);
                    keyHistory.push_back(lastEntry);
                } else { // Flag last seen one as latest valid
                    Hash& lastEntry = keyHistory.back();
                    lastEntry.setAttribute("v", "isLast", reason);
                }
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


        void FileDataLogger::slotSchemaUpdated(const karabo::util::Schema& schema, const std::string& deviceId) {

            boost::mutex::scoped_lock lock(m_systemHistoryMutex);

            string path("device." + deviceId + ".schema");
            if (m_systemHistory.has(path)) {
                Hash val("v", schema);
                Timestamp().toHashAttributes(val.getAttributes("v"));
                vector<Hash>& schemas = m_systemHistory.get<vector<Hash> >(path);
                schemas.push_back(val);
            } else {
                KARABO_LOG_WARN << "Could not find: " << path << " in " << m_systemHistory;
            }
        }


        void FileDataLogger::persistDataThread() {

            try {

                TimeProfiler profiler("Persist data");
                while (m_persistData) {

                    // Profile the persisting time
                    profiler.open();
                    profiler.startPeriod();
                    KARABO_LOG_FRAMEWORK_DEBUG << "Start flushing memory to file";

                    m_systemHistoryMutex.lock();

                    Hash& devices = m_systemHistory.get<Hash>("device");
                    for (Hash::iterator it = devices.begin(); it != devices.end(); ++it) { // Loops deviceIds
                        try {

                            // Persist
                            appendDeviceConfigurationToFile(it->getKey(), it->getValue<Hash>());

                            // Release memory
                            it->setValue(Hash("schema", vector<Hash>(), "configuration", Hash()));

                        } catch (const Exception& e) {
                            KARABO_LOG_ERROR << e;
                        } catch (...) {
                            KARABO_LOG_ERROR << "Encountered unknown exception whilst persisting data for instance " << it->getKey();
                        }
                    }

                    profiler.close();
                    profiler.stopPeriod();
                    set("lastFlushDuration", profiler.getPeriod().getDuration().getSeconds());

                    m_systemHistoryMutex.unlock();
                    boost::this_thread::sleep(boost::posix_time::seconds(this->get<int>("flushInterval")));
                }
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << e;
            } catch (...) {
                KARABO_LOG_ERROR << "Encountered unknown exception whilst persisting karabo device data to file";
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
                    if (t1 > current) collect = true; // Time until is bigger then current timestamp, so collect
                    if (collect) result.push_back(*rit);
                    if (t0 > current) { // Time from is now bigger than current flag -> we are done
                        done = true;
                        break;
                    }
                }
                if (!done) { // Puuh! Go further back!
                    KARABO_LOG_FRAMEWORK_DEBUG << "Fetching from historical files...";
                    // Find the latest used file index
                    int i = 0;
                    while (boost::filesystem::exists(boost::filesystem::path("karaboHistory/" + deviceId + "_" + karabo::util::toString(i) + "." + get<string>("fileFormat")))) i++;
                    for (int j = i - 1; j >= 0 && !done; j--) {

                        vector<Hash> tmp = getData(deviceId, key, j);
                        bool collect = false;
                        bool done = false;
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
                    }
                }
                reply(result);
                return result;
            } catch (Exception& e) {
                KARABO_LOG_ERROR << e.userFriendlyMsg();
            }
            return vector<Hash>();
        }


        vector<Hash> FileDataLogger::getData(const std::string& deviceId, const std::string& key, const int i) {
            vector<Hash> data;
            string memoryPath("device." + deviceId + ".configuration." + key);

            boost::filesystem::path filePath;
            if (i == -1) {
                filePath = boost::filesystem::path("karaboHistory/" + deviceId + "." + get<string>("fileFormat"));
            } else {
                filePath = boost::filesystem::path("karaboHistory/" + deviceId + "_" + karabo::util::toString(i) + "." + get<string>("fileFormat"));
            }

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
