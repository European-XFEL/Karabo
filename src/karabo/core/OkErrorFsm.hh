/*
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
/*
 * File:   OkErrorFsm.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on April 10, 2013, 2:39 PM
 */

#ifndef KARABO_CORE_OK_ERROR_FSM_HH
#define KARABO_CORE_OK_ERROR_FSM_HH


#include "BaseFsm.hh"
#include "karabo/core/BaseFsm.hh"
#include "karabo/util/Configurator.hh"
#include "karabo/util/State.hh"
#include "karabo/util/karaboDll.hh"
#include "karabo/xms/SignalSlotable.hh" // for SLOT_ELEMENT
#include "karabo/xms/SlotElement.hh"

namespace karabo {
    namespace util {
        class Schema;
    }

    namespace core {

        /**
         * @class OkErrorFsm
         * @brief A simple finite state machine knowing either NORMAL or ERROR States
         *
         * NORMAL (ErrorFoundEvent) -> (ErrorFoundAction) ERROR
         * ERROR (ResetEvent) -> (ResetAction) NORMAL
         */
        class OkErrorFsm : public BaseFsm {
           public:
            KARABO_CLASSINFO(OkErrorFsm, "OkErrorFsm", "0.1")


            static void expectedParameters(karabo::util::Schema& expected) {
                using namespace karabo::xms;
                using namespace karabo::util;

                SLOT_ELEMENT(expected)
                      .key("reset")
                      .displayedName("Reset")
                      .description("Resets the device in case of an error")
                      .allowedStates(State::ERROR)
                      .commit();
            }

            void initFsmSlots() {
                KARABO_SLOT(reset);
                KARABO_SLOT(errorFound, std::string, std::string)
            }

           public:
            virtual ~OkErrorFsm() {}

            /**************************************************************/
            /*                        Events                              */
            /**************************************************************/

            KARABO_FSM_EVENT2(m_fsm, ErrorFoundEvent, errorFound, std::string, std::string)

            KARABO_FSM_EVENT0(m_fsm, ResetEvent, reset)

            /**************************************************************/
            /*                        States                              */
            /**************************************************************/

            KARABO_FSM_STATE_VE_EE(NORMAL, okStateOnEntry, okStateOnExit)

            KARABO_FSM_STATE_VE_EE(ERROR, errorStateOnEntry, errorStateOnExit)

            /**************************************************************/
            /*                    Transition Actions                      */
            /**************************************************************/

            KARABO_FSM_VE_ACTION2(ErrorFoundAction, errorFoundAction, std::string, std::string);

            KARABO_FSM_VE_ACTION0(ResetAction, resetAction);

            /**************************************************************/
            /*                      Top Machine                         */
            /**************************************************************/

            //  Source-State    Event        Target-State    Action         Guard

            KARABO_FSM_TABLE_BEGIN(StateMachineTransitionTable)
            Row<NORMAL, ErrorFoundEvent, ERROR, ErrorFoundAction, none>,
                  Row<ERROR, ResetEvent, NORMAL, ResetAction, none> KARABO_FSM_TABLE_END


                  //                       Name          Transition-Table             Initial-State Context
                  KARABO_FSM_STATE_MACHINE(StateMachine, StateMachineTransitionTable, NORMAL, Self)


                        void startFsm() {
                KARABO_FSM_CREATE_MACHINE(StateMachine, m_fsm);
                KARABO_FSM_SET_CONTEXT_TOP(this, m_fsm)
                KARABO_FSM_START_MACHINE(m_fsm)
            }

           private: // members
            KARABO_FSM_DECLARE_MACHINE(StateMachine, m_fsm);
        };


    } // namespace core
} // namespace karabo
#endif
