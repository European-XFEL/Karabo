/**
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_CORE_DATALOGGER_HH
#define	KARABO_CORE_DATALOGGER_HH

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

        struct DeviceData;
        typedef boost::shared_ptr<DeviceData> DeviceDataPointer;

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

            // https://www.quora.com/Is-it-thread-safe-to-write-to-distinct-keys-different-key-for-each-thread-in-a-std-map-in-C-for-keys-that-have-existing-entries-in-the-map
            typedef std::unordered_map<std::string, DeviceDataPointer> DeviceDataMap;
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

        private: // Functions

            void initialize();

            void setupDirectory(const DeviceDataPointer& data) const;

            void initConnection(const DeviceDataPointer& data,
                                const boost::shared_ptr<std::atomic<unsigned int> >& counter);

            void slotChanged(const karabo::util::Hash& configuration, const std::string& deviceId);

            void handleChanged(const karabo::util::Hash& config, const std::string& user, const DeviceDataPointer& data);

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

            /// Helper function to update data.m_idxprops, returns whether data.m_idxprops changed.
            bool updatePropsToIndex(DeviceData& data);

            /// Helper to ensure archive file is closed.
            /// Must only be called from functions posted on 'data.m_strand'.
            void ensureFileClosed(DeviceData& data);

            void slotSchemaUpdated(const karabo::util::Schema& schema, const std::string& deviceId);

            void handleSchemaUpdated(const karabo::util::Schema& schema, const DeviceDataPointer& data);
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

            void handleFailure(const std::string& reason, const DeviceDataPointer& data,
                               const boost::shared_ptr<std::atomic<unsigned int> >& counter);

            void handleSchemaConnected(const DeviceDataPointer& data,
                                       const boost::shared_ptr<std::atomic<unsigned int> >& counter);

            void handleSchemaReceived(const karabo::util::Schema& schema, const std::string& deviceId,
                                      const DeviceDataPointer& data,
                                      const boost::shared_ptr<std::atomic<unsigned int> >& counter);

            void handleSchemaReceived2(const karabo::util::Schema& schema, const DeviceDataPointer& data,
                                       const boost::shared_ptr<std::atomic<unsigned int> >& counter);

            /// Helper for connecting to both signalChanged and signalStateChanged
            void handleConfigConnected(const DeviceDataPointer& data,
                                       const boost::shared_ptr<std::atomic<unsigned int> >& counter);

            void checkReady(std::atomic<unsigned int>& counter);

            bool stopLogging(const std::string& deviceId);

            int determineLastIndex(const std::string& deviceId) const;

            int incrementLastIndex(const std::string& deviceId);

            void flushActor(const boost::system::error_code& e);

            void doFlush();

            void flushOne(const DeviceDataPointer& data);

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
