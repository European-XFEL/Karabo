/* 
 * File:   States.hh
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on May 31, 2016, 10:54 AM
 */

#ifndef STATES_HH
#define	STATES_HH

#include <iostream>
#include <karabo/util/Factory.hh>
#include "State.hh"

namespace karabo {
    namespace core {
            
        State::Pointer createState(const std::string& stateName);

        namespace states {
          

#define KARABO_FSM_DECLARE_FIXED_STATE(X, Parent) \
            struct X : public Parent {\
                KARABO_CLASSINFO(X, #X, "1.0")\
                X() {setStateName(#X); setParent(#Parent);}\
                virtual ~X() {}\
            };\
            extern State::Pointer X ## _p;\
            extern const State& g_ ## X;
        

            /**
             * Base (Meta) states.  It corresponds the pic from
             * "Karabo concept", Chapter 9, "STATES AND STATUSES"
             */

            KARABO_FSM_DECLARE_FIXED_STATE(UNKNOWN, State)

            KARABO_FSM_DECLARE_FIXED_STATE(KNOWN, State)

            KARABO_FSM_DECLARE_FIXED_STATE(INIT, State)

            KARABO_FSM_DECLARE_FIXED_STATE(DISABLED, KNOWN)

            KARABO_FSM_DECLARE_FIXED_STATE(ERROR, KNOWN)

            KARABO_FSM_DECLARE_FIXED_STATE(NORMAL, KNOWN)

            KARABO_FSM_DECLARE_FIXED_STATE(STATIC, NORMAL)

            KARABO_FSM_DECLARE_FIXED_STATE(CHANGING, NORMAL)

            KARABO_FSM_DECLARE_FIXED_STATE(PASSIVE, STATIC)

            KARABO_FSM_DECLARE_FIXED_STATE(ACTIVE, STATIC)

            KARABO_FSM_DECLARE_FIXED_STATE(DECREASING, CHANGING)

            KARABO_FSM_DECLARE_FIXED_STATE(INCREASING, CHANGING)

            /**
             *
             */

            KARABO_FSM_DECLARE_FIXED_STATE(INTERLOCKED, DISABLED)

            KARABO_FSM_DECLARE_FIXED_STATE(COOLED, ACTIVE)

            KARABO_FSM_DECLARE_FIXED_STATE(HEATED, ACTIVE)

            KARABO_FSM_DECLARE_FIXED_STATE(EVACUATED, ACTIVE)

            KARABO_FSM_DECLARE_FIXED_STATE(CLOSED, ACTIVE)

            KARABO_FSM_DECLARE_FIXED_STATE(ON, ACTIVE)

            KARABO_FSM_DECLARE_FIXED_STATE(EXTRACTED, ACTIVE)

            KARABO_FSM_DECLARE_FIXED_STATE(STARTED, ACTIVE)

            KARABO_FSM_DECLARE_FIXED_STATE(LOCKED, ACTIVE)

            KARABO_FSM_DECLARE_FIXED_STATE(ENGAGED, ACTIVE)


            KARABO_FSM_DECLARE_FIXED_STATE(WARM, PASSIVE)

            KARABO_FSM_DECLARE_FIXED_STATE(COLD, PASSIVE)

            KARABO_FSM_DECLARE_FIXED_STATE(PRESSURIZED, PASSIVE)

            KARABO_FSM_DECLARE_FIXED_STATE(OPENED, PASSIVE)

            KARABO_FSM_DECLARE_FIXED_STATE(OFF, PASSIVE)

            KARABO_FSM_DECLARE_FIXED_STATE(INSERTED, PASSIVE)

            KARABO_FSM_DECLARE_FIXED_STATE(STOPPED, PASSIVE)

            KARABO_FSM_DECLARE_FIXED_STATE(UNLOCKED, PASSIVE)

            KARABO_FSM_DECLARE_FIXED_STATE(DISENGAGED, PASSIVE)


            KARABO_FSM_DECLARE_FIXED_STATE(ROTATING, CHANGING)

            KARABO_FSM_DECLARE_FIXED_STATE(MOVING, CHANGING)

            KARABO_FSM_DECLARE_FIXED_STATE(SWITCHING, CHANGING)


            KARABO_FSM_DECLARE_FIXED_STATE(HEATING, INCREASING)

            KARABO_FSM_DECLARE_FIXED_STATE(MOVING_RIGHT, INCREASING)

            KARABO_FSM_DECLARE_FIXED_STATE(MOVING_UP, INCREASING)

            KARABO_FSM_DECLARE_FIXED_STATE(MOVING_FORWARD, INCREASING)

            KARABO_FSM_DECLARE_FIXED_STATE(ROTATING_CLK, INCREASING)

            KARABO_FSM_DECLARE_FIXED_STATE(RAMPING_UP, INCREASING)

            KARABO_FSM_DECLARE_FIXED_STATE(INSERTING, INCREASING)

            KARABO_FSM_DECLARE_FIXED_STATE(STARTING, INCREASING)

            KARABO_FSM_DECLARE_FIXED_STATE(FILLING, INCREASING)

            KARABO_FSM_DECLARE_FIXED_STATE(ENGAGING, INCREASING)

            KARABO_FSM_DECLARE_FIXED_STATE(SWITCHING_ON, INCREASING)


            KARABO_FSM_DECLARE_FIXED_STATE(COOLING, DECREASING)

            KARABO_FSM_DECLARE_FIXED_STATE(MOVING_LEFT, DECREASING)

            KARABO_FSM_DECLARE_FIXED_STATE(MOVING_DOWN, DECREASING)

            KARABO_FSM_DECLARE_FIXED_STATE(MOVING_BACK, DECREASING)

            KARABO_FSM_DECLARE_FIXED_STATE(ROTATING_CNTCLK, DECREASING)

            KARABO_FSM_DECLARE_FIXED_STATE(RAMPING_DOWN, DECREASING)

            KARABO_FSM_DECLARE_FIXED_STATE(EXTRACTING, DECREASING)

            KARABO_FSM_DECLARE_FIXED_STATE(STOPPING, DECREASING)

            KARABO_FSM_DECLARE_FIXED_STATE(EMPTYING, DECREASING)

            KARABO_FSM_DECLARE_FIXED_STATE(DISENGAGING, DECREASING)

            KARABO_FSM_DECLARE_FIXED_STATE(SWITCHING_OFF, DECREASING)

#undef KARABO_FSM_DECLARE_FIXED_STATE
            
            struct StateSignifier {

                StateSignifier(const std::vector<karabo::core::State::Pointer>& trumpList = std::vector<karabo::core::State::Pointer>(),
                        const karabo::core::State::Pointer& staticMoreSignificant = createState("PASSIVE"),
                        const karabo::core::State::Pointer& changingMoreSignificant = createState("DECREASING"));

                State::Pointer returnMostSignificant(const std::vector<State::Pointer>& listOfStates);

                const std::vector<State::Pointer>& getTrumpList() const {
                    return m_trumpList;
                }
                
            private:
                
                size_t rankedAt(const State::Pointer& sp, const std::vector<State::Pointer>& tl);
                
            protected:
                std::vector<State::Pointer> m_trumpList;
            };
            
            struct StatesRegistrator {
                
                StatesRegistrator();
                boost::shared_ptr<StateSignifier> stateSignifier;
            };

            static StatesRegistrator statesRegistrator;
            
            extern State::Pointer returnMostSignificant(const std::vector<State::Pointer>& listOfStates);
        }
    }
}

//  Use macro CREATE_STATE to check validity of the given state at compile time
// State::Pointer cooling;
// CREATE_STATE(cooling, Cooling)

#define CREATE_STATE(x, X) { static struct X: public karabo::core::states::X {} s_ ## X; x = karabo::core::createState(#X); }

#endif	/* STATES_HH */

