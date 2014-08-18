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

#include "Device.hh"

namespace karabo {
    namespace core {

        class CameraFsm : public BaseFsm {
                       
        public:

            KARABO_CLASSINFO(CameraFsm, "CameraFsm", "1.0")

            static void expectedParameters(karabo::util::Schema& expected) {
                using namespace karabo::xms;


                SLOT_ELEMENT(expected).key("acquire")
                        .displayedName("Acquire")
                        .description("Instructs camera to go into acquisition state")
                        .allowedStates("Ok.Ready")
                        .commit();

                SLOT_ELEMENT(expected).key("trigger")
                        .displayedName("Trigger")
                        .description("Sends a software trigger to the camera")
                        .allowedStates("Ok.Acquisition")
                        .commit();

                SLOT_ELEMENT(expected).key("stop")
                        .displayedName("Stop")
                        .description("Instructs camera to stop current acquisition")
                        .allowedStates("Ok.Acquisition")
                        .commit();

                SLOT_ELEMENT(expected).key("reset")
                        .displayedName("Reset")
                        .description("Resets the camera in case of an error")
                        .allowedStates("Error")
                        .commit();
            }

            void initFsmSlots() {
                SLOT0(acquire);
                SLOT0(trigger);
                SLOT0(stop);
                SLOT0(reset);
            }


        public:

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

            KARABO_FSM_STATE_VE_EE(Error, errorStateOnEntry, errorStateOnExit)

            KARABO_FSM_STATE_VE_EE(Initialization, initializationStateOnEntry, initializationStateOnExit)

            KARABO_FSM_STATE_VE_EE(Acquisition, acquisitionStateOnEntry, acquisitionStateOnExit)

            KARABO_FSM_STATE_VE_EE(Ready, readyStateOnEntry, readyStateOnExit)

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
            Row< Ready, AcquireEvent, Acquisition, AcquireAction, none >,
            Row< Acquisition, StopEvent, Ready, StopAction, none >,
            Row< Acquisition, TriggerEvent, none, TriggerAction, none >
            KARABO_FSM_TABLE_END

            // Name, Transition-Table, Initial-State, Context
            KARABO_FSM_STATE_MACHINE(Ok, OkStateTransitionTable, Ready, Self)

            /**************************************************************/
            /*                      Top Machine                         */
            /**************************************************************/

            // Source-State, Event, Target-State, Action, Guard
            KARABO_FSM_TABLE_BEGIN(TransitionTable)
            Row< Initialization, none, Ok, none, none >,
            Row< Ok, ErrorFoundEvent, Error, ErrorFoundAction, none >,
            Row< Error, ResetEvent, Ok, ResetAction, none >
            KARABO_FSM_TABLE_END


            // Name, Transition-Table, Initial-State, Context
            KARABO_FSM_STATE_MACHINE(StateMachine, TransitionTable, Initialization, Self)


            void startFsm() {

                KARABO_FSM_CREATE_MACHINE(StateMachine, m_fsm);
                KARABO_FSM_SET_CONTEXT_TOP(this, m_fsm)
                KARABO_FSM_SET_CONTEXT_SUB(this, m_fsm, Ok)
                KARABO_FSM_START_MACHINE(m_fsm)
            }
            
        private:
            
            KARABO_FSM_DECLARE_MACHINE(StateMachine, m_fsm);
            
        };
    }
}

#endif

