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

#include <karabo/util/Configurator.hh>
#include <karabo/util/PluginLoader.hh>
#include <karabo/xms/SignalSlotable.hh>
#include <karabo/log/Logger.hh>
#include <karabo/util/Version.hh>
#include "coredll.hh"

#include "FsmMacros.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package core
     */
    namespace core {

        /**
         * The DeviceServer class.
         */
        class DeviceServer : public karabo::xms::SignalSlotable {
            typedef std::map<std::string, boost::thread*> DeviceInstanceMap;

            krb_log4cpp::Category* m_log;
            karabo::log::Logger::Pointer m_logger;

            karabo::util::PluginLoader::Pointer m_pluginLoader;
            boost::thread m_pluginThread;
            bool m_doScanPlugins;
            bool m_serverIsRunning;

            bool m_isMaster;
            bool m_debugMode;
            std::vector<karabo::util::Hash> m_autoStart;
            bool m_scanPlugins;

            karabo::util::Hash m_availableDevices;
            boost::thread_group m_deviceThreads;
            DeviceInstanceMap m_deviceInstanceMap;
            boost::mutex m_deviceInstanceMutex;
            std::map<std::string, unsigned int> m_deviceInstanceCount;
            int m_visibility;

            karabo::net::BrokerConnection::Pointer m_connection;

            karabo::util::Hash m_connectionConfiguration;
            std::string m_serverId;

            int m_heartbeatIntervall;
            
            karabo::util::Hash m_loggerConfiguration;
            
        public:

            KARABO_CLASSINFO(DeviceServer, "DeviceServer", karabo::util::Version::getVersion())
            KARABO_CONFIGURATION_BASE_CLASS


            static void expectedParameters(karabo::util::Schema&);

            DeviceServer(const karabo::util::Hash&);

            virtual ~DeviceServer();

            void run();

            bool isRunning() const;

            bool isDebugMode();


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

            KARABO_FSM_STATE_V_E(Ok, okStateOnEntry)

            KARABO_FSM_STATE(Error)

            /**************************************************************/
            /*                    Transition Actions                      */
            /**************************************************************/

            KARABO_FSM_V_ACTION2(ErrorFoundAction, errorFoundAction, std::string, std::string);

            /**************************************************************/
            /*                      Top Machine                         */
            /**************************************************************/

            //  Source-State    Event        Target-State    Action         Guard

            KARABO_FSM_TABLE_BEGIN(StateMachineTransitionTable)
            Row< Ok, ErrorFoundEvent, Error, ErrorFoundAction, none >,
            Row< Error, ResetEvent, Ok, none, none >
            KARABO_FSM_TABLE_END


            //                       Name          Transition-Table             Initial-State Context
            KARABO_FSM_STATE_MACHINE(StateMachine, StateMachineTransitionTable, Ok, Self)


            void startFsm() {

                KARABO_FSM_CREATE_MACHINE(StateMachine, m_fsm);
                KARABO_FSM_SET_CONTEXT_TOP(this, m_fsm)
                KARABO_FSM_START_MACHINE(m_fsm)
            }

        private: // Functions

            void newPluginAvailable();

            void slotStartDevice(const karabo::util::Hash& configuration);

            void onStateUpdate(const std::string& currentState);

            void loadLogger(const karabo::util::Hash& input);

            void loadPluginLoader(const karabo::util::Hash& input);

            void slotKillServer();

            void stopDeviceServer();

            krb_log4cpp::Category& log();

            void registerAndConnectSignalsAndSlots();

            void updateAvailableDevices();

            void scanPlugins();

            void sayHello();

            void slotDeviceGone(const std::string& instanceId);

            void slotGetClassSchema(const std::string& classId);

            std::string generateDefaultServerId() const;

            std::string generateDefaultDeviceId(const std::string& classId);

            void instantiateOld(const karabo::util::Hash& hash);

            void instantiateNew(const karabo::util::Hash& hash);

            void slotLoggerPriority(const std::string& prio);

            KARABO_FSM_DECLARE_MACHINE(StateMachine, m_fsm);


        };
    }
}

// TODO windows
//KARABO_REGISTER_FACTORY_BASE_HH(karabo::core::DeviceServer, TEMPLATE_CORE, DECLSPEC_CORE)

#endif
