/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on October 4, 2011, 7:20 PM
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */


#ifndef KARABO_CORE_CAMERAFSM_HH
#define KARABO_CORE_CAMERAFSM_HH

#include "Device.hh"
#include "karabo/core/BaseFsm.hh"
#include "karabo/util/State.hh"
#include "karabo/util/VectorElement.hh"
#include "karabo/xms/SlotElement.hh"

namespace karabo {
    namespace util {
        class Schema;
    }
    namespace core {

        /**
         * @class CameraFsm
         * @brief A finite state machine designed to be used for camera-type devices
         *
         * A finite state machine designed to be used for camera-type devices. It uses
         * an ERROR-NORMAL state machine (karabo::core::OkErrorFsm type).
         * In the NORMAL region the following state transition table is used:
         *
         * ON (AcquireEvent) -> (AcquireAction) ACQUIRING
         * ACQUIRING (StopEvent) -> (StopAction) ON
         * ACQUIRING (TriggerEvent) -> (TriggerAction) ACQUIRING
         */
        class CameraFsm : public BaseFsm {
           public:
            KARABO_CLASSINFO(CameraFsm, "CameraFsm", "1.0")

            static void expectedParameters(karabo::util::Schema& expected) {
                using namespace karabo::xms;
                using namespace karabo::util;

                SLOT_ELEMENT(expected)
                      .key("connectCamera")
                      .displayedName("Connect")
                      .description("Connects to the hardware")
                      .allowedStates(State::UNKNOWN)
                      .commit();

                SLOT_ELEMENT(expected)
                      .key("acquire")
                      .displayedName("Acquire")
                      .description("Instructs camera to go into acquisition state")
                      .allowedStates(State::ON)
                      .commit();

                SLOT_ELEMENT(expected)
                      .key("trigger")
                      .displayedName("Trigger")
                      .description("Sends a software trigger to the camera")
                      .allowedStates(State::ACQUIRING)
                      .commit();

                SLOT_ELEMENT(expected)
                      .key("stop")
                      .displayedName("Stop")
                      .description("Instructs camera to stop current acquisition")
                      .allowedStates(State::ACQUIRING)
                      .commit();

                SLOT_ELEMENT(expected)
                      .key("reset")
                      .displayedName("Reset")
                      .description("Resets the camera in case of an error")
                      .allowedStates(State::ERROR)
                      .commit();

                VECTOR_STRING_ELEMENT(expected)
                      .key("interfaces")
                      .displayedName("Interfaces")
                      .description("Describes the interfaces of this device")
                      .readOnly()
                      .initialValue({"Camera"})
                      .commit();
            }

            void initFsmSlots() {
                KARABO_SLOT(connectCamera);
                KARABO_SLOT(acquire);
                KARABO_SLOT(trigger);
                KARABO_SLOT(stop);
                KARABO_SLOT(reset);
                KARABO_SLOT(errorFound, std::string, std::string);
            }


           public:
            virtual ~CameraFsm() {}

            /**************************************************************/
            /*                        Events                              */
            /**************************************************************/

            KARABO_FSM_EVENT2(m_fsm, ErrorFoundEvent, errorFound, std::string, std::string)

            KARABO_FSM_EVENT0(m_fsm, ConnectEvent, connectCamera)

            KARABO_FSM_EVENT0(m_fsm, DisconnectEvent, disconnectCamera)

            KARABO_FSM_EVENT0(m_fsm, ResetEvent, reset)

            KARABO_FSM_EVENT0(m_fsm, AcquireEvent, acquire)

            KARABO_FSM_EVENT0(m_fsm, StopEvent, stop)

            KARABO_FSM_EVENT0(m_fsm, TriggerEvent, trigger)

            /**************************************************************/
            /*                        States                              */
            /**************************************************************/

            KARABO_FSM_STATE_VE_EE(ERROR, errorStateOnEntry, errorStateOnExit)

            KARABO_FSM_STATE_VE_EE(INIT, initializationStateOnEntry, initializationStateOnExit)

            KARABO_FSM_STATE_VE_EE(UNKNOWN, unknownStateOnEntry, unknownStateOnExit)

            KARABO_FSM_STATE_VE_EE(ACQUIRING, acquisitionStateOnEntry, acquisitionStateOnExit)

            KARABO_FSM_STATE_VE_EE(ON, readyStateOnEntry, readyStateOnExit)

            /**************************************************************/
            /*                    Transition Actions                      */
            /**************************************************************/

            KARABO_FSM_VE_ACTION2(ErrorFoundAction, errorFoundAction, std::string, std::string);

            KARABO_FSM_VE_ACTION0(ResetAction, resetAction)

            KARABO_FSM_VE_ACTION0(ConnectAction, connectAction)

            KARABO_FSM_VE_ACTION0(DisconnectAction, disconnectAction)

            KARABO_FSM_VE_ACTION0(AcquireAction, acquireAction)

            KARABO_FSM_VE_ACTION0(StopAction, stopAction)

            KARABO_FSM_VE_ACTION0(TriggerAction, triggerAction)

            KARABO_FSM_VE_GUARD0(ConnectGuard, connectGuard)

            /**************************************************************/
            /*                       OkState Machine                      */
            /**************************************************************/

            KARABO_FSM_TABLE_BEGIN(OkStateTransitionTable)
            // Source-State, Event, Target-State, Action, Guard
            Row<ON, AcquireEvent, ACQUIRING, AcquireAction, none>, Row<ACQUIRING, StopEvent, ON, StopAction, none>,
                  Row<ACQUIRING, TriggerEvent, none, TriggerAction, none> KARABO_FSM_TABLE_END

                  // Name, Transition-Table, Initial-State, Context
                  KARABO_FSM_STATE_MACHINE(NORMAL, OkStateTransitionTable, ON, Self)

                  /**************************************************************/
                  /*                     KnownState Machine                     */
                  /**************************************************************/

                  // Source-State, Event, Target-State, Action, Guard
                  KARABO_FSM_TABLE_BEGIN(
                        KnownTransitionTable) Row<NORMAL, ErrorFoundEvent, ERROR, ErrorFoundAction, none>,
                  Row<ERROR, ResetEvent, NORMAL, ResetAction, none> KARABO_FSM_TABLE_END

                  // Name, Transition-Table, Initial-State, Context
                  KARABO_FSM_STATE_MACHINE(KNOWN, KnownTransitionTable, NORMAL, Self)

                  /**************************************************************/
                  /*                      Top Machine                         */
                  /**************************************************************/

                  // Source-State, Event, Target-State, Action, Guard
                  KARABO_FSM_TABLE_BEGIN(TransitionTable) Row<INIT, none, UNKNOWN, none, none>,
                  Row<UNKNOWN, ConnectEvent, KNOWN, ConnectAction, ConnectGuard>,
                  Row<KNOWN, DisconnectEvent, UNKNOWN, DisconnectAction, none> KARABO_FSM_TABLE_END


                  // Name, Transition-Table, Initial-State, Context
                  KARABO_FSM_STATE_MACHINE(StateMachine, TransitionTable, INIT, Self)


                        void startFsm() {
                KARABO_FSM_CREATE_MACHINE(StateMachine, m_fsm)
                KARABO_FSM_SET_CONTEXT_TOP(this, m_fsm)
                KARABO_FSM_SET_CONTEXT_SUB1(this, m_fsm, KNOWN)
                KARABO_FSM_SET_CONTEXT_SUB2(this, m_fsm, KNOWN, NORMAL)
                KARABO_FSM_START_MACHINE(m_fsm)
            }

           private:
            KARABO_FSM_DECLARE_MACHINE(StateMachine, m_fsm);
        };
    } // namespace core
} // namespace karabo

#endif
