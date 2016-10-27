/* 
 * File:   RunConfigurationGroup.hh
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on September 9, 2016, 10:44 AM
 */

#ifndef KARABO_DEVICES_RUNCONFIGURATIONGROUP_HH
#define	KARABO_DEVICES_RUNCONFIGURATIONGROUP_HH

#include <karabo/util.hpp>
#include <karabo/core.hpp>

#define OUTPUT_CHANNEL_SEPARATOR ":"

namespace karabo {
    namespace devices {


        class RunControlDataSource {

        public:
            KARABO_CLASSINFO(RunControlDataSource, "RunControlDataSource", "1.5")
            KARABO_CONFIGURATION_BASE_CLASS

            static void expectedParameters(karabo::util::Schema& expected);

            RunControlDataSource(const karabo::util::Hash& input);

            virtual ~RunControlDataSource();

        private:

            void preReconfigure(karabo::util::Hash& incomingReconfiguration);

        };

        class RunConfigurationGroup : public karabo::core::Device<> {

        public:

            KARABO_CLASSINFO(RunConfigurationGroup, "RunConfigurationGroup", "1.5")

            static void expectedParameters(karabo::util::Schema& expected);

            RunConfigurationGroup(const karabo::util::Hash& input);

            virtual ~RunConfigurationGroup();

            const karabo::util::Hash& getGroup() const {
                return get<karabo::util::Hash>("group");
            }

        private:

            void initialize();

            void slotGetGroup();

            void saveGroupConfiguration();

            void preReconfigure(karabo::util::Hash& incomingReconfiguration);

            void fillTable(const std::vector<karabo::util::Hash>& current,
                           std::vector<karabo::util::Hash>& input,
                           std::vector<karabo::util::Hash>& table);

        };
    }
}

#endif	/* KARABO_DEVICES_RUNCONFIGURATIONGROUP_HH */

