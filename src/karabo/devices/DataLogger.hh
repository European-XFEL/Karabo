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

#include "karabo/util/DataLogUtils.hh"
#include "karabo/core/Device.hh"
#include "karabo/core/OkErrorFsm.hh"
#include "karabo/core/Worker.hh"


/**
 * The main karabo namespace
 */
namespace karabo {

    /**
     * Namespace for package core
     */
    namespace devices {

        /**
         * @class DataLogger
         * @brief A DataLogger device is created on a 1:1 mapping for each device in the distributed
         *        system and logs its slow control data.
         * 
         * A DataLogger device is created on a 1:1 mapping for each device in the distributed
         * system and logs its slow control data. DataLoggers are managed by the
         * karabo::devices::DataLoggerManager. Information is passed to them using a dedicated p2p
         * channel, thus logging does not result in additional broker load.
         */
        class DataLogger : public karabo::core::Device<karabo::core::OkErrorFsm> {

            std::string m_deviceToBeLogged;

            boost::mutex m_currentSchemaMutex;
            karabo::util::Schema m_currentSchema;

            boost::mutex m_configMutex;
            std::fstream m_configStream;

            unsigned int m_lastIndex;
            std::string m_user;
            karabo::util::Timestamp m_lastDataTimestamp;
            bool m_pendingLogin;

            std::map<std::string, karabo::util::MetaData::Pointer> m_idxMap; // protect by m_configMutex!
            std::vector<std::string> m_idxprops; // needs no mutex as long as used only in slotChanged
            size_t m_propsize;
            time_t m_lasttime;

            boost::asio::deadline_timer m_flushDeadline;
            bool m_doFlushFiles;
            unsigned int m_flushInterval;

        public:

            KARABO_CLASSINFO(DataLogger, "DataLogger", "1.0")

            static void expectedParameters(karabo::util::Schema& expected);

            DataLogger(const karabo::util::Hash& input);

            virtual ~DataLogger();

        private: // Functions

            void okStateOnEntry();

            void slotChanged(const karabo::util::Hash& configuration, const std::string& deviceId);

            /// Helper function to update m_idxprops, returns whether m_idxprops changed.
            bool updatePropsToIndex();

            /// Helper to ensure archive file is closed.
            /// Must be called under protection of the 'm_configMutex' mutex.
            void ensureFileClosed();

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

            void handleFirstSchemaRequest(const karabo::util::Schema& schema, const std::string& deviceId);

            void errorHandleFirstSchemaRequest();

            int determineLastIndex(const std::string& deviceId);

            int incrementLastIndex(const std::string& deviceId);

            void flushActor(const boost::system::error_code& e);

            void doFlush();
            
            /**
             * This device may not be locked
             * @return false
             */
            bool allowLock() const {
                return false;
            }
        };
    }
}

#endif