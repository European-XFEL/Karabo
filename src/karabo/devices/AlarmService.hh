/*
 * File:   AlarmService.hh
 * Author: haufs
 *
 * Created on August 5, 2016, 11:30 AM
 */

#ifndef KARABO_ALARMSERVICE_HH
#define	KARABO_ALARMSERVICE_HH

#include <boost/thread.hpp>
#include <boost/atomic.hpp>

#include "karabo/core/Device.hh"
#include "karabo/core/OkErrorFsm.hh"


/**
 * The main karabo namespace
 */
namespace karabo {

    /**
     * Namespace for package core
     */
    namespace devices {

        class AlarmService : public karabo::core::Device<> {

        public:

            KARABO_CLASSINFO(AlarmService, "AlarmService", "2.0")

            static void expectedParameters(karabo::util::Schema& expected);

            AlarmService(const karabo::util::Hash& input);

            virtual ~AlarmService();

        private: // Functions

            void initialize();

            /**
             * Callback for the instanceNew monitor. Connects this device's slotUpdateAlarms function
             * to the new devices signalAlarmUpdate signal. If the device was previously known it will ask this
             * device to submit its current alarm state.
             */
            void registerNewDevice(const karabo::util::Hash& topologyEntry);

            /**
             * Called when a device instance disappears from the distributed system. It will trigger the alarm
             * service to set all alarms pending for this device to need acknowledgement and acknowledgeable. This
             * means alarms will not silently disappear, but because it can't be assured that the device instance
             * that disappeared will ever clear them for acknowledgment they are acknowledgeble. Note that if the
             * device instance does happen to reappear it will be asked to resubmit its current alarm state, bringing
             * all alarms pertinent to it back into a consistent needsacknowledging, acknowledging and cleared condition.
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
             * We check if
             *
             * @param incomingReconfiguration
             *
             * contains any requests for acknowledgement, and then if these
             * are indeed allowed.
             */
            void preReconfigure(karabo::util::Hash& incomingReconfiguration);


            /**
             * Updates the alarm table of this device to reflect the entries in m_alarms
             */
            void updateAlarmTable();

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



        private: // members

            std::map<std::string, karabo::util::Hash> m_registeredDevices;
            karabo::util::Hash m_alarms;
            boost::shared_mutex m_deviceRegisterMutex;

            boost::thread m_flushWorker;

            mutable boost::shared_mutex m_alarmChangeMutex;
            boost::atomic<bool> m_flushRunning;
            std::string m_flushFilePath;



        };
    }
}

#endif	/* KARABO_ALARMSERVICE_HH */

