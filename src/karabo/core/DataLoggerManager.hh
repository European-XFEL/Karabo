/*
 * $Id$
 *
 * Author:
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_CORE_DATALOGGERMANAGER_HH
#define	KARABO_CORE_DATALOGGERMANAGER_HH

#include <boost/filesystem.hpp>

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

        class DataLoggerManager : public Device<OkErrorFsm> {
           
            
        public:

            KARABO_CLASSINFO(DataLoggerManager, "DataLoggerManager", "1.0")

            static void expectedParameters(karabo::util::Schema& expected);

            DataLoggerManager(const karabo::util::Hash& input);

            virtual ~DataLoggerManager();


        private: // Functions

            void okStateOnEntry();
            
            void instanceNewHandler(const karabo::util::Hash& topologyEntry);
            
            void instanceGoneHandler(const std::string& instanceId, const karabo::util::Hash& instanceInfo);
                        
        };
    }
}

#endif	
