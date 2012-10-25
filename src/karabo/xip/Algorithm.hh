/*
 * $Id$
 *
 * File:   Algorithm.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on November 18, 2011, 3:08 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_XIP_ALGORITHM_HH
#define	KARABO_XIP_ALGORITHM_HH

#include <karabo/util/Factory.hh>
#include <karabo/core/Device.hh>
#include <karabo/xms/SlotElement.hh>

//#include "Output.hh"
//#include "Input.hh"

namespace karabo {

    namespace xip {

        class Algorithm : public karabo::core::Device {
            
            typedef std::map<std::string, AbstractInput::Pointer> InputChannels;
            typedef std::map<std::string, AbstractOutput::Pointer> OutputChannels;
            
        public:

            KARABO_CLASSINFO(Algorithm, "Algorithm", "1.0")
            KARABO_FACTORY_BASE_CLASS


            template <class Derived>
            Algorithm(Derived* derived) : Device(derived) {
            }

            virtual ~Algorithm() {
            }

            /**
             * Necessary method as part of the factory/configuration system
             * @param expected [out] Description of expected parameters for this object (Schema)
             */
            static void expectedParameters(karabo::util::Schema& expected);
            

            /**
             * If this object is constructed using the factory/configuration system this method is called
             * @param input Validated (@see expectedParameters) and default-filled configuration
             */
            void configure(const karabo::util::Hash & input);
            
           
            
            
             /**************************************************************/
            /*                        Events                              */
            /**************************************************************/

            KARABO_FSM_EVENT2(m_fsm, ErrorFoundEvent, onException, std::string, std::string)

            KARABO_FSM_EVENT0(m_fsm, EndErrorEvent, slotEndError)

            KARABO_FSM_EVENT0(m_fsm, ComputeEvent, slotCompute)

            KARABO_FSM_EVENT0(m_fsm, StartRunEvent, slotStartRun)
            
            KARABO_FSM_EVENT0(m_fsm, EndOfStreamEvent, slotEndOfStream)
            
            KARABO_FSM_EVENT0(m_fsm, PauseEvent, slotPause)
            
            KARABO_FSM_EVENT0(m_fsm, AbortEvent, slotAbort)
            
            KARABO_FSM_EVENT0(m_fsm, IOEvent, slotIOEvent)
            
            KARABO_FSM_EVENT0(m_fsm, ComputeFinishedEvent, computeFinished)
            
            KARABO_FSM_EVENT0(m_fsm, ResetEvent, slotReset)

            /**************************************************************/
            /*                        States                              */
            /**************************************************************/
            
            KARABO_FSM_STATE(Idle)

            KARABO_FSM_STATE(WaitingIO)

            KARABO_FSM_STATE(Ready)

            KARABO_FSM_STATE_V_EE(Computing, computingStateOnEntry, computingStateOnExit)
            
            KARABO_FSM_STATE(Paused)
            
            KARABO_FSM_STATE(Finished)
            
            KARABO_FSM_STATE(Aborted)
            
            KARABO_FSM_STATE(Ok)
            
            KARABO_FSM_STATE(Error)

            /**************************************************************/
            /*                    Transition Actions                      */
            /**************************************************************/

            KARABO_FSM_V_ACTION0(StartRunAction, onStartRun)
            
            /**************************************************************/
            /*                           Guards                           */
            /**************************************************************/
            
            KARABO_FSM_V_GUARD0(CanCompute, canCompute)

            /**************************************************************/
            /*                      AllOkState Machine                    */
            /**************************************************************/

            KARABO_FSM_TABLE_BEGIN(TransitionTable)
            //  Source-State      Event    Target-State    Action     Guard
            Row< Idle, StartRunEvent, WaitingIO, StartRunAction, none >,
            //Row< WaitingIO, GoIdleEvent, Idle, none, none >,
            Row< WaitingIO, none, Ready, none, CanCompute >,
            Row< WaitingIO, IOEvent, Ready, none, CanCompute >,
            Row< WaitingIO, PauseEvent, Paused, none, none >,
            Row< WaitingIO, AbortEvent, Aborted, none, none >,
            Row< WaitingIO, EndOfStreamEvent, Finished, none, none >,
            Row< Ready, ComputeEvent, Computing, none, none >,
            Row< Ready, PauseEvent, Paused, none, none >,
            Row< Ready, AbortEvent, Aborted, none, none >,
            Row< Computing, ComputeFinishedEvent, WaitingIO, none, none >,
            Row< Aborted, ResetEvent, Idle, none, none >,
            Row< Finished, ResetEvent, Idle, none, none >,
            Row< Ok, ErrorFoundEvent, Error, ErrorFoundAction, none >,
            Row< Error, ResetEvent, Ok, none, none >
            KARABO_FSM_TABLE_END
 
            //                         Name       Transition-Table      Initial-State         Context
            KARABO_FSM_STATE_MACHINE(StateMachine, TransitionTable, KARABO_FSM_REGION(Ok, Idle), Self)

            void startStateMachine() {

                KARABO_FSM_CREATE_MACHINE(StateMachine, m_fsm);
                KARABO_FSM_SET_CONTEXT_TOP(this, m_fsm)
                KARABO_FSM_START_MACHINE(m_fsm)
            }
            
             void run() {
                 startStateMachine();
                 runEventLoop(); // This will block
             }

             virtual void compute() = 0;
            
           
            
        private: //functions
            
            void connectDeviceInputs();
            
            void slotGetOutputChannelInformation(const std::string& ioChannelId, const std::string& senderInstanceId);
            
            void slotInputChannelCanRead(const std::string& ioChannelId, const std::string& senderInstanceId);
            
            void doCompute();
            
            void notifyOutputChannelsForPossibleRead();
            
            void notifyOutputChannelForPossibleRead(const AbstractInput::Pointer& channel);
            
            void canReadEvent(const AbstractInput::Pointer& channel) {
                notifyOutputChannelForPossibleRead(channel);
            }
            
            

           

        private: // members
            
            KARABO_FSM_DECLARE_MACHINE(StateMachine, m_fsm);

            InputChannels m_inputChannels;
            OutputChannels m_outputChannels;
            
            boost::thread m_computeThread;

        };

    }
}



#endif
