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
            karabo::util::Hash m_runtimeSystemDescription;
            
            boost::mutex m_runtimeSystemDescriptionMutex;
            
            
            /**
             * device +
             *   <deviceId> @
             *     [0]
             *       description t0 = <timestamp> @
             *         [0]
             *           val t="<timestamp>" => SCHEMA
             *       configuration t0 = <timestamp> +
             *         <key> @
             *           [0]
             *             val t="<timestamp>" => VALUE
             *          
             */
            karabo::util::Hash m_systemArchive;
            
        public:

            KARABO_CLASSINFO(MasterDevice2, "MasterDevice2", "1.0")

            static void expectedParameters(karabo::util::Schema& expected);

            MasterDevice2(const karabo::util::Hash& input);
            
            virtual ~MasterDevice2();


        private: // Functions
            
            void setupSlots();
            
            void cacheAvailableInstances();
            
            void cacheAvailableContent();
           
                        
        };
    }
}

#endif	
