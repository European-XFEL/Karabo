/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_CORE_MASTERDEVICE_HH
#define	KARABO_CORE_MASTERDEVICE_HH

#include "Device.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package core
     */
    namespace core {

        class MasterDevice : public Device {
        public:

            KARABO_CLASSINFO(MasterDevice, "MasterDevice", "1.0")

            MasterDevice() : Device(this) {
            }

            virtual ~MasterDevice();

            static void expectedParameters(karabo::util::Schema& expected);

            void configure(const karabo::util::Hash& input);

            void run();

        public:

        private: // Functions
            
            void initialize();
            
            void slotDeviceServerProvideName(const std::string& hostname);

            void slotNewDeviceServerAvailable(const std::string& hostname, const std::string& deviceServerInstanceId);

            void slotNewStandaloneDeviceInstanceAvailable(const std::string& hostname, const karabo::util::Hash& config, const std::string& deviceInstanceId, const std::string& xsd);

            void slotNewDeviceClassAvailable(const std::string& deviceServerInstanceId, const std::string& classId, const std::string& xsd);

            void slotNewDeviceInstanceAvailable(const std::string& deviceInstanceId, const karabo::util::Hash& config);

            void slotSchemaUpdated(const std::string& schema, const std::string& instanceId, const std::string& classId);

            void slotDeviceServerInstanceGone(const std::string& deviceServerInstanceId);

            void slotDeviceInstanceGone(const std::string& deviceServerInstanceId, const std::string& deviceInstanceId);
            
            void slotSelect(const std::string& fields, const std::string& table);
            
            void instanceNotAvailable(const std::string& networkId);

            void deviceServerInstanceNotAvailable(const std::string& networkId);

            void deviceInstanceNotAvailable(const std::string& networkId);

            void instanceAvailableAgain(const std::string& networkId);
            
            void slotCreateNewDeviceClassPlugin(const std::string& devSerInsId, const std::string& devClaId, const std::string& newDevClaId);

            
            /**
             * Tracks the existence of all device server AND device instances.
             */
            void trackInstances();
            
            void sanifyDeviceServerInstanceId(std::string& originalInstanceId) const;
                        
        private: // Members

        };
    }
}

#endif	
