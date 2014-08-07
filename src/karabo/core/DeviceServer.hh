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


        public:

            KARABO_CLASSINFO(DeviceServer, "DeviceServer", karabo::util::Version::getVersion())
            KARABO_CONFIGURATION_BASE_CLASS


            static void expectedParameters(karabo::util::Schema&);

            DeviceServer(const karabo::util::Hash&);

            virtual ~DeviceServer();

            void run();

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

            // Standard events
            KARABO_FSM_EVENT2(m_fsm, ErrorFoundEvent, errorFound, std::string, std::string)

            KARABO_FSM_EVENT0(m_fsm, EndErrorEvent, endError)

            KARABO_FSM_EVENT0(m_fsm, NewPluginAvailableEvent, newPluginAvailable)

            KARABO_FSM_EVENT0(m_fsm, InbuildDevicesAvailableEvent, inbuildDevicesAvailable)

            KARABO_FSM_EVENT1(m_fsm, StartDeviceEvent, slotStartDevice, karabo::util::Hash)

            /**************************************************************/
            /*                        States                              */
            /**************************************************************/

            KARABO_FSM_STATE(ErrorState)

            KARABO_FSM_STATE_V_E(IdleState, idleStateOnEntry)

            KARABO_FSM_STATE(ServingState)

            /**************************************************************/
            /*                    Transition  Actions                      */
            /**************************************************************/

            KARABO_FSM_V_ACTION2(ErrorFoundAction, errorFoundAction, std::string, std::string)

            KARABO_FSM_V_ACTION0(EndErrorAction, endErrorAction)

            KARABO_FSM_V_ACTION0(NotifyNewDeviceAction, notifyNewDeviceAction)

            KARABO_FSM_V_ACTION1(StartDeviceAction, startDeviceAction, karabo::util::Hash)

            /**************************************************************/
            /*                      AllOk Machine                         */
            /**************************************************************/

            KARABO_FSM_TABLE_BEGIN(AllOkStateTransitionTable)
            Row< IdleState, NewPluginAvailableEvent, none, NotifyNewDeviceAction, none >,
            Row< IdleState, InbuildDevicesAvailableEvent, none, NotifyNewDeviceAction, none >,
            Row< IdleState, StartDeviceEvent, ServingState, StartDeviceAction, none >,
            Row< ServingState, StartDeviceEvent, none, StartDeviceAction, none>,
            Row< ServingState, NewPluginAvailableEvent, none, NotifyNewDeviceAction, none >
            KARABO_FSM_TABLE_END

            //                       Name          Transition-Table     Initial-State  Context
            KARABO_FSM_STATE_MACHINE(AllOkState, AllOkStateTransitionTable, IdleState, Self)


            /**************************************************************/
            /*                      Top Machine                           */
            /**************************************************************/

            KARABO_FSM_TABLE_BEGIN(DeviceServerMachineTransitionTable)
            Row< AllOkState, ErrorFoundEvent, ErrorState, ErrorFoundAction, none >,
            Row< ErrorState, EndErrorEvent, AllOkState, EndErrorAction, none >
            KARABO_FSM_TABLE_END


            KARABO_FSM_STATE_MACHINE(DeviceServerMachine, DeviceServerMachineTransitionTable, AllOkState, Self)


            void startFsm() {

                KARABO_FSM_CREATE_MACHINE(DeviceServerMachine, m_fsm);

                KARABO_FSM_SET_CONTEXT_TOP(this, m_fsm);
                KARABO_FSM_SET_CONTEXT_SUB(this, m_fsm, AllOkState);

                KARABO_FSM_START_MACHINE(m_fsm);
            }



        private: // Functions

            void onStateUpdate(const std::string& currentState);

            void loadLogger(const karabo::util::Hash& input);

            void loadPluginLoader(const karabo::util::Hash& input);

            void slotKillServer();

            void stopDeviceServer();

            log4cpp::Category& log();

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

        private: // Member variables

            log4cpp::Category* m_log;

            KARABO_FSM_DECLARE_MACHINE(DeviceServerMachine, m_fsm);

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
            std::map<std::string, unsigned int> m_deviceInstanceCount;
            int m_visibility;

            karabo::net::BrokerConnection::Pointer m_connection;

            karabo::util::Hash m_connectionConfiguration;
            std::string m_serverId;

        public:
            bool isDebugMode();

        };
    }
}

// TODO windows
//KARABO_REGISTER_FACTORY_BASE_HH(karabo::core::DeviceServer, TEMPLATE_CORE, DECLSPEC_CORE)

#endif
