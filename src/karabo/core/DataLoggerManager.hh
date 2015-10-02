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

        class DataLoggerManager : public karabo::core::Device<karabo::core::OkErrorFsm> {
            class Registry {
            public:
                
                Registry() {}
                
                virtual ~Registry() {}
                
                bool has(const std::string& id) const;
                
                bool has(const std::string& serverId, const std::string& deviceId) const;
                
                void insert(const std::string& serverId, const std::string& deviceId);
                
                void erase(const std::string& serverId);
                
                void clear();
                
                void printContent();
                
            private:
                
                std::map<std::string, std::vector<std::string> > m_registered;
            };
            
            
        public:

            KARABO_CLASSINFO(DataLoggerManager, "DataLoggerManager", "1.0")

            static void expectedParameters(karabo::util::Schema& expected);

            DataLoggerManager(const karabo::util::Hash& input);

            virtual ~DataLoggerManager();

        private: // Functions

            void okStateOnEntry();

            void instanceNewHandler(const karabo::util::Hash& topologyEntry);
            
            void instanceGoneHandler(const std::string& instanceId, const karabo::util::Hash& instanceInfo);

            void slotGetLoggerMap();
            
            void instantiateReaders(const std::string& serverId);
            
        private: // Data
            std::vector<std::string> m_serverList;
            size_t m_serverIndex;
            karabo::util::Hash m_loggerMap;
            bool m_saved;
            std::vector<std::string> m_maintainedDevices;
            Registry m_instantiated;
            boost::mutex m_handlerMutex;
        };
    }
}

#endif	
