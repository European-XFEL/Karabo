/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on October 4, 2011, 7:20 PM
 *
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */


#ifndef KARABO_CORE_START_STOP_FSM_HH
#define KARABO_CORE_START_STOP_FSM_HH

#include <karabo/core/BaseFsm.hh>
#include <karabo/util/State.hh>
#include <karabo/xms/SlotElement.hh>

#include "Device.hh"

namespace karabo {
    namespace util {
        class Schema;
    }
    namespace core {

        /**
         * @class StartStopFsm
         * @brief A finite state machine designed to be used binary state (start - stop) devices
         *
         * A finite state machine designed to be used for binary state (start - stop)  devices. It uses
         * an ERROR-NORMAL top state machine (karabo::core::OkErrorFsm type).
         * In the NORMAL region the following state transition table is used:
         *
         * STOPPED (StartEvent) -> (StartAction) STARTED
         * STARTED (StopEvent) -> (StopAction) STOPPED
         */
        class StartStopFsm : public BaseFsm {
           public:
            KARABO_CLASSINFO(StartStopFsm, "StartStopFsm", "1.0")


            static void expectedParameters(karabo::util::Schema& expected) {
                using namespace karabo::xms;
                using namespace karabo::util;

                SLOT_ELEMENT(expected)
                      .key("start")
                      .displayedName("Start")
                      .description("Instructs device to go to started state")
                      .allowedStates(State::STOPPED)
                      .commit();

                SLOT_ELEMENT(expected)
                      .key("stop")
                      .displayedName("Stop")
                      .description("Instructs device to go to stopped state")
                      .allowedStates(State::STARTED)
                      .commit();


                SLOT_ELEMENT(expected)
                      .key("reset")
                      .displayedName("Reset")
                      .description("Resets the device in case of an error")
                      .allowedStates(State::ERROR)
                      .commit();
            }

            void initFsmSlots() {
                KARABO_SLOT(start);
                KARABO_SLOT(stop);
                KARABO_SLOT(reset);
                KARABO_SLOT(errorFound, std::string, std::string);
            }

           public:
            virtual ~StartStopFsm() {}

            /**************************************************************/
            /*                        Events                              */
            /**************************************************************/

            KARABO_FSM_EVENT2(m_fsm, ErrorFoundEvent, errorFound, std::string, std::string)

            KARABO_FSM_EVENT0(m_fsm, ResetEvent, reset)

            KARABO_FSM_EVENT0(m_fsm, StartEvent, start)

            KARABO_FSM_EVENT0(m_fsm, StopEvent, stop)

            /**************************************************************/
            /*                        States                              */
            /**************************************************************/

            KARABO_FSM_STATE_VE_EE(ERROR, errorStateOnEntry, errorStateOnExit)

            KARABO_FSM_STATE_VE_EE(INIT, initializationStateOnEntry, initializationStateOnExit)

            KARABO_FSM_STATE_VE_EE(STARTED, startedStateOnEntry, startedStateOnExit)

            KARABO_FSM_STATE_VE_EE(STOPPED, stoppedStateOnEntry, stoppedStateOnExit)

            /**************************************************************/
            /*                    Transition Actions                      */
            /**************************************************************/

            KARABO_FSM_VE_ACTION2(ErrorFoundAction, errorFoundAction, std::string, std::string);

            KARABO_FSM_VE_ACTION0(ResetAction, resetAction)

            KARABO_FSM_VE_ACTION0(StartAction, startAction)

            KARABO_FSM_VE_ACTION0(StopAction, stopAction)


            /**************************************************************/
            /*                      AllOkState Machine                    */
            /**************************************************************/

            KARABO_FSM_TABLE_BEGIN(OkStateTransitionTable)
            // Source-State, Event, Target-State, Action, Guard
            Row<STOPPED, StartEvent, STARTED, StartAction, none>,
                  Row<STARTED, StopEvent, STOPPED, StopAction, none> KARABO_FSM_TABLE_END

                  //                       Name    Transition-Table  Initial-State  Context
                  KARABO_FSM_STATE_MACHINE(NORMAL, OkStateTransitionTable, STOPPED, Self)

                  /**************************************************************/
                  /*                      Top Machine                         */
                  /**************************************************************/

                  // Source-State, Event, Target-State, Action, Guard
                  KARABO_FSM_TABLE_BEGIN(TransitionTable) Row<INIT, none, NORMAL, none, none>,
                  Row<NORMAL, ErrorFoundEvent, ERROR, ErrorFoundAction, none>,
                  Row<ERROR, ResetEvent, NORMAL, ResetAction, none> KARABO_FSM_TABLE_END


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
    } // namespace core
} // namespace karabo

#endif
