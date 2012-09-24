/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on October 4, 2011, 7:20 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef EXFEL_CORE_CAMERAFSM_HH
#define	EXFEL_CORE_CAMERAFSM_HH

#include "Device.hh"

namespace exfel {
    namespace core {

        class CameraFsm : public exfel::core::Device {
        public:

            EXFEL_CLASSINFO(CameraFsm, "CameraFsm", "1.0")

            template <class Derived>
            CameraFsm(Derived* derived) : Device(derived) {
            }

            virtual ~CameraFsm() {
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

            EXFEL_FSM_EVENT0(m_fsm, AcquireEvent, slotAcquire)

            EXFEL_FSM_EVENT0(m_fsm, StopEvent, slotStop)

            EXFEL_FSM_EVENT0(m_fsm, TriggerEvent, slotTrigger)

            /**************************************************************/
            /*                        States                              */
            /**************************************************************/

            EXFEL_FSM_STATE_V_EE(Error, errorStateOnEntry, errorStateOnExit)

            EXFEL_FSM_STATE_V_EE(HardwareSetup, hardwareSetupStateOnEntry, hardwareSetupStateOnExit)

            EXFEL_FSM_STATE_V_EE(Acquisition, acquisitionStateOnEntry, acquisitionStateOnExit)

            EXFEL_FSM_STATE_V_EE(Ready, readyStateOnEntry, readyStateOnExit)

            /**************************************************************/
            /*                    Transition Actions                      */
            /**************************************************************/

            EXFEL_FSM_V_ACTION0(AcquireAction, acquireAction)

            EXFEL_FSM_V_ACTION0(StopAction, stopAction)

            EXFEL_FSM_V_ACTION0(TriggerAction, triggerAction)

            /**************************************************************/
            /*                      AllOkState Machine                    */
            /**************************************************************/

            EXFEL_FSM_TABLE_BEGIN(AllOkStateTransitionTable)
            //  Source-State      Event    Target-State    Action     Guard
            Row< Ready, AcquireEvent, Acquisition, AcquireAction, none >,
            Row< Acquisition, StopEvent, Ready, StopAction, none >,
            Row< Acquisition, TriggerEvent, none, TriggerAction, none >
            EXFEL_FSM_TABLE_END

            //                       Name      Transition-Table           Initial-State  Context
            EXFEL_FSM_STATE_MACHINE(AllOk, AllOkStateTransitionTable, Ready, Self)

            /**************************************************************/
            /*                      Top Machine                         */
            /**************************************************************/

            //  Source-State    Event        Target-State    Action         Guard
            EXFEL_FSM_TABLE_BEGIN(CameraMachineTransitionTable)
            Row< HardwareSetup, none, AllOk, none, none >,
            Row< AllOk, ErrorFoundEvent, Error, ErrorFoundAction, none >,
            Row< Error, EndErrorEvent, AllOk, none, none >
            EXFEL_FSM_TABLE_END


            //                                 Name                   Transition-Table       Initial-State Context
            EXFEL_FSM_STATE_MACHINE(CameraMachine, CameraMachineTransitionTable, HardwareSetup, Self)


            void startStateMachine() {

                EXFEL_FSM_CREATE_MACHINE(CameraMachine, m_fsm);
                EXFEL_FSM_SET_CONTEXT_TOP(this, m_fsm)
                EXFEL_FSM_SET_CONTEXT_SUB(this, m_fsm, AllOk)
                EXFEL_FSM_START_MACHINE(m_fsm)
            }


            // Override this function if you need to handle the reconfigured data (e.g. send to a hardware)

            virtual void onReconfigure(exfel::util::Hash& incomingReconfiguration) {
            }


        private: // functions

            
        private: // members

            EXFEL_FSM_DECLARE_MACHINE(CameraMachine, m_fsm);

        };

    }
}

#endif

