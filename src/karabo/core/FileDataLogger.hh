/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_CORE_FILEDATALOGGER_HH
#define	KARABO_CORE_FILEDATALOGGER_HH

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
            
            void createDeviceEntry(const std::string& deviceId);

            //void instanceUpdatedHandler(const karabo::util::Hash& topologyEntry);

            void instanceGoneHandler(const std::string& instanceId);
            
            void createLastValidConfiguration(karabo::util::Hash& config);
            
            void onInstanceNewForSystemHistory(const std::string& deviceId, const karabo::util::Hash& instanceInfo);
            
            void fetchConfiguration(const std::string& deviceId, karabo::util::Hash& configuration) const;
                
            void fetchSchema(const std::string& deviceId, karabo::util::Schema& schema) const;
                        
            void slotChanged(const karabo::util::Hash& changedConfig, const std::string& deviceId);
            
            void persistDataThread();
            
            std::vector<karabo::util::Hash> slotGetFromPast(const std::string& deviceId, const std::string& key, const std::string& from, const std::string& to);
            
            std::vector<karabo::util::Hash> getData(const std::string& deviceId, const std::string& key);
            
        };
    }
}

#endif	
