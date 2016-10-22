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
#include "karabo/core/OkErrorFsm.hh"

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
        class DataLoggerManager : public karabo::core::Device<karabo::core::OkErrorFsm> {

        public:

            KARABO_CLASSINFO(DataLoggerManager, "DataLoggerManager", "1.0")

            static void expectedParameters(karabo::util::Schema& expected);

            DataLoggerManager(const karabo::util::Hash& input);

            virtual ~DataLoggerManager();

        private: // Functions

            void okStateOnEntry();

            void ensureLoggerRunning(const karabo::util::Hash& topologyEntry);

            void instanceGoneHandler(const std::string& instanceId, const karabo::util::Hash& instanceInfo);

            /**
             * Request the current mapping of loggers to servers. The reply
             * contains a Hash where the the logger id is the key and the server id
             * the value.
             */
            void slotGetLoggerMap();

            void instantiateReaders(const std::string& serverId);

            void restartReadersAndLoggers();
            
            /**
             * This device may not be locked
             * @return false
             */
            bool allowLock() const {
                return false;
            }

        private: // Data
            std::vector<std::string> m_serverList;
            size_t m_serverIndex;
            boost::mutex m_loggerMapMutex;
            karabo::util::Hash m_loggerMap;
            const std::string m_loggerMapFile;
        };
    }
}

#endif
