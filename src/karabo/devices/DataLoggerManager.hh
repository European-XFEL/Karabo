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

            void slotGetLoggerMap();

            void instantiateReaders(const std::string& serverId);

            void restartReadersAndLoggers();

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
