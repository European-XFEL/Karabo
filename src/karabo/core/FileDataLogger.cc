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
                    .assignmentOptional().defaultValue(100)
                    .commit();

            FLOAT_ELEMENT(expected).key("lastFlushDuration")
                    .displayedName("Last flush duration")
                    .description("Time needed for the last flush")
                    .unit(Unit::SECOND)
                    .readOnly()
                    .warnHigh(20)
                    .alarmHigh(40)
                    .commit();

            OVERWRITE_ELEMENT(expected).key("visibility")
                    .setNewDefaultValue(5)
                    .commit();
            
            OVERWRITE_ELEMENT(expected).key("deviceId")
                    .setNewDefaultValue("Karabo_FileDataLogger_0")
                    .commit();
        }


        FileDataLogger::FileDataLogger(const Hash& input) : Device<OkErrorFsm>(input) {

            // Initialize the memory data structure (currently only devices are supported)
            m_systemHistory.set("device", Hash());
            SIGNAL3("signalPropertyHistory", string /*deviceId*/, string /*property*/, vector<Hash>);
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
            remote().registerInstanceGoneMonitor(boost::bind(&karabo::core::FileDataLogger::instanceGoneHandler, this, _1, _2));

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
            SLOT3(slotGetPropertyHistory, string /*deviceId*/, string /*key*/, Hash /*to (string) from (string) maxNumData (unsigned int)*/);
            SLOT2(slotGetConfigurationFromPast, string /*deviceId*/, string /*timepoint*/)
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


        void FileDataLogger::instanceGoneHandler(const std::string& instanceId, const karabo::util::Hash& instanceInfo) {
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

                            KARABO_LOG_INFO << "File size for " << deviceId << " is too large, log-rotating...";

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
                if (it->is<Hash>()) {
                    createLastValidConfiguration(it->getValue<Hash>(), wasValidUpToNow, reason);
                    return;
                }
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

            string memoryPath("device." + deviceId + ".configuration");
            if (m_systemHistory.has(memoryPath)) {
                // Get schema for this device
                Schema schema = remote().getDeviceSchema(deviceId);
                Hash& tmp = m_systemHistory.get<Hash>(memoryPath);
                vector<string> paths;
                changedConfig.getPaths(paths);
                for (size_t i = 0; i < paths.size(); ++i) {                    
                    const string& path = paths[i];
                    const Hash::Node& leafNode = changedConfig.getNode(path);
                    // Skip those elements which should not be archived
                    if (!schema.has(path) || (schema.hasArchivePolicy(path) && (schema.getArchivePolicy(path) == Schema::NO_ARCHIVING))) continue;
                    Hash val("v", leafNode.getValueAsAny());
                    val.setAttributes("v", leafNode.getAttributes());
                    boost::optional<Hash::Node&> node = tmp.find(path);
                    if (node) node->getValue<vector<Hash> >().push_back(val);
                    else tmp.set(path, std::vector<Hash>(1, val));
                }
            } else
                KARABO_LOG_WARN << "Could not find: " << memoryPath << " in " << m_systemHistory;
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
                
                Hash systemHistoryCopy;

                TimeProfiler profiler("Persist data");
                while (m_persistData) {

                    // Profile the persisting time
                    profiler.open();
                    profiler.startPeriod();
                    KARABO_LOG_FRAMEWORK_DEBUG << "Start flushing memory to file";
                    
                    copyAndClearSystemConfiguration(systemHistoryCopy);

                    Hash& devices = systemHistoryCopy.get<Hash>("device");
                    for (Hash::iterator it = devices.begin(); it != devices.end(); ++it) { // Loops deviceIds
                        try {

                            // Persist
                            appendDeviceConfigurationToFile(it->getKey(), it->getValue<Hash>());

                        } catch (const Exception& e) {
                            KARABO_LOG_ERROR << e;
                        } catch (...) {
                            KARABO_LOG_ERROR << "Encountered unknown exception whilst persisting data for instance " << it->getKey();
                        }
                    }

                    profiler.close();
                    profiler.stopPeriod();
                    TimeValue nSeconds = profiler.getPeriod().getDuration().getSeconds();
                    set("lastFlushDuration", nSeconds);

                    boost::this_thread::sleep(boost::posix_time::seconds(this->get<int>("flushInterval")));
                }
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << e;
            } catch (...) {
                KARABO_LOG_ERROR << "Encountered unknown exception whilst persisting karabo device data to file";
            }
        }
        
        
        void FileDataLogger::copyAndClearSystemConfiguration(karabo::util::Hash& copy) {
            boost::mutex::scoped_lock lock(m_systemHistoryMutex);
            copy = m_systemHistory;
            Hash& devices = m_systemHistory.get<Hash>("device");
            for (Hash::iterator it = devices.begin(); it != devices.end(); ++it) { // Loops deviceIds
                // Release memory
                it->setValue(Hash("schema", vector<Hash>(), "configuration", Hash()));
            }
        }
        
        
        void FileDataLogger::slotGetPropertyHistory(const std::string& deviceId, const std::string& property, const Hash& params) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "slotGetPropertyHistory()";

                vector<Hash> result;

                Epochstamp from;
                if (params.has("from")) from = Epochstamp(params.get<string>("from"));

                Epochstamp to;
                if (params.has("to")) to = Epochstamp(params.get<string>("to"));

                unsigned int maxNumData = 0;
                if (params.has("maxNumData")) maxNumData = params.get<int>("maxNumData");

                KARABO_LOG_FRAMEWORK_DEBUG << "From (UTC): " << from.getSeconds();
                KARABO_LOG_FRAMEWORK_DEBUG << "To (UTC):   " << to.getSeconds();

                // Reads all data from latest file into vector<Hash>
                vector<Hash> tmp = getPropertyData(deviceId, property);

                // tmp.front() reflects the oldest entry
                // tmp.back() reflects the newest entry
                Epochstamp oldest = extractRange(tmp, from, to, result);

                if (from < oldest) { // Puuh! Go further back!
                    KARABO_LOG_FRAMEWORK_DEBUG << "Fetching from historical files...";

                    // Find the latest used file index
                    int i = 0;
                    while (boost::filesystem::exists(boost::filesystem::path("karaboHistory/" + deviceId + "_" + karabo::util::toString(i) + "." + get<string>("fileFormat")))) i++;

                    // Loop all historical files
                    for (int j = i - 1; j >= 0; j--) {

                        // Load the data from archive file into memory
                        tmp = getPropertyData(deviceId, property, j);

                        // Extracts data from archive into result (according to what is defined in from and to)
                        oldest = extractRange(tmp, from, to, result);

                        // From is bigger than the oldest value in this file, stop!
                        if (from >= oldest) break;
                    }
                }

                // Perform data reduction here
                if (maxNumData && (result.size() > maxNumData)) {
                    int factor = result.size() / maxNumData;
                    // Special case: maxNumData is not even half as small, still skip every 2nd element
                    if (factor == 1) factor = 2;
                    size_t returnSize = result.size() / factor;
                    vector<Hash> reduced;
                    reduced.reserve(returnSize * 1.1); // This fudge factor is intended for the flagged values which have to stay
                    KARABO_LOG_FRAMEWORK_DEBUG << "Reducing data by a factor of " << factor << ". Will return ~" << returnSize << " data points";
                    size_t idx = 0;
                    for (vector<Hash>::const_reverse_iterator rit = result.rbegin(); rit != result.rend(); rit++) {
                        if (rit->hasAttribute("v", "isLast")) reduced.push_back(*rit);
                        else if (idx++ % factor == 0) reduced.push_back(*rit);
                    }
                    result.swap(reduced);
                } else {
                    // At least reverse result
                    std::reverse(result.begin(), result.end());
                }

                reply(result);
                emit("signalPropertyHistory", deviceId, property, result);

            } catch (...) {

            }
        }


        vector<Hash> FileDataLogger::getPropertyData(const std::string& deviceId, const std::string& key, const int idx) {

            vector<Hash> data;
            string memoryPath("device." + deviceId + ".configuration." + key);

            boost::filesystem::path filePath = getArchiveFile(deviceId, idx);

            if (boost::filesystem::exists(filePath)) {
                // Read file
                Hash file;
                loadFromFile(file, filePath.string());
                if (file.has("configuration." + key)) {
                    const vector<Hash>& tmp = file.get<vector<Hash> >("configuration." + key);
                    data.insert(data.end(), tmp.begin(), tmp.end());
                }
            }
            // i == -1 is used as flag indicating the most recent archive file. 
            // In this condition, we are adding any data still resident in memory
            if (idx == -1 && m_systemHistory.has(memoryPath)) {
                boost::mutex::scoped_lock lock(m_systemHistoryMutex);
                const vector<Hash>& tmp = m_systemHistory.get<vector<Hash> >(memoryPath);
                data.insert(data.end(), tmp.begin(), tmp.end());
            }
            return data;
        }


        boost::filesystem::path FileDataLogger::getArchiveFile(const std::string& deviceId, const int idx) {
            boost::filesystem::path filePath;
            if (idx == -1) { // Most recent file (currently written to)
                filePath = boost::filesystem::path("karaboHistory/" + deviceId + "." + get<string>("fileFormat"));
            } else { // File got already log-rotated
                filePath = boost::filesystem::path("karaboHistory/" + deviceId + "_" + karabo::util::toString(idx) + "." + get<string>("fileFormat"));
            }
            return filePath;
        }


        karabo::util::Epochstamp FileDataLogger::extractRange(const vector<Hash>& archive, const Epochstamp& from, const Epochstamp& to, vector<Hash>& result) {

            // Archive is a time range beginning with the oldest entry and ending with the newest one 
            // From(old) - to(new) reflects the range to be returned
            // Result is a time range beginning with the newest entry and ending with the oldest one            

            Epochstamp oldest = Epochstamp::fromHashAttributes(archive.front().getAttributes("v"));
            KARABO_LOG_FRAMEWORK_DEBUG << "Oldest in range:   " << oldest.getSeconds();
            Epochstamp newest = Epochstamp::fromHashAttributes(archive.back().getAttributes("v"));
            KARABO_LOG_FRAMEWORK_DEBUG << "Newest in range:   " << newest.getSeconds();

            if (from <= oldest && to >= newest) { // Collect all data from this range
                KARABO_LOG_FRAMEWORK_DEBUG << "Fetching whole range";
                result.insert(result.end(), archive.rbegin(), archive.rend());

            } else if (to < oldest) { // Collect no data from this range
                KARABO_LOG_FRAMEWORK_DEBUG << "Skipping whole range";
                // Do nothing by purpose

            } else { // Go through the data and sort out what to collect

                bool collect = false;
                for (vector<Hash>::const_reverse_iterator rit = archive.rbegin(); rit != archive.rend(); ++rit) {
                    try {
                        Epochstamp current(Epochstamp::fromHashAttributes(rit->getAttributes("v")));



                        if (current < to) collect = true; // Current is smaller than to, collect!
                        if (collect) {
                            KARABO_LOG_FRAMEWORK_DEBUG << "Current:   " << current.getSeconds();
                            result.push_back(*rit);
                        }
                        if (current <= from) break; // Current is smaller or equal to from, stop!

                    } catch (Exception& e) {
                        cout << "!!! SHOULD NOT HAPPEN !!!" << endl;
                        cout << e << endl;
                        continue;
                    }
                }
            }
            return oldest;
        }


        void FileDataLogger::slotGetConfigurationFromPast(const std::string& deviceId, const std::string& timepoint) {
            try {

                Hash hash;
                Epochstamp tgt(timepoint);
                Epochstamp creationTime;
                Hash file;
                int idx = -1;
                
                KARABO_LOG_FRAMEWORK_DEBUG << "Requested time point: " << tgt.getSeconds();

                do {
                    // Read file
                    boost::filesystem::path filePath = getArchiveFile(deviceId, idx);
                    if (boost::filesystem::exists(filePath)) {
                        loadFromFile(file, filePath.string());
                        creationTime = Epochstamp::fromHashAttributes(file.getAttributes("schema"));
                        KARABO_LOG_FRAMEWORK_DEBUG << "Oldest schema in range: " << creationTime.toFormattedString();
                        ++idx;
                    } else {

                        reply(Hash(), Schema()); // Requested time is before any logger data
                        KARABO_LOG_WARN << "Requested time point for device configuration is earlier than anything logged";
                        return;
                    }

                } while (tgt < creationTime); // Check within this file

                // Retrieve proper Schema
                const vector<Hash>& schemas = file.get<vector<Hash> >("schema");
                vector<Hash>::const_reverse_iterator rit = schemas.rbegin();
                for (; rit != schemas.rend(); ++rit) {
                    Epochstamp current(Epochstamp::fromHashAttributes(rit->getAttributes("v")));
                    if (current <= tgt) break;
                }
                const Schema& schema = rit->get<Schema>("v");
                vector<string> paths = schema.getPaths();


                BOOST_FOREACH(string path, paths) {
                    vector<Hash> archive = getPropertyData(deviceId, path, -1);
                    for (vector<Hash>::const_reverse_iterator rjt = archive.rbegin(); rjt != archive.rend(); ++rjt) {
                        try {
                            Epochstamp current(Epochstamp::fromHashAttributes(rjt->getAttributes("v")));

                            if (current <= tgt) {
                                const Hash::Node& tmpNode = rjt->getNode("v");
                                hash.set(path, tmpNode.getValueAsAny());
                                hash.setAttributes(path, tmpNode.getAttributes());
                                break; // Current is smaller or equal to tgt, stop!  
                            }

                        } catch (Exception& e) {
                            cout << "!!! SHOULD NOT HAPPEN !!!" << endl;
                            cout << e << endl;
                            continue;
                        }
                    }

                }
                reply(hash, schema);
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void FileDataLogger::slotGetFromPast(const std::string& deviceId, const std::string& key, const std::string& from, const std::string & to) {
            KARABO_LOG_FRAMEWORK_DEBUG << "slotGetFromPast()";
            vector<Hash> result;
            try {
                Epochstamp t0(from);
                Epochstamp t1(to);
                KARABO_LOG_FRAMEWORK_DEBUG << "From: " << from << " <-> " << t0.toFormattedString();
                KARABO_LOG_FRAMEWORK_DEBUG << "To:   " << to << " <-> " << t1.toFormattedString();

                vector<Hash> tmp = getPropertyData(deviceId, key);
                bool collect = false;
                bool done = false;
                //bool isLastFlagIsUp = false; // TODO continue here
                for (vector<Hash>::reverse_iterator rit = tmp.rbegin(); rit != tmp.rend(); ++rit) {
                    //KARABO_LOG_FRAMEWORK_DEBUG << *rit;
                    try {
                        Epochstamp current(Epochstamp::fromHashAttributes(rit->getAttributes("v")));

                        KARABO_LOG_FRAMEWORK_DEBUG << "Current:   " << current.toIso8601();
                        if (t1 > current) collect = true; // Time until is bigger then current timestamp, so collect
                        if (collect) result.push_back(*rit);
                        if (t0 >= current) { // Time from is now bigger than current flag -> we are done
                            done = true;
                            break;
                        }
                    } catch (Exception& e) {
                        // TODO Clean this
                        continue;
                    }
                }
                if (!done) { // Puuh! Go further back!
                    KARABO_LOG_FRAMEWORK_DEBUG << "Fetching from historical files...";
                    // Find the latest used file index
                    int i = 0;
                    while (boost::filesystem::exists(boost::filesystem::path("karaboHistory/" + deviceId + "_" + karabo::util::toString(i) + "." + get<string>("fileFormat")))) i++;
                    for (int j = i - 1; j >= 0 && !done; j--) {

                        vector<Hash> tmp = getPropertyData(deviceId, key, j);
                        bool collect = false;
                        //bool done = false;
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
            } catch (Exception& e) {
                KARABO_LOG_ERROR << e.userFriendlyMsg();
            }
            //return vector<Hash>();
        }



    }
}
