/**
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_DEVICES_DATALOGGER_HH
#define	KARABO_DEVICES_DATALOGGER_HH

#include <fstream>

#include "karabo/util/DataLogUtils.hh"
#include "karabo/core/Device.hh"


/**
 * The main karabo namespace
 */
namespace karabo {

    /**
     * Namespace for package core
     */
    namespace devices {

        struct DeviceData {

            KARABO_CLASSINFO(DeviceData, "DataLoggerDeviceData", "2.6")

            enum class InitLevel {

                NONE = 0, /// DeviceData is created
                STARTED, /// connecting to device's signals has started
                CONNECTED, /// all connections are established (and first Schema received in between)
                COMPLETE /// the initial configuration has arrived
            };

            DeviceData(const karabo::util::Hash& input);

            virtual ~DeviceData();

            /**
             * Process configuration by writing to files or sending to DB server
             * @param config
             * @param user
             * @param data
             */
            virtual void handleChanged(const karabo::util::Hash& config, const std::string& user) = 0;

            virtual void flushOne() = 0;

            /**
             * Store updated schema into file hierarchy or in database tables
             */
            virtual void handleSchemaUpdated(const karabo::util::Schema& schema) = 0;

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

            std::string m_deviceToBeLogged; // same as this DeviceData's key in DeviceDataMap

            InitLevel m_initLevel;

            karabo::net::Strand::Pointer m_strand;

            karabo::util::Schema m_currentSchema;

            std::string m_user;

            boost::mutex m_lastTimestampMutex;

            karabo::util::Timestamp m_lastDataTimestamp;

            bool m_updatedLastTimestamp;

            bool m_pendingLogin;
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
        class DataLogger : public karabo::core::Device<> {

        protected:
            bool m_useP2p;

            // https://www.quora.com/Is-it-thread-safe-to-write-to-distinct-keys-different-key-for-each-thread-in-a-std-map-in-C-for-keys-that-have-existing-entries-in-the-map
            typedef std::unordered_map<std::string, DeviceData::Pointer> DeviceDataMap;
            DeviceDataMap m_perDeviceData;
            boost::mutex m_perDeviceDataMutex;
            boost::mutex m_changeVectorPropMutex;

            boost::asio::deadline_timer m_flushDeadline;
            unsigned int m_flushInterval;



        public:

            KARABO_CLASSINFO(DataLogger, "DataLogger", "2.6")

            static void expectedParameters(karabo::util::Schema& expected);

            DataLogger(const karabo::util::Hash& input);

            virtual ~DataLogger();

            // Functions

            virtual DeviceData::Pointer createDeviceData(const karabo::util::Hash& config) = 0;

            void initialize();

            /**
             * Setup directory as a root of file hierarchy or as database location
             * @param data
             */
            virtual void initializeBackend(const DeviceData::Pointer& data) = 0;

            void initConnection(const DeviceData::Pointer& data,
                                const boost::shared_ptr<std::atomic<unsigned int> >& counter);

            void slotChanged(const karabo::util::Hash& configuration, const std::string& deviceId);

            /**
             * Helper to remove an element from a vector<string> element.
             * Note that if the same element is in the vector more than once, only the first one is removed.
             *
             * @param str the element to remove
             * @param vectorProp the key of the vector<string> element
             *
             * @return whether could be removed
             */
            bool removeFrom(const std::string& str, const std::string& vectorProp);

            /**
             * Helper to add an element to a vector<string> element.
             * Note that if the same element is already in, it will not be added again.
             *
             * @param str the element to add
             * @param vectorProp the key of the vector<string> element
             *
             * @return whether it was added (i.e. false if 'str' was already in the vectorProperty
             */
            bool appendTo(const std::string& str, const std::string& vectorProp);

            void slotSchemaUpdated(const karabo::util::Schema& schema, const std::string& deviceId);

            /**
             * FIXME: Update text
             * This tags a device to be discontinued, three cases have to be distinguished
             *
             * (a) Regular shut-down of the device (wasValidUpToNow = true, reason = 'D')
             * (b) Silent death of the device (wasValidUpToNow = true, reason = 'D')
             * (c) Start-up of this (DataLogger) device whilst the device was alive (wasValidUpToNow = false, reason = 'L')
             *
             * This slot will be called by the DataLoggerManager
             *
             */
            void slotTagDeviceToBeDiscontinued(const std::string& reason, const std::string& deviceId);

            void slotAddDevicesToBeLogged(const std::vector<std::string>& deviceId);

            void handleFailure(const std::string& reason, const DeviceData::Pointer& data,
                               const boost::shared_ptr<std::atomic<unsigned int> >& counter);

            void handleSchemaConnected(const DeviceData::Pointer& data,
                                       const boost::shared_ptr<std::atomic<unsigned int> >& counter);

            void handleSchemaReceived(const karabo::util::Schema& schema, const std::string& deviceId,
                                      const DeviceData::Pointer& data,
                                      const boost::shared_ptr<std::atomic<unsigned int> >& counter);

            void handleSchemaReceived2(const karabo::util::Schema& schema, const DeviceData::Pointer& data,
                                       const boost::shared_ptr<std::atomic<unsigned int> >& counter);

            /// Helper for connecting to both signalChanged and signalStateChanged
            void handleConfigConnected(const DeviceData::Pointer& data,
                                       const boost::shared_ptr<std::atomic<unsigned int> >& counter);

            void checkReady(std::atomic<unsigned int>& counter);

            bool stopLogging(const std::string& deviceId);

            void flushActor(const boost::system::error_code& e);

            /**
             * Flush data in file hierarchy or to the database tables
             */
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

        };
    }
}

#endif /* KARABO_DEVICES_DATALOGGER_HH */
