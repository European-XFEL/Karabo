/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */
#include <map>
#include <karabo/io/Input.hh>
#include "DataLoggerManager.hh"
#include "karabo/io/FileTools.hh"

namespace karabo {
    namespace core {

        struct DataLoggerIndexEntry {
            karabo::util::Epochstamp startEpoch;
            karabo::util::Trainstamp startTrain;
            long startPos;
            karabo::util::Epochstamp endEpoch;
            karabo::util::Trainstamp endTrain;
            long endPos;
            char reason;
            boost::filesystem::path configFile;

            DataLoggerIndexEntry()
            : startEpoch()
            , startTrain()
            , startPos(0)
            , endEpoch()
            , endTrain()
            , endPos(0)
            , reason('X')
            , configFile() {
            }

            DataLoggerIndexEntry(const karabo::util::Epochstamp& se, const karabo::util::Trainstamp& ts, const long& sp,
                    const karabo::util::Epochstamp& ee, const karabo::util::Trainstamp& es, const long& ep,
                    const char ch, const boost::filesystem::path& path)
            : startEpoch(se)
            , startTrain(ts)
            , startPos(sp)
            , endEpoch(ee)
            , endTrain(es)
            , endPos(ep)
            , reason(ch)
            , configFile(path) {
            }
        };

        using namespace krb_log4cpp;
        using namespace std;
        using namespace karabo::util;
        using namespace karabo::io;

        KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<OkErrorFsm>, DataLoggerManager)

        static DataLoggerIndexEntry findDataLoggerIndexEntry(const std::string& deviceId, const std::string& timepoint) {
            unsigned long long startSeconds, startFraction, startTrainId;
            unsigned long long endSeconds, endFraction, endTrainId;
            long startPos, endPos;
            char reason;
            DataLoggerIndexEntry entry;

            Epochstamp target(timepoint);

            bool stopSearching = false;
            for (int i = 1; !stopSearching; ++i) {
                boost::filesystem::path indexpath = DataLoggerManager::getIndexFile(deviceId, i);
                if (!boost::filesystem::exists(indexpath))
                    break;
                boost::filesystem::path confile = DataLoggerManager::getArchiveFile(deviceId, i);
                ifstream ifs(indexpath.string().c_str());
                while (ifs >> startSeconds >> startFraction >> startTrainId >> startPos >> endSeconds >> endFraction >> endTrainId >> endPos >> reason) {
                    Epochstamp sstamp(startSeconds, startFraction);
                    Trainstamp strain(startTrainId);
                    Epochstamp estamp(endSeconds, endFraction);
                    Trainstamp etrain(endTrainId);
                    if (target >= sstamp) {
                        if (target > estamp)
                            continue;
                        entry.startEpoch = sstamp;
                        entry.startTrain = strain;
                        entry.startPos = startPos;
                        entry.endEpoch = estamp;
                        entry.endTrain = etrain;
                        entry.endPos = endPos;
                        entry.reason = reason;
                        entry.configFile = confile;
                        ifs.close();
                        return entry;
                    } else {
                        stopSearching = true;
                        break;
                    }
                }
                ifs.close();
            }
            if (stopSearching)
                return entry;
            boost::filesystem::path indexpath = DataLoggerManager::getIndexFile(deviceId, -1);
            if (!boost::filesystem::exists(indexpath))
                return entry;
            boost::filesystem::path confile = DataLoggerManager::getArchiveFile(deviceId, -1);
            ifstream indexstream(indexpath.string().c_str());
            while (indexstream >> startSeconds >> startFraction >> startTrainId >> startPos >> endSeconds >> endFraction >> endTrainId >> endPos >> reason) {
                Epochstamp sstamp(startSeconds, startFraction);
                Trainstamp strain(startTrainId);
                Epochstamp estamp(endSeconds, endFraction);
                Trainstamp etrain(endTrainId);
                if (target >= sstamp) {
                    if (target > estamp)
                        continue;
                    entry.startEpoch = sstamp;
                    entry.startTrain = strain;
                    entry.startPos = startPos;
                    entry.endEpoch = estamp;
                    entry.endTrain = etrain;
                    entry.endPos = endPos;
                    entry.reason = reason;
                    entry.configFile = confile;
                    break;
                } else
                    break;
            }
            indexstream.close();
            return entry;
        }

        void DataLoggerManager::expectedParameters(Schema& expected) {

            INT32_ELEMENT(expected).key("flushInterval")
                    .displayedName("Flush interval")
                    .description("The interval after which the memory accumulated data is made persistent")
                    .unit(Unit::SECOND)
                    .assignmentOptional().defaultValue(40)
                    .reconfigurable()
                    .commit();

            INT32_ELEMENT(expected).key("maximumFileSize")
                    .displayedName("Maximum file size")
                    .description("After any archived file has reached this size it will be time-stamped and not appended anymore")
                    .unit(Unit::BYTE)
                    .metricPrefix(MetricPrefix::MEGA)
                    .reconfigurable()
                    .assignmentOptional().defaultValue(100)
                    .commit();

            OVERWRITE_ELEMENT(expected).key("visibility")
                    .setNewDefaultValue(5)
                    .commit();

            OVERWRITE_ELEMENT(expected).key("deviceId")
                    .setNewDefaultValue("Karabo_DataLoggerManager_0")
                    .commit();

            // Slow beats
            OVERWRITE_ELEMENT(expected).key("heartbeatInterval")
                    .setNewDefaultValue(60)
                    .commit();
        }

        DataLoggerManager::DataLoggerManager(const Hash& input) : Device<OkErrorFsm>(input) {
        }

        DataLoggerManager::~DataLoggerManager() {
        }

        void DataLoggerManager::okStateOnEntry() {
            // Register handlers
            remote().registerInstanceNewMonitor(boost::bind(&karabo::core::DataLoggerManager::instanceNewHandler, this, _1));
            remote().registerInstanceGoneMonitor(boost::bind(&karabo::core::DataLoggerManager::instanceGoneHandler, this, _1, _2));

            // Prepare backend to persist data (later we should use brokerhost/brokerport/brokertopic)
            if (!boost::filesystem::exists("karaboHistory")) {
                boost::filesystem::create_directory("karaboHistory");
            }

            // Get all current instances in the system
            const Hash& systemTopology = remote().getSystemTopology();
            boost::optional<const Hash::Node&> node = systemTopology.find("device");
            if (node) {
                const Hash& devices = node->getValue<Hash>();
                for (Hash::const_iterator it = devices.begin(); it != devices.end(); ++it) { // Loop all devices ...
                    // ... but consider only those to be archived
                    if (it->hasAttribute("archive") && (it->getAttribute<bool>("archive") == true)) {
                        const std::string& deviceId = it->getKey();
                        if (deviceId == m_instanceId) continue; // Skip myself

                        // Check if deviceId is known in the world
                        string loggerId = "DataLogger-" + deviceId;
                        if (!remote().exists(loggerId).first) {
                            Hash config;
                            config.set("DataLogger.deviceId", loggerId);
                            config.set("DataLogger.deviceToBeLogged", deviceId);
                            config.set("DataLogger.directory", "karaboHistory");
                            config.set("DataLogger.maximumFileSize", get<int>("maximumFileSize"));
                            config.set("DataLogger.flushInterval", get<int>("flushInterval"));
                            remote().instantiateNoWait(getServerId(), config);
                        }
                    }
                }
            }

            SLOT3(slotGetPropertyHistory, string /*deviceId*/, string /*key*/, Hash /*params*/);
            SLOT2(slotGetConfigurationFromPast, string /*deviceId*/, string /*timepoint*/)
        }

        void DataLoggerManager::instanceNewHandler(const karabo::util::Hash& topologyEntry) {

            try {
                const std::string& type = topologyEntry.begin()->getKey();
                KARABO_LOG_FRAMEWORK_DEBUG << "instanceNewHandler --> " << type;

                if (type == "device") { // Take out only devices for the time being
                    const Hash& entry = topologyEntry.begin()->getValue<Hash>();
                    const string& deviceId = entry.begin()->getKey();

                    // Consider the devices that should be archived 
                    if (entry.hasAttribute(deviceId, "archive") && (entry.getAttribute<bool>(deviceId, "archive") == true)) {
                        // Check whether according logger device exists (it should not) and instantiate
                        string loggerId = "DataLogger-" + deviceId;
                        if (!remote().exists(loggerId).first) {
                            Hash config;
                            config.set("DataLogger.deviceId", loggerId);
                            config.set("DataLogger.deviceToBeLogged", deviceId);
                            config.set("DataLogger.directory", "karaboHistory");
                            config.set("DataLogger.maximumFileSize", get<int>("maximumFileSize"));
                            config.set("DataLogger.flushInterval", get<int>("flushInterval"));
                            remote().instantiateNoWait(getServerId(), config);
                        }
                    }
                }

            } catch (const Exception& e) {
                KARABO_LOG_ERROR << e;
            }
        }

        void DataLoggerManager::instanceGoneHandler(const std::string& instanceId, const karabo::util::Hash& instanceInfo) {
            try {
                string loggerId = "DataLogger-" + instanceId;
                this->call(loggerId, "slotTagDeviceToBeDiscontinued", true, 'D');
                remote().killDeviceNoWait(loggerId);
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << e;
            }
        }

        karabo::util::Epochstamp DataLoggerManager::extractRange(const vector<Hash>& archive, const Epochstamp& from, const Epochstamp& to, vector<Hash>& result) {

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

        void DataLoggerManager::slotGetPropertyHistory(const std::string& deviceId, const std::string& property, const Hash& params) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "slotGetPropertyHistory()";

                vector<Hash> result;

                Epochstamp from;
                if (params.has("from")) from = Epochstamp(params.get<string>("from"));

                Epochstamp to;
                if (params.has("to")) to = Epochstamp(params.get<string>("to"));

                unsigned int maxNumData = 0;
                if (params.has("maxNumData")) maxNumData = params.getAs<int>("maxNumData");

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
                    while (boost::filesystem::exists(boost::filesystem::path("karaboHistory/" + deviceId + "_configuration_" + karabo::util::toString(i) + ".txt")))
                        i++;

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

                std::string senderId = getSenderInfo("slotGetPropertyHistory")->getInstanceIdOfSender();
                call(senderId, "slotPropertyHistory", deviceId, property, result);

                //emit("signalPropertyHistory", deviceId, property, result);

            } catch (...) {

            }
        }

        vector<Hash> DataLoggerManager::getPropertyData(const std::string& deviceId, const std::string& key, const int idx) {
            typedef unsigned long long uint64_t;
            vector<Hash> data;
            
            boost::filesystem::path filePath = getArchiveFile(deviceId, idx);

            if (boost::filesystem::exists(filePath)) {
                Hash hash;
                // Read file
                std::ifstream infile(filePath.string().c_str());
                unsigned long long seconds, fraction, trainId;
                string path, type, value, user, flag;
                while (infile >> seconds >> fraction >> trainId >> path >> type >> value >> user >> flag) {
                    Epochstamp epochstamp(seconds, fraction);
                    //KARABO_LOG_FRAMEWORK_DEBUG << epochstamp.toIso8601() << " " << trainId << " " << path << " " << type << " " << value << " " << user << " " << flag;
                    if (path != "." && path != key)
                        continue;
                    if (flag == "L")
                        continue;
                    if (flag == "D") {
                        // ignore path, type, value, user and use timestamp, trainId and flag
                        hash.setAttribute("v", "isLast", flag.at(0));
                        data.push_back(hash);
                        continue;
                    } 
                    if (path == key) {
                        hash.clear();
                        Hash::Node& node = hash.set<string>("v", value);
                        node.setType(Types::from<FromLiteral>(type));
                    }
                    Timestamp timestamp(epochstamp, Trainstamp(trainId));
                    Hash::Attributes& attrs = hash.getAttributes("v");
                    timestamp.toHashAttributes(attrs);
                    data.push_back(hash);
                }
                infile.close();
            }
            return data;
        }

        boost::filesystem::path DataLoggerManager::getArchiveFile(const std::string& deviceId, const int idx) {
            boost::filesystem::path filePath;
            if (idx == -1) { // Most recent file (currently written to)
                filePath = boost::filesystem::path("karaboHistory/" + deviceId + "_configuration.txt");
            } else { // File got already log-rotated

                filePath = boost::filesystem::path("karaboHistory/" + deviceId + "_configuration_" + karabo::util::toString(idx) + ".txt");
            }
            return filePath;
        }

        boost::filesystem::path DataLoggerManager::getSchemaFile(const std::string& deviceId) {
            return boost::filesystem::path("karaboHistory/" + deviceId + "_schema.txt");
        }

        boost::filesystem::path DataLoggerManager::getIndexFile(const std::string& deviceId, const int idx) {
            boost::filesystem::path filePath;
            if (idx == -1) { // Most recent file (currently written to)
                filePath = boost::filesystem::path("karaboHistory/" + deviceId + "_index.txt");
            } else { // File got already log-rotated

                filePath = boost::filesystem::path("karaboHistory/" + deviceId + "_index_" + karabo::util::toString(idx) + ".txt");
            }
            return filePath;
        }

        void DataLoggerManager::slotGetConfigurationFromPast(const std::string& deviceId, const std::string& timepoint) {
            try {

                Hash hash;
                Schema schema;
                Epochstamp tgt(timepoint);

                KARABO_LOG_FRAMEWORK_DEBUG << "Requested time point: " << tgt.getSeconds();

                // Retrieve proper Schema
                boost::filesystem::path schemaPath = getSchemaFile(deviceId);
                if (boost::filesystem::exists(schemaPath)) {
                    std::ifstream schemastream(schemaPath.string().c_str());
                    unsigned long long seconds;
                    unsigned long long fraction;
                    unsigned long long trainId;
                    string archived;
                    while (schemastream >> seconds >> fraction >> trainId) {
                        Epochstamp current(seconds, fraction);
                        if (current <= tgt) {
                            archived.clear();
                            if (!getline(schemastream, archived))
                                break;
                        } else
                            break;
                    }
                    schemastream.close();
                    if (archived.empty()) {
                        reply(Hash(), Schema()); // Requested time is before any logger data
                        KARABO_LOG_WARN << "Requested time point for device configuration is earlier than anything logged";
                        return;
                    }
                    TextSerializer<Schema>::Pointer serializer = TextSerializer<Schema>::create("Xml");
                    serializer->load(schema, archived);
                }
                vector<string> paths = schema.getPaths();

                DataLoggerIndexEntry index = findDataLoggerIndexEntry(deviceId, timepoint);

                if (index.reason == 'X') {
                    reply(Hash(), Schema()); // Requested time is out of any logger data
                    KARABO_LOG_WARN << "Requested time point for device configuration is out of any valid logged data";
                    return;
                }

                {
                    unsigned long long secs, frac, train;
                    string path, type, val, user, flag;
                    ifstream file(index.configFile.string().c_str());
                    file.seekg(index.startPos);
                    while (file >> secs >> frac >> train >> path >> type >> val >> user >> flag) {
                        Epochstamp current(secs, frac);
                        if (current > tgt)
                            break;
                        Trainstamp trainstamp(train);
                        Timestamp timestamp(current, trainstamp);
                        Hash::Node& node = hash.set<string>(path, val);
                        node.setType(Types::from<FromLiteral>(type));
                        Hash::Attributes& attrs = node.getAttributes();
                        timestamp.toHashAttributes(attrs);
                        if (index.endPos <= file.tellg())
                            break;
                    }
                    file.close();
                }

//                BOOST_FOREACH(string path, paths) {
//                    vector<Hash> archive = getPropertyData(deviceId, path, -1);
//                    for (vector<Hash>::const_reverse_iterator rjt = archive.rbegin(); rjt != archive.rend(); ++rjt) {
//                        try {
//                            Epochstamp current(Epochstamp::fromHashAttributes(rjt->getAttributes("v")));
//
//                            if (current <= tgt) {
//                                const Hash::Node& tmpNode = rjt->getNode("v");
//                                hash.set(path, tmpNode.getValueAsAny());
//                                hash.setAttributes(path, tmpNode.getAttributes());
//                                break; // Current is smaller or equal to tgt, stop!  
//                            }
//
//                        } catch (Exception& e) {
//                            cout << "!!! SHOULD NOT HAPPEN !!!" << endl;
//                            cout << e << endl;
//                            continue;
//                        }
//                    }
//
//                }
                reply(hash, schema);
            } catch (...) {
                KARABO_RETHROW
            }
        }

    }
}
