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
        namespace states {

            typedef State KrbState;
            

#define KARABO_FSM_DECLARE_FIXED_STATE(X, parent) \
            struct X : public State {\
                KARABO_CLASSINFO(X, #X, "1.0")\
                X() {setStateName(#X); setParentName(parent);}\
                virtual ~X() {}\
            };
        

            /**
             * Base (Meta) states.  It corresponds the pic from
             * "Karabo concept", Chapter 9, "STATES AND STATUSES"
             */

            KARABO_FSM_DECLARE_FIXED_STATE(UNKNOWN, "UNKNOWN")

            KARABO_FSM_DECLARE_FIXED_STATE(KNOWN, "KNOWN")

            KARABO_FSM_DECLARE_FIXED_STATE(INIT, "INIT")

            KARABO_FSM_DECLARE_FIXED_STATE(DISABLED, "KNOWN")

            KARABO_FSM_DECLARE_FIXED_STATE(ERROR, "KNOWN")

            KARABO_FSM_DECLARE_FIXED_STATE(NORMAL, "KNOWN")

            KARABO_FSM_DECLARE_FIXED_STATE(STATIC, "NORMAL")

            KARABO_FSM_DECLARE_FIXED_STATE(CHANGING, "NORMAL")

            KARABO_FSM_DECLARE_FIXED_STATE(PASSIVE, "STATIC")

            KARABO_FSM_DECLARE_FIXED_STATE(ACTIVE, "STATIC")

            KARABO_FSM_DECLARE_FIXED_STATE(DECREASING, "CHANGING")

            KARABO_FSM_DECLARE_FIXED_STATE(INCREASING, "CHANGING")

            /**
             *
             */

            KARABO_FSM_DECLARE_FIXED_STATE(INTERLOCKED, "DISABLED")

            KARABO_FSM_DECLARE_FIXED_STATE(COOLED, "ACTIVE")

            KARABO_FSM_DECLARE_FIXED_STATE(HEATED, "ACTIVE")

            KARABO_FSM_DECLARE_FIXED_STATE(EVACUATED, "ACTIVE")

            KARABO_FSM_DECLARE_FIXED_STATE(CLOSED, "ACTIVE")

            KARABO_FSM_DECLARE_FIXED_STATE(ON, "ACTIVE")

            KARABO_FSM_DECLARE_FIXED_STATE(EXTRACTED, "ACTIVE")

            KARABO_FSM_DECLARE_FIXED_STATE(STARTED, "ACTIVE")

            KARABO_FSM_DECLARE_FIXED_STATE(LOCKED, "ACTIVE")

            KARABO_FSM_DECLARE_FIXED_STATE(ENGAGED, "ACTIVE")


            KARABO_FSM_DECLARE_FIXED_STATE(WARM, "PASSIVE")

            KARABO_FSM_DECLARE_FIXED_STATE(COLD, "PASSIVE")

            KARABO_FSM_DECLARE_FIXED_STATE(PRESSURIZED, "PASSIVE")

            KARABO_FSM_DECLARE_FIXED_STATE(OPENED, "PASSIVE")

            KARABO_FSM_DECLARE_FIXED_STATE(OFF, "PASSIVE")

            KARABO_FSM_DECLARE_FIXED_STATE(INSERTED, "PASSIVE")

            KARABO_FSM_DECLARE_FIXED_STATE(STOPPED, "PASSIVE")

            KARABO_FSM_DECLARE_FIXED_STATE(UNLOCKED, "PASSIVE")

            KARABO_FSM_DECLARE_FIXED_STATE(DISENGAGED, "PASSIVE")


            KARABO_FSM_DECLARE_FIXED_STATE(ROTATING, "CHANGING")

            KARABO_FSM_DECLARE_FIXED_STATE(MOVING, "CHANGING")

            KARABO_FSM_DECLARE_FIXED_STATE(SWITCHING, "CHANGING")


            KARABO_FSM_DECLARE_FIXED_STATE(HEATING, "INCREASING")

            KARABO_FSM_DECLARE_FIXED_STATE(MOVING_RIGHT, "INCREASING")

            KARABO_FSM_DECLARE_FIXED_STATE(MOVING_UP, "INCREASING")

            KARABO_FSM_DECLARE_FIXED_STATE(MOVING_FORWARD, "INCREASING")

            KARABO_FSM_DECLARE_FIXED_STATE(ROTATING_CLK, "INCREASING")

            KARABO_FSM_DECLARE_FIXED_STATE(RAMPING_UP, "INCREASING")

            KARABO_FSM_DECLARE_FIXED_STATE(INSERTING, "INCREASING")

            KARABO_FSM_DECLARE_FIXED_STATE(STARTING, "INCREASING")

            KARABO_FSM_DECLARE_FIXED_STATE(FILLING, "INCREASING")

            KARABO_FSM_DECLARE_FIXED_STATE(ENGAGING, "INCREASING")

            KARABO_FSM_DECLARE_FIXED_STATE(SWITCHING_ON, "INCREASING")


            KARABO_FSM_DECLARE_FIXED_STATE(COOLING, "DECREASING")

            KARABO_FSM_DECLARE_FIXED_STATE(MOVING_LEFT, "DECREASING")

            KARABO_FSM_DECLARE_FIXED_STATE(MOVING_DOWN, "DECREASING")

            KARABO_FSM_DECLARE_FIXED_STATE(MOVING_BACK, "DECREASING")

            KARABO_FSM_DECLARE_FIXED_STATE(ROTATING_CNTCLK, "DECREASING")

            KARABO_FSM_DECLARE_FIXED_STATE(RAMPING_DOWN, "DECREASING")

            KARABO_FSM_DECLARE_FIXED_STATE(EXTRACTING, "DECREASING")

            KARABO_FSM_DECLARE_FIXED_STATE(STOPPING, "DECREASING")

            KARABO_FSM_DECLARE_FIXED_STATE(EMPTYING, "DECREASING")

            KARABO_FSM_DECLARE_FIXED_STATE(DISENGAGING, "DECREASING")

            KARABO_FSM_DECLARE_FIXED_STATE(SWITCHING_OFF, "DECREASING")

#undef KARABO_FSM_DECLARE_FIXED_STATE
            
            struct StateSignifier {

                StateSignifier(const std::vector<std::string>& trumpList = std::vector<std::string>(),
                        const std::string& staticSignificant = "PASSIVE",
                        const std::string& changingSignificant = "DECREASING") : m_trumpList() {
                    using namespace karabo::util;
                    
#define REGISTER_STATE(X) if (!Factory<State>::has(#X)) Factory<State>::registerClass<X>(#X);
                    
                    REGISTER_STATE(UNKNOWN)
                    REGISTER_STATE(KNOWN)
                    REGISTER_STATE(INIT)
                    REGISTER_STATE(DISABLED)
                    REGISTER_STATE(ERROR)
                    REGISTER_STATE(NORMAL)
                    REGISTER_STATE(STATIC)
                    REGISTER_STATE(CHANGING)
                    REGISTER_STATE(PASSIVE)
                    REGISTER_STATE(ACTIVE)
                    REGISTER_STATE(DECREASING)
                    REGISTER_STATE(INCREASING)
                    REGISTER_STATE(INTERLOCKED)
                    REGISTER_STATE(COOLED)
                    REGISTER_STATE(HEATED)
                    REGISTER_STATE(EVACUATED)
                    REGISTER_STATE(CLOSED)
                    REGISTER_STATE(ON)
                    REGISTER_STATE(EXTRACTED)
                    REGISTER_STATE(STARTED)
                    REGISTER_STATE(LOCKED)
                    REGISTER_STATE(ENGAGED)
                    REGISTER_STATE(WARM)
                    REGISTER_STATE(COLD)
                    REGISTER_STATE(PRESSURIZED)
                    REGISTER_STATE(OPENED)
                    REGISTER_STATE(OFF)
                    REGISTER_STATE(INSERTED)
                    REGISTER_STATE(STOPPED)
                    REGISTER_STATE(UNLOCKED)
                    REGISTER_STATE(DISENGAGED)
                    REGISTER_STATE(ROTATING)
                    REGISTER_STATE(MOVING)
                    REGISTER_STATE(SWITCHING)
                    REGISTER_STATE(HEATING)
                    REGISTER_STATE(MOVING_RIGHT)
                    REGISTER_STATE(MOVING_UP)
                    REGISTER_STATE(MOVING_FORWARD)
                    REGISTER_STATE(ROTATING_CLK)
                    REGISTER_STATE(RAMPING_UP)
                    REGISTER_STATE(INSERTING)
                    REGISTER_STATE(STARTING)
                    REGISTER_STATE(FILLING)
                    REGISTER_STATE(ENGAGING)
                    REGISTER_STATE(SWITCHING_ON)
                    REGISTER_STATE(COOLING)
                    REGISTER_STATE(MOVING_LEFT)
                    REGISTER_STATE(MOVING_DOWN)
                    REGISTER_STATE(MOVING_BACK)
                    REGISTER_STATE(ROTATING_CNTCLK)
                    REGISTER_STATE(RAMPING_DOWN)
                    REGISTER_STATE(EXTRACTING)
                    REGISTER_STATE(STOPPING)
                    REGISTER_STATE(EMPTYING)
                    REGISTER_STATE(DISENGAGING)
                    REGISTER_STATE(SWITCHING_OFF)

#undef REGISTER_STATE
                    
                    if (trumpList.empty()) {
                        if (!Factory<State>::has("DISABLED")) Factory<State>::registerClass<DISABLED>("DISABLED");
                        m_trumpList.push_back(Factory<State>::create("DISABLED"));
                        m_trumpList.push_back(Factory<State>::create("INIT"));
                        
                        if (staticSignificant == "PASSIVE") {
                            m_trumpList.push_back(Factory<State>::create("ACTIVE"));
                            m_trumpList.push_back(Factory<State>::create("PASSIVE"));
                        } else if (staticSignificant == "ACTIVE") {
                            m_trumpList.push_back(Factory<State>::create("PASSIVE"));
                            m_trumpList.push_back(Factory<State>::create("ACTIVE"));
                        }
                        
                        m_trumpList.push_back(Factory<State>::create("STATIC"));
                        
                        if (changingSignificant == "DECREASING") {
                            m_trumpList.push_back(Factory<State>::create("INCREASING"));
                            m_trumpList.push_back(Factory<State>::create("DECREASING"));
                        } else if (changingSignificant == "INCREASING") {
                            m_trumpList.push_back(Factory<State>::create("DECREASING"));
                            m_trumpList.push_back(Factory<State>::create("INCREASING"));
                        }
                        
                        m_trumpList.push_back(Factory<State>::create("CHANGING"));
                        m_trumpList.push_back(Factory<State>::create("ERROR"));
                        m_trumpList.push_back(Factory<State>::create("UNKNOWN"));
                    } else {
                        m_trumpList.clear();
                        for (size_t i = 0; i < trumpList.size(); i++) m_trumpList.push_back(Factory<State>::create(trumpList[i]));
                    }
                }

                State returnMostSignificant(const std::vector<State>& listOfStates);

                const std::vector<State::Pointer>& getTrumpList() const {
                    return m_trumpList;
                }

            protected:
                std::vector<State::Pointer> m_trumpList;
            };

            static StateSignifier stateSignifier;
        }
            
        State::Pointer createState(const std::string& stateName);
    }
}

//#define KARABO_FSM_STATE(X) struct X : karabo::core::states::X {};

#endif	/* STATES_HH */

