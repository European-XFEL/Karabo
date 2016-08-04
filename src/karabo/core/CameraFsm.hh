/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on October 4, 2011, 7:20 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_CORE_CAMERAFSM_HH
#define	KARABO_CORE_CAMERAFSM_HH

#include <karabo/core/BaseFsm.hh>
#include "Device.hh"
#include <karabo/util/State.hh>

namespace karabo {
    namespace core {

        class CameraFsm : public BaseFsm {

        public:

            KARABO_CLASSINFO(CameraFsm, "CameraFsm", "1.0")

            static void expectedParameters(karabo::util::Schema& expected) {
                using namespace karabo::xms;
                using namespace karabo::util;


                SLOT_ELEMENT(expected).key("acquire")
                        .displayedName("Acquire")
                        .description("Instructs camera to go into acquisition state")
                        .allowedStates(State::STOPPED)
                        .commit();

                SLOT_ELEMENT(expected).key("trigger")
                        .displayedName("Trigger")
                        .description("Sends a software trigger to the camera")
                        .allowedStates(State::STARTED)
                        .commit();

                SLOT_ELEMENT(expected).key("stop")
                        .displayedName("Stop")
                        .description("Instructs camera to stop current acquisition")
                        .allowedStates(State::STARTED)
                        .commit();

                SLOT_ELEMENT(expected).key("reset")
                        .displayedName("Reset")
                        .description("Resets the camera in case of an error")
                        .allowedStates(State::ERROR)
                        .commit();
            }

            void initFsmSlots() {
                SLOT0(acquire);
                SLOT0(trigger);
                SLOT0(stop);
                SLOT0(reset);
                SLOT2(errorFound, std::string, std::string);
            }


        public:

            virtual ~CameraFsm() {
            }

            /**************************************************************/
            /*                        Events                              */
            /**************************************************************/

            KARABO_FSM_EVENT2(m_fsm, ErrorFoundEvent, errorFound, std::string, std::string)

            KARABO_FSM_EVENT0(m_fsm, ResetEvent, reset)

            KARABO_FSM_EVENT0(m_fsm, AcquireEvent, acquire)

            KARABO_FSM_EVENT0(m_fsm, StopEvent, stop)

            KARABO_FSM_EVENT0(m_fsm, TriggerEvent, trigger)

            /**************************************************************/
            /*                        States                              */
            /**************************************************************/

            KARABO_FSM_STATE_VE_EE(ERROR, errorStateOnEntry, errorStateOnExit)

            KARABO_FSM_STATE_VE_EE(INIT, initializationStateOnEntry, initializationStateOnExit)

            KARABO_FSM_STATE_VE_EE(STARTED, acquisitionStateOnEntry, acquisitionStateOnExit)

            KARABO_FSM_STATE_VE_EE(STOPPED, readyStateOnEntry, readyStateOnExit)

            /**************************************************************/
            /*                    Transition Actions                      */
            /**************************************************************/

            KARABO_FSM_VE_ACTION2(ErrorFoundAction, errorFoundAction, std::string, std::string);

            KARABO_FSM_VE_ACTION0(ResetAction, resetAction)

            KARABO_FSM_VE_ACTION0(AcquireAction, acquireAction)

            KARABO_FSM_VE_ACTION0(StopAction, stopAction)

            KARABO_FSM_VE_ACTION0(TriggerAction, triggerAction)

            /**************************************************************/
            /*                      OkState Machine                    */
            /**************************************************************/

            KARABO_FSM_TABLE_BEGIN(OkStateTransitionTable)
            // Source-State, Event, Target-State, Action, Guard
            Row< STOPPED, AcquireEvent, STARTED, AcquireAction, none >,
            Row< STARTED, StopEvent, STOPPED, StopAction, none >,
            Row< STARTED, TriggerEvent, none, TriggerAction, none >
            KARABO_FSM_TABLE_END

            // Name, Transition-Table, Initial-State, Context
            KARABO_FSM_STATE_MACHINE(NORMAL, OkStateTransitionTable, STOPPED, Self)

            /**************************************************************/
            /*                      Top Machine                         */
            /**************************************************************/

            // Source-State, Event, Target-State, Action, Guard
            KARABO_FSM_TABLE_BEGIN(TransitionTable)
            Row< INIT, none, NORMAL, none, none >,
            Row< NORMAL, ErrorFoundEvent, ERROR, ErrorFoundAction, none >,
            Row< ERROR, ResetEvent, NORMAL, ResetAction, none >
            KARABO_FSM_TABLE_END


            // Name, Transition-Table, Initial-State, Context
            KARABO_FSM_STATE_MACHINE(StateMachine, TransitionTable, INIT, Self)


            void startFsm() {

                KARABO_FSM_CREATE_MACHINE(StateMachine, m_fsm);
                KARABO_FSM_SET_CONTEXT_TOP(this, m_fsm)
                KARABO_FSM_SET_CONTEXT_SUB(this, m_fsm, NORMAL)
                KARABO_FSM_START_MACHINE(m_fsm)
            }

        private:

            KARABO_FSM_DECLARE_MACHINE(StateMachine, m_fsm);

        };
    }
}

#endif

