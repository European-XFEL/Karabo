/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on October 4, 2011, 7:20 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef EXFEL_CORE_STARTSTOPFSM_HH
#define	EXFEL_CORE_STARTSTOPFSM_HH

#include "Device.hh"

namespace exfel {
    namespace core {

        class StartStopFsm : public exfel::core::Device {
        public:

            EXFEL_CLASSINFO(StartStopFsm, "StartStopFsm", "1.0")

            template <class Derived>
            StartStopFsm(Derived* derived) : Device(derived) {
            }

            virtual ~StartStopFsm() {
            }

            static void expectedParameters(exfel::util::Schema& expected);

            void configure(const exfel::util::Hash& input);
            
            virtual void run();

        public:

            /**************************************************************/
            /*                        Events                              */
            /**************************************************************/

            // Standard events

            EXFEL_FSM_EVENT2(m_fsm, ErrorFoundEvent, onException, std::string, std::string)

            EXFEL_FSM_EVENT0(m_fsm, EndErrorEvent, slotEndError)

            EXFEL_FSM_EVENT0(m_fsm, StartEvent, slotStart)

            EXFEL_FSM_EVENT0(m_fsm, StopEvent, slotStop)

            /**************************************************************/
            /*                        States                              */
            /**************************************************************/

            EXFEL_FSM_STATE_V_EE(ErrorState, errorStateOnEntry, errorStateOnExit)
            
            EXFEL_FSM_STATE_V_EE(InitializationState, initializationStateOnEntry, initializationStateOnExit)

            EXFEL_FSM_STATE_V_EE(StartedState, startedStateOnEntry, startedStateOnExit)

            EXFEL_FSM_STATE_V_EE(StoppedState, stoppedStateOnEntry, stoppedStateOnExit)

            /**************************************************************/
            /*                    Transition Actions                      */
            /**************************************************************/

            EXFEL_FSM_V_ACTION0(StartAction, startAction)

            EXFEL_FSM_V_ACTION0(StopAction, stopAction)

            /**************************************************************/
            /*                      AllOkState Machine                    */
            /**************************************************************/

            EXFEL_FSM_TABLE_BEGIN(AllOkStateTransitionTable)
            //  Source-State      Event    Target-State    Action     Guard
            Row< StoppedState, StartEvent, StartedState, StartAction, none >,
            Row< StartedState, StopEvent, StoppedState, StopAction, none >
            EXFEL_FSM_TABLE_END

            //                       Name      Transition-Table           Initial-State  Context
            EXFEL_FSM_STATE_MACHINE(AllOkState, AllOkStateTransitionTable, StoppedState, Self)

            /**************************************************************/
            /*                      Top Machine                         */
            /**************************************************************/

            //  Source-State    Event        Target-State    Action         Guard
            EXFEL_FSM_TABLE_BEGIN(StartStopMachineTransitionTable)
            Row< InitializationState, none, AllOkState, none, none >,
            Row< AllOkState, ErrorFoundEvent, ErrorState, ErrorFoundAction, none >,
            Row< ErrorState, EndErrorEvent, AllOkState, none, none >
            EXFEL_FSM_TABLE_END


            //                                 Name                   Transition-Table       Initial-State Context
            EXFEL_FSM_STATE_MACHINE(StartStopMachine, StartStopMachineTransitionTable, InitializationState, Self)


            void startStateMachine() {

                EXFEL_FSM_CREATE_MACHINE(StartStopMachine, m_fsm);
                EXFEL_FSM_SET_CONTEXT_TOP(this, m_fsm)
                EXFEL_FSM_SET_CONTEXT_SUB(this, m_fsm, AllOkState)
                EXFEL_FSM_START_MACHINE(m_fsm)
            }


            // Override this function if you need to handle the reconfigured data (e.g. send to a hardware)
            virtual void onReconfigure(exfel::util::Hash& incomingReconfiguration) {
            }


        private: // functions

            
        private: // members

            EXFEL_FSM_DECLARE_MACHINE(StartStopMachine, m_fsm);

        };

    }
}

#endif

