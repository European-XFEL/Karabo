/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on October 4, 2011, 7:20 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_CORE_RECONFIGURABLEFSM_HH
#define	KARABO_CORE_RECONFIGURABLEFSM_HH

#include "Device.hh"

namespace karabo {
    namespace core {

        /**
         * ReconfigurableFsm
         * 
         * The ReconfigurableFsm is one of the simplest state machines.
         * Only two states are available: AllOkState and ErrorState
         *
         * 
         * You may override none or more of the following call-backs:
         * 
         * @code
         * 
         * void onReconfigure(const karabo::util::Hash& incomingConfiguration);
         * 
         * void allOkStateOnEntry();
         * void allOkStateOnExit();            
         * void errorStateOnEntry();
         * void errorStateOnExit();
         * 
         * @endcode
         * 
         */
        class ReconfigurableFsm : public karabo::core::Device {

        public:

            KARABO_CLASSINFO(ReconfigurableFsm, "ReconfigurableFsm", "1.0")

            template <class Derived>
            ReconfigurableFsm(Derived* derived) : Device(derived) {
            }

            virtual ~ReconfigurableFsm() {
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

            /**************************************************************/
            /*                        States                              */
            /**************************************************************/

            KARABO_FSM_STATE_V_EE(AllOkState, allOkStateOnEntry, allOkStateOnExit)

            KARABO_FSM_STATE_V_EE(ErrorState, errorStateOnEntry, errorStateOnExit)

            /**************************************************************/
            /*                    Transition Actions                      */
            /**************************************************************/



            /**************************************************************/
            /*                      Top Machine                         */
            /**************************************************************/

            //  Source-State    Event        Target-State    Action         Guard
            KARABO_FSM_TABLE_BEGIN(ReconfigureMachineTransitionTable)
            Row< AllOkState, ErrorFoundEvent, ErrorState, ErrorFoundAction, none >,
            Row< ErrorState, EndErrorEvent, AllOkState, none, none >
            KARABO_FSM_TABLE_END


            //                                 Name                   Transition-Table       Initial-State Context
            KARABO_FSM_STATE_MACHINE(ReconfigureDeviceMachine, ReconfigureMachineTransitionTable, AllOkState, Self)


            void startStateMachine() {

                KARABO_FSM_CREATE_MACHINE(ReconfigureDeviceMachine, m_fsm);
                KARABO_FSM_SET_CONTEXT_TOP(this, m_fsm)
                KARABO_FSM_START_MACHINE(m_fsm)
            }


            // Override this function if you need to handle the reconfigured data (e.g. send to a hardware)

            virtual void onReconfigure(karabo::util::Hash& incomingReconfiguration) {
            }


        private: // functions

            void applyReconfiguration(const karabo::util::Hash& reconfiguration);

        private:

            KARABO_FSM_DECLARE_MACHINE(ReconfigureDeviceMachine, m_fsm);

        };

    }
}

#endif

