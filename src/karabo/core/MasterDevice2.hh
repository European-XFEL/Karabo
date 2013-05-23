/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_CORE_MASTERDEVICE2_HH
#define	KARABO_CORE_MASTERDEVICE2_HH

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

        class MasterDevice2 : public Device<OkErrorFsm> {

            /**
             * server +
             *   <serverId> type host deviceClasses version +
             *     classes +
             *       <classId> +
             *         description SCHEMA
             *         configuration HASH
             *     description SCHEMA
             *     configuration HASH
             *     
             * device +
             *   <deviceId> type host classId serverId version +
             *      description => SCHEMA
             *      configuration => HASH
             *   
             */
            karabo::util::Hash m_runtimeSystemTopology;

            boost::mutex m_runtimeSystemTopologyMutex;


            /**
             * device +
             *   <deviceId> +
             *     description t0 = <timestamp> @
             *       [0]
             *         val t="<timestamp>" => SCHEMA
             *     configuration t0 = <timestamp> +
             *       <key> @
             *         [0]
             *           val t="<timestamp>" => VALUE
             *          
             */
            karabo::util::Hash m_systemArchive;

            boost::mutex m_systemArchiveMutex;
            
            bool m_persistData;
            
            boost::thread m_persistDataThread;
            
        public:

            KARABO_CLASSINFO(MasterDevice2, "MasterDevice2", "1.0")

            static void expectedParameters(karabo::util::Schema& expected);

            MasterDevice2(const karabo::util::Hash& input);

            virtual ~MasterDevice2();


        private: // Functions

            void okStateOnEntry();
            
            void setupSlots();

            void cacheAvailableInstances();

            void handleInstanceUpdateForSystemTopology(const std::string& instanceId, const karabo::util::Hash& instanceInfo);
            
            void handleDeviceInstanceUpdateForSystemArchive(const std::string& deviceId);
            
            void handleInstanceGoneForSystemTopology(const std::string& instanceId, const karabo::util::Hash& instanceInfo);
            
            void handleDeviceInstanceGoneForSystemArchive(const std::string& deviceId);
            
            void slotInstanceUpdated(const std::string& instanceId, const karabo::util::Hash& instanceInfo);
            
            void slotInstanceGone(const std::string& instanceId, const karabo::util::Hash& instanceInfo);
            
            void instanceNotAvailable(const std::string& instanceId);
            
            void slotChanged(const karabo::util::Hash& changedConfig, const std::string& deviceId);
            
            void persistDataThread();
            
        };
    }
}

#endif	
