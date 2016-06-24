/* 
 * File:   States.hh
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on May 31, 2016, 10:54 AM
 */

#ifndef KARABO_CORE_STATES_HH
#define	KARABO_CORE_STATES_HH

#include <iostream>
#include "State.hh"

namespace karabo {
    namespace core {
            
        State::Pointer createState(const std::string& stateName);

        namespace states {
          
            // There is a special treatment of parent name "State"
#define KARABO_DECLARE_FIXED_BASE_STATE(X)\
            struct X ## _class : public State {\
                KARABO_CLASSINFO(X ## _class, #X, "1.0") \
                X ## _class() {setStateName(#X); setParent("State");}\
                virtual ~X ## _class() {}\
            };\
            extern const State::Pointer X;

            // Factory key for class "X ## _class" is X
#define KARABO_DECLARE_FIXED_STATE(X, Parent)\
            struct X ## _class : public Parent ## _class {\
                KARABO_CLASSINFO(X ## _class, #X, "1.0")\
                X ## _class() {setStateName(#X); setParent(#Parent);}\
                virtual ~X ## _class() {}\
            };\
            extern const State::Pointer X;

            /**
             * Base (Meta) states.  It corresponds the pic from
             * "Karabo concept", Chapter 9, "STATES AND STATUSES"
             */

            // The base states that have no parent:

            KARABO_DECLARE_FIXED_BASE_STATE(UNKNOWN)

            KARABO_DECLARE_FIXED_BASE_STATE(KNOWN)

            KARABO_DECLARE_FIXED_BASE_STATE(INIT)

            // The derived states with their parents:

            KARABO_DECLARE_FIXED_STATE(DISABLED, KNOWN)

            KARABO_DECLARE_FIXED_STATE(ERROR, KNOWN)

            KARABO_DECLARE_FIXED_STATE(NORMAL, KNOWN)

            KARABO_DECLARE_FIXED_STATE(STATIC, NORMAL)

            KARABO_DECLARE_FIXED_STATE(CHANGING, NORMAL)

            KARABO_DECLARE_FIXED_STATE(PASSIVE, STATIC)

            KARABO_DECLARE_FIXED_STATE(ACTIVE, STATIC)

            KARABO_DECLARE_FIXED_STATE(DECREASING, CHANGING)

            KARABO_DECLARE_FIXED_STATE(INCREASING, CHANGING)

            /**
             *
             */

            KARABO_DECLARE_FIXED_STATE(INTERLOCKED, DISABLED)

            KARABO_DECLARE_FIXED_STATE(COOLED, ACTIVE)

            KARABO_DECLARE_FIXED_STATE(HEATED, ACTIVE)

            KARABO_DECLARE_FIXED_STATE(EVACUATED, ACTIVE)

            KARABO_DECLARE_FIXED_STATE(CLOSED, ACTIVE)

            KARABO_DECLARE_FIXED_STATE(ON, ACTIVE)

            KARABO_DECLARE_FIXED_STATE(EXTRACTED, ACTIVE)

            KARABO_DECLARE_FIXED_STATE(STARTED, ACTIVE)

            KARABO_DECLARE_FIXED_STATE(LOCKED, ACTIVE)

            KARABO_DECLARE_FIXED_STATE(ENGAGED, ACTIVE)


            KARABO_DECLARE_FIXED_STATE(WARM, PASSIVE)

            KARABO_DECLARE_FIXED_STATE(COLD, PASSIVE)

            KARABO_DECLARE_FIXED_STATE(PRESSURIZED, PASSIVE)

            KARABO_DECLARE_FIXED_STATE(OPENED, PASSIVE)

            KARABO_DECLARE_FIXED_STATE(OFF, PASSIVE)

            KARABO_DECLARE_FIXED_STATE(INSERTED, PASSIVE)

            KARABO_DECLARE_FIXED_STATE(STOPPED, PASSIVE)

            KARABO_DECLARE_FIXED_STATE(UNLOCKED, PASSIVE)

            KARABO_DECLARE_FIXED_STATE(DISENGAGED, PASSIVE)


            KARABO_DECLARE_FIXED_STATE(ROTATING, CHANGING)

            KARABO_DECLARE_FIXED_STATE(MOVING, CHANGING)

            KARABO_DECLARE_FIXED_STATE(SWITCHING, CHANGING)


            KARABO_DECLARE_FIXED_STATE(HEATING, INCREASING)

            KARABO_DECLARE_FIXED_STATE(MOVING_RIGHT, INCREASING)

            KARABO_DECLARE_FIXED_STATE(MOVING_UP, INCREASING)

            KARABO_DECLARE_FIXED_STATE(MOVING_FORWARD, INCREASING)

            KARABO_DECLARE_FIXED_STATE(ROTATING_CLK, INCREASING)

            KARABO_DECLARE_FIXED_STATE(RAMPING_UP, INCREASING)

            KARABO_DECLARE_FIXED_STATE(INSERTING, INCREASING)

            KARABO_DECLARE_FIXED_STATE(STARTING, INCREASING)

            KARABO_DECLARE_FIXED_STATE(FILLING, INCREASING)

            KARABO_DECLARE_FIXED_STATE(ENGAGING, INCREASING)

            KARABO_DECLARE_FIXED_STATE(SWITCHING_ON, INCREASING)


            KARABO_DECLARE_FIXED_STATE(COOLING, DECREASING)

            KARABO_DECLARE_FIXED_STATE(MOVING_LEFT, DECREASING)

            KARABO_DECLARE_FIXED_STATE(MOVING_DOWN, DECREASING)

            KARABO_DECLARE_FIXED_STATE(MOVING_BACK, DECREASING)

            KARABO_DECLARE_FIXED_STATE(ROTATING_CNTCLK, DECREASING)

            KARABO_DECLARE_FIXED_STATE(RAMPING_DOWN, DECREASING)

            KARABO_DECLARE_FIXED_STATE(EXTRACTING, DECREASING)

            KARABO_DECLARE_FIXED_STATE(STOPPING, DECREASING)

            KARABO_DECLARE_FIXED_STATE(EMPTYING, DECREASING)

            KARABO_DECLARE_FIXED_STATE(DISENGAGING, DECREASING)

            KARABO_DECLARE_FIXED_STATE(SWITCHING_OFF, DECREASING)

#undef KARABO_DECLARE_FIXED_BASE_STATE
#undef KARABO_DECLARE_FIXED_STATE
            
            struct StateSignifier {

                StateSignifier(const std::vector<karabo::core::State::Pointer>& trumpList = std::vector<karabo::core::State::Pointer>(),
                        const karabo::core::State::Pointer& staticMoreSignificant = states::PASSIVE,
                        const karabo::core::State::Pointer& changingMoreSignificant = states::DECREASING);

                State::Pointer returnMostSignificant(const std::vector<State::Pointer>& listOfStates);

                const std::vector<State::Pointer>& getTrumpList() const {
                    return m_trumpList;
                }
                
            private:
                
                size_t rankedAt(const State::Pointer& sp);
                
            protected:
                std::vector<State::Pointer> m_trumpList;
            };
            
            struct StatesRegistrator {
                
                StatesRegistrator();
            };

            static StatesRegistrator statesRegistrator;
        }
    }
}


#endif	/* KARABO_CORE_STATES_HH */

