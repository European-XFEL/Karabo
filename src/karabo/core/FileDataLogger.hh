/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_CORE_FILEDATALOGGER_HH
#define	KARABO_CORE_FILEDATALOGGER_HH

#include <boost/filesystem.hpp>

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

        class FileDataLogger : public Device<OkErrorFsm> {

            /**
             * device +
             *   <deviceId> +
             *     schema t0 = <timestamp> @
             *       [0]
             *         v t="<timestamp>" => SCHEMA
             *     configuration t0 = <timestamp> +
             *       <key> @
             *         [0]
             *           v t="<timestamp>" [isLast] => VALUE
             *          
             */
            karabo::util::Hash m_systemHistory;
            
            mutable boost::mutex m_systemHistoryMutex;
            
            bool m_persistData;
            
            boost::thread m_persistDataThread;
            
        public:

            KARABO_CLASSINFO(FileDataLogger, "FileDataLogger", "1.0")

            static void expectedParameters(karabo::util::Schema& expected);

            FileDataLogger(const karabo::util::Hash& input);

            virtual ~FileDataLogger();


        private: // Functions

            void okStateOnEntry();
            
            void instanceNewHandler(const karabo::util::Hash& topologyEntry);
            
            void ensureProperDeviceEntry(const std::string& deviceId);
            
            void refreshDeviceInformation(const std::string& deviceId);

            //void instanceUpdatedHandler(const karabo::util::Hash& topologyEntry);

            void instanceGoneHandler(const std::string& instanceId, const karabo::util::Hash& instanceInfo);
            
            void tagDeviceToBeDiscontinued(const std::string& instanceId, const bool wasValidUpToNow, const char reason);
            
            void appendDeviceConfigurationToFile(const std::string& instanceId, const karabo::util::Hash& deviceEntry);
            
            void createLastValidConfiguration(karabo::util::Hash& config, const bool wasValidUpToNow, const char reason);
            
            void onInstanceNewForSystemHistory(const std::string& deviceId, const karabo::util::Hash& instanceInfo);
            
            void slotChanged(const karabo::util::Hash& changedConfig, const std::string& deviceId);
            
            void slotSchemaUpdated(const karabo::util::Schema& schema, const karabo::util::Hash& configuration, const std::string& deviceId);
            
            void persistDataThread();
            
            void copyAndClearSystemConfiguration(karabo::util::Hash& copy);
            
            void slotGetFromPast(const std::string& deviceId, const std::string& key, const std::string& from, const std::string& to);
            
            void slotGetPropertyHistory(const std::string& deviceId, const std::string& property, const karabo::util::Hash& params);
            
            void slotGetConfigurationFromPast(const std::string& deviceId, const std::string& timepoint);
            
            std::vector<karabo::util::Hash> getPropertyData(const std::string& deviceId, const std::string& key, const int i = -1);
            
            boost::filesystem::path getArchiveFile(const std::string& deviceId, const int idx);
            
            karabo::util::Epochstamp extractRange(const std::vector<karabo::util::Hash>& archive, const karabo::util::Epochstamp& from, const karabo::util::Epochstamp& to, std::vector<karabo::util::Hash>& result);
                        
            karabo::util::Hash getDeviceData(const std::string& deviceId, const karabo::util::Epochstamp& tp);
        };
    }
}

#endif	
