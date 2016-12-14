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



        private:

            /**
             * Initialize the device. Will scan the system topology for already available run configuration groups
             */
            void initialize();

            /**
             * Initialize the available run configuration groups
             */
            void initAvailableGroups();

            /**
             * React on updates in run configuration groups known by this device, i.e. update their list of sources
             */
            void updateAvailableGroups();

            /**
             * Update the compiled source list, i.e. the flattened sources from all run configuration groups selected
             * to be used
             */
            void updateCompiledSourceList();

            /**
             * Return a deviceId from a groupId
             * @param groupId
             */
            const std::string& getDeviceIdByGroupId(const std::string& groupId);

            /**
             * Format the compiled source list into a configuration as required by the run controller and send this
             * to the distributed system.           
             */
            void buildConfigurationInUse();

            /*
             * Used by buildConfigurationInUse to build properties for each data source
             */
            void buildDataSourceProperties(const std::vector<karabo::util::Hash>& table,
                                           const std::string& groupId,
                                           bool expertFlag,
                                           bool userFlag,
                                           karabo::util::Hash& result);

            /**
             * In the preReconfigure we check if new run configuration groups have been selected and update the
             * compiled sources accordingly
             */
            void preReconfigure(karabo::util::Hash& incomingReconfiguration);

            /**
             * In postReconfigure debug output is provided if requested
             */
            void postReconfigure();

            /**
             * Helper function for reconfiguring the groups from preReconfigure
             */
            void reconfigureAvailableGroups(const std::vector<karabo::util::Hash>& groups, const karabo::util::Schema& schema);

            /**
             * Print the current run configuration
             */
            void printConfig() const;

            /**
             * Handle new run configuration group devices appearing
             */
            void newDeviceHandler(const karabo::util::Hash& topologyEntry);

            /**
             * Handle run configuration group devices dissappearing
             */
            void deviceGoneHandler(const std::string& instanceId, const karabo::util::Hash& instanceInfo);

            /**
             * Handle new run configuration group devices updating their source information
             */
            void deviceUpdatedHandler(const std::string& deviceId, const karabo::util::Hash& update);

            /**
             * Helper function to update group information if determined necessary in deviceUpdatedHandler
             */
            void updateGroupConfiguration(const std::string& deviceId, const karabo::util::Hash& update = karabo::util::Hash());

            /**
             * Return the sources in a group
             */
            void slotGetSourcesInGroup(const std::string& group);

            /**
             * Helper function to combine sources from a group  into a result hash.
             */
            void makeGroupSourceConfig(karabo::util::Hash& result, const std::string& deviceId) const;

        private:

            std::map<std::string, karabo::util::Hash> m_configurations;
            std::map<std::string, std::string> m_groupDeviceMapping;

        };
    }
}

#endif	/* KARABO_DEVICES_RUNCONFIGURATOR_HH */

