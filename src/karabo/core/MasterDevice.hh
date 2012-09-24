/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef EXFEL_CORE_MASTERDEVICE_HH
#define	EXFEL_CORE_MASTERDEVICE_HH

#include "Device.hh"

/**
 * The main European XFEL namespace
 */
namespace exfel {

    /**
     * Namespace for package core
     */
    namespace core {

        class MasterDevice : public Device {
        public:

            EXFEL_CLASSINFO(MasterDevice, "MasterDevice", "1.0")

            MasterDevice() : Device(this) {
            }

            virtual ~MasterDevice();

            static void expectedParameters(exfel::util::Schema& expected);

            void configure(const exfel::util::Hash& input);

            void run();

        public:

        private: // Functions
            
            void initialize();
            
            void slotDeviceServerProvideName(const std::string& hostname);

            void slotNewDeviceServerAvailable(const std::string& hostname, const std::string& deviceServerInstanceId);

            void slotNewStandaloneDeviceInstanceAvailable(const std::string& hostname, const exfel::util::Hash& config, const std::string& deviceInstanceId, const std::string& xsd);

            void slotNewDeviceClassAvailable(const std::string& deviceServerInstanceId, const std::string& classId, const std::string& xsd);

            void slotNewDeviceInstanceAvailable(const std::string& deviceInstanceId, const exfel::util::Hash& config);

            void slotSchemaUpdated(const std::string& schema, const std::string& instanceId, const std::string& classId);

            void slotDeviceServerInstanceGone(const std::string& deviceServerInstanceId);

            void slotDeviceInstanceGone(const std::string& deviceServerInstanceId, const std::string& deviceInstanceId);
            
            void slotSelect(const std::string& fields, const std::string& table);
            
            void instanceNotAvailable(const std::string& networkId);

            void deviceServerInstanceNotAvailable(const std::string& networkId);

            void deviceInstanceNotAvailable(const std::string& networkId);

            void instanceAvailableAgain(const std::string& networkId);
            
            /**
             * Tracks the existence of all device server AND device instances.
             */
            void trackInstances();

        private: // Members

        };
    }
}

#endif	
