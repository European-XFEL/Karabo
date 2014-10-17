/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on December 22, 2011, 09:19 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <iostream>
#include <cassert>
#include <string>
#include <krb_log4cpp/Category.hh>

#include <exfel/util/Factory.hh>
#include <exfel/util/Test.hh>

#include "../FsmMacros.hh"

using namespace std;
using namespace exfel::util;

namespace exfel {
    namespace core {

        class StateMachineTest {
        public:

            StateMachineTest() : m_log(getLog<StateMachineTest>()), m_log2(std::cout) {
            }

            krb_log4cpp::Category& log() {
                return m_log;
            }
            
            std::ostream& log2() {
                return m_log2;
            }
           
            EXFEL_CLASSINFO(StateMachineTest, "StateMachineTest", "1.0")

            /**************************************************************/
            /*                 Special Functions                          */
            /**************************************************************/
           
            EXFEL_FSM_LOGGER(log, krb_log4cpp::CategoryStream, krb_log4cpp::Priority::DEBUG)
                    
            //EXFEL_FSM_LOGGER(log2, std::ostream&, "\nFSM " ) // Another alternative
                    
            EXFEL_FSM_ON_EXCEPTION(onException) // This function will be called if an exception is triggered within the FSM

            EXFEL_FSM_NO_TRANSITION_V_ACTION(noStateTransition)
                    
            EXFEL_FSM_ON_CURRENT_STATE_CHANGE(updateCurrentState)

            /**************************************************************/
            /*                        Events                              */
            /**************************************************************/

            EXFEL_FSM_EVENT2(m_fsm, ErrorFoundEvent, onException, std::string, std::string)

            EXFEL_FSM_EVENT0(m_fsm, EndErrorEvent, endErrorEvent)

            EXFEL_FSM_EVENT0(m_fsm, GoToB, goToB);
            
            EXFEL_FSM_EVENT1(m_fsm, GoToA, goToA, int);
            
            EXFEL_FSM_EVENT0(m_fsm, GoToA1, goToA1);
            
            EXFEL_FSM_EVENT0(m_fsm, GoToD, goToD);

            /**************************************************************/
            /*                        States                              */
            /**************************************************************/

            EXFEL_FSM_STATE_V_EE(Error, errorStateOnEntry, errorStateOnExit)
            
            EXFEL_FSM_STATE_V_EE(Ok, okOnExit, okOnEntry)

            EXFEL_FSM_STATE_V_EE(A, aOnEntry, aOnExit)
            
            EXFEL_FSM_STATE_V_EE(A1, a1OnEntry, a1OnExit)
            
            //EXFEL_FSM_STATE_V_EE(B, bOnEntry, bOnExit)
            
            EXFEL_FSM_STATE(C)
            
            EXFEL_FSM_STATE(D)

            /**************************************************************/
            /*                    Transition Actions                      */
            /**************************************************************/

            EXFEL_FSM_V_ACTION0(A2BAction, a2BAction)
            
            EXFEL_FSM_V_ACTION0(A2A1Action, a2A1Action)
            
            EXFEL_FSM_V_ACTION1(B2AAction, b2AAction, int)

            EXFEL_FSM_V_ACTION2(ErrorFoundAction, errorFoundAction, std::string, std::string)

            EXFEL_FSM_V_ACTION0(EndErrorAction, endErrorAction)
            
            /**************************************************************/
            /*                        Guards                              */
            /**************************************************************/

            EXFEL_FSM_V_GUARD1(GoToAGuard, goToAGuard, int)
            
            
            /**************************************************************/
            /*                      Sub Machine                           */
            /**************************************************************/
            EXFEL_FSM_TABLE_BEGIN(BTransitionTable)
            Row< C, GoToD, D, none, none >
            EXFEL_FSM_TABLE_END
            
            EXFEL_FSM_STATE_MACHINE(B, BTransitionTable, C, Self)
            

            /**************************************************************/
            /*                      Top Machine                           */
            /**************************************************************/

            EXFEL_FSM_TABLE_BEGIN(TestDeviceMachineTransitionTable)
            //  Source-State    Event        Target-State    Action         Guard
            Row< A, GoToB, B, A2BAction, none >,
            Row< A, GoToA1, A1, A2A1Action, none>,
            Row< B, GoToA, A, none, GoToAGuard>,
            Row< Ok, ErrorFoundEvent, Error, ErrorFoundAction, none >,
            Row< Error, EndErrorEvent, Ok, EndErrorAction, none >
            EXFEL_FSM_TABLE_END

            //                            Name            Transition-Table                Initial-State                        Context 
            EXFEL_FSM_STATE_MACHINE(TestDeviceMachine, TestDeviceMachineTransitionTable, EXFEL_FSM_REGION(Ok, A), Self)

            void startStateMachine() {

                EXFEL_FSM_CREATE_MACHINE(TestDeviceMachine, m_fsm)

                EXFEL_FSM_SET_CONTEXT_TOP(this, m_fsm)
                        
                EXFEL_FSM_SET_CONTEXT_SUB(this, m_fsm, B);
                        
                EXFEL_FSM_START_MACHINE(m_fsm)
            }

            template <class T>
            static krb_log4cpp::Category& getLog() {
                return krb_log4cpp::Category::getInstance(T::classInfo().getLogCategory());
            }


        private: // Members

            EXFEL_FSM_DECLARE_MACHINE(TestDeviceMachine, m_fsm)

            krb_log4cpp::Category& m_log;
            
            std::ostream& m_log2;
        };

    }
}


