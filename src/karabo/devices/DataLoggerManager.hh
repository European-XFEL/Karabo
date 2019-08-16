/*
 * $Id$
 *
 * Author:
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_CORE_DATALOGGERMANAGER_HH
#define	KARABO_CORE_DATALOGGERMANAGER_HH

#include <map>
#include <vector>
#include <sstream>

#include <boost/filesystem.hpp>
#include <boost/asio/deadline_timer.hpp>

#include "karabo/core/Device.hh"
#include "karabo/util/DataLogUtils.hh"
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
    }
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
        class DataLoggerManager : public karabo::core::Device<> {

        public:

            KARABO_CLASSINFO(DataLoggerManager, "DataLoggerManager", "1.0")

            static void expectedParameters(karabo::util::Schema& expected);

            DataLoggerManager(const karabo::util::Hash& input);

            virtual ~DataLoggerManager();
            
        private: // Functions

            void initialize();

            void checkLoggerMap();

            void topologyCheck_slotForceCheck();

            void launchTopologyCheck();

            void topologyCheck(const boost::system::error_code& e);

            void topologyCheckOnStrand();

            /**
             * Print internal cash of logger status.
             * Needs to be protected by m_strand.
             */
            void printLoggerData() const;

            void checkLoggerConfig(bool ok, const boost::shared_ptr<std::atomic<size_t> >& counter,
                                   const karabo::util::Hash& config, const std::string& loggerId);

            void checkLoggerConfigOnStrand(bool ok, const boost::shared_ptr<std::atomic<size_t> >& counter,
                                           const karabo::util::Hash& config, const std::string& loggerId);
            /**
             * If deviceId's logging status is fishy, re-add to its logger.
             * Needs to be protected by m_strand.
             *
             * @param deviceId the fishy device
             */
            void forceDeviceToBeLogged(const std::string& deviceId);

            void checkDeviceConfig(bool ok, const boost::shared_ptr<std::atomic<size_t> >& loggerCounter,
                                   const std::string& loggerId, unsigned int toleranceSec,
                                   const boost::shared_ptr<std::atomic<size_t> >& loggedDevCounter,
                                   karabo::util::Epochstamp lastUpdateLogger, const karabo::util::Hash& config,
                                   const std::string& deviceId);

            void checkDeviceConfigOnStrand(bool ok, const boost::shared_ptr<std::atomic<size_t> >& loggerCounter,
                                           const std::string& loggerId, unsigned int toleranceSec,
                                           const boost::shared_ptr<std::atomic<size_t> >& loggedDevCounter,
                                           karabo::util::Epochstamp lastUpdateLogger, const karabo::util::Hash& config,
                                           const std::string& deviceId);

            karabo::util::Epochstamp mostRecentEpochstamp(const karabo::util::Hash& config,
                                                          karabo::util::Epochstamp oldStamp = karabo::util::Epochstamp(0ull, 0ull)) const;

            void instanceNewHandler(const karabo::util::Hash& topologyEntry);

            void instanceNewOnStrand(const karabo::util::Hash& topologyEntry);

            void newDeviceToLog(const std::string& deviceId);

            void newLogger(const std::string& loggerId);

            void addDevicesToBeLogged(const std::string& loggerId, karabo::util::Hash& serverData);

            void addDevicesDone(bool ok, const std::string& loggerId,
                                const std::unordered_set<std::string>& calledDevices,
                                const std::vector<std::string>& alreadyLoggedDevices);

            void addDevicesDoneOnStrand(bool ok, const std::string& loggerId,
                                        const std::unordered_set<std::string>& calledDevices,
                                        const std::vector<std::string>& alreadyLoggedDevices);

            void newLoggerServer(const std::string& serverId);

            void instantiateLogger(const std::string& serverId);

            void instanceGoneHandler(const std::string& instanceId, const karabo::util::Hash& instanceInfo);

            void instanceGoneOnStrand(const std::string& instanceId, const karabo::util::Hash& instanceInfo);

            void goneDeviceToLog(const std::string& deviceId);

            void goneLogger(const std::string& loggerId);

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
                // Just remove the prefix
                return loggerId.substr(strlen(karabo::util::DATALOGGER_PREFIX));
            }

            void instantiateReaders(const std::string& serverId);

            /**
             * This device may not be locked
             * @return false
             */
            bool allowLock() const {
                return false;
            }

        private: // Data
            const std::vector<std::string> m_serverList;
            size_t m_serverIndex;

            boost::mutex m_loggerMapMutex;
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
            // "lastCheckEmptyUpdate": optional key to hold devices for which the data logger did not yet have a
            //                         timestamp when the last check was run
            karabo::util::Hash m_loggerData; /// 1st level keys: entries in m_serverList, 2nd level: "state", "backlog", "beingAdded" and "devices" (and optionally "lastCheckEmptyUpdate")
            std::ostringstream m_checkStatus;
            karabo::net::Strand::Pointer m_strand;

            boost::asio::deadline_timer m_topologyCheckTimer;
        };
    }
}

#endif