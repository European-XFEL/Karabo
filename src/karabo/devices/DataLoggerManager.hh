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

#include <boost/filesystem.hpp>

#include "karabo/core/Device.hh"
#include "karabo/util/DataLogUtils.hh"

/**
 * The main karabo namespace
 */
namespace karabo {

    /**
     * Namespace for package devices
     */
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
         * - serverList: a list of device servers on which the loggers should run. 
         *               Each device in the distributed system has its own logger, and 
         *               loggers are added to servers in this list in a round robin fashion,
         *               allowing for load balancing.
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

            void prepareForLoggerMap();

            void instanceNewHandler(const karabo::util::Hash& topologyEntry);

            void instanceNewOnStrand(const karabo::util::Hash& topologyEntry);

            void newDeviceToLog(const std::string& deviceId);

            void newLogger(const std::string& loggerId);

            void instantiateLogger(const std::string& serverId);

            void instanceGoneHandler(const std::string& instanceId, const karabo::util::Hash& instanceInfo);

            void instanceGoneOnStrand(const std::string& instanceId, const karabo::util::Hash& instanceInfo);

            void goneDeviceToLog(const std::string& deviceId);

            void goneLogger(const std::string& loggerId);

            void goneServer(const std::string& serverId);

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
             * Get id of DataLogger running on server with id 'serverId'
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
            // "devices": all that the logger has been told to log,
            // "backlog: all that the logger still has to be told to log
            karabo::util::Hash m_loggerData; /// 1st level keys: entries in m_serverList, 2nd level: "state", "backlog" and "devices"
            karabo::net::Strand::Pointer m_strand;
        };
    }
}

#endif