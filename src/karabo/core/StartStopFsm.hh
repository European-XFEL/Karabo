/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on October 4, 2011, 7:20 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_CORE_STARTSTOPFSM_HH
#define	KARABO_CORE_STARTSTOPFSM_HH

#include "Device.hh"

namespace karabo {
    namespace core {

        class StartStopFsm : public karabo::core::Device {
        public:

            KARABO_CLASSINFO(StartStopFsm, "StartStopFsm", "1.0")

            template <class Derived>
            StartStopFsm(Derived* derived) : Device(derived) {
            }

            virtual ~StartStopFsm() {
            }

            static void expectedParameters(karabo::util::Schema& expected);

            void configure(const karabo::util::Hash& input);
            
            virtual void run();

        public:

            /**************************************************************/
            /*                        Events                              */
            /**************************************************************/

            // Standard events

            KARABO_FSM_EVENT2(m_fsm, ErrorFoundEvent, onException, std::string, std::string)

            KARABO_FSM_EVENT0(m_fsm, EndErrorEvent, slotEndError)

            KARABO_FSM_EVENT0(m_fsm, StartEvent, slotStart)

            KARABO_FSM_EVENT0(m_fsm, StopEvent, slotStop)

            /**************************************************************/
            /*                        States                              */
            /**************************************************************/

            KARABO_FSM_STATE_V_EE(ErrorState, errorStateOnEntry, errorStateOnExit)
            
            KARABO_FSM_STATE_V_EE(InitializationState, initializationStateOnEntry, initializationStateOnExit)

            KARABO_FSM_STATE_V_EE(StartedState, startedStateOnEntry, startedStateOnExit)

            KARABO_FSM_STATE_V_EE(StoppedState, stoppedStateOnEntry, stoppedStateOnExit)

            /**************************************************************/
            /*                    Transition Actions                      */
            /**************************************************************/

            KARABO_FSM_V_ACTION0(StartAction, startAction)

            KARABO_FSM_V_ACTION0(StopAction, stopAction)

            /**************************************************************/
            /*                      AllOkState Machine                    */
            /**************************************************************/

            KARABO_FSM_TABLE_BEGIN(AllOkStateTransitionTable)
            //  Source-State      Event    Target-State    Action     Guard
            Row< StoppedState, StartEvent, StartedState, StartAction, none >,
            Row< StartedState, StopEvent, StoppedState, StopAction, none >
            KARABO_FSM_TABLE_END

            //                       Name      Transition-Table           Initial-State  Context
            KARABO_FSM_STATE_MACHINE(AllOkState, AllOkStateTransitionTable, StoppedState, Self)

            /**************************************************************/
            /*                      Top Machine                         */
            /**************************************************************/

            //  Source-State    Event        Target-State    Action         Guard
            KARABO_FSM_TABLE_BEGIN(StartStopMachineTransitionTable)
            Row< InitializationState, none, AllOkState, none, none >,
            Row< AllOkState, ErrorFoundEvent, ErrorState, ErrorFoundAction, none >,
            Row< ErrorState, EndErrorEvent, AllOkState, none, none >
            KARABO_FSM_TABLE_END


            //                                 Name                   Transition-Table       Initial-State Context
            KARABO_FSM_STATE_MACHINE(StartStopMachine, StartStopMachineTransitionTable, InitializationState, Self)


            void startStateMachine() {

                KARABO_FSM_CREATE_MACHINE(StartStopMachine, m_fsm);
                KARABO_FSM_SET_CONTEXT_TOP(this, m_fsm)
                KARABO_FSM_SET_CONTEXT_SUB(this, m_fsm, AllOkState)
                KARABO_FSM_START_MACHINE(m_fsm)
            }


            // Override this function if you need to handle the reconfigured data (e.g. send to a hardware)
            virtual void onReconfigure(karabo::util::Hash& incomingReconfiguration) {
            }


        private: // functions

            
        private: // members

            KARABO_FSM_DECLARE_MACHINE(StartStopMachine, m_fsm);

        };

    }
}

#endif

