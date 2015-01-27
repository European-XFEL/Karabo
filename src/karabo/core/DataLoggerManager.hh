/*
 * $Id$
 *
 * Author:
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_CORE_DATALOGGERMANAGER_HH
#define	KARABO_CORE_DATALOGGERMANAGER_HH

#include <boost/filesystem.hpp>
#include <map>
#include <vector>
#include "Device.hh"
#include "OkErrorFsm.hh"

/**
 * The main karabo namespace
 */
namespace karabo {

    /**
     * Namespace for package core
     */
    namespace core {

        struct DataLoggerIndex {
            std::string m_event;
            karabo::util::Epochstamp m_epoch;
            unsigned long long m_train;
            long m_position;
            std::string m_user;
            int m_fileindex;

            DataLoggerIndex()
            : m_event()
            , m_epoch(0, 0)
            , m_train(0)
            , m_position(-1)
            , m_user(".")
            , m_fileindex(-1) {
            }
        };

        class DataLoggerManager : public karabo::core::Device<karabo::core::OkErrorFsm> {
        public:

            KARABO_CLASSINFO(DataLoggerManager, "DataLoggerManager", "1.0")

            static void expectedParameters(karabo::util::Schema& expected);

            DataLoggerManager(const karabo::util::Hash& input);

            virtual ~DataLoggerManager();

        private: // Functions

            void okStateOnEntry();

            void instanceNewHandler(const karabo::util::Hash& topologyEntry);

            void instanceGoneHandler(const std::string& instanceId, const karabo::util::Hash& instanceInfo);

            void slotGetPropertyHistory(const std::string& deviceId, const std::string& property, const karabo::util::Hash& params);

            void slotGetConfigurationFromPast(const std::string& deviceId, const std::string& timepoint);

            DataLoggerIndex findLoggerIndexTimepoint(const std::string& deviceId, const std::string& timepoint);

            DataLoggerIndex findNearestLoggerIndex(const std::string& deviceId, const karabo::util::Epochstamp& timepoint);

            int getFileIndex(const std::string& deviceId);

        private: // Data
            std::vector<std::string> m_serverList;
            size_t m_serverIndex;
            std::map<std::string, std::string> m_serverMap;
        };
    }
}

#endif	
