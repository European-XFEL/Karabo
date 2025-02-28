/*
 * $Id$
 *
 * Author:
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


#ifndef KARABO_CORE_DATALOGGERMANAGER_HH
#define KARABO_CORE_DATALOGGERMANAGER_HH

#include <boost/asio/deadline_timer.hpp>
#include <filesystem>
#include <map>
#include <vector>

#include "karabo/core/Device.hh"
#include "karabo/util/DataLogUtils.hh"
#include "karabo/util/Version.hh"
#include "karabo/xms/SlotElement.hh"

/**
 * The main karabo namespace
 */
namespace karabo {

    /**
     * Namespace for package devices
     */
    namespace util {
        // Forward declare
        class Epochstamp;
    } // namespace util
    namespace devices {

        /**
         * @class DataLoggerManager
         * @brief The DataLoggerManager manages device loggers in the distributed system.
         *
         * In the Karabo distributed system two types of data archiving exist:
         *
         * - run associated archival of scientific data through the DAQ system
         * - constant device state and slow control logging using the data logger services
         *
         * This device manages the data loggers used in the second logging scenario. It is
         * the central configuration point to set via its expected parameters the
         *
         * - flushInterval: at which loggers flush their data to disk
         * - maximumFileSize: of log files after which a new log file chunk is created
         * - directory: the directory into which loggers should write their data
         * - serverList: a list of device servers which each runs one logger.
         *               Each device in the distributed system is assigned to one logger.
         *               They are added to the loggers on these servers in a round robin fashion,
         *               allowing for load balancing. Assignment is made permanent in a loggermap.xml
         *               file that is regularly written to disk. This allows to distribute the servers
         *               in the serverList to be distributed among several hosts and still have fixed
         *               places for reading the data back.
         *
         */
        class DataLoggerManager : public karabo::core::Device {
           public:
            KARABO_CLASSINFO(DataLoggerManager, "DataLoggerManager", "karabo-" + karabo::util::Version::getVersion())

            static void expectedParameters(karabo::util::Schema& expected);

            DataLoggerManager(const karabo::util::Hash& input);

            virtual ~DataLoggerManager();

           protected:
            void preReconfigure(karabo::util::Hash& incomingReconfiguration) override;

            void postReconfigure() override;

           private: // Functions
            void initialize();

            void checkLoggerMap();

            void topologyCheck_slotForceCheck();

            void launchTopologyCheck();

            /**
             * Assemble summary from m_checkStatus and clear for next use.
             * The boolean returned tells whether the status requires operator attention.
             */
            std::pair<bool, std::string> checkSummary();

            void topologyCheck(const boost::system::error_code& e);

            void topologyCheckOnStrand();

            /**
             * Print internal cash of logger status.
             * Needs to be protected by m_strand.
             */
            void printLoggerData() const;

            void checkLoggerConfig(bool ok, const std::shared_ptr<std::atomic<size_t>>& counter,
                                   const karabo::util::Hash& config, const std::string& loggerId);

            void checkLoggerConfigOnStrand(const std::string& errorTxt,
                                           const std::shared_ptr<std::atomic<size_t>>& counter,
                                           const karabo::util::Hash& config, const std::string& loggerId);
            /**
             * If deviceId's logging status is fishy, re-add to its logger.
             * Needs to be protected by m_strand.
             *
             * @param deviceId the fishy device
             */
            void forceDeviceToBeLogged(const std::string& deviceId);

            void checkDeviceConfig(bool ok, const std::shared_ptr<std::atomic<size_t>>& loggerCounter,
                                   const std::string& loggerId, unsigned int toleranceSec,
                                   const std::shared_ptr<std::atomic<size_t>>& loggedDevCounter,
                                   karabo::util::Epochstamp lastUpdateLogger, const karabo::util::Hash& config,
                                   const std::string& deviceId);

            void checkDeviceConfigOnStrand(const std::string& errorTxt,
                                           const std::shared_ptr<std::atomic<size_t>>& loggerCounter,
                                           const std::string& loggerId, unsigned int toleranceSec,
                                           const std::shared_ptr<std::atomic<size_t>>& loggedDevCounter,
                                           karabo::util::Epochstamp lastUpdateLogger, const karabo::util::Hash& config,
                                           const std::string& deviceId);

            karabo::util::Epochstamp mostRecentEpochstamp(
                  const karabo::util::Hash& config,
                  karabo::util::Epochstamp oldStamp = karabo::util::Epochstamp(0ull, 0ull)) const;

            void instanceNewHandler(const karabo::util::Hash& topologyEntry);

            void instanceNewOnStrand(const karabo::util::Hash& topologyEntry);

            void newDeviceToLog(const std::string& deviceId);

            void newLogger(const std::string& loggerId);

            void addDevicesToBeLogged(const std::string& loggerId, karabo::util::Hash& serverData);

            void addDevicesDone(bool ok, const std::string& loggerId,
                                const std::unordered_set<std::string>& calledDevices,
                                const std::vector<std::string>& alreadyLoggedDevices);

            void addDevicesDoneOnStrand(const std::string& errorTxt, const std::string& loggerId,
                                        const std::unordered_set<std::string>& calledDevices,
                                        const std::vector<std::string>& alreadyLoggedDevices);

            void newLoggerServer(const std::string& serverId);

            void instantiateLogger(const std::string& serverId);

            void loggerInstantiationHandler(bool ok, const std::string& devId, bool isFailure);

            void instanceGoneHandler(const std::string& instanceId, const karabo::util::Hash& instanceInfo);

            void instanceGoneOnStrand(const std::string& instanceId, const karabo::util::Hash& instanceInfo);

            void goneDeviceToLog(const std::string& deviceId);

            void goneLogger(const std::string& loggerId);

            void goneReader(const std::string& readerId);

            void goneLoggerServer(const std::string& serverId);

            /**
             * Request the current mapping of loggers to servers. The reply
             * contains a Hash where the the logger id is the key and the server id
             * the value.
             */
            void slotGetLoggerMap();

            /**
             * Get id of server that should run logger for given device that should be logged
             *
             * @param deviceId the device that should be logged
             * @param addIfNotYetInMap whether to create a server/logger relation in the logger map
             *                         in case it does not yet exist for deviceId
             * @return the server id - can be empty if addIfNotYetInMap == false
             */
            std::string loggerServerId(const std::string& deviceId, bool addIfNotYetInMap);

            /**
             * Get id of DataLogger running on server with id 'serverId'
             */
            inline std::string serverIdToLoggerId(const std::string& serverId) const {
                // Just prepend the prefix
                return karabo::util::DATALOGGER_PREFIX + serverId;
            }

            /**
             * Get id of server that should run logger with id 'loggerId'
             */
            inline std::string loggerIdToServerId(const std::string& loggerId) const {
                // Just remove the prefix - but be prepared for loggers not started by this manager
                // and thus not following the naming convention.
                const size_t posPrefix = loggerId.find(karabo::util::DATALOGGER_PREFIX);
                if (posPrefix == 0ul) {
                    return loggerId.substr(strlen(karabo::util::DATALOGGER_PREFIX));
                } else {
                    // wrong or even no prefix
                    return std::string();
                }
            }

            inline std::string serverIdToReaderId(const std::string& serverId, unsigned int readerNum = 0u) const {
                return karabo::util::DATALOGREADER_PREFIX + karabo::util::toString(readerNum) + "-" + serverId;
            }

            inline std::string readerIdToServerId(const std::string& readerId) const {
                const size_t posPrefix = readerId.find(karabo::util::DATALOGREADER_PREFIX);
                const size_t posLastHifen = readerId.rfind("-");
                if (posPrefix == 0ul && posLastHifen != std::string::npos && posLastHifen < readerId.size() - 1) {
                    // We have a properly formatted readerId.
                    return readerId.substr(posLastHifen + 1);
                } else {
                    // wrong or even no prefix
                    return std::string();
                }
            }

            void instantiateReaders(const std::string& serverId);

            /**
             * This device may not be locked
             * @return false
             */
            bool allowLock() const {
                return false;
            }

            /**
             * @brief Evaluate old and new configuration in order to possibly start/stop archiving
             *        for some devices/device classes
             *
             * @param oldList   old configuration for blocked devices/classes
             * @param newList   new configuration for blocked devices/classes
             */
            void evaluateBlockedOnStrand(const karabo::util::Hash& oldList, const karabo::util::Hash& newList);

            bool isDeviceBlocked(const std::string& deviceId);

            bool isClassBlocked(const std::string& classId);

            bool isBlocked(const std::string& id, const std::string& typeIds);

            /**
             * @brief create a vector of hashes that can be loaded in a table from
             *        the Hash with the mapping device -> logger
             */
            std::vector<karabo::util::Hash> makeLoggersTable();

           private: // Data
            const std::vector<std::string> m_serverList;
            size_t m_serverIndex;

            std::mutex m_loggerMapMutex;
            karabo::util::Hash m_loggerMap;
            const std::string m_loggerMapFile;

            enum class LoggerState {
                OFFLINE = 0,
                INSTANTIATING,
                RUNNING

            };
            // Both m_loggerData and m_checkStatus are to be touched only in functions running on m_strand
            //
            // "devices": all devices that the logger has confirmed to log,
            // "beingAdded": all devices that the logger has been told to log, but which it did not yet confirm,
            // "backlog: all that the logger still has to be told to log
            karabo::util::Hash m_loggerData; /// 1st level keys: entries in m_serverList, 2nd level: "state", "backlog",
                                             /// "beingAdded" and "devices"
            karabo::util::Hash m_checkStatus; /// Keep track of all important stuff during check
            std::unordered_map<std::string, std::set<std::string>> m_knownClasses; /// to be accessed on the strand
            karabo::net::Strand::Pointer m_strand;

            boost::asio::steady_timer m_topologyCheckTimer;
            std::string m_loggerClassId;
            std::string m_readerClassId;

            std::mutex m_blockedMutex;
            karabo::util::Hash m_blocked;      /// Hash with 'deviceIds' and 'classIds' entries
            const std::string m_blockListFile; /// File name of blocked devices and/or device classes
        };
    } // namespace devices
} // namespace karabo

#endif
