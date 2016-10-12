/*
 * File:   OkErrorFsm.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on April 10, 2013, 2:39 PM
 */

#ifndef KARABO_CORE_OK_ERROR_FSM_HH
#define	KARABO_CORE_OK_ERROR_FSM_HH


#include <karabo/util/Configurator.hh>
#include <karabo/xms/SlotElement.hh>
#include <karabo/core/BaseFsm.hh>
#include <karabo/util/karaboDll.hh>
#include <karabo/util/State.hh>

#include "BaseFsm.hh"

namespace karabo {
    namespace core {

        class OkErrorFsm : public BaseFsm {

        public:

            KARABO_CLASSINFO(OkErrorFsm, "OkErrorFsm", "0.1")


            static void expectedParameters(karabo::util::Schema& expected) {
                using namespace karabo::xms;
                using namespace karabo::util;

                SLOT_ELEMENT(expected).key("reset")
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

            virtual ~OkErrorFsm() {
            }

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
            Row< NORMAL, ErrorFoundEvent, ERROR, ErrorFoundAction, none >,
            Row< ERROR, ResetEvent, NORMAL, ResetAction, none >
            KARABO_FSM_TABLE_END


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


    }
}
#endif
