/*
 * File:   AlarmService.hh
 * Author: haufs
 *
 * Created on August 5, 2016, 11:30 AM
 */

#ifndef KARABO_ALARMSERVICE_HH
#define	KARABO_ALARMSERVICE_HH

#include <set>

#include "Device.hh"
#include "OkErrorFsm.hh"

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

            void registerAlarmWithNewDevice(const karabo::util::Hash& topologyEntry);

            void slotRegisterExistingDevice(const karabo::util::Hash& instanceInfo);

            /**
             * This slot is to be called to the the alarm service device know of an update
             * in device alarms. As only parameter it expects an alarmInfo Hash, which should
             * have the following structure:
             *
             *   deviceId -> toClear -> property A -> alarm type 1 -> bool
             *                       -> property A -> alarm type 2 -> bool
             *                       -> property B -> ....
             *
             *  The bool field is not evaluated but serves as a placeholder
             *
             *   deviceId -> toClear -> property A -> alarm type 1 -> Hash()
             *   deviceId -> toClear -> property A -> alarm type 2 -> Hash()
             *   deviceId -> toClear -> property B -> ...
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
            void slotUpdateAlarms(const karabo::util::Hash& alarmInfo);

            void slotUpdateAlarmTable();

            void setupSignalsAndSlots();

        private: // members

            std::set<karabo::util::Hash> m_registeredDevices;
            karabo::util::Hash m_alarms;

            void updateAlarmTable();


        };
    }
}

#endif	/* KARABO_ALARMSERVICE_HH */

