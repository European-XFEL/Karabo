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

/**
 * The main karabo namespace
 */
namespace karabo {

    /**
     * Namespace for package core
     */
    namespace core {

        class AlarmService : public karabo::core::Device<karabo::core::OkErrorFsm> {

        public:

            KARABO_CLASSINFO(AlarmService, "AlarmService", "2.0")

            static void expectedParameters(karabo::util::Schema& expected);

            AlarmService(const karabo::util::Hash& input);

            virtual ~AlarmService();

        private: // Functions

            void okStateOnEntry();

        };
    }
}

#endif	/* KARABO_ALARMSERVICE_HH */

