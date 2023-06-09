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
 * File:   AlarmService.hh
 * Author: haufs
 *
 * Created on August 5, 2016, 11:30 AM
 */

#ifndef KARABO_ALARMSERVICE_HH
#define KARABO_ALARMSERVICE_HH

#include <atomic>
#include <boost/thread.hpp>

#include "karabo/core/Device.hh"
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
         * @class AlarmService
         * @brief The AlarmService device keeps track of alarms raised in the distributed system.
         *
         * The AlarmService device keeps track of alarms raised in the distributed system.
         * It registers itself to the alarm signals and maintains a list of currently known alarms,
         * when they were first and last raised, their severity, additional information and whether
         * they need acknowledging before they disappear.
         *
         * The device provides interfaces for clients to query this information and interact with
         * the alarms known to the system. Specifically, clients may send requests to acknowledge
         * a pending alarm.
         *
         * Additionally, the alarm service periodically saves alarms it manages to disk. This is done
         * to allow for quick recovery from system wide errors: if the alarm service for which-ever
         * reason needs to be restarted, it will query only the differences of the last persisted
         * alarms from the distributed system. For this purpose a storagePath and flushIntervall may
         * be configured in the device's expected parameters.
         *
         */
        class AlarmService : public karabo::core::Device<> {
           public:
            KARABO_CLASSINFO(AlarmService, "AlarmService", "karabo-" + karabo::util::Version::getVersion())

            static void expectedParameters(karabo::util::Schema& expected);

            AlarmService(const karabo::util::Hash& input);

            virtual ~AlarmService();

           private: // Functions
            void initialize();

            void preDestruction() override;

            /**
             * Callback for the instanceNew monitor. Connects this device's slotUpdateAlarms function
             * to the new devices signalAlarmUpdate signal. If the device was previously known it will ask this
             * device to submit its current alarm state.
             */
            void registerNewDevice(const karabo::util::Hash& topologyEntry);

            /**
             * Callback for connecting to a device's "signalAlarmUpdate"
             */
            void connectedHandler(const std::string& deviceId);

            /**
             * Called when a device instance disappears from the distributed system. It will trigger the alarm
             * service to set all alarms pending for this device to need acknowledgement and acknowledgeable. This
             * means alarms will not silently disappear, but because it can't be assured that the device instance
             * that disappeared will ever clear them for acknowledgment they are acknowledgeble. Note that if the
             * device instance does happen to reappear it will be asked to resubmit its current alarm state, bringing
             * all alarms pertinent to it back into a consistent needsacknowledging, acknowledging and cleared
             * condition.
             *
             * @param instanceId: the instance id of the device the disappeared
             * @param instanceInfo: not used but forwarded by the device client
             */
            void instanceGoneHandler(const std::string& instanceId, const karabo::util::Hash& instanceInfo);

            /**
             * This slot is to be called to let the alarm service device know of an update
             * in device alarms. As parameters it expects a device id, alarmInfo Hash, which should
             * have the following structure:
             *
             *   toClear -> property A -> alarm type 1 -> bool
             *           -> property A -> alarm type 2 -> bool
             *           -> property B -> ....
             *
             *  The bool field is not evaluated but serves as a placeholder
             *
             *   deviceId -> toAdd -> property A -> alarm type 1 -> Hash()
             *   deviceId -> toAdd -> property A -> alarm type 2 -> Hash()
             *   deviceId -> toAdd -> property B -> ...
             *
             * Entries underneath the "toClear" hierarchy are used to evaluated clearing of existing
             * alarms. Two scenarios are handled:
             *
             *  - alarm <b>does not</b> require acknowledging -> it is deleted from the alarm list
             *  - alarm <b>does</b> require acknowledging -> it is made acknowledgable
             *
             * Entries in the "toAdd" hierarchy result in one of the following two scenarios
             *
             *  - an alarm for this property and type <b>does not</b> yet exist -> it is added
             *  - an alarm for this property and type <b>does</b> exist -> it is updated but the
             *    first occurance is set to that of the existing alarm.
             */

            void slotUpdateAlarms(const std::string& deviceId, const karabo::util::Hash& alarmInfo);

            /**
             * Add signals and slots which need to be set up during initialization in this function
             */
            void setupSignalsAndSlots();

            /**
             * Runner for flushing table
             */
            void flushRunner() const;

            /**
             * Reinitializes the alarm services state from its persisted information
             */
            void reinitFromFile();

            /**
             * Add an update to a row in the alarm system
             * @param updateType: type of update: init, update, delete, acknowledgeable, deviceKilled
             * @param entry: alarm entry
             * @return: a Hash to add to the updated rows.
             */
            karabo::util::Hash addRowUpdate(const std::string& updateType, const karabo::util::Hash& entry) const;

            /**
             * Add/update the alarms for a device
             * @param deviceId: A device ID
             * @param alarms: A hash of property names -> hashes of alarm type entry hashes
             * @param rowUpdates: A Hash of updated rows which will be emitted to connected slots
             */
            void addDeviceAlarms(const std::string& deviceId, const karabo::util::Hash& alarms,
                                 karabo::util::Hash& rowUpdates);

            /**
             * Clear the alarms for a device
             * @param deviceId: A device ID
             * @param alarms: A hash of property names -> hashes of alarm type entry hashes
             * @param rowUpdates: A Hash of updated rows which will be emitted to connected slots
             */
            void removeDeviceAlarms(const std::string& deviceId, const karabo::util::Hash& alarms,
                                    karabo::util::Hash& rowUpdates);

            /**
             * Slot to be called if a client wishes to acknowledge an alarm
             * @param alarmServiceId: Alarm service the alarm should be registered at. Should be this device's instance
             * id
             * @param acknowledgedRows: the rows which should be acknowledged. It is a Hash where the keys give the
             * unique row id, the value is currently not evaluated.
             */
            void slotAcknowledgeAlarm(const karabo::util::Hash& acknowledgedRows);

            /**
             * Request a dump of all alarms currently managed by this alarm service device
             */
            void slotRequestAlarmDump();

            /**
             * Request a dump of all alarms currently managed by this alarm service device generically
             */
            void slotRequestAlarms(const karabo::util::Hash& info);

            /**
             * Implementation method to reply the alarmInformation for `slotRequestAlarms` and `slotRequestAlarms`
             */
            void sendAlarmInformation();

            /**
             * This device may not be locked
             * @return false
             */
            bool allowLock() const {
                return false;
            }

            /**
             * Make all alarm types which are of a lower significance 'acknowledgeable'
             * if 'needsAcknowledging' is set for them.
             *
             * NOTE: `m_alarmChangeMutex` must be locked when calling this method!
             *
             * @param propertyAlarms: The sub-Hash of m_alarms containing all the alarm type hashes for a single
             * property
             * @param lastAdded: An alarm type to compare against
             * @param rowUpdates: A row updates Hash which will be emitted to connected slots
             */
            void makeMoreSignificantAcknowledgeable(karabo::util::Hash& propertyAlarms,
                                                    const karabo::util::AlarmCondition& lastAdded,
                                                    karabo::util::Hash& rowUpdates);

            /* Internal method to send a bulk hash of alarm system updates */
            void sendAlarmUpdates(const boost::system::error_code& error);

           private: // members
            std::map<std::string, karabo::util::Hash> m_registeredDevices;
            karabo::util::Hash m_alarms; // base data container, organized hierarchically

            /*
             These two maps contain the indexing and addressing information for alarms
             * in m_alarms. The index (key in the first map, value in the second for
             * reverse look-up) uniquely counts upwards. Hence, newly incoming alarms
             * are always added to the end of the map. The value of the first map,
             * key of the second is a pointer to the entry for this alarm in m_alarms.
             */
            std::map<unsigned long long, karabo::util::Hash::Node*> m_alarmsMap;
            std::map<karabo::util::Hash::Node*, unsigned long long> m_alarmsMap_r;

            boost::shared_mutex m_deviceRegisterMutex;

            boost::thread m_flushWorker;

            mutable boost::shared_mutex m_alarmChangeMutex;
            boost::atomic<bool> m_flushRunning;
            std::string m_flushFilePath;
            karabo::util::Hash m_updateHash; // our bulkset hash
            boost::mutex m_updateMutex;
            boost::asio::deadline_timer m_updateTimer; // our update timer for bulksets

            std::atomic<unsigned long long> m_alarmIdCounter;
        };
    } // namespace devices
} // namespace karabo

#endif /* KARABO_ALARMSERVICE_HH */
