/* 
 * File:   RunConfigurator.hh
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on September 9, 2016, 10:17 AM
 */

#ifndef KARABO_DEVICES_RUNCONFIGURATOR_HH
#define	KARABO_DEVICES_RUNCONFIGURATOR_HH

#include <map>
#include <vector>
#include <string>
#include <karabo/util.hpp>
#include <karabo/util/SimpleElement.hh>
#include "karabo/core/Device.hh"

namespace karabo {
    namespace devices {
        
        class RunConfigurator : public karabo::core::Device<> {
        public:
            
            KARABO_CLASSINFO(RunConfigurator, "RunConfigurator", "1.5")
            
            typedef std::map<std::string, karabo::util::Hash> MapGroup;
            
            static void expectedParameters(karabo::util::Schema& expected);
            
            RunConfigurator(const karabo::util::Hash& input);
            
            virtual ~RunConfigurator();
            
            void initialize();
            
            void initAvailableGroups();
            
            void newDeviceHandler(const karabo::util::Hash& topologyEntry);
            
            void deviceGoneHandler(const std::string& instanceId, const karabo::util::Hash& instanceInfo);
            
            void updateAvailableGroups();
            
            void updateCurrentGroup();
            
            void updateCurrentSources();
            
            void updateCompiledSourceList();
            
        private:
            
            std::vector<std::string> getConfigurationGroupDeviceIds() const;
            
            std::vector<std::string> getConfigurationGroupIds() const;
            
            const std::string& getDeviceIdByGroupId(const std::string& groupId, const std::string& failure = "") const;
            
            std::vector<karabo::util::Hash> getAllSources(const std::string& groupId) const;
            
            std::vector<std::string> getAllSourceNames(const std::string& groupId) const;
            
            karabo::util::Hash filterDataSchema(const std::string& deviceId, const karabo::util::Schema& schema);
            
            void convertSchemaHash(const karabo::util::Hash& fullHash, karabo::util::Hash& hash);
            
            void buildConfigurationInUse();
            
            void buildDataSourceProperties(const std::vector<karabo::util::Hash>& table,
                                           const std::string& groupId,
                                           bool expertFlag,
                                           bool userFlag,
                                           karabo::util::Hash& result);
            
            void preReconfigure(karabo::util::Hash& incomingReconfiguration);
            
            void postReconfigure();
            
            void reconfigureAvailableGroups(const std::vector<karabo::util::Hash>& groups, const karabo::util::Schema& schema);
            
            void reconfigureSourcesOnly(const std::vector<karabo::util::Hash>& groups, const karabo::util::Schema& schema);
                        
            void reconfigureSources(const std::vector<karabo::util::Hash>& groups, const karabo::util::Schema& schema);
            
            void printConfig() const;
                        
        private:
            
            std::map<std::string, karabo::util::Hash> m_configurations;
            std::string m_cursor;
            std::vector<std::string> m_deviceIds;
            std::vector<std::string> m_groupIds;
        };
    }
}

#endif	/* KARABO_DEVICES_RUNCONFIGURATOR_HH */

