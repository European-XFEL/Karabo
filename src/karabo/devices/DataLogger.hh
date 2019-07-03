/**
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_CORE_DATALOGGER_HH
#define	KARABO_CORE_DATALOGGER_HH

#include <fstream>

#include "karabo/util/DataLogUtils.hh"
#include "karabo/net/Strand.hh"
#include "karabo/core/Device.hh"


/**
 * The main karabo namespace
 */
namespace karabo {

    /**
     * Namespace for package core
     */
    namespace devices {

        struct DeviceData;
        typedef boost::shared_ptr<DeviceData> DeviceDataPointer;

        /**
         * @class DataLogger
         * @brief A DataLogger device is assigned devices in the distributed
         * system and logs their slow control data.
         * 
         * DataLoggers are managed by the karabo::devices::DataLoggerManager.
         *
         * FIXME:
         * When it is ready to log data its state changes from INIT to NORMAL.
         */
        class DataLogger : public karabo::core::Device<> {

            // https://www.quora.com/Is-it-thread-safe-to-write-to-distinct-keys-different-key-for-each-thread-in-a-std-map-in-C-for-keys-that-have-existing-entries-in-the-map
            typedef std::unordered_map<std::string, DeviceDataPointer> DeviceDataMap;
            DeviceDataMap m_perDeviceData;
            boost::mutex m_perDeviceDataMutex;
            std::atomic<unsigned int> m_numConnected;


            boost::asio::deadline_timer m_flushDeadline;
            bool m_doFlushFiles;
            unsigned int m_flushInterval;



        public:

            KARABO_CLASSINFO(DataLogger, "DataLogger", "2.6")

            static void expectedParameters(karabo::util::Schema& expected);

            DataLogger(const karabo::util::Hash& input);

            virtual ~DataLogger();

        private: // Functions

            void initialize();

            void slotChanged(const karabo::util::Hash& configuration, const std::string& deviceId);

            // FIXME: May want AsyncReply??
            void handleChanged(const karabo::util::Hash& config, const DeviceDataPointer& data);

            /// Helper function to update data.m_idxprops, returns whether data.m_idxprops changed.
            bool updatePropsToIndex(DeviceData& data);

            /// Helper to ensure archive file is closed.
            /// Must be called under protection of the 'data.m_configMutex' mutex.
            void ensureFileClosed(DeviceData& data);

            void slotSchemaUpdated(const karabo::util::Schema& schema, const std::string& deviceId);

            // FIXME: May want AsyncReply??
            void handleSchemaUpdated(const karabo::util::Schema& schema, const DeviceDataPointer& data);
            /**
             * This tags a device to be discontinued, three cases have to be distinguished
             *
             * (a) Regular shut-down of the device (wasValidUpToNow = true, reason = 'D')
             * (b) Silent death of the device (wasValidUpToNow = true, reason = 'D')
             * (c) Start-up of this (DataLogger) device whilst the device was alive (wasValidUpToNow = false, reason = 'L')
             *
             * This slot will be called by the DataLoggerManager
             *
             */
            void slotTagDeviceToBeDiscontinued(const bool wasValidUpToNow, const char reason, const std::string& deviceId);

            // FIXME: May need AsyncReply??
            void handleTagDeviceToBeDiscontinued(const bool wasValidUpToNow, const char reason, DeviceDataPointer data);

            /// Helper used as error callback that triggers device suicide.
            void errorToDieHandle(const std::string& reason) const;

            void handleSchemaConnected(const std::string& deviceId);

            void handleSchemaReceived(const karabo::util::Schema& schema, const std::string& deviceId);

            /// Helper for connecting to both signalChanged and signalStateChanged
            void handleConfigConnected(const std::string& deviceId);

            int determineLastIndex(const std::string& deviceId);

            int incrementLastIndex(const std::string& deviceId);

            void flushActor(const boost::system::error_code& e);

            void doFlush();

            // The flush slot
            void flush();
            
            /**
             * This device may not be locked
             * @return false
             */
            bool allowLock() const {
                return false;
            }

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
            void getPathsForConfiguration(const karabo::util::Hash& configuration,
                                          const karabo::util::Schema& schema,
                                          std::vector<std::string>& paths) const;

        };
    }
}

#endif
