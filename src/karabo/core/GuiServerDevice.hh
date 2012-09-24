/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef EXFEL_CORE_GUISERVERDEVICE_HH
#define	EXFEL_CORE_GUISERVERDEVICE_HH

#include <karabo/net/Connection.hh>
#include <karabo/net/Channel.hh>
#include <karabo/net/BrokerConnection.hh>
#include <karabo/net/BrokerChannel.hh>

#include "Device.hh"
/**
 * The main European XFEL namespace
 */
namespace exfel {

    /**
     * Namespace for package sink
     */
    namespace core {

        class GuiServerDevice : public exfel::core::Device {
        public:

            EXFEL_CLASSINFO(GuiServerDevice, "GuiServerDevice", "1.0")

            /**
             * Constructor explicitly calling base Device constructor
             */
            GuiServerDevice() : Device(this) {
            }

            virtual ~GuiServerDevice() {
            }

            /**
             * Necessary method as part of the factory/configuration system
             * @param expected Will contain a description of expected parameters for this device
             */
            static void expectedParameters(exfel::util::Schema& expected);

            /**
             * If this object is constructed using the factory/configuration system this method is called
             * upon construction (can be regarded as a second constructor)
             * @param input Validated (@see expectedParameters) and default-filled configuration
             */
            void configure(const exfel::util::Hash& input);

            /**
             * Typically starts the state machine and the signals and slots event loop
             */
            void run();

        public:

            /**************************************************************/
            /*                        Events                              */
            /**************************************************************/

            // Standard events

            EXFEL_FSM_EVENT2(m_fsm, ErrorFoundEvent, onException, std::string, std::string)

            EXFEL_FSM_EVENT0(m_fsm, EndErrorEvent, slotEndError)


            /**************************************************************/
            /*                        States                              */

            /**************************************************************/

            EXFEL_FSM_STATE(AllOkState)

            EXFEL_FSM_STATE(ErrorState)


            /**************************************************************/
            /*                    Transition Actions                      */
            /**************************************************************/



            /**************************************************************/
            /*                      Top Machine                           */
            /**************************************************************/

            EXFEL_FSM_TABLE_BEGIN(GuiServerDeviceMachineTransitionTable)
            //  Source-State    Event        Target-State    Action         Guard
            Row< AllOkState, ErrorFoundEvent, ErrorState, ErrorFoundAction, none >,
            Row< ErrorState, EndErrorEvent, AllOkState, none, none >
            EXFEL_FSM_TABLE_END


            //                                 Name                Transition-Table              Initial-State Context Entry-function
            EXFEL_FSM_STATE_MACHINE_V_E(GuiServerDeviceMachine, GuiServerDeviceMachineTransitionTable, AllOkState, Self, startServer)

            void startStateMachine() {

                EXFEL_FSM_CREATE_MACHINE(GuiServerDeviceMachine, m_fsm);
                EXFEL_FSM_SET_CONTEXT_TOP(this, m_fsm)
                EXFEL_FSM_START_MACHINE(m_fsm)
            }

        private: // Members

            EXFEL_FSM_DECLARE_MACHINE(GuiServerDeviceMachine, m_fsm);

            exfel::net::IOService::Pointer m_ioService;
            exfel::net::Connection::Pointer m_dataConnection;
            
            exfel::io::Format<exfel::util::Hash>::Pointer m_xmlSerializer;
            exfel::io::Format<exfel::util::Hash>::Pointer m_binarySerializer;
            std::map<exfel::net::Channel::Pointer, std::set<std::string> > m_channels;
            boost::mutex m_channelMutex;
            
            exfel::net::BrokerConnection::Pointer m_loggerConnection;
            exfel::net::BrokerIOService::Pointer m_loggerIoService;
            exfel::net::BrokerChannel::Pointer m_loggerChannel;
            
            

        private: // Functions

            void onError(exfel::net::Channel::Pointer channel, const std::string& errorMessage);

            void onConnect(exfel::net::Channel::Pointer channel);

            void onRead(exfel::net::Channel::Pointer channel, const std::string& body, const exfel::util::Hash& header);
            
            void onLogin(exfel::net::Channel::Pointer channel, const std::string& body);
            
            void onRefreshInstance(exfel::net::Channel::Pointer channel, const exfel::util::Hash& header);
            
            void onReconfigure(const exfel::util::Hash& header, const std::string& body);
            
            void onInitDevice(const exfel::util::Hash& header, const std::string& body);
            
            void onKillDeviceServerInstance(const exfel::util::Hash& header, const std::string& body);
            
            void onKillDeviceInstance(const exfel::util::Hash& header, const std::string& body);
            
            void onSlotCommand(const exfel::util::Hash& header, const std::string& body);
            
            void onNewVisibleDeviceInstance(exfel::net::Channel::Pointer channel, const exfel::util::Hash& header);
            
            void onRemoveVisibleDeviceInstance(exfel::net::Channel::Pointer channel, const exfel::util::Hash& header);

            void sendCurrentIds(exfel::net::Channel::Pointer channel);

            void registerConnect(const exfel::net::Channel::Pointer& channel);

            void slotNewNode(const exfel::util::Hash& row);
            
            void slotNewDeviceServerInstance(const exfel::util::Hash& row);

            void slotNewDeviceClass(const exfel::util::Hash& row);

            void slotNewDeviceInstance(const exfel::util::Hash& row);
            
            void slotUpdateDeviceServerInstance(const exfel::util::Hash& row);
            
            void slotUpdateDeviceInstance(const exfel::util::Hash& row);

//            void slotNoTransition(const std::string& what, const std::string& who);
//
//            void slotBadReconfiguration(const std::string& what, const std::string& who);
//
//            void slotConnected(const std::string& signal, const std::string& slot);
//
            void slotChanged(const exfel::util::Hash& what, const std::string& instanceId, const std::string& classId);
//

            void onLog(exfel::net::BrokerChannel::Pointer channel, const std::string& logMessage, const exfel::util::Hash& header);
            
            void slotErrorFound(const std::string& timeStamp, const std::string& shortMessage, const std::string& detailedMessage, const std::string& instanceId);
            
            void slotWarning(const std::string& timeStamp, const std::string& warnMessage, const std::string& instanceId, const std::string& priority);
            
            void slotAlarm(const std::string& timeStamp, const std::string& alarmMessage, const std::string& instanceId, const std::string& priority);
            
            void slotSchemaUpdatedToGui(const std::string& schema, const std::string& instanceId, const std::string& classId);

        };

    }
}

#endif	/* EXFEL_GUISERVER_GUISERVERDEVICE_HH */
