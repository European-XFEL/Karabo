/*
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */
/*
 * File:   DataLogReader.hh
 * Author: Sergey Esenov serguei.essenov@xfel.eu
 *
 * Created on January 29, 2015, 11:04 AM
 */

#ifndef DATALOGREADER_HH
#define DATALOGREADER_HH

#include "karabo/core/Device.hh"
#include "karabo/net/Strand.hh"
#include "karabo/util/DataLogUtils.hh"
#include "karabo/util/Version.hh"

/**
 * The main karabo namespace
 */
namespace karabo {

    /**
     * Namespace for package core
     */
    namespace devices {

        /**
         * @class DataLogReader
         * @brief DataLogReader devices read archived information from the Karabo data loggers
         *
         * DataLogReader devices read archived information from the Karabo data loggers. They
         * are managed by karabo::devices::DataLoggerManager devices. Calls to them should usually
         * not happen directly, but rather through a karabo::core::DeviceClient and it's
         * karabo::core::DeviceClient::getPropertyHistory and karabo::core::DeviceClient::getConfigurationFromPast
         * methods.
         *
         */
        class DataLogReader : public karabo::core::Device {
           public:
            KARABO_CLASSINFO(DataLogReader, "DataLogReader", "karabo-" + karabo::util::Version::getVersion())

            static void expectedParameters(karabo::data::Schema& schema);

            DataLogReader(const karabo::data::Hash& input);

            virtual ~DataLogReader();

           protected:
            /**
             * Use this slot to get the history of a given property
             * @param deviceId for which to get the history for
             * @param property path to the property for which to get the history from
             * @param params Hash containing optional limitations for the query:
             *   - from: iso8601 timestamp indicating the start of the interval to get history from
             *   - to: iso8601 timestamp indicating the end of the interval to get history from
             *   - maxNumData: maximum number of data points to retrieve starting from start
             *
             * The slot replies a vector of Hashes where each entry consists of a Hash with a key "v" holding the value
             * of the property at timepoint signified in "v"'s attributes using a format compatible with
             * karabo::data::Timestamp::fromHashAttributes
             */
            void slotGetPropertyHistory(const std::string& deviceId, const std::string& property,
                                        const karabo::data::Hash& params);

            /**
             * Request the configuration Hash and schema of a device at a given point at time.
             * Depending on the device status and on the availability of logged data, the configuration and schema
             * returned will be:
             *
             *   1. If the device was online and logging data at the given timepoint, the configuration and the schema
             *      will be the ones that were active at the timepoint;
             *   2. If the device was offline at the given timepoint, but there is data logged for it before the
             *      timepoint, the last active configuration and schema before that timepoint will be returned;
             *   3. If the device was offline at the given timepoint and there's no data logged before the timepoint, an
             *      empty configuration and an empty schema will be returned.
             *
             * @param deviceId of the device to get the configuration from
             * @param timepoint in iso8601 format for which to get the information
             *
             * The slot replies with a tuple of 4 values. The first two are the Hash and Schema objects containing the
             * configuration Hash and the corresponding device schema for timepoint. The third is a boolean whose true
             * value indicates the device was online and actively logging data at the timepoint. The fourth value is
             * the string form of the timepoint for the configuration returned and will be the latest timestamp among
             * the timestamps of the properties in the configuration returned.
             *
             * An important note: if no configuration is found for the device at the timepoint (or before the
             * timepoint), the third value in the reply will be false and the fourth will be the string form of the
             * Epoch (01/01/1970 at 00:00:00).
             */
            void slotGetConfigurationFromPast(const std::string& deviceId, const std::string& timepoint);

            /**
             * helper functions to handle state transition in derived classes:
             * onOk: sets the State to ON
             */
            void onOk();
            /**
             * helper functions to handle state transition in derived classes:
             * onException: sets the State to ERROR, logs the exception trace to status and to the Karabo Logs
             *
             * @param message a string to be prepended to the trace
             * @return the exception trace
             */
            const std::string onException(const std::string& message);


           protected:
            virtual void slotGetPropertyHistoryImpl(const std::string& deviceId, const std::string& property,
                                                    const karabo::data::Hash& params) = 0;

            virtual void slotGetConfigurationFromPastImpl(const std::string& deviceId,
                                                          const std::string& timepoint) = 0;

           private: // Functions
            // The initialization function. Currently simply calls the `onOk` helper method.
            void initialize();
        };
    } // namespace devices
} // namespace karabo

#endif /* DATALOGREADER_HH */
