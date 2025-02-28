/**
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


#ifndef KARABO_DEVICES_DATALOGGER_HH
#define KARABO_DEVICES_DATALOGGER_HH

#include <fstream>

#include "karabo/core/Device.hh"
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

        struct DeviceData : public std::enable_shared_from_this<DeviceData> {
            KARABO_CLASSINFO(DeviceData, "DataLoggerDeviceData", "karabo-" + karabo::util::Version::getVersion())

            enum class InitLevel {


                NONE = 0,  /// DeviceData is created
                STARTED,   /// connecting to device's signals has started
                CONNECTED, /// all connections are established (and first Schema received in between)
                COMPLETE   /// the initial configuration has arrived
            };

            DeviceData(const karabo::util::Hash& input);

            virtual ~DeviceData();

            /**
             * Called when configuration updates arrive for logging
             * @param config a Hash with the updates and their timestamps
             * @param the user responsible for this update - if any
             */
            virtual void handleChanged(const karabo::util::Hash& config, const std::string& user) = 0;

            /**
             * Called when a Schema update arrive for logging
             * @param schema - the new one
             * @param stamp - the timestamp to be assigned for that update
             */
            virtual void handleSchemaUpdated(const karabo::util::Schema& schema,
                                             const karabo::util::Timestamp& stamp) = 0;

            /**
             * Retrieves the paths of the leaf nodes in a given configuration. The paths are returned in
             * ascending order of their corresponding nodes timestamps.
             *
             * @param configuration A configuration with the nodes corresponding to the paths.
             * @param schema The schema for the configuration hash.
             * @param paths The paths of the leaf nodes in the configuration, sorted by nodes timestamps.
             *
             * @note karabo::devices::DataLogReader depends on the configuration items being properly sorted
             * in time to retrieve configuration changes.
             */
            void getPathsForConfiguration(const karabo::util::Hash& configuration, const karabo::util::Schema& schema,
                                          std::vector<std::string>& paths) const;

            virtual void stopLogging() {}

            std::string m_deviceToBeLogged; // same as this DeviceData's key in DeviceDataMap

            InitLevel m_initLevel;

            karabo::net::Strand::Pointer m_strand;

            karabo::util::Schema m_currentSchema;

            std::string m_user;

            std::mutex m_lastTimestampMutex;

            karabo::util::Timestamp m_lastDataTimestamp;

            bool m_updatedLastTimestamp;

            bool m_pendingLogin;

            unsigned int m_onDataBeforeComplete; // Only to avoid spamming...
        };

        /**
         * @class DataLogger
         * @brief A DataLogger device is assigned devices in the distributed
         * system and logs their slow control data.
         *
         * DataLoggers are managed by the karabo::devices::DataLoggerManager.
         *
         * Each is able to log any number of devices. This list can be specified at instantiation,
         * but can also dynamically changed by the slots slotTagDeviceToBeDiscontinued and slotAddDevicesToBeLogged.
         * When the logger is ready to log data, its state changes from INIT to NORMAL.
         */
        class DataLogger : public karabo::core::Device {
           protected:
            // https://www.quora.com/Is-it-thread-safe-to-write-to-distinct-keys-different-key-for-each-thread-in-a-std-map-in-C-for-keys-that-have-existing-entries-in-the-map
            typedef std::unordered_map<std::string, DeviceData::Pointer> DeviceDataMap;
            std::mutex m_perDeviceDataMutex;
            DeviceDataMap m_perDeviceData;
            std::unordered_map<std::string, unsigned int>
                  m_nonTreatedSlotChanged; // also needs m_perDeviceDataMutex protection

           private:
            boost::asio::steady_timer m_flushDeadline;
            unsigned int m_flushInterval;

           public:
            KARABO_CLASSINFO(DataLogger, "DataLogger", "2.6")

            static void expectedParameters(karabo::util::Schema& expected);

            DataLogger(const karabo::util::Hash& input);

            virtual ~DataLogger();

            // Functions
           protected:
            virtual DeviceData::Pointer createDeviceData(const karabo::util::Hash& config) = 0;

            /**
             * Do some actions here that may require asynchronous logic ...
             * and, finally, startConnection() should be called
             * This function may be overridden by derived classes but at
             * the end the 'startConnection' function should be called as
             * a last step of initialization
             */
            virtual void initializeLoggerSpecific();

            void startConnection();

            void initConnection(const DeviceData::Pointer& data,
                                const std::shared_ptr<std::atomic<unsigned int>>& counter);

            /**
             * Helper to remove an element from a vector<string> element - needs protection by m_perDeviceDataMutex.
             * Note that if the same element is in the vector more than once, only the first one is removed.
             *
             * @param str the element to remove
             * @param vectorProp the key of the vector<string> element
             *
             * @return whether could be removed
             */
            bool removeFrom(const std::string& str, const std::string& vectorProp);

            /**
             * Helper to add an element to a vector<string> element - needs protection by m_perDeviceDataMutex.
             * Note that if the same element is already in, it will not be added again.
             *
             * @param str the element to add
             * @param vectorProp the key of the vector<string> element
             *
             * @return whether it was added (i.e. false if 'str' was already in the vectorProperty
             */
            bool appendTo(const std::string& str, const std::string& vectorProp);

            /**
             * Override preDestruction from Device class
             */
            void preDestruction() override;

           private:
            void initialize();

            void slotChanged(const karabo::util::Hash& configuration, const std::string& deviceId);

            void slotSchemaUpdated(const karabo::util::Schema& schema, const std::string& deviceId);

            /**
             * FIXME: Update text
             * This tags a device to be discontinued, three cases have to be distinguished
             *
             * (a) Regular shut-down of the device (wasValidUpToNow = true, reason = 'D')
             * (b) Silent death of the device (wasValidUpToNow = true, reason = 'D')
             * (c) Start-up of this (DataLogger) device whilst the device was alive (wasValidUpToNow = false, reason =
             * 'L')
             *
             * This slot will be called by the DataLoggerManager
             *
             */
            void slotTagDeviceToBeDiscontinued(const std::string& reason, const std::string& deviceId);

            void slotAddDevicesToBeLogged(const std::vector<std::string>& deviceId);

            void handleFailure(const std::string& reason, const DeviceData::Pointer& data,
                               const std::shared_ptr<std::atomic<unsigned int>>& counter);

            void handleSchemaConnected(const DeviceData::Pointer& data,
                                       const std::shared_ptr<std::atomic<unsigned int>>& counter);

            void handleSchemaReceived(const karabo::util::Schema& schema, const std::string& deviceId,
                                      const DeviceData::Pointer& data,
                                      const std::shared_ptr<std::atomic<unsigned int>>& counter);

            void handleSchemaReceived2(const karabo::util::Schema& schema, const karabo::util::Timestamp& stamp,
                                       const DeviceData::Pointer& data,
                                       const std::shared_ptr<std::atomic<unsigned int>>& counter);

            /// Helper for connecting to both signalChanged and signalStateChanged
            void handleConfigConnected(const DeviceData::Pointer& data,
                                       const std::shared_ptr<std::atomic<unsigned int>>& counter);

            void checkReady(std::atomic<unsigned int>& counter);

            void disconnect(const std::string& deviceId);

            void disconnectHandler(bool isFailure, const std::string& devId, const std::string& signal,
                                   const std::shared_ptr<std::atomic<int>>& counter);

            void flushActor(const boost::system::error_code& e);

            /**
             * Flush data in file hierarchy or to the database tables
             * @param aReplyPtr if pointer to an AsyncReply that (if non-empty) has to be called without
             *                  argument when done
             */
            void updateTableAndFlush(const std::shared_ptr<SignalSlotable::AsyncReply>& aReplyPtr);

            // The flush slot
            void flush();

            /**
             * "Flush" data accumulated in the internal cache to the external storage (file, database,...)
             */
            virtual void flushImpl(const std::shared_ptr<SignalSlotable::AsyncReply>& aReplyPtr) = 0;

            /**
             * This device may not be locked
             * @return false
             */
            bool allowLock() const {
                return false;
            }
        };
    } // namespace devices
} // namespace karabo

#endif /* KARABO_DEVICES_DATALOGGER_HH */
