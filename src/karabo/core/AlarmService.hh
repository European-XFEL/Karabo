/*
 * File:   AlarmService.hh
 * Author: haufs
 *
 * Created on August 5, 2016, 11:30 AM
 */

#ifndef KARABO_ALARMSERVICE_HH
#define	KARABO_ALARMSERVICE_HH

#include "Device.hh"
#include "OkErrorFsm.hh"
#include <boost/thread.hpp>

/**
 * The main karabo namespace
 */
namespace karabo {

    /**
     * Namespace for package core
     */
    namespace core {

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
             * to the new devices signalAlarmUpdate signal.
             */
            void registerNewDevice(const karabo::util::Hash& topologyEntry);

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




        private: // members

            std::map<std::string, karabo::util::Hash> m_registeredDevices;
            karabo::util::Hash m_alarms;
            boost::shared_mutex m_deviceRegisterMutex;

            /**
             Updates the alarm table of this device to reflect the entries in m_alarms
             */
            void updateAlarmTable();

            /**
             Add signals and slots which need to be set up during initialization in this function
             */
            void setupSignalsAndSlots();

        };
    }
}

#endif	/* KARABO_ALARMSERVICE_HH */

