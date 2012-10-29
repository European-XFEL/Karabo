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

#include <karabo/util/Factory.hh>
#include <karabo/util/PluginLoader.hh>
#include <karabo/xms/SignalSlotable.hh>
#include <karabo/log/Logger.hh>
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

            KARABO_CLASSINFO(DeviceServer, "DeviceServer", "1.0")
            KARABO_FACTORY_BASE_CLASS

            DeviceServer();

            virtual ~DeviceServer();

            static void expectedParameters(karabo::util::Schema&);

            void configure(const karabo::util::Hash&);

            void run();
            
            /**************************************************************/
            /*                 Special Functions                          */
            /**************************************************************/
           
            KARABO_FSM_LOGGER(log, log4cpp::CategoryStream, log4cpp::Priority::DEBUG)                    
                    
            KARABO_FSM_ON_EXCEPTION(errorFound)

            KARABO_FSM_NO_TRANSITION_V_ACTION(noStateTransition)
            
            KARABO_FSM_ON_CURRENT_STATE_CHANGE(updateCurrentState)

            /**************************************************************/
            /*                        Events                              */
            /**************************************************************/

            // Standard events
            KARABO_FSM_EVENT2(m_fsm, ErrorFoundEvent, errorFound, std::string, std::string)

            KARABO_FSM_EVENT0(m_fsm, EndErrorEvent, endError)

            KARABO_FSM_EVENT0(m_fsm, NewPluginAvailableEvent, newPluginAvailable)

            KARABO_FSM_EVENT0(m_fsm, InbuildDevicesAvailableEvent, inbuildDevicesAvailable)

            KARABO_FSM_EVENT1(m_fsm, StartDeviceEvent, slotStartDevice, karabo::util::Hash)
            
            KARABO_FSM_EVENT1(m_fsm, RegistrationOkEvent, slotRegistrationOk, std::string)
            
            KARABO_FSM_EVENT1(m_fsm, RegistrationFailedEvent, slotRegistrationFailed, std::string)
                      
            /**************************************************************/
            /*                        States                              */
            /**************************************************************/

            KARABO_FSM_STATE_V_E(RegistrationState, registrationStateOnEntry)
            
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
            
            KARABO_FSM_V_ACTION1(RegistrationFailedAction, registrationFailed, std::string)
            
            KARABO_FSM_V_ACTION1(RegistrationOkAction, registrationOk, std::string)
            
            /**************************************************************/
            /*                      AllOk Machine                         */
            /**************************************************************/

            KARABO_FSM_TABLE_BEGIN(AllOkStateTransitionTable)
            Row< RegistrationState, RegistrationOkEvent, IdleState, RegistrationOkAction, none>,
            Row< RegistrationState, RegistrationFailedEvent, ErrorState, RegistrationFailedAction, none>,
            Row< IdleState, NewPluginAvailableEvent, none, NotifyNewDeviceAction, none >,
            Row< IdleState, InbuildDevicesAvailableEvent, none, NotifyNewDeviceAction, none >,
            Row< IdleState, StartDeviceEvent, ServingState, StartDeviceAction, none >,
            Row< ServingState, StartDeviceEvent, none, StartDeviceAction, none>
            KARABO_FSM_TABLE_END
            
            //                       Name          Transition-Table     Initial-State  Context
            KARABO_FSM_STATE_MACHINE(AllOkState, AllOkStateTransitionTable, RegistrationState, Self)


            /**************************************************************/
            /*                      Top Machine                           */
            /**************************************************************/

            KARABO_FSM_TABLE_BEGIN(DeviceServerMachineTransitionTable)
            Row< AllOkState, ErrorFoundEvent, ErrorState, ErrorFoundAction, none >,
            Row< ErrorState, EndErrorEvent, AllOkState, EndErrorAction, none >
            KARABO_FSM_TABLE_END

            
            KARABO_FSM_STATE_MACHINE(DeviceServerMachine, DeviceServerMachineTransitionTable, AllOkState, Self)


            void startStateMachine() {

                KARABO_FSM_CREATE_MACHINE(DeviceServerMachine, m_fsm);

                KARABO_FSM_SET_CONTEXT_TOP(this, m_fsm);
                KARABO_FSM_SET_CONTEXT_SUB(this, m_fsm, AllOkState);
                        
                KARABO_FSM_START_MACHINE(m_fsm);
            }
            
            

        private: // Functions
            
            void loadLogger(const karabo::util::Hash& input);

            void loadPluginLoader(const karabo::util::Hash& input);
            
            void stopDeviceServer();

            log4cpp::Category& log();
            
            void registerAndConnectSignalsAndSlots();

            void updateAvailableDevices();

            void scanPlugins();

            void sayHello();
            
            void slotKillDeviceServerInstance();
            
            void slotKillDeviceInstance(const std::string& instanceId);
            
            std::string generateDefaultDeviceInstanceId(const std::string& classId);
            
        private: // Member variables
            
            log4cpp::Category* m_log;
            
            KARABO_FSM_DECLARE_MACHINE(DeviceServerMachine, m_fsm);

            karabo::util::PluginLoader::Pointer m_pluginLoader;
            boost::thread m_pluginThread;
            bool m_deviceServerStopped;

            bool m_isMaster;
            unsigned int m_nameRequestTimeout;
            bool m_gotName;

            karabo::util::Hash m_availableDevices;
            std::vector<karabo::util::Hash> m_autoStart;
            boost::thread_group m_deviceThreads;
            DeviceInstanceMap m_deviceInstanceMap;
            unsigned int m_deviceInstanceCount;
                        
            karabo::io::Format<karabo::util::Schema>::Pointer m_format;
            
            std::string m_devSrvInstId;
            karabo::net::BrokerConnection::Pointer m_connection;
            
            karabo::util::Hash m_connectionConfig;
            
            
        };
    } 
} 

KARABO_REGISTER_FACTORY_BASE_HH(karabo::core::DeviceServer, TEMPLATE_CORE, DECLSPEC_CORE)

#endif
