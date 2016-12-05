/*
 * $Id$
 *
 * Author: burkhard.heisen@xfel.eu>
 *
 * Created on February 9, 2011, 2:24 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_CORE_DEVICESERVER_HH
#define	KARABO_CORE_DEVICESERVER_HH

#include "FsmMacros.hh"

#include "karabo/log/Logger.hh"
#include "karabo/util/Configurator.hh"
#include "karabo/util/PluginLoader.hh"
#include "karabo/util/Version.hh"
#include "karabo/util/State.hh"
#include "karabo/xms/SignalSlotable.hh"

#include "boost/asio/deadline_timer.hpp"

/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package core
     */
    namespace core {

        class BaseDevice;

        /**
         * @class DeviceServer
         * @brief The DeviceServer class hosts device instances
         * 
         * The DeviceServer class hosts device instances. It monitors the system
         * for new class plugins appearing and notifies the distributed system
         * of these and their static information.
         * 
         * The device server uses an ok-error FSM, which knows the ERROR and 
         * NORMAL karabo::util::State states.
         */
        class DeviceServer : public karabo::xms::SignalSlotable {

            krb_log4cpp::Category* m_log;
            karabo::log::Logger::Pointer m_logger;

            karabo::util::PluginLoader::Pointer m_pluginLoader;
            boost::asio::deadline_timer m_scanPluginsTimer;
            bool m_scanPlugins;
            bool m_serverIsRunning;

            std::vector<karabo::util::Hash> m_autoStart;

            karabo::util::Hash m_availableDevices;
            std::vector<std::string> m_devices;

            typedef std::map<std::string, boost::shared_ptr<BaseDevice> > DeviceInstanceMap;
            DeviceInstanceMap m_deviceInstanceMap;
            boost::mutex m_deviceInstanceMutex;
            std::map<std::string, unsigned int> m_deviceInstanceCount;
            int m_visibility;

            karabo::net::JmsConnection::Pointer m_connection;

            std::string m_serverId;

        public:

            KARABO_CLASSINFO(DeviceServer, "DeviceServer", karabo::util::Version::getVersion())
            KARABO_CONFIGURATION_BASE_CLASS


            /**
             * Static properties of the device server
             * @param to inject these properties to
             */
            static void expectedParameters(karabo::util::Schema&);

            /**
             * The constructor expects a configuration Hash. The following 
             * configuration options are supported:
             * 
             * - serverId: a string giving the server's id
             * - autostart: a vector of Hashes containing configurations for device that are to be automatically started by the server
             * - scanPlugins: a boolean indicating if this server should scan for additional plugins
             * - visibility: an integer indicating device server visibility in the distributed system
             * - debugMode: a boolean indicating if the server should run in debugMode
             * - connection: a Hash containing the connection information for a karabo::net::JmsConnection
             * - pluginDirectory: a path to the plugin directory for this device server
             * - heartbeatInterval: interval in seconds at which this server sends heartbeats to the distributed system
             * - nThreads: number of threads to use in this device server
             * @param 
             */
            DeviceServer(const karabo::util::Hash&);

            virtual ~DeviceServer();

            void finalizeInternalInitialization();

            /**
             * Check if the device server is running
             * @return 
             */
            bool isRunning() const;


            /**************************************************************/
            /*                 Special Functions                          */

            /**************************************************************/

            KARABO_FSM_ON_EXCEPTION(errorFound)

            KARABO_FSM_NO_TRANSITION_V_ACTION(noStateTransition)

            KARABO_FSM_ON_CURRENT_STATE_CHANGE(onStateUpdate)


            /**************************************************************/
            /*                        Events                              */
            /**************************************************************/

            KARABO_FSM_EVENT2(m_fsm, ErrorFoundEvent, errorFound, std::string, std::string)

            KARABO_FSM_EVENT0(m_fsm, ResetEvent, reset)

            /**************************************************************/
            /*                        States                              */
            /**************************************************************/

            KARABO_FSM_STATE_V_E(NORMAL, okStateOnEntry)

            KARABO_FSM_STATE(ERROR)

            /**************************************************************/
            /*                    Transition Actions                      */
            /**************************************************************/

            KARABO_FSM_V_ACTION2(ErrorFoundAction, errorFoundAction, std::string, std::string);

            /**************************************************************/
            /*                      Top Machine                         */
            /**************************************************************/

            //  Source-State    Event        Target-State    Action         Guard

            KARABO_FSM_TABLE_BEGIN(StateMachineTransitionTable)
            Row< NORMAL, ErrorFoundEvent, ERROR, ErrorFoundAction, none >,
            Row< ERROR, ResetEvent, NORMAL, none, none >,
            Row< ERROR, ErrorFoundEvent, ERROR, ErrorFoundAction, none >
            KARABO_FSM_TABLE_END


            //                       Name          Transition-Table             Initial-State Context
            KARABO_FSM_STATE_MACHINE(StateMachine, StateMachineTransitionTable, NORMAL, Self)


            void startFsm() {

                KARABO_FSM_CREATE_MACHINE(StateMachine, m_fsm);
                KARABO_FSM_SET_CONTEXT_TOP(this, m_fsm)
                KARABO_FSM_START_MACHINE(m_fsm)
            }

        private: // Functions

            void newPluginAvailable();

            void slotStartDevice(const karabo::util::Hash& configuration);

            void onStateUpdate(const karabo::util::State& currentState);

            void loadLogger(const karabo::util::Hash& input);

            void loadPluginLoader(const karabo::util::Hash& input);

            void slotKillServer();

            void stopDeviceServer();

            krb_log4cpp::Category& log();

            void registerSlots();

            void updateAvailableDevices();

            void scanPlugins(const boost::system::error_code& e);

            void slotDeviceGone(const std::string& instanceId);

            void slotGetClassSchema(const std::string& classId);

            std::string generateDefaultServerId() const;

            std::string generateDefaultDeviceId(const std::string& classId);

            /// Helper to create input passed to instantiate.
            /// Returns a tuple of the deviceId, the classId and the configuration.
            boost::tuple<std::string, std::string, util::Hash>
            prepareInstantiate(const util::Hash& configuration);

            /// Helper for slotStartDevice - e.g. sets the reply.
            void instantiate(const std::string& deviceId, const std::string& classId, const util::Hash& config);

            void slotLoggerPriority(const std::string& prio);

            KARABO_FSM_DECLARE_MACHINE(StateMachine, m_fsm);
        };

    }
}


#endif
