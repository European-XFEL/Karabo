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

            void setupSignalsAndSlots();

        private: // members

            std::map<std::string, karabo::util::Hash> m_registeredDevices;
            karabo::util::Hash m_alarms;



        };
    }
}

#endif	/* KARABO_ALARMSERVICE_HH */

