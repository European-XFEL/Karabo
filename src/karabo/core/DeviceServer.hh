/*
 * $Id$
 *
 * Author: burkhard.heisen@xfel.eu>
 *
 * Created on February 9, 2011, 2:24 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef EXFEL_CORE_DEVICESERVER_HH
#define	EXFEL_CORE_DEVICESERVER_HH

#include <karabo/util/Factory.hh>
#include <karabo/util/PluginLoader.hh>
#include <karabo/xms/SignalSlotable.hh>
#include <karabo/log/Logger.hh>
#include "coredll.hh"

#include "FsmMacros.hh"

/**
 * The main European XFEL namespace
 */
namespace exfel {

    /**
     * Namespace for package core
     */
    namespace core {

        /**
         * The DeviceServer class.
         */
        class DeviceServer : public exfel::xms::SignalSlotable {
            
            typedef std::map<std::string, boost::thread*> DeviceInstanceMap;
            
            
        public:

            EXFEL_CLASSINFO(DeviceServer, "DeviceServer", "1.0")
            EXFEL_FACTORY_BASE_CLASS

            DeviceServer();

            virtual ~DeviceServer();

            static void expectedParameters(exfel::util::Schema&);

            void configure(const exfel::util::Hash&);

            void run();
            
            /**************************************************************/
            /*                 Special Functions                          */
            /**************************************************************/
           
            EXFEL_FSM_LOGGER(log, log4cpp::CategoryStream, log4cpp::Priority::DEBUG)                    
                    
            EXFEL_FSM_ON_EXCEPTION(errorFound)

            EXFEL_FSM_NO_TRANSITION_V_ACTION(noStateTransition)
            
            EXFEL_FSM_ON_CURRENT_STATE_CHANGE(updateCurrentState)

            /**************************************************************/
            /*                        Events                              */
            /**************************************************************/

            // Standard events
            EXFEL_FSM_EVENT2(m_fsm, ErrorFoundEvent, errorFound, std::string, std::string)

            EXFEL_FSM_EVENT0(m_fsm, EndErrorEvent, endError)

            EXFEL_FSM_EVENT0(m_fsm, NewPluginAvailableEvent, newPluginAvailable)

            EXFEL_FSM_EVENT0(m_fsm, InbuildDevicesAvailableEvent, inbuildDevicesAvailable)

            EXFEL_FSM_EVENT1(m_fsm, StartDeviceEvent, slotStartDevice, exfel::util::Hash)
            
            EXFEL_FSM_EVENT1(m_fsm, RegistrationOkEvent, slotRegistrationOk, std::string)
            
            EXFEL_FSM_EVENT1(m_fsm, RegistrationFailedEvent, slotRegistrationFailed, std::string)
                      
            /**************************************************************/
            /*                        States                              */
            /**************************************************************/

            EXFEL_FSM_STATE_V_E(RegistrationState, registrationStateOnEntry)
            
            EXFEL_FSM_STATE(ErrorState)

            EXFEL_FSM_STATE_V_E(IdleState, idleStateOnEntry)

            EXFEL_FSM_STATE(ServingState)

            /**************************************************************/
            /*                    Transition  Actions                      */
            /**************************************************************/

            EXFEL_FSM_V_ACTION2(ErrorFoundAction, errorFoundAction, std::string, std::string)

            EXFEL_FSM_V_ACTION0(EndErrorAction, endErrorAction)

            EXFEL_FSM_V_ACTION0(NotifyNewDeviceAction, notifyNewDeviceAction)

            EXFEL_FSM_V_ACTION1(StartDeviceAction, startDeviceAction, exfel::util::Hash)
            
            EXFEL_FSM_V_ACTION1(RegistrationFailedAction, registrationFailed, std::string)
            
            EXFEL_FSM_V_ACTION1(RegistrationOkAction, registrationOk, std::string)
            
            /**************************************************************/
            /*                      AllOk Machine                         */
            /**************************************************************/

            EXFEL_FSM_TABLE_BEGIN(AllOkStateTransitionTable)
            Row< RegistrationState, RegistrationOkEvent, IdleState, RegistrationOkAction, none>,
            Row< RegistrationState, RegistrationFailedEvent, ErrorState, RegistrationFailedAction, none>,
            Row< IdleState, NewPluginAvailableEvent, none, NotifyNewDeviceAction, none >,
            Row< IdleState, InbuildDevicesAvailableEvent, none, NotifyNewDeviceAction, none >,
            Row< IdleState, StartDeviceEvent, ServingState, StartDeviceAction, none >,
            Row< ServingState, StartDeviceEvent, none, StartDeviceAction, none>
            EXFEL_FSM_TABLE_END
            
            //                       Name          Transition-Table     Initial-State  Context
            EXFEL_FSM_STATE_MACHINE(AllOkState, AllOkStateTransitionTable, RegistrationState, Self)


            /**************************************************************/
            /*                      Top Machine                           */
            /**************************************************************/

            EXFEL_FSM_TABLE_BEGIN(DeviceServerMachineTransitionTable)
            Row< AllOkState, ErrorFoundEvent, ErrorState, ErrorFoundAction, none >,
            Row< ErrorState, EndErrorEvent, AllOkState, EndErrorAction, none >
            EXFEL_FSM_TABLE_END

            
            EXFEL_FSM_STATE_MACHINE(DeviceServerMachine, DeviceServerMachineTransitionTable, AllOkState, Self)


            void startStateMachine() {

                EXFEL_FSM_CREATE_MACHINE(DeviceServerMachine, m_fsm);

                EXFEL_FSM_SET_CONTEXT_TOP(this, m_fsm);
                EXFEL_FSM_SET_CONTEXT_SUB(this, m_fsm, AllOkState);
                        
                EXFEL_FSM_START_MACHINE(m_fsm);
            }
            
            

        private: // Functions
            
            void loadLogger(const exfel::util::Hash& input);

            void loadPluginLoader(const exfel::util::Hash& input);
            
            void stopDeviceServer();

            log4cpp::Category& log();
            
            void registerAndConnectSignalsAndSlots();

            void updateAvailableDevices();

            void scanPlugins();

            void sayHello();
            
            void slotKillDeviceServerInstance();
            
            void slotKillDeviceInstance(const std::string& instanceId);

        private: // Member variables
            
            log4cpp::Category* m_log;
            
            EXFEL_FSM_DECLARE_MACHINE(DeviceServerMachine, m_fsm);

            exfel::util::PluginLoader::Pointer m_pluginLoader;
            boost::thread m_pluginThread;
            bool m_deviceServerStopped;

            bool m_isMaster;
            unsigned int m_nameRequestTimeout;
            bool m_gotName;

            exfel::util::Hash m_availableDevices;
            std::vector<exfel::util::Hash> m_autoStart;
            boost::thread_group m_deviceThreads;
            DeviceInstanceMap m_deviceInstanceMap;
            
            exfel::io::Format<exfel::util::Schema>::Pointer m_format;
            
            std::string m_devSrvInstId;
            exfel::net::BrokerConnection::Pointer m_connection;
            
            exfel::util::Hash m_connectionConfig;
            
        };
    } 
} 

EXFEL_REGISTER_FACTORY_BASE_HH(exfel::core::DeviceServer, TEMPLATE_CORE, DECLSPEC_CORE)

#endif
