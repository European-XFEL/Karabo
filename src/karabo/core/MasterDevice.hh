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
#include "OkErrorFsm.hh"

/**
 * The main karabo namespace
 */
namespace karabo {

    /**
     * Namespace for package core
     */
    namespace core {

        class MasterDevice : public Device<OkErrorFsm> {

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
            karabo::util::Hash m_systemNow;

            boost::mutex m_systemNowMutex;


            /**
             * device +
             *   <deviceId> +
             *     description t0 = <timestamp> @
             *       [0]
             *         val t="<timestamp>" => SCHEMA
             *     configuration t0 = <timestamp> +
             *       <key> @
             *         [0]
             *           val t="<timestamp>" [isLast] => VALUE
             *          
             */
            karabo::util::Hash m_systemHistory;

            boost::mutex m_systemHistoryMutex;
            
            bool m_persistData;
            
            boost::thread m_persistDataThread;
            
        public:

            KARABO_CLASSINFO(MasterDevice, "MasterDevice", "1.0")

            static void expectedParameters(karabo::util::Schema& expected);

            MasterDevice(const karabo::util::Hash& input);

            virtual ~MasterDevice();


        private: // Functions

            size_t getNumberOfServersOnHost(const std::string& hostname);
            
            void okStateOnEntry();
            
            void setupSlots();

            void slotInstanceNew(const std::string& instanceId, const karabo::util::Hash& instanceInfo);

            void onInstanceNewForSystemNow(const std::string& instanceId, const karabo::util::Hash& instanceInfo);
            
            std::string getInstanceType(const karabo::util::Hash& instanceInfo) const;
            
            void onInstanceNewForSystemHistory(const std::string& deviceId, const karabo::util::Hash& instanceInfo);
            
            void fetchConfiguration(const std::string& deviceId, karabo::util::Hash& configuration) const;
                
            void fetchDescription(const std::string& deviceId, karabo::util::Schema& description) const;
                        
            void onInstanceGoneForSystemNow(const std::string& instanceId, const karabo::util::Hash& instanceInfo);
            
            void onInstanceGoneForSystemHistory(const std::string& deviceId, const karabo::util::Hash& instanceInfo);
            
            
            
            void slotInstanceGone(const std::string& instanceId, const karabo::util::Hash& instanceInfo);
            
            void instanceNotAvailable(const std::string& instanceId);
            
            void slotChanged(const karabo::util::Hash& changedConfig, const std::string& deviceId);
            
            void persistDataThread();
            
        };
    }
}

#endif	
