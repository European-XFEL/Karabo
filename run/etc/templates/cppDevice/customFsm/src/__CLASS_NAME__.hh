/*
 * $Id$
 *
 * Author: <__EMAIL__>
 * 
 * Created on __DATE__
 *
 * Copyright (c) 2010-2013 European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO___CLASS_NAME_ALL_CAPS___HH
#define KARABO___CLASS_NAME_ALL_CAPS___HH


#include <karabo/karabo.hpp>

/**
 * The main Karabo namespace
 */
namespace karabo {

    class __CLASS_NAME__ : public karabo::core::Device<> {

    public:

        // Add reflection and version information to this class
        KARABO_CLASSINFO(__CLASS_NAME__, "__CLASS_NAME__", "1.2")

        /**
         * Necessary method as part of the factory/configuration system
         * @param expected Will contain a description of expected parameters for this device
         */
        static void expectedParameters(karabo::util::Schema& expected);

        /**
         * Constructor providing the initial configuration in form of a Hash object.
         * If this class is constructed using the configuration system the Hash object will
         * already be validated using the information of the expectedParameters function.
         * The configuration is provided in a key/value fashion. 
         */
        __CLASS_NAME__(const karabo::util::Hash& config);

        /**
         * The destructor will be called in case the device gets killed (i.e. the event-loop returns)
         */
        virtual ~__CLASS_NAME__();
        
        
        /**
         * This function acts as a hook and is called after an reconfiguration request was received,
         * but BEFORE this reconfiguration request is actually merged into this device's state.
         * 
         * The reconfiguration information is contained in the Hash object provided as an argument.
         * You have a chance to change the content of this Hash before it is merged into the device's current state.
         * 
         * NOTE: (a) The incomingReconfiguration was validated before
         *       (b) If you do not need to handle the reconfigured data, there is no need to implement this function.
         *           You can actually completely delete this function from the class in case not needed.
         *           The reconfiguration will automatically be applied to the current state.
         * @param incomingReconfiguration The reconfiguration information as was triggered externally
         */
        virtual void preReconfigure(karabo::util::Hash& incomingReconfiguration);
        
        
        /**
         * This function acts as a hook and is called after an reconfiguration request was received,
         * and AFTER this reconfiguration request got merged into this device's current state.
         * You may access any (updated or not) parameters using the usual getters and setters.
         * NOTE: You may just delete this function from the class in case not needed.
         * @code
         * int i = get<int>("myParam");
         * @endcode
         */
        virtual void postReconfigure();

        
        /** Potential user reactions on state-machine activity:
         * State-machines are triggered by events only, if no event occurs nothing happens
         * In case of an event-trigger the following sequential process will happen in this order:
         * 
         * 1) If a guard (boolean function) is defined it is evaluated. 
         *    Only if no guard is defined, or the defined one returns true, we continue.
         * 2) If a target state is defined, the source state is being left.
         * 3) If an action is defined the action is executed.
         * 4) If an target state is defined, the target state is being entered.
         *
         * 
         * Consequently, up to 5 potential "hooks" to the state-machine activity are available, 
         * which can be implemented in corresponding functions:
         * 
         * 1) onGuardCall
         * 2) onStateExit
         * 3) onActionCall
         * 4) onStateEntry
         * 
         * HINT: The ErrorFoundAction is standardized and inherited from the BaseFsm class (template default)
         */
        
        /**************************************************************/
        /*                        Events                              */
        /**************************************************************/

        KARABO_FSM_EVENT1(m_fsm, ErrorFoundEvent, errorFound, std::string)
                
        KARABO_FSM_EVENT0(m_fsm, ResetEvent, reset)

        KARABO_FSM_EVENT0(m_fsm, InjectEvent, inject)

        KARABO_FSM_EVENT0(m_fsm, UninjectEvent, uninject)

        
        /**************************************************************/
        /*                        States                              */
        /**************************************************************/

        KARABO_FSM_STATE(Uninjected)

        KARABO_FSM_STATE(Injected)

        KARABO_FSM_STATE(Error)

        /**************************************************************/
        /*                    Transition Actions                      */
        /**************************************************************/
                
        KARABO_FSM_V_ACTION1(ErrorFoundAction, errorFoundAction, std::string);

        KARABO_FSM_V_ACTION0(InjectAction, injectAction);

        KARABO_FSM_V_ACTION0(UninjectAction, uninjectAction);

        /**************************************************************/
        /*                      AllOkState Machine                    */

        /**************************************************************/

        KARABO_FSM_TABLE_BEGIN(OkStateTransitionTable)
        // Source-State, Event, Target-State, Action, Guard
        Row< Uninjected, InjectEvent, Injected, InjectAction, none >,
        Row< Injected, UninjectEvent, Uninjected, UninjectAction, none >
        KARABO_FSM_TABLE_END

        //                       Name      Transition-Table           Initial-State  Context
        KARABO_FSM_STATE_MACHINE(Ok, OkStateTransitionTable, Uninjected, Self)


        /**************************************************************/
        /*                      Top Machine                         */
        /**************************************************************/

        //  Source-State    Event        Target-State    Action         Guard

        KARABO_FSM_TABLE_BEGIN(StateMachineTransitionTable)
        Row< Ok, ErrorFoundEvent, Error, ErrorFoundAction, none >,
        Row< Error, ResetEvent, Ok, none, none >
        KARABO_FSM_TABLE_END


        //                       Name          Transition-Table             Initial-State Context
        KARABO_FSM_STATE_MACHINE(StateMachine, StateMachineTransitionTable, Ok, Self)


        void startFsm() {

            KARABO_FSM_CREATE_MACHINE(StateMachine, m_fsm);
            KARABO_FSM_SET_CONTEXT_TOP(this, m_fsm)
            KARABO_FSM_SET_CONTEXT_SUB(this, m_fsm, Ok)
            KARABO_FSM_START_MACHINE(m_fsm)

        }

    private: // members

        KARABO_FSM_DECLARE_MACHINE(StateMachine, m_fsm);

    };

}

#endif
