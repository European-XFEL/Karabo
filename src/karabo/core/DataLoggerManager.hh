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

        class DataLoggerManager : public Device<OkErrorFsm> {


        public:

            KARABO_CLASSINFO(DataLoggerManager, "DataLoggerManager", "1.0")

            static void expectedParameters(karabo::util::Schema& expected);

            DataLoggerManager(const karabo::util::Hash& input);

            virtual ~DataLoggerManager();

            static boost::filesystem::path getArchiveFile(const std::string& deviceId, const int idx);

            static boost::filesystem::path getSchemaFile(const std::string& deviceId);

            static boost::filesystem::path getIndexFile(const std::string& deviceId, const int idx);


        private: // Functions

            void okStateOnEntry();

            void instanceNewHandler(const karabo::util::Hash& topologyEntry);

            void instanceGoneHandler(const std::string& instanceId, const karabo::util::Hash& instanceInfo);

            karabo::util::Epochstamp extractRange(const std::vector<karabo::util::Hash>& archive, const karabo::util::Epochstamp& from, const karabo::util::Epochstamp& to, std::vector<karabo::util::Hash>& result);

            void slotGetPropertyHistory(const std::string& deviceId, const std::string& property, const karabo::util::Hash& params);

            std::vector<karabo::util::Hash> getPropertyData(const std::string& deviceId, const std::string& key, const int i = -1);

            void slotGetConfigurationFromPast(const std::string& deviceId, const std::string& timepoint);

        private: // Data

        };
    }
}

#endif	
