/**
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
 *
 * The main logic of the managing of the data loggers and of the devices that they should log is as follows:
 * * Register handlers for new and gone instances before topology gathering starts.
 *   o By that we get rid of an initial treatment of the topology and of everything that comes afterwards. With
 *      multithreading it is difficult to get a clear cut between these two states.
 *   o Now we rely on the fact that the instanceNew handler is called for all instances discovered, be it in the
 *     initial topology gathering or later.
 * * We track a state for each logger server - it consist of
 *   o a LoggerState, i.e. whether the server is still OFFLINE, or whether the server was discovered and therefore
 *     we are INSTANTIATING the logger or whether the logger is RUNNING.
 *   o a "backlog" of all devices that should be logged by this server,
 *   o the devices "beingAdded", i.e. for which we told the logger to take care, but did not yet get confirmation,
 *   o and those "devices" that the logger has acknowledged to take care of.
 * * All actions that work on this state are sequentialised by using a strand to be sure about the state.
 * * Once a server is discovered, the logger is instantiated.
 *   o Special treatment is needed for cases where instantiation fails since device is already online because no
 *     instanceNew comes. (Can happen if logger is re-discovered after loss of heartbeats.)
 * * Once a logger is discovered, it is told to log all devices that are found to be logged by it and have been
 *   discovered before
 * * Once a device to be logged is discovered (this could even be a logger), it is checked which logger server is
 *   responsible
 *   o either the logger is told to log this device
 *   o or, if not yet ready, the device is added to the "backlog".
 * * If a device to be logged is gone, its logger is informed to stop logging
 * * If a logger goes down, it is restarted (well, we try - if also the server is down, restart will fail)
 *   before all logged "devices" and those "beingAdded" are put into "backlog".
 * * If a logger server goes down, "devices" and those "beingAdded" are put into "backlog" as well.
 * * The case that
 *   o the loggers and the manager work fine,
 *   o but then the manager is shutdown
 *   o and a logged device stops afterwards,
 *   o and then the manager starts again
 *  is handled within the regular checks described in the following.
 *
 * Besides taking care to instruct loggers to log devices, a regular sanity check based on Epochstamps is done:
 * * Every "topologyCheck.interval" minutes, each DataLogger that is running is checked.
 * * This check accesses the "lastUpdatesUtc" tables of all devices:
 *   o If there is an empty time stamp, that should mean that logging of the device in question has just started. The
 *     check procedure is continued with the other devices, but id of the device is noted and in case that during
 *     the next check there is still no timestamp, stop and start of logging that device is enforced.
 *   o Otherwise each timestamp is compared with 'now'.
 * * If the time difference is smaller than "topologyCheck.toleranceLogged", the logging of the device is considered
 *   to be OK.
 * * Otherwise the configuration of the device in question is queried:
 *   o If the time stamp in "lastUpdatesUtc" is not more than "topologyCheck.toleranceDiff" behind the most recent
 *     timestamp of the requested device configuration, the logging is considered OK.
 *   o If it is more behind, this is considered to be a problem and stop and start of logging is enforced.
 *   o If the query for the device configuration fails, the internal book keeping is cross checked whether the device
 *     is really supposed to be logged. If not, the logger is called to stop logging this device. (This situation
 *     can appear if the manager was down when the device went down so the manager could not inform the logger.)
 * * When treatment of all loggers and their devices is finished, a summary of the findings are logged and published
 *   as "topologyCheck.lastCheckResult" and the procedure is triggered again in "topologyCheck.interval" minutes.
 */

#include "DataLoggerManager.hh"

#include <string.h> // strlen

#include <algorithm> // std::find, std::max
#include <boost/algorithm/string.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <chrono>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

#include "karabo/io/FileTools.hh"
#include "karabo/net/EventLoop.hh"
#include "karabo/util/ChoiceElement.hh"
#include "karabo/util/DataLogUtils.hh"
#include "karabo/util/Epochstamp.hh"
#include "karabo/util/SimpleElement.hh"
#include "karabo/util/StringTools.hh"
#include "karabo/util/TableElement.hh"
#include "karabo/util/VectorElement.hh"

KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::BaseDevice, karabo::core::Device, karabo::devices::DataLoggerManager)

using std::placeholders::_1;
using std::placeholders::_2;
namespace karabo {
    namespace devices {

        using namespace std::chrono;
        using namespace std;
        using namespace karabo::util;
        using namespace karabo::io;
        using namespace std::placeholders;
        using karabo::xms::SLOT_ELEMENT;


        /**
         * Helper function used below:
         * Add object to set<T> at key's position in h - if no such key exists, create one
         */
        template <class T>
        void addToSetOrCreate(Hash& h, const std::string& key, const T& object) {
            if (h.has(key)) {
                h.get<std::set<T>>(key).insert(object);
            } else {
                h.set(key, std::set<T>({object}));
            }
        }


        static void trim_vector_elements(std::vector<std::string>& v) {
            for (auto& str : v) {
                boost::algorithm::trim(str);
            }
        }


        void DataLoggerManager::expectedParameters(Schema& expected) {
            OVERWRITE_ELEMENT(expected)
                  .key("state")
                  .setNewOptions(State::INIT, State::ON, State::MONITORING, State::ERROR)
                  .setNewDefaultValue(State::INIT)
                  .commit();

            OVERWRITE_ELEMENT(expected).key("performanceStatistics.enable").setNewDefaultValue(true).commit();

            OVERWRITE_ELEMENT(expected).key("deviceId").setNewDefaultValue("Karabo_DataLoggerManager_0").commit();

            INT32_ELEMENT(expected)
                  .key("flushInterval")
                  .displayedName("Flush interval")
                  .description("The interval after which the memory accumulated data is made persistent")
                  .unit(Unit::SECOND)
                  .assignmentOptional()
                  .defaultValue(40)
                  .minInc(1)
                  .reconfigurable()
                  .commit();

            BOOL_ELEMENT(expected)
                  .key("enablePerformanceStats")
                  .displayedName("Performance stats on/off")
                  .description("Value of 'performanceStatistics.enable' used when instantiating loggers")
                  .reconfigurable()
                  .assignmentOptional()
                  .defaultValue(true)
                  .commit();

            STRING_ELEMENT(expected)
                  .key("logger")
                  .displayedName("Logger Type")
                  .description("Logger type variable to define if influx or file based data logging is used")
                  .init()
                  .options(std::vector<std::string>{"FileDataLogger", "InfluxDataLogger"})
                  .assignmentOptional()
                  .defaultValue("FileDataLogger")
                  .commit();

            NODE_ELEMENT(expected)
                  .key("fileDataLogger")
                  .displayedName("FileDataLogger")
                  .description("File based data logging")
                  .commit();

            STRING_ELEMENT(expected)
                  .key("fileDataLogger.directory")
                  .displayedName("Directory")
                  .description("The directory where the log files should be placed")
                  .assignmentOptional()
                  .defaultValue("karaboHistory")
                  .commit();

            INT32_ELEMENT(expected)
                  .key("fileDataLogger.maximumFileSize")
                  .displayedName("Maximum file size")
                  .description(
                        "After any archived file has reached this size it will be time-stamped and not appended "
                        "anymore")
                  .unit(Unit::BYTE)
                  .metricPrefix(MetricPrefix::MEGA)
                  .assignmentOptional()
                  .defaultValue(100)
                  .commit();

            NODE_ELEMENT(expected)
                  .key("influxDataLogger")
                  .displayedName("InfluxDataLogger")
                  .description("Influxdb based data logging")
                  .commit();

            STRING_ELEMENT(expected)
                  .key("influxDataLogger.urlWrite")
                  .displayedName("Logger InfluxDB URL")
                  .description("URL for writing")
                  .assignmentOptional()
                  .defaultValue("tcp://localhost:8086")
                  .init()
                  .commit();

            STRING_ELEMENT(expected)
                  .key("influxDataLogger.urlRead")
                  .displayedName("Reader InfluxDB URL")
                  .description("URL for reading configurations and schema from past (typically longer retention time)")
                  .assignmentOptional()
                  .defaultValue("tcp://localhost:8086")
                  .init()
                  .commit();

            STRING_ELEMENT(expected)
                  .key("influxDataLogger.urlReadPropHistory")
                  .displayedName("Reader InfluxDB URL (Prop History)")
                  .description(
                        "URL for reading property history (typically shorter retention time)."
                        "If empty (default), use value of 'Reader InfluxDB URL'.")
                  .assignmentOptional()
                  .defaultValue(std::string())
                  .init()
                  .commit();

            STRING_ELEMENT(expected)
                  .key("influxDataLogger.dbname")
                  .displayedName("Database name")
                  .description("Name of the database of the data. If empty, fall back to broker topic.")
                  .assignmentOptional()
                  .defaultValue("")
                  .init()
                  .commit();

            UINT32_ELEMENT(expected)
                  .key("influxDataLogger.maxBatchPoints")
                  .displayedName("Max batch points")
                  .description("Max number of InfluxDB points in the batch")
                  .assignmentOptional()
                  .defaultValue(200)
                  .init()
                  .commit();

            INT32_ELEMENT(expected)
                  .key("influxDataLogger.maxTimeAdvance")
                  .displayedName("Max Time Advance")
                  .description(
                        "Maximum time advance allowed for data. "
                        "Data too far ahead in the future will be dropped. "
                        "Negative values or 0 means no limit.")
                  .assignmentOptional()
                  .defaultValue(7200) // 2 hours
                  .unit(Unit::SECOND)
                  .init()
                  .commit();

            UINT32_ELEMENT(expected)
                  .key("influxDataLogger.maxVectorSize")
                  .displayedName("Max Vector Size")
                  .description("Vector properties longer than this are skipped and not written to the database.")
                  .assignmentOptional()
                  .defaultValue(4 * 2700) // four times number of bunches per EuXFEL train
                  .init()
                  .commit();

            UINT32_ELEMENT(expected)
                  .key("influxDataLogger.maxValueStringSize")
                  .displayedName("Max String Size")
                  .description(
                        "Maximum size, in characters, for a property value to be inserted into Influx and for a schema "
                        "chunk. "
                        "(All values are feed to Influx as strings in a text format called Line Protocol)")
                  .assignmentOptional()
                  .defaultValue(MAX_INFLUX_VALUE_LENGTH - (MAX_INFLUX_VALUE_LENGTH / 10))
                  .maxInc(MAX_INFLUX_VALUE_LENGTH)
                  .init()
                  .commit();

            UINT32_ELEMENT(expected)
                  .key("influxDataLogger.maxPerDevicePropLogRate")
                  .displayedName("Max per Device Property Logging Rate (Kb/sec)")
                  .description(
                        "Entries for a device property that would move its logging rate above this threshold are "
                        "skipped.")
                  .assignmentOptional()
                  .defaultValue(5 * 1024) // 5 Mb/s
                  .minInc(1)              // 1 Kb/s
                  .init()
                  .commit();

            UINT32_ELEMENT(expected)
                  .key("influxDataLogger.propLogRatePeriod")
                  .displayedName("Interval for logging rate calc")
                  .description("Interval for calculating per device property logging rate")
                  .assignmentOptional()
                  .defaultValue(5)
                  .minInc(1)
                  .maxInc(60)
                  .unit(Unit::SECOND)
                  .init()
                  .commit();

            UINT32_ELEMENT(expected)
                  .key("influxDataLogger.maxSchemaLogRate")
                  .displayedName("Max Schema Logging Rate (Kb/sec)")
                  .description(
                        "Schema updates for a device that would move its schema logging rate above this threshold are "
                        "skipped. Sizes are for the base64 encoded form of the binary serialized schema.")
                  .assignmentOptional()
                  .defaultValue(5 * 1024) // 5 Mb/s
                  .minInc(1)              // 1 Kb/s
                  .init()
                  .commit();

            UINT32_ELEMENT(expected)
                  .key("influxDataLogger.schemaLogRatePeriod")
                  .displayedName("Interval for schema logging rate calc")
                  .description("Interval for calculating per device schema logging rate")
                  .assignmentOptional()
                  .defaultValue(5)
                  .minInc(1)
                  .maxInc(60)
                  .unit(Unit::SECOND)
                  .init()
                  .commit();

            DOUBLE_ELEMENT(expected)
                  .key("influxDataLogger.safeSchemaRetentionPeriod")
                  .displayedName("Period for safe schema retention")
                  .description(
                        "For how long can a stored schema be safely assumed to be kept? Must be an "
                        "interval smaller than the database retention policy")
                  .assignmentOptional()
                  .defaultValue(2.0)
                  .minExc(0.0)
                  .unit(Unit::YEAR)
                  .init()
                  .commit();

            VECTOR_STRING_ELEMENT(expected)
                  .key("serverList")
                  .displayedName("Server list")
                  .description(
                        "List of device server IDs where the DataLogger instance run. "
                        "The load balancing is round-robin. Must not be empty")
                  .init()
                  .minSize(1)
                  .assignmentMandatory()
                  .commit();

            UINT32_ELEMENT(expected)
                  .key("timeout")
                  .displayedName("Timeout")
                  .description("Timeout of requests to DataLogger's or during checks")
                  .unit(Unit::SECOND)
                  .metricPrefix(MetricPrefix::MILLI)
                  // Defaults to 15 s.
                  // 2 s. lead to many timeouts. During tests, 4 s. latencies have been observed.
                  // 15 s. adds an extra safety margin.
                  .assignmentOptional()
                  .defaultValue(15000)
                  .reconfigurable()
                  .minInc(100)
                  .maxInc(60000) // 100 ms to 1 minute
                  .commit();

            NODE_ELEMENT(expected)
                  .key("topologyCheck")
                  .displayedName("Topology check")
                  .description("Status and parameters of regular topology checks")
                  .commit();

            SLOT_ELEMENT(expected)
                  .key("topologyCheck.slotForceCheck")
                  .displayedName("Force check")
                  .description("Immediately launch a check")
                  .allowedStates(State::ON)
                  .commit();

            INT8_ELEMENT(expected)
                  .key("topologyCheck.loggingProblem")
                  .displayedName("Logging problem")
                  .description("Non-zero if topology check discovered a problem")
                  .readOnly()
                  .initialValue(0)
                  .commit();

            STRING_ELEMENT(expected)
                  .key("topologyCheck.lastCheckStartedUtc")
                  .displayedName("Check started (UTC)")
                  .description("Last time a check was initiated")
                  .readOnly()
                  .initialValue("")
                  .commit();

            STRING_ELEMENT(expected)
                  .key("topologyCheck.lastCheckDoneUtc")
                  .displayedName("Check finished (UTC)")
                  .description("Last time a check was finished")
                  .readOnly()
                  .initialValue("")
                  .commit();

            STRING_ELEMENT(expected)
                  .key("topologyCheck.lastCheckResult")
                  .displayedName("Check result")
                  .description("Result of last running check")
                  .readOnly()
                  .initialValue("")
                  .commit();

            UINT32_ELEMENT(expected)
                  .key("topologyCheck.interval")
                  .displayedName("Check interval")
                  .description(
                        "Interval in between regular checks on the topology whether "
                        "everything is logged as it should")
                  .unit(Unit::MINUTE)
                  .assignmentOptional()
                  .defaultValue(30)
                  .reconfigurable()
                  .minInc(1)
                  .maxInc(1440) // min every minute, max every 24 h
                  .commit();

            UINT32_ELEMENT(expected)
                  .key("topologyCheck.toleranceLogged")
                  .displayedName("Tolerance logged")
                  .description("How old last logged update may be")
                  .unit(Unit::MINUTE)
                  .assignmentOptional()
                  .defaultValue(10)
                  .reconfigurable()
                  .minInc(1)
                  .maxInc(60)
                  .commit();

            UINT32_ELEMENT(expected)
                  .key("topologyCheck.toleranceDiff")
                  .displayedName("Tolerance diff")
                  .description(
                        "Tolerated difference between logged data and latest update when last logged data is old")
                  .unit(Unit::SECOND)
                  .assignmentOptional()
                  .defaultValue(30)
                  .reconfigurable()
                  .minInc(1)
                  .maxInc(600)
                  .commit();

            STRING_ELEMENT(expected)
                  .key("loggermap")
                  .displayedName("Logger map file")
                  .assignmentOptional()
                  .defaultValue("loggermap.xml")
                  .commit();

            STRING_ELEMENT(expected)
                  .key("blocklistfile")
                  .displayedName("Blocklist file")
                  .assignmentOptional()
                  .defaultValue("blocklist.xml")
                  .init()
                  .commit();

            NODE_ELEMENT(expected)
                  .key("blocklist")
                  .displayedName("blockList")
                  .description("Hash of 'deviceIds' and 'classIds' lists that are not archived")
                  .expertAccess()
                  .commit();

            VECTOR_STRING_ELEMENT(expected)
                  .key("blocklist.deviceIds")
                  .displayedName("Blocked deviceIds")
                  .description("Comma-separated list of deviceIds that are not archived by DataLogger")
                  .reconfigurable()
                  .expertAccess()
                  .assignmentOptional()
                  .defaultValue({})
                  .commit();

            VECTOR_STRING_ELEMENT(expected)
                  .key("blocklist.classIds")
                  .displayedName("Blocked classIds")
                  .description("Comma-separated list of classIds of devices that are not archived by DataLogger")
                  .reconfigurable()
                  .expertAccess()
                  .assignmentOptional()
                  .defaultValue({})
                  .commit();

            Schema loggerMap_tableColumn;
            STRING_ELEMENT(loggerMap_tableColumn) //
                  .key("device")
                  .displayedName("Device")
                  .description("Device")
                  .readOnly()
                  .commit();

            STRING_ELEMENT(loggerMap_tableColumn) //
                  .key("dataLogger")
                  .displayedName("Data Logger")
                  .description("Logger that receives the messages")
                  .readOnly()
                  .commit();

            TABLE_ELEMENT(expected)
                  .key("loggerMap")
                  .displayedName("Loggers Map")
                  .description("Table with the destination of each devices's log")
                  .setColumns(loggerMap_tableColumn)
                  .readOnly()
                  .commit();
        }

        DataLoggerManager::DataLoggerManager(const Hash& input)
            : karabo::core::Device(input),
              m_serverList(input.get<vector<string>>("serverList")),
              m_serverIndex(0),
              m_loggerMapFile(input.get<string>("loggermap")),
              m_strand(std::make_shared<karabo::net::Strand>(karabo::net::EventLoop::getIOService())),
              m_topologyCheckTimer(karabo::net::EventLoop::getIOService()),
              m_loggerClassId("Unsupported"),
              m_blocked(input.get<Hash>("blocklist")),
              m_blockListFile(input.get<string>("blocklistfile")) {
            m_visibility = karabo::util::Schema::ADMIN;
            const std::string loggerType = input.get<std::string>("logger");
            if (loggerType == "FileDataLogger") {
                m_loggerClassId = "FileDataLogger";
                m_readerClassId = "FileLogReader";
            } else if (loggerType == "InfluxDataLogger") {
                m_loggerClassId = "InfluxDataLogger";
                m_readerClassId = "InfluxLogReader";
            }
            m_loggerMap.clear();
            if (std::filesystem::exists(m_loggerMapFile)) {
                karabo::io::loadFromFile(m_loggerMap, m_loggerMapFile);
            }

            KARABO_SYSTEM_SIGNAL("signalLoggerMap", Hash /*loggerMap*/);
            KARABO_SLOT(slotGetLoggerMap);
            KARABO_SLOT(topologyCheck_slotForceCheck);

            if (std::filesystem::exists(m_blockListFile)) {
                Hash blocked;
                karabo::io::loadFromFile(blocked, m_blockListFile);
                if (blocked.has("deviceIds")) {
                    auto& ids = blocked.get<std::vector<std::string>>("deviceIds");
                    trim_vector_elements(ids);
                    m_blocked.set("deviceIds", ids); // overwrite 'input' version
                }
                if (blocked.has("classIds")) {
                    auto& ids = blocked.get<std::vector<std::string>>("classIds");
                    trim_vector_elements(ids);
                    m_blocked.set("classIds", ids); // overwrite 'input' version
                }
            }

            KARABO_INITIAL_FUNCTION(initialize);
        }

        DataLoggerManager::~DataLoggerManager() {}


        void DataLoggerManager::preReconfigure(karabo::util::Hash& incomingReconfiguration) {
            if (incomingReconfiguration.has("blocklist")) {
                std::lock_guard<std::mutex> lock(m_blockedMutex);
                Hash oldList = m_blocked; // save old version
                const Hash& config = incomingReconfiguration.get<Hash>("blocklist");
                if (config.has("deviceIds")) {
                    auto ids = config.get<std::vector<std::string>>("deviceIds");
                    trim_vector_elements(ids);
                    m_blocked.set("deviceIds", ids);
                }
                if (config.has("classIds")) {
                    auto ids = config.get<std::vector<std::string>>("classIds");
                    trim_vector_elements(ids);
                    m_blocked.set("classIds", ids);
                }
                m_strand->post(bind_weak(&Self::evaluateBlockedOnStrand, this, oldList, m_blocked));
            }
        }


        void DataLoggerManager::postReconfigure() {
            std::lock_guard<std::mutex> lock(m_blockedMutex);
            karabo::io::saveToFile(m_blocked, m_blockListFile);
        }


        void DataLoggerManager::evaluateBlockedOnStrand(const karabo::util::Hash& oldHash,
                                                        const karabo::util::Hash& newHash) {
            // Previous config: collect all devices without duplicates ...
            auto oldSet = std::set<std::string>();
            {
                const auto& oldDeviceVec = oldHash.get<std::vector<std::string>>("deviceIds");
                const auto& oldClassVec = oldHash.get<std::vector<std::string>>("classIds");
                oldSet.insert(oldDeviceVec.begin(), oldDeviceVec.end());
                for (const auto& cls : oldClassVec) {
                    oldSet.insert(m_knownClasses[cls].begin(), m_knownClasses[cls].end());
                }
            }
            // Current config: collect all devices without duplicates ...
            auto newSet = std::set<std::string>();
            {
                const auto& newDeviceVec = newHash.get<std::vector<std::string>>("deviceIds");
                const auto& newClassVec = newHash.get<std::vector<std::string>>("classIds");
                newSet.insert(newDeviceVec.begin(), newDeviceVec.end());
                for (const auto& cls : newClassVec) {
                    newSet.insert(m_knownClasses[cls].begin(), m_knownClasses[cls].end());
                }
            }
            // get devices for which archiving (logging) should be stopped
            std::vector<std::string> goneCandidates(newSet.size()); // 0 <= goneCandidates.size() <= newSet.size()
            {
                std::vector<std::string>::iterator it;
                it = std::set_difference(newSet.begin(), newSet.end(), oldSet.begin(), oldSet.end(),
                                         goneCandidates.begin());
                goneCandidates.resize(it - goneCandidates.begin());
            }
            // get devices for which the logging should be started
            std::vector<std::string> newCandidates(oldSet.size()); // 0 << newCandidates.size() <= oldSet.size()
            {
                std::vector<std::string>::iterator it;
                it = std::set_difference(oldSet.begin(), oldSet.end(), newSet.begin(), newSet.end(),
                                         newCandidates.begin());
                newCandidates.resize(it - newCandidates.begin());
            }
            // stop logging ...
            for (const auto& deviceId : goneCandidates) {
                // Function 'goneDeviceToLog' is protected in case of deviceId is not logged
                KARABO_LOG_FRAMEWORK_INFO << "Block list changes cause logging for \"" << deviceId << "\" to stop";
                goneDeviceToLog(deviceId);
            }
            // start logging ...
            for (const auto& deviceId : newCandidates) {
                // Function 'newDeviceToLog' is protected against duplicates...
                KARABO_LOG_FRAMEWORK_INFO << "Block list changes cause logging for \"" << deviceId << "\" to start";
                newDeviceToLog(deviceId);
            }
        }


        void DataLoggerManager::initialize() {
            if (!m_blocked.empty()) {
                // Fill out the values from file to the blocklist node for proper rendering in GUI
                std::lock_guard<std::mutex> lock(m_blockedMutex);
                set("blocklist", m_blocked);
            }

            std::string exceptTxt;
            try {
                checkLoggerMap(); // throws if loggerMap and serverList are inconsistent

                // Setup m_loggerData from server list
                const Hash data("state", LoggerState::OFFLINE, "backlog", std::unordered_set<std::string>(),
                                "beingAdded", std::unordered_set<std::string>(), "devices",
                                std::unordered_set<std::string>());
                for (const std::string& server : m_serverList) {
                    m_loggerData.set(server, data);
                }

                if (m_loggerClassId == "InfluxDataLogger" && get<std::string>("influxDataLogger.dbname").empty()) {
                    // Initialise DB name from broker topic
                    const std::string dbName(getTopic());
                    KARABO_LOG_FRAMEWORK_INFO << "Switch to Influx DB name '" << dbName << "'";
                    set("influxDataLogger.dbname", dbName);
                }

                // Register handlers here
                remote().registerInstanceNewMonitor(std::bind(&DataLoggerManager::instanceNewHandler, this, _1));
                remote().registerInstanceGoneMonitor(std::bind(&DataLoggerManager::instanceGoneHandler, this, _1, _2));

                // Switch on instance tracking - which is blocking a while.
                // Note that instanceNew(..) will be called for all instances already in the game.
                remote().enableInstanceTracking();

                // Publish logger map read from disc. Do that as late as possible in the initialization procedure
                // to give those interested the chance to register their slots after we sent signalInstanceNew.
                {
                    std::lock_guard<std::mutex> lock(
                          m_loggerMapMutex); // m_loggerMap must not be changed while we process it
                    set("loggerMap", makeLoggersTable());
                    emit<Hash>("signalLoggerMap", m_loggerMap);
                }

                // Start regular topology checks (and update State to ON)
                m_strand->post(bind_weak(&Self::launchTopologyCheck, this));

            } catch (const karabo::util::Exception& ke) {
                exceptTxt = ke.userFriendlyMsg(true);
            } catch (const std::exception& e) {
                exceptTxt = e.what();
            }
            if (!exceptTxt.empty()) {
                std::string msg("Failure in initialize(), likely a restart is needed: ");
                msg += exceptTxt;
                KARABO_LOG_FRAMEWORK_ERROR << msg;
                updateState(State::ERROR, Hash("status", msg));
            }
        }


        void DataLoggerManager::checkLoggerMap() {
            // Check that all servers that are supposed to host DataLoggers are in server list.

            // First get server ids - the values of the logger map.
            std::unordered_set<std::string> serversInMap; // use set to filter out duplications
            {
                std::lock_guard<std::mutex> lock(
                      m_loggerMapMutex); // m_loggerMap must not be changed while we process it
                for (Hash::const_iterator it = m_loggerMap.begin(), itEnd = m_loggerMap.end(); it != itEnd; ++it) {
                    serversInMap.insert(it->getValue<std::string>());
                }
            }
            // Now loop and check that all from logger map are also in configured server list
            for (const std::string& serverInMap : serversInMap) {
                if (find(m_serverList.begin(), m_serverList.end(), serverInMap) == m_serverList.end()) {
                    throw KARABO_LOGIC_EXCEPTION("Inconsistent '" + m_loggerMapFile +
                                                 "' and 'serverList' configuration: '" + serverInMap +
                                                 "' is in map, but not in list.");
                }
            }
        }


        void DataLoggerManager::topologyCheck_slotForceCheck() {
            if (m_topologyCheckTimer.cancel() > 0) {
                topologyCheck(boost::system::error_code());
            }
        }


        void DataLoggerManager::launchTopologyCheck() {
            // Publish last results except if in INIT (because then there was no last run!):
            if (getState() != State::INIT) {
                const std::pair<bool, std::string> badAndStatus(checkSummary());
                KARABO_LOG_FRAMEWORK_INFO << "Check finished - " << badAndStatus.second;
                set(Hash("topologyCheck",
                         Hash("lastCheckDoneUtc", Epochstamp().toFormattedString(), "loggingProblem",
                              static_cast<signed char>(badAndStatus.first), "lastCheckResult", badAndStatus.second)));
            }

            updateState(State::ON);
            m_topologyCheckTimer.expires_after(minutes(get<unsigned int>("topologyCheck.interval")));
            m_topologyCheckTimer.async_wait(bind_weak(&Self::topologyCheck, this, boost::asio::placeholders::error));
        }


        std::pair<bool, std::string> DataLoggerManager::checkSummary() {
            std::stringstream checkResult;
            Hash newCheckStatus;

            bool bad = false;
            if (m_checkStatus.has("offline")) {
                checkResult << "   Offline logger servers: "
                            << toString(m_checkStatus.get<std::set<std::string>>("offline")) << "\n";
                m_checkStatus.erase("offline"); // skip it from loop below!
                bad = true;
            }

            // The remaining keys are serverIds:
            for (const Hash::Node& node : m_checkStatus) {
                const Hash& serverHash = node.getValue<Hash>();
                const std::string& serverId = node.getKey();
                checkResult << "\n   " << serverId << ": ";
                if (serverHash.has("loggerQueryFailed")) {
                    checkResult << "Query to logger failed.\n";
                    bad = true;
                    continue;
                }
                if (serverHash.has("emptyTimestamp")) {
                    const auto& devs = serverHash.get<std::set<std::string>>("emptyTimestamp");
                    checkResult << "\n      Empty time stamps for " << toString(devs);
                    // Keep info for next round, but with new key to check next time whether still empty timestamp:
                    newCheckStatus.set(serverId, Hash("emptyTimestampLast", devs));
                }
                // No need to summarize "emptyTimestampLast" here: It was in summary in last round and
                // - either is fine now,
                // - or is not online anymore,
                // - or appears in "forced" below

                if (serverHash.has("forced")) {
                    const auto& devs = serverHash.get<std::set<std::string>>("forced");
                    checkResult << "\n      Re-enforced logging for " << devs.size() << " devices: " << toString(devs);
                    bad = true;
                }
                if (serverHash.has("detailsRequested")) {
                    const auto& devs = serverHash.get<std::set<std::string>>("detailsRequested");
                    if (devs.size() > 5) {
                        // Prints the first three and the total number of devices.
                        checkResult << "\n      Details requested for " << *std::next(devs.begin(), 0) << ", "
                                    << *std::next(devs.begin(), 1) << ", " << *std::next(devs.begin(), 2) << " (and "
                                    << devs.size() - 3 << " more devices...)";
                    } else {
                        checkResult << "\n      Details requested for " << toString(devs);
                    }
                }
                if (serverHash.has("deviceQueryFailed")) {
                    const auto& devs = serverHash.get<std::set<std::string>>("deviceQueryFailed");
                    checkResult << "\n      " << devs.size() << " device queries failed: " << toString(devs);
                    bad = true; // Could just being shutdown and logger was not yet aware...
                }
                if (serverHash.has("stopped")) {
                    const auto& devs = serverHash.get<std::set<std::string>>("stopped");
                    checkResult << "\n      " << devs.size()
                                << " devices now offline, logging stopped: " << toString(devs);
                }
            }
            // Clear check status, but keep what is needed for next check
            m_checkStatus = std::move(newCheckStatus);

            const std::string checkResultStr(checkResult.str());
            std::string prefix(bad ? "Found problems:" : "Ok");
            if (!bad) {
                prefix += (checkResultStr.empty() ? "." : ", just note:");
            }
            return std::make_pair(bad, prefix += checkResultStr);
        }

        void DataLoggerManager::topologyCheck(const boost::system::error_code& e) {
            if (e == boost::asio::error::operation_aborted) {
                return;
            }
            KARABO_LOG_FRAMEWORK_INFO << "Launching topology check from state " << getState().name();
            updateState(State::MONITORING);
            set("topologyCheck.lastCheckStartedUtc", Epochstamp().toFormattedString());
            m_strand->post(bind_weak(&Self::topologyCheckOnStrand, this));
        }


        void DataLoggerManager::topologyCheckOnStrand() {
            printLoggerData();

            auto loggerCounter = std::make_shared<std::atomic<size_t>>(m_loggerData.size());
            const unsigned int timeout = get<unsigned int>("timeout");
            for (const Hash::Node& serverNode : m_loggerData) {
                const std::string& serverId = serverNode.getKey();
                const Hash& serverData = serverNode.getValue<Hash>();
                if (serverData.get<LoggerState>("state") != LoggerState::RUNNING) {
                    KARABO_LOG_FRAMEWORK_INFO << "Skip checking '" << serverId << "' since not running";
                    addToSetOrCreate(m_checkStatus, "offline", serverId);
                    --(*loggerCounter);
                    continue;
                }
                const std::string loggerId(serverIdToLoggerId(serverId));
                KARABO_LOG_FRAMEWORK_DEBUG << "Request config of logger '" << loggerId << "'";
                call(loggerId, "flush"); // Force flushing and thus update of "lastUpdatesUtc"
                auto okHandler = bind_weak(&Self::checkLoggerConfig, this, true, loggerCounter, _1, _2);
                auto failHandler = bind_weak(&Self::checkLoggerConfig, this, false, loggerCounter, Hash(), loggerId);
                request(loggerId, "slotGetConfiguration")
                      .timeout(timeout)
                      .receiveAsync<Hash, std::string>(okHandler, failHandler);
            }
            if (0 == *loggerCounter) { // All loggers are not (yet) running
                m_strand->post(bind_weak(&Self::launchTopologyCheck, this));
            }
        }


        void DataLoggerManager::printLoggerData() const {
            std::stringstream out;
            for (const Hash::Node& serverNode : m_loggerData) {
                const Hash& serverHash = serverNode.getValue<Hash>();
                const LoggerState state = serverHash.get<LoggerState>("state");
                out << "\n  Server " << serverNode.getKey() << " is "
                    << (state == LoggerState::OFFLINE ? "offline"
                                                      : (state == LoggerState::RUNNING ? "running" : "instantiating"));
                for (const std::string& key : std::vector<std::string>({"devices", "backlog", "beingAdded"})) {
                    out << "\n    " << key << ": ";
                    size_t nChars = 0;
                    for (const std::string& id : serverHash.get<std::unordered_set<std::string>>(key)) {
                        // If line gets too long, start a new one - but at least print one id per line:
                        if ((nChars += (id.size())) > 55 && nChars != id.size()) {
                            out << "\n             ";
                            nChars = 0;
                        }
                        out << id << ", ";
                        nChars += 2; // for comma and space
                    }
                }
            }

            KARABO_LOG_FRAMEWORK_INFO << "Internal logger info:" << out.str();
        }


        void DataLoggerManager::checkLoggerConfig(bool ok, const std::shared_ptr<std::atomic<size_t>>& loggerCounter,
                                                  const Hash& config, const std::string& loggerId) {
            // Put on strand to be sequential, but take care that the re-throw trick for exception details
            // will not work anymore.
            std::string errorTxt;
            if (!ok) {
                try {
                    throw;
                } catch (const std::exception& e) {
                    errorTxt = e.what();
                }
            }
            m_strand->post(
                  bind_weak(&Self::checkLoggerConfigOnStrand, this, errorTxt, loggerCounter, config, loggerId));
        }


        void DataLoggerManager::checkLoggerConfigOnStrand(const std::string& errorTxt,
                                                          const std::shared_ptr<std::atomic<size_t>>& loggerCounter,
                                                          const Hash& config, const std::string& loggerId) {
            Epochstamp now;
            const std::string serverId(loggerIdToServerId(loggerId));
            if (errorTxt.empty()) {
                const TimeDuration tolerance(0, 0, get<unsigned int>("topologyCheck.toleranceLogged"), 0ull,
                                             0ull); // from minutes
                const std::vector<Hash>& updates = config.get<std::vector<Hash>>("lastUpdatesUtc");
                // DataLogger logic ensures that the content of the last updates table matches devicesToBeLogged
                auto loggedDevCounter = std::make_shared<std::atomic<size_t>>(updates.size());
                const unsigned int timeout = get<unsigned int>("timeout");
                std::vector<std::string> idsWithoutTimestamp;
                for (const Hash& row : updates) {
                    const Hash::Node& lastUpdateNode = row.getNode("lastUpdateUtc");
                    const std::string& lastUpdateStr = lastUpdateNode.getValue<std::string>();
                    const std::string& deviceId = row.get<std::string>("deviceId");

                    if (lastUpdateStr.empty() ||
                        !Epochstamp::hashAttributesContainTimeInformation(lastUpdateNode.getAttributes())) {
                        // No update yet, so DataLogging for that device likely being started - book-keeping to check
                        // next time:
                        idsWithoutTimestamp.push_back(deviceId);
                        // Check the last try:
                        const std::string keyEmpty(serverId + ".emptyTimestampLast");
                        if (m_checkStatus.has(keyEmpty)) {
                            std::set<std::string>& emptyLast = m_checkStatus.get<std::set<std::string>>(keyEmpty);
                            auto it = std::find(emptyLast.begin(), emptyLast.end(), deviceId);
                            if (it != emptyLast.end()) {
                                KARABO_LOG_FRAMEWORK_WARN << "Device '" << deviceId << "' logged by '" << loggerId
                                                          << "' still has no last update stamp - force logging";
                                forceDeviceToBeLogged(deviceId);
                                addToSetOrCreate(m_checkStatus, serverId + ".forced", deviceId);
                            }
                        }
                        continue;
                    }
                    const Epochstamp lastUpdate(Epochstamp::fromHashAttributes(lastUpdateNode.getAttributes()));
                    if (now > lastUpdate &&
                        now.elapsed(lastUpdate) > tolerance) { // Caveat: TimeDuration is always positive!
                        KARABO_LOG_FRAMEWORK_DEBUG << loggerId << " logged last data from '" << deviceId
                                                   << "' rather long ago: " << lastUpdateStr << " (UTC)";
                        addToSetOrCreate(m_checkStatus, serverId + ".detailsRequested", deviceId);

                        const unsigned int toleranceSec = std::max(get<unsigned int>("topologyCheck.toleranceDiff"),
                                                                   config.get<unsigned int>("flushInterval"));
                        auto okHandler = bind_weak(&Self::checkDeviceConfig, this, true, loggerCounter, loggerId,
                                                   toleranceSec, loggedDevCounter, lastUpdate, _1, _2);
                        auto failHandler = bind_weak(&Self::checkDeviceConfig, this, false, loggerCounter, loggerId,
                                                     toleranceSec, loggedDevCounter, lastUpdate, Hash(), deviceId);
                        request(deviceId, "slotGetConfiguration")
                              .timeout(timeout)
                              .receiveAsync<Hash, std::string>(okHandler, failHandler);
                    } else {
                        KARABO_LOG_FRAMEWORK_DEBUG << "Device '" << deviceId << "' OK according to logger '" << loggerId
                                                   << "': " << *loggedDevCounter - 1 << " left ";
                        --(*loggedDevCounter);
                    }
                }
                if (!idsWithoutTimestamp.empty()) {
                    (*loggedDevCounter) -= idsWithoutTimestamp.size();
                    KARABO_LOG_FRAMEWORK_INFO << "Logger " << loggerId << " lacks last update timestamp of "
                                              << toString(idsWithoutTimestamp);
                    m_checkStatus.set(serverId + ".emptyTimestamp",
                                      std::set<std::string>(idsWithoutTimestamp.begin(), idsWithoutTimestamp.end()));
                }

                if (0 == *loggedDevCounter) {
                    // No suspicious devices logged by this logger
                    KARABO_LOG_FRAMEWORK_INFO << "All devices logged by " << loggerId << " are recently logged.";
                    --(*loggerCounter);
                }
            } else {
                // Failure handler
                KARABO_LOG_FRAMEWORK_INFO << "Failed to query configuration of " << loggerId << ": " << errorTxt
                                          << "\n logger left: " << *loggerCounter - 1;
                m_checkStatus.set(serverId + ".loggerQueryFailed", true);
                --(*loggerCounter);
            }

            if (0 == *loggerCounter) {
                m_strand->post(bind_weak(&Self::launchTopologyCheck, this));
            }
        }


        void DataLoggerManager::forceDeviceToBeLogged(const std::string& deviceId) {
            goneDeviceToLog(deviceId);
            newDeviceToLog(deviceId);
        }


        void DataLoggerManager::checkDeviceConfig(bool ok, const std::shared_ptr<std::atomic<size_t>>& loggerCounter,
                                                  const std::string& loggerId, unsigned int toleranceSec,
                                                  const std::shared_ptr<std::atomic<size_t>>& loggedDevCounter,
                                                  Epochstamp lastUpdateLogger, const Hash& config,
                                                  const std::string& deviceId) {
            std::string errorTxt;
            if (!ok) {
                // Note that this can e.g. happen when 'deviceId' went down when the manager was offline and then
                // the manager is restarted. The failure would be a timeout then.
                // Trick:
                // For a failure handler, re-throwing the exception does not work when posting further.
                try {
                    throw;
                } catch (const std::exception& e) {
                    errorTxt = e.what();
                }
            }
            m_strand->post(bind_weak(&Self::checkDeviceConfigOnStrand, this, errorTxt, loggerCounter, loggerId,
                                     toleranceSec, loggedDevCounter, lastUpdateLogger, config, deviceId));
        }


        void DataLoggerManager::checkDeviceConfigOnStrand(const std::string& errorTxt,
                                                          const std::shared_ptr<std::atomic<size_t>>& loggerCounter,
                                                          const std::string& loggerId, unsigned int toleranceSec,
                                                          const std::shared_ptr<std::atomic<size_t>>& loggedDevCounter,
                                                          Epochstamp lastUpdateLogger, const Hash& config,
                                                          const std::string& deviceId) {
            if (errorTxt.empty()) {
                const Epochstamp lastDeviceUpdate = mostRecentEpochstamp(config);
                const TimeDuration tolerance(0, 0, 0, toleranceSec, 0ull);
                if (lastDeviceUpdate > lastUpdateLogger && lastDeviceUpdate.elapsed(lastUpdateLogger) > tolerance) {
                    std::stringstream logTxt;
                    logTxt << deviceId << " had last update at " << lastDeviceUpdate.toFormattedString()
                           << " UTC, but most recent data logged by " << loggerId << " is from "
                           << lastUpdateLogger.toFormattedString() << " UTC.";
                    KARABO_LOG_FRAMEWORK_WARN << logTxt.str() << " - force logging to start again";
                    forceDeviceToBeLogged(deviceId);
                    const std::string serverId(loggerIdToServerId(loggerId));
                    addToSetOrCreate(m_checkStatus, serverId + ".forced", deviceId);
                } else {
                    KARABO_LOG_FRAMEWORK_DEBUG << "Last update of " << deviceId << " at "
                                               << lastDeviceUpdate.toFormattedString() << " UTC: logger not behind.";
                }
            } else {
                // Failure handler:
                KARABO_LOG_FRAMEWORK_INFO << "Failed to query device configuration of " << deviceId << ": " << errorTxt;
                const std::string serverId(loggerIdToServerId(loggerId));
                addToSetOrCreate(m_checkStatus, serverId + ".deviceQueryFailed", deviceId);
                // If request failed, it maybe that the device went offline and the manager did not notice since
                // it was down at that time and restarted later. In that case the device should not be in the logger
                // data - so better check that!
                // But do not trust remote().getSystemInformation() - it is not synchronised with our m_strand.
                const auto& knownDevs = m_loggerData.get<std::unordered_set<std::string>>(serverId + ".devices");
                if (knownDevs.find(deviceId) == knownDevs.end()) {
                    KARABO_LOG_FRAMEWORK_WARN << "Device " << deviceId << " not known for logger " << loggerId
                                              << " - stop logging it!";
                    addToSetOrCreate(m_checkStatus, serverId + ".stopped", deviceId);
                    call(loggerId, "slotTagDeviceToBeDiscontinued", "D", deviceId);
                }
            }

            if (0 == --(*loggedDevCounter)) {
                // Last device of this logger, so decrease also the logger counter
                if (0 == --(*loggerCounter)) {
                    // This was the last logger
                    m_strand->post(bind_weak(&Self::launchTopologyCheck, this));
                }
            }
        }


        karabo::util::Epochstamp DataLoggerManager::mostRecentEpochstamp(const Hash& config,
                                                                         Epochstamp oldStamp) const {
            for (const Hash::Node& node : config) {
                if (Epochstamp::hashAttributesContainTimeInformation(node.getAttributes())) {
                    const Epochstamp stamp(Epochstamp::fromHashAttributes(node.getAttributes()));
                    if (stamp > oldStamp) {
                        oldStamp = stamp;
                    }
                }
                // If it is a Hash, we recurse inside the NODE.
                // A vector<Hash> would be a TABLE_ELEMENT that we treat atomically, so we do not recurse.
                if (node.is<Hash>()) {
                    oldStamp = mostRecentEpochstamp(node.getValue<Hash>(), oldStamp);
                }
            }
            return oldStamp;
        }


        void DataLoggerManager::instantiateReaders(const std::string& serverId) {
            // For now we continue to instantiate all DATALOGREADERS_PER_SERVER log reader instances per server.
            // But that is only since clients running in releases before 2.17.0 assume their existence.
            // Once no pre-2.17.0 clients are supported anymore, switch to a single reader per server.
            for (unsigned int i = 0; i < DATALOGREADERS_PER_SERVER; ++i) {
                const std::string readerId = serverIdToReaderId(serverId, i);
                if (!remote().exists(readerId).first) {
                    Hash hash;
                    Hash config;
                    if (m_loggerClassId == "FileDataLogger") {
                        hash.set("classId", "FileLogReader");
                        hash.set("deviceId", readerId);
                        config.set("directory", get<string>("fileDataLogger.directory"));
                    } else if (m_loggerClassId == "InfluxDataLogger") {
                        hash.set("classId", "InfluxLogReader");
                        hash.set("deviceId", readerId);
                        config.set("urlConfigSchema", get<string>("influxDataLogger.urlRead"));
                        // Schema description assumes that InfluxLogReader treats empty value of "urlReadPropHistory"
                        config.set("urlPropHistory", get<string>("influxDataLogger.urlReadPropHistory"));
                        config.set("dbname", get<string>("influxDataLogger.dbname"));
                    }
                    hash.set("configuration", config);
                    const std::string& xLogReader = hash.get<std::string>("classId");
                    KARABO_LOG_FRAMEWORK_INFO << "Trying to instantiate '" << readerId << "' of type '" << xLogReader
                                              << "' on server '" << serverId << "'";

                    remote().instantiateNoWait(serverId, hash);
                }
            }
        }

        void DataLoggerManager::slotGetLoggerMap() {
            std::lock_guard<std::mutex> lock(m_loggerMapMutex); // m_loggerMap must not be changed while we process  it
            reply(m_loggerMap);
        }


        std::string DataLoggerManager::loggerServerId(const std::string& deviceId, bool addIfNotYetInMap) {
            std::string serverId;

            const std::string& deviceIdInMap(DATALOGGER_PREFIX +
                                             deviceId); // DATALOGGER_PREFIX for xml files from < 2.6.0
            std::lock_guard<std::mutex> lock(m_loggerMapMutex);
            if (m_loggerMap.has(deviceIdInMap)) {
                serverId = m_loggerMap.get<string>(deviceIdInMap);
            } else if (addIfNotYetInMap) {
                if (m_serverList.empty()) {
                    // Cannot happen but for better diagnostics in case it does:
                    throw KARABO_PARAMETER_EXCEPTION(
                          "List of servers for data logging is empty."
                          " You have to define one data logger server, at least!");
                }
                m_serverIndex %= m_serverList.size();
                serverId = m_serverList[m_serverIndex++];
                m_loggerMap.set(deviceIdInMap, serverId);

                // Logger map changed, so publish - online and as backup
                set("loggerMap", makeLoggersTable());
                emit<Hash>("signalLoggerMap", m_loggerMap);
                karabo::io::saveToFile(m_loggerMap, m_loggerMapFile);
            }
            return serverId;
        }


        void DataLoggerManager::instanceNewHandler(const karabo::util::Hash& topologyEntry) {
            m_strand->post(bind_weak(&DataLoggerManager::instanceNewOnStrand, this, topologyEntry));
        }


        void DataLoggerManager::instanceNewOnStrand(const karabo::util::Hash& topologyEntry) {
            const std::string& type = topologyEntry.begin()->getKey(); // fails if empty...
            // const ref is fine even for temporary std::string
            const std::string& instanceId = (topologyEntry.has(type) && topologyEntry.is<Hash>(type)
                                                   ? topologyEntry.get<Hash>(type).begin()->getKey()
                                                   : std::string("?"));

            KARABO_LOG_FRAMEWORK_INFO << "instanceNew --> instanceId: '" << instanceId << "', type: '" << type << "'";

            if (type == "device") {
                const Hash& entry = topologyEntry.begin()->getValue<Hash>();
                const std::string classId = (entry.hasAttribute(instanceId, "classId")
                                                   ? entry.getAttribute<std::string>(instanceId, "classId")
                                                   : std::string(""));
                if (!classId.empty()) m_knownClasses[classId].insert(instanceId);
                if (!isDeviceBlocked(instanceId) && !isClassBlocked(classId)) {
                    newDeviceToLog(instanceId);
                } else {
                    KARABO_LOG_FRAMEWORK_INFO << "Logging of instance '" << instanceId << "' blocked.";
                }
                if (classId == m_loggerClassId) {
                    // A new logger has started - check whether there is more work for it to do
                    newLogger(instanceId);
                }
            } else if (type == "server") {
                if (m_loggerData.has(instanceId)) {
                    // One of our servers!
                    newLoggerServer(instanceId);
                }
            }
        }


        void DataLoggerManager::newDeviceToLog(const std::string& deviceId) {
            // Figure out which server and thus which logger this runs:
            const std::string serverId(loggerServerId(deviceId, true));

            // Put deviceId to backlog - independent of state:
            Hash& data = m_loggerData.get<Hash>(serverId);
            data.get<std::unordered_set<std::string>>("backlog").insert(deviceId);

            // If logger is already running, transfer the (likely new and size-1-) backlog to it
            if (data.get<LoggerState>("state") == LoggerState::RUNNING) {
                addDevicesToBeLogged(serverIdToLoggerId(serverId), data);
            } else {
                KARABO_LOG_FRAMEWORK_INFO << "New device '" << deviceId << "' to be logged, but logger not yet running";
            }
        }


        void DataLoggerManager::newLogger(const std::string& loggerId) {
            const std::string serverId(loggerIdToServerId(loggerId));
            if (serverId.empty()) {
                // E.g. a logger started by hand by someone
                KARABO_LOG_FRAMEWORK_WARN << "Discovered logger with unexpected id '" << loggerId << "', cannot treat.";
                return;
            }
            // Get data for this server to access backlog and state
            Hash& data = m_loggerData.get<Hash>(serverId);
            data.set("state", LoggerState::RUNNING);
            // Any "devices" or "beingAdded" left do not need to be added to "backlog" here for cases where logger is
            // killed and the loss of heart beats was not yet discovered:
            // The instanceNew that triggers this newLogger here has triggered an instanceGone before anyway.
            addDevicesToBeLogged(loggerId, data);
        }


        void DataLoggerManager::addDevicesToBeLogged(const std::string& loggerId, Hash& serverData) {
            std::unordered_set<std::string>& backlog = serverData.get<std::unordered_set<std::string>>("backlog");
            if (!backlog.empty()) {
                // Keep track of what is being added
                std::unordered_set<std::string>& beingAdded =
                      serverData.get<std::unordered_set<std::string>>("beingAdded");
                beingAdded.insert(backlog.begin(), backlog.end());
                KARABO_LOG_FRAMEWORK_INFO << "For '" << loggerId << "', adding devices: '" << toString(backlog);
                auto successHandler = bind_weak(&DataLoggerManager::addDevicesDone, this, true, loggerId, backlog, _1);
                auto failureHandler = bind_weak(&DataLoggerManager::addDevicesDone, this, false, loggerId, backlog,
                                                std::vector<std::string>());
                const unsigned int timeout = get<unsigned int>("timeout");
                request(loggerId, "slotAddDevicesToBeLogged", std::vector<std::string>(backlog.begin(), backlog.end()))
                      .timeout(timeout)
                      .receiveAsync<std::vector<std::string>>(successHandler, failureHandler);

                backlog.clear();
            }
        }


        void DataLoggerManager::addDevicesDone(bool ok, const std::string& loggerId,
                                               const std::unordered_set<std::string>& calledDevices,
                                               const std::vector<std::string>& alreadyLoggedDevices) {
            // Put on strand to be sequential, but take care that the re-throw trick for exception details
            // will not work anymore.
            std::string errorTxt;
            if (!ok) {
                try {
                    throw;
                } catch (const std::exception& e) {
                    errorTxt = e.what();
                }
            }
            m_strand->post(bind_weak(&DataLoggerManager::addDevicesDoneOnStrand, this, errorTxt, loggerId,
                                     calledDevices, alreadyLoggedDevices));
        }


        void DataLoggerManager::addDevicesDoneOnStrand(const std::string& errorTxt, const std::string& loggerId,
                                                       const std::unordered_set<std::string>& calledDevices,
                                                       const std::vector<std::string>& alreadyLoggedDevices) {
            const std::string serverId(loggerIdToServerId(loggerId));
            Hash& data = m_loggerData.get<Hash>(serverId);

            if (errorTxt.empty()) {
                if (alreadyLoggedDevices.empty()) {
                    KARABO_LOG_FRAMEWORK_INFO << "For '" << loggerId << "', added devices to be logged: '"
                                              << toString(calledDevices) << "'";
                } else {
                    // Can happen when, during initialising, a logger is discovered that was running since before
                    // DataLoggerManager was instantiated.
                    KARABO_LOG_FRAMEWORK_WARN << "Added '" << toString(calledDevices) << "' to be logged by '"
                                              << loggerId << "', but '" << toString(alreadyLoggedDevices)
                                              << "' were already logged.";
                }
                // Remove from "beingAdded" and add to "devices" since done, even those that were already logged:
                // We just did not yet know about it (see above).
                std::unordered_set<std::string>& beingAdded = data.get<std::unordered_set<std::string>>("beingAdded");
                for (const std::string& calledDevice : calledDevices) {
                    beingAdded.erase(calledDevice);
                }
                data.get<std::unordered_set<std::string>>("devices").insert(calledDevices.begin(), calledDevices.end());
            } else {
                // Can happen as timeout when logger just shutdown
                KARABO_LOG_FRAMEWORK_ERROR << "For '" << loggerId << "', failed to add '" << toString(calledDevices)
                                           << "' to be logged since: " << errorTxt;

                // Put devices to log back to backlog,
                // but only those "beingAdded" (others could have shutdown meanwhile)
                std::unordered_set<std::string>& beingAdded = data.get<std::unordered_set<std::string>>("beingAdded");
                std::unordered_set<std::string>& backlog = data.get<std::unordered_set<std::string>>("backlog");
                for (const std::string& calledDevice : calledDevices) {
                    auto it = beingAdded.find(calledDevice);
                    if (it != beingAdded.end()) {
                        backlog.insert(calledDevice);
                        beingAdded.erase(it);
                    }
                }
                if (data.get<LoggerState>("state") == LoggerState::RUNNING) {
                    // Try again, logger likely just came up:
                    addDevicesToBeLogged(loggerId, data);
                }
            }
        }

        void DataLoggerManager::newLoggerServer(const std::string& serverId) {
            instantiateLogger(serverId);
            instantiateReaders(serverId);
        }

        void DataLoggerManager::instantiateLogger(const std::string& serverId) {
            // Get data for this server to access backlog and state
            Hash& data = m_loggerData.get<Hash>(serverId);

            std::string logMsg;
            const auto previousState = data.get<LoggerState>("state");
            switch (previousState) {
                case LoggerState::OFFLINE:
                    break;                       // Most expected state
                case LoggerState::INSTANTIATING: // Not so clear when this happens
                                                 // No 'break;'
                case LoggerState::RUNNING:
                    // Likely since logger discovered before its server when manager starts into a running
                    // system. Our try to instantiate below will then fail since logger is already running!
                    // Or lack of heartbeats lets the server be seen dead and we first get the 'gone' of the device
                    // that the client injected. Instantiation timeout since server dead.
                    logMsg += ". Note: State before was ";
                    logMsg += (previousState == LoggerState::RUNNING ? "RUNNING" : "INSTANTIATING");
                    // 'default:' not needed
            }
            data.set("state", LoggerState::INSTANTIATING);

            // Instantiate logger, but do not yet specify "devicesToBeLogged":
            // Having one channel only to transport this info (slotAddDevicesToBeLogged) simplifies logic.
            Hash config;
            if (m_loggerClassId == "FileDataLogger") {
                config = get<Hash>("fileDataLogger");
            } else if (m_loggerClassId == "InfluxDataLogger") {
                config = get<Hash>("influxDataLogger");
                config.erase("urlReadPropHistory"); // logger needs read address only for schema
            }
            config.set("flushInterval", get<int>("flushInterval"));
            config.set("performanceStatistics.enable", get<bool>("enablePerformanceStats"));
            const std::string loggerId(serverIdToLoggerId(serverId));
            const Hash hash("classId", m_loggerClassId, "deviceId", loggerId, "configuration", config);

            KARABO_LOG_FRAMEWORK_INFO << "Trying to instantiate '" << loggerId << "' of type '" << m_loggerClassId
                                      << "' on server '" << serverId << "'" << logMsg;
            auto success = bind_weak(&DataLoggerManager::loggerInstantiationHandler, this, _1, _2, false);
            auto failure = bind_weak(&DataLoggerManager::loggerInstantiationHandler, this, false, loggerId, true);
            request(serverId, "slotStartDevice", hash).receiveAsync<bool, string>(success, failure);
        }

        void DataLoggerManager::loggerInstantiationHandler(bool ok, const std::string& devId, bool isFailure) {
            if (isFailure) { // Called as failure handler ==> can throw to figure out which exception we have.
                try {
                    throw;
                } catch (const RemoteException& e) {
                    const std::string details(e.detailedMsg());
                    if (details.find("already running/starting on this server") ==
                              std::string::npos ||                                       // from DeviceServer.cc
                        details.find("Another instance with ID") == std::string::npos) { // from SignalSlotable.cc
                        // Instantiating failed since already there! Treat as new.
                        m_strand->post(bind_weak(&DataLoggerManager::newLogger, this, devId));
                    } else {
                        KARABO_LOG_FRAMEWORK_ERROR << "Unexpected failure to instantiate '" << devId
                                                   << "', will try again: " << details;
                        m_strand->post(
                              bind_weak(&DataLoggerManager::instantiateLogger, this, loggerIdToServerId(devId)));
                    }
                } catch (const TimeoutException&) {
                    // Server unreachable - the instanceNew of its restart will get (or maybe already got) us going
                    Exception::clearTrace();
                    KARABO_LOG_FRAMEWORK_WARN << "Instantiating " << devId << " timed out";
                } catch (const std::exception& e) { // How to get here?
                    KARABO_LOG_FRAMEWORK_ERROR << "Unknown failure to instantiate '" << devId
                                               << "' (will try again): " << e.what();
                    m_strand->post(bind_weak(&DataLoggerManager::instantiateLogger, this, loggerIdToServerId(devId)));
                }
            } else if (!ok) { // Should never happen - C++ server never returns that!
                KARABO_LOG_FRAMEWORK_ERROR << "Unexpected failure to instantiate (will try again): " << devId;
                m_strand->post(bind_weak(&DataLoggerManager::instantiateLogger, this, loggerIdToServerId(devId)));
            } else {
                // Properly instantiated - instanceNew will trigger (or has already triggered) further action
                KARABO_LOG_FRAMEWORK_INFO << "Sucessfully instantiated " << devId;
            }
        }

        void DataLoggerManager::instanceGoneHandler(const std::string& instanceId,
                                                    const karabo::util::Hash& instanceInfo) {
            m_strand->post(bind_weak(&DataLoggerManager::instanceGoneOnStrand, this, instanceId, instanceInfo));
        }


        void DataLoggerManager::instanceGoneOnStrand(const std::string& instanceId,
                                                     const karabo::util::Hash& instanceInfo) {
            // const ref is fine even for temporary std::string
            const std::string& type = (instanceInfo.has("type") && instanceInfo.is<std::string>("type")
                                             ? instanceInfo.get<std::string>("type")
                                             : std::string("unknown"));
            const std::string& serverId = (instanceInfo.has("serverId") && instanceInfo.is<std::string>("serverId")
                                                 ? instanceInfo.get<string>("serverId")
                                                 : std::string("?"));

            KARABO_LOG_FRAMEWORK_INFO << "instanceGoneHandler -->  instanceId : '" << instanceId << "', type : " << type
                                      << " on server '" << serverId << "'";

            if (type == "device") {
                // Figure out who logs and tell to stop
                goneDeviceToLog(instanceId);
                const std::string& classId = (instanceInfo.has("classId") && instanceInfo.is<std::string>("classId")
                                                    ? instanceInfo.get<string>("classId")
                                                    : std::string(""));
                if (classId == m_loggerClassId) {
                    goneLogger(instanceId);
                } else if (classId == m_readerClassId) {
                    goneReader(instanceId);
                }
                if (!classId.empty()) m_knownClasses[classId].erase(instanceId);
            } else if (type == "server") {
                if (m_loggerData.has(instanceId)) {
                    // It is one of our logger servers
                    goneLoggerServer(instanceId);
                }
            }
        }


        void DataLoggerManager::goneDeviceToLog(const std::string& deviceId) {
            const std::string serverId(loggerServerId(deviceId, false));
            if (!serverId.empty()) { // else device not in map and thus neither logged
                // Remove from any tracking:
                Hash& data = m_loggerData.get<Hash>(serverId);
                std::unordered_set<std::string>& backlog = data.get<std::unordered_set<std::string>>("backlog");
                std::unordered_set<std::string>& beingAdded = data.get<std::unordered_set<std::string>>("beingAdded");
                std::unordered_set<std::string>& loggedIds = data.get<std::unordered_set<std::string>>("devices");
                backlog.erase(deviceId);
                beingAdded.erase(deviceId);
                loggedIds.erase(deviceId);

                const LoggerState state = data.get<LoggerState>("state");
                switch (state) {
                    case LoggerState::RUNNING:
                        // Likely a normal device shutdown - inform the logger:
                        call(serverIdToLoggerId(serverId), "slotTagDeviceToBeDiscontinued", "D", deviceId);
                        // Add a consistency check:
                        if (!backlog.empty()) {
                            KARABO_LOG_FRAMEWORK_WARN << "Backlog for running server '" << serverId
                                                      << "' not empty, but contains '" << toString(backlog) << "'";
                        }
                        break;
                    case LoggerState::OFFLINE:
                    case LoggerState::INSTANTIATING:
                        // Add a consistency check:
                        if (!loggedIds.empty()) {
                            KARABO_LOG_FRAMEWORK_WARN << "Logged devices for "
                                                      << (state == LoggerState::OFFLINE ? "offline" : "instantiating")
                                                      << "  server '" << serverId << "' not empty, but contains "
                                                      << toString(loggedIds);
                        }
                        break;
                }
            }
        }


        void DataLoggerManager::goneLogger(const std::string& loggerId) {
            const std::string serverId(loggerIdToServerId(loggerId));
            if (serverId.empty()) {
                // E.g. a logger started by hand by someone
                KARABO_LOG_FRAMEWORK_WARN << "Discovered shutdown of logger with unexpected id '" << loggerId
                                          << "', will not treat.";
                return;
            }

            Hash& data = m_loggerData.get<Hash>(serverId);
            std::unordered_set<std::string>& backlog = data.get<std::unordered_set<std::string>>("backlog");
            std::unordered_set<std::string>& beingAdded = data.get<std::unordered_set<std::string>>("beingAdded");
            std::unordered_set<std::string>& loggedIds = data.get<std::unordered_set<std::string>>("devices");

            switch (data.get<LoggerState>("state")) {
                case LoggerState::OFFLINE:
                    KARABO_LOG_FRAMEWORK_WARN << "Logger '" << loggerId << "' gone, but its server gone before.";
                    // But nothing more to do, backlog, beingAdded and loggedIds treated in goneLoggerServer(..)
                    break;
                case LoggerState::INSTANTIATING:
                    KARABO_LOG_FRAMEWORK_WARN << "Logger '" << loggerId << "' gone again while instantiating.";
                    // no 'break;'!
                case LoggerState::RUNNING:
                    // Append logged devices as well as those being added to backlog.
                    // Note: Relying on treatment of those "beingAdded" in failure handling of addDevicesDoneOnStrand
                    //       could be too late if the below instantiateLogger succeeds
                    backlog.insert(loggedIds.begin(), loggedIds.end());
                    loggedIds.clear();
                    backlog.insert(beingAdded.begin(), beingAdded.end());
                    beingAdded.clear();
                    // Instantiate again -- will set "state" appropriately
                    instantiateLogger(serverId);
            }
        }


        void DataLoggerManager::goneReader(const std::string& readerId) {
            const std::string serverId = readerIdToServerId(readerId);
            // instantiateReaders is smart enough to handle already instantiated readers (currently there's more than
            // one LogReader instance per device server).
            instantiateReaders(serverId);
        }


        void DataLoggerManager::goneLoggerServer(const std::string& serverId) {
            Hash& data = m_loggerData.get<Hash>(serverId);

            switch (data.get<LoggerState>("state")) {
                case LoggerState::OFFLINE:
                    KARABO_LOG_FRAMEWORK_ERROR << "Server '" << serverId
                                               << "' gone, but it was already gone before: " << data;
                    // Weird situation - move "devices"/"beingAdded" to "backlog" as in other cases...
                    break;
                case LoggerState::INSTANTIATING:
                    // Expected nice behaviour: Already took note that logger is gone and so tried to start again.
                    // Nothing to do.
                    KARABO_LOG_FRAMEWORK_INFO << "Server '" << serverId << "' gone while instantiating "
                                              << m_loggerClassId << ".";
                    break;
                case LoggerState::RUNNING:
                    // We could come here if instanceGone of a server was detected by lack of heartbeats AND if the
                    // DeviecClient injects the instanceGone of the devices after the instanceGone of the server.
                    // That happened since at least 2.6.0, but should be fixed in 2.20.0.
                    // So: We should never come here!
                    KARABO_LOG_FRAMEWORK_WARN << "Server '" << serverId << "' gone while " << m_loggerClassId
                                              << " still alive.";
                    // Also then we have to move "devices"/"beingAdded" to "backlog".
                    break;
            }

            // Append logged and "being added" devices to backlog - better do for all situations...
            std::unordered_set<std::string>& backlog = data.get<std::unordered_set<std::string>>("backlog");
            std::unordered_set<std::string>& beingAdded = data.get<std::unordered_set<std::string>>("beingAdded");
            std::unordered_set<std::string>& loggedIds = data.get<std::unordered_set<std::string>>("devices");
            backlog.insert(loggedIds.begin(), loggedIds.end());
            loggedIds.clear();
            backlog.insert(beingAdded.begin(), beingAdded.end());
            beingAdded.clear();

            data.set("state", LoggerState::OFFLINE);
        }


        bool DataLoggerManager::isDeviceBlocked(const std::string& deviceId) {
            return isBlocked(deviceId, "deviceIds");
        }


        bool DataLoggerManager::isClassBlocked(const std::string& classId) {
            return isBlocked(classId, "classIds");
        }


        bool DataLoggerManager::isBlocked(const std::string& id, const std::string& typeIds) {
            if (typeIds != "classIds" && typeIds != "deviceIds") {
                throw KARABO_PARAMETER_EXCEPTION("Valid blocklist types are \"classIds\" and \"deviceIds\"");
            }
            std::lock_guard<std::mutex> lock(m_blockedMutex);
            if (!m_blocked.has(typeIds)) return false;
            const auto& ids = m_blocked.get<std::vector<std::string>>(typeIds);
            const auto it = std::find(ids.begin(), ids.end(), id);
            return (it != ids.end());
        }

        std::vector<karabo::util::Hash> DataLoggerManager::makeLoggersTable() {
            std::vector<std::string> keys;
            m_loggerMap.getKeys(keys);

            // Sort the names (case-insensitive) that will be displayed on the table
            std::sort(keys.begin(), keys.end(),
                      [](const std::string& x, const std::string& y) { return strcasecmp(x.c_str(), y.c_str()) < 0; });

            std::vector<karabo::util::Hash> table;
            table.reserve(m_loggerMap.size());
            const auto prefix_length = std::strlen(DATALOGGER_PREFIX);
            for (const std::string& device : keys) {
                table.emplace_back("device", device.substr(prefix_length), "dataLogger",
                                   serverIdToLoggerId(m_loggerMap.get<std::string>(device)));
            }

            return table;
        }

    } // namespace devices
} // namespace karabo
