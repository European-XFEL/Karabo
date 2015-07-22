/*
 * $Id$
 *
 * Author:
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_CORE_DATALOGGER_HH
#define	KARABO_CORE_DATALOGGER_HH

#include <fstream>
#include <boost/filesystem.hpp>

#include "Device.hh"
#include "OkErrorFsm.hh"
#include "Worker.hh"
#include "DataLoggerStructs.hh"

/**
 * The main karabo namespace
 */
namespace karabo {

    /**
     * Namespace for package core
     */
    namespace core {

        class DataLogger : public karabo::core::Device<karabo::core::OkErrorFsm> {
            std::string m_deviceToBeLogged;

            karabo::util::Schema m_currentSchema;

            boost::mutex m_configMutex;
            std::fstream m_configStream;

            unsigned int m_lastIndex;
            std::string m_user;
            karabo::util::Timestamp m_lastDataTimestamp;
            bool m_pendingLogin;

            long m_startPosition;
            boost::thread m_flushThread;
            
            std::map<std::string, karabo::core::MetaData::Pointer> m_idxMap; 

        public:

            KARABO_CLASSINFO(DataLogger, "DataLogger", "1.0")

            static void expectedParameters(karabo::util::Schema& expected);

            DataLogger(const karabo::util::Hash& input);

            virtual ~DataLogger() {
                slotTagDeviceToBeDiscontinued(true, 'L');
                KARABO_LOG_INFO << "dead.";
            }


        private: // Functions

            void okStateOnEntry();

            void slotChanged(const karabo::util::Hash& configuration, const std::string& deviceId);

            void slotSchemaUpdated(const karabo::util::Schema& schema, const std::string& deviceId);

            /**
             * This tags a device to be discontinued, three cases have to be distinguished
             * 
             * (a) Regular shut-down of the device (wasValidUpToNow = true, reason = 'D')
             * (b) Silent death of the device (wasValidUpToNow = true, reason = 'D')
             * (c) Start-up of this (DataLogger) device whilst the device was alive (wasValidUpToNow = false, reason = 'L')
             * 
             * This slot will be called by the DataLoggerManager
             * 
             */
            void slotTagDeviceToBeDiscontinued(const bool wasValidUpToNow, const char reason);

            void refreshDeviceInformation();

            int determineLastIndex(const std::string& deviceId);

            int incrementLastIndex(const std::string& deviceId);
            
            void flushThread();
        };
    }
}

#endif	
