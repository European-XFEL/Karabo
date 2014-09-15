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

        using namespace krb_log4cpp;
        using namespace std;
        using namespace karabo::util;
        using namespace karabo::io;

        KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<OkErrorFsm>, DataLoggerManager)

        void DataLoggerManager::expectedParameters(Schema& expected) {

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

        std::string DataLoggerManager::generateNewDataLoggerInstanceId(const std::string& managerId) {
            static unsigned long long seq = 0;
            stringstream ss;
            ss << managerId << "-DataLogger_" << ++seq;
            return ss.str();
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
                for (Hash::const_iterator it = devices.begin(); it != devices.end(); ++it) { // Loop all devices
                    // TODO: May be to archive ONLY those that HAVE "archive" attribute and it IS true
                    if (it->hasAttribute("archive") && (it->getAttribute<bool>("archive") == false)) continue;
                    const std::string& deviceId = it->getKey();
                    if (deviceId == m_instanceId) continue; // Skip myself

                    // Check whether according logger devices exists and if not instantiate
                    // The key is device to be logged, the value is corresponding logger device

                    // Check if deviceId registered
                    boost::mutex::scoped_lock lock(m_loggedDevicesMutex);
                    map<string, string>::iterator devit = m_loggedDevices.find(deviceId);
                    if (devit == m_loggedDevices.end()) {
                        // instantiate the logger associated with "deviceId"
                        string loggerId = generateNewDataLoggerInstanceId(m_instanceId);
                        Hash config;
                        config.set("DataLogger.deviceId", loggerId);
                        config.set("DataLogger.deviceToBeLogged", deviceId);
                        config.set("DataLogger.directory", "karaboHistory");
                        config.set("DataLogger.maximumFileSize", get<int>("maximumFileSize"));
                        remote().instantiateNoWait(getServerId(), config);
                        m_loggedDevices[deviceId] = loggerId;
                        m_dataLoggers[loggerId] = deviceId;
                    }
                }
            }
        }

        void DataLoggerManager::instanceNewHandler(const karabo::util::Hash& topologyEntry) {

            try {
                const std::string& type = topologyEntry.begin()->getKey();
                KARABO_LOG_FRAMEWORK_DEBUG << "instanceNewHandler --> " << type;

                if (type == "device") { // Take out only devices for the time being
                    const Hash& entry = topologyEntry.begin()->getValue<Hash>();
                    const string& deviceId = entry.begin()->getKey();

                    // Skip devices that are marked to globally prevent archiving
                    if (entry.hasAttribute(deviceId, "archive") && (entry.getAttribute<bool>(deviceId, "archive") == false)) {
                        KARABO_LOG_FRAMEWORK_DEBUG << "instanceNewHandler --> deviceId = " << deviceId << " NOT archived";
                        return;
                    }

                    // Check whether according logger device exists (it should not) and instantiate
                    boost::mutex::scoped_lock lock(m_loggedDevicesMutex);
                    map<string, string>::iterator devit = m_loggedDevices.find(deviceId);
                    if (devit == m_loggedDevices.end()) {
                        string loggerId = generateNewDataLoggerInstanceId(m_instanceId);
                        Hash config;
                        config.set("DataLogger.deviceId", loggerId);
                        config.set("DataLogger.deviceToBeLogged", deviceId);
                        config.set("DataLogger.directory", "karaboHistory");
                        config.set("DataLogger.maximumFileSize", get<int>("maximumFileSize"));
                        remote().instantiateNoWait(getServerId(), config);
                        m_loggedDevices[deviceId] = loggerId;
                        m_dataLoggers[loggerId] = deviceId;
                    }
                }

            } catch (const Exception& e) {
                KARABO_LOG_ERROR << e;
            }
        }

        void DataLoggerManager::instanceGoneHandler(const std::string& instanceId, const karabo::util::Hash& instanceInfo) {

            // Call slotTagDeviceToBeDiscontinued and than kill the logger device
            try {
                map<string, string>::iterator devit = m_loggedDevices.find(instanceId);
                if (devit != m_loggedDevices.end()) {
                    boost::mutex::scoped_lock lock(m_loggedDevicesMutex);
                    string loggerId = devit->second;
                    remote().execute<bool, char>(loggerId, "slotTagDeviceToBeDiscontinued", true, 'D');
                    remote().killDeviceNoWait(loggerId);
                    m_loggedDevices.erase(devit);
                    m_dataLoggers.erase(loggerId);
                }
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
                    if (path != "." && path != key)
                        continue;

                    if (flag == "D" || flag == "L") {
                        // ignore path, type, value, user and use timestamp, trainId and flag
                        hash.setAttribute("v", "isLast", flag.at(0));
                    } else if (path == key) {
                        hash.clear();
                        Hash::Node& node = hash.set<string>("v", value);
                        node.setType(Types::from<FromLiteral>(type));
                    }
                    Timestamp stamp(Epochstamp(seconds, fraction), Trainstamp(trainId));
                    Hash::Attributes& attrs = hash.getAttributes("v");
                    stamp.toHashAttributes(attrs);
                    data.push_back(hash);
                }
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
    }
}
