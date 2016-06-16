#include <assert.h>
#include <karabo/util/StringTools.hh>
#include "States.hh"


using namespace std;
using namespace karabo::util;

namespace karabo {
    namespace core {
        namespace states {
            
#define KARABO_FSM_INIT_FIXED_STATE(X, Y) State::Pointer X ## _p(new X); const State& g_ ## X = *X ## _p;

            KARABO_FSM_INIT_FIXED_STATE(UNKNOWN, State)

            KARABO_FSM_INIT_FIXED_STATE(KNOWN, State)

            KARABO_FSM_INIT_FIXED_STATE(INIT, State)

            KARABO_FSM_INIT_FIXED_STATE(DISABLED, KNOWN)

            KARABO_FSM_INIT_FIXED_STATE(ERROR, KNOWN)

            KARABO_FSM_INIT_FIXED_STATE(NORMAL, KNOWN)

            KARABO_FSM_INIT_FIXED_STATE(STATIC, NORMAL)

            KARABO_FSM_INIT_FIXED_STATE(CHANGING, NORMAL)

            KARABO_FSM_INIT_FIXED_STATE(PASSIVE, STATIC)

            KARABO_FSM_INIT_FIXED_STATE(ACTIVE, STATIC)

            KARABO_FSM_INIT_FIXED_STATE(DECREASING, CHANGING)

            KARABO_FSM_INIT_FIXED_STATE(INCREASING, CHANGING)

            /**
             *
             */

            KARABO_FSM_INIT_FIXED_STATE(INTERLOCKED, DISABLED)

            KARABO_FSM_INIT_FIXED_STATE(COOLED, ACTIVE)

            KARABO_FSM_INIT_FIXED_STATE(HEATED, ACTIVE)

            KARABO_FSM_INIT_FIXED_STATE(EVACUATED, ACTIVE)

            KARABO_FSM_INIT_FIXED_STATE(CLOSED, ACTIVE)

            KARABO_FSM_INIT_FIXED_STATE(ON, ACTIVE)

            KARABO_FSM_INIT_FIXED_STATE(EXTRACTED, ACTIVE)

            KARABO_FSM_INIT_FIXED_STATE(STARTED, ACTIVE)

            KARABO_FSM_INIT_FIXED_STATE(LOCKED, ACTIVE)

            KARABO_FSM_INIT_FIXED_STATE(ENGAGED, ACTIVE)


            KARABO_FSM_INIT_FIXED_STATE(WARM, PASSIVE)

            KARABO_FSM_INIT_FIXED_STATE(COLD, PASSIVE)

            KARABO_FSM_INIT_FIXED_STATE(PRESSURIZED, PASSIVE)

            KARABO_FSM_INIT_FIXED_STATE(OPENED, PASSIVE)

            KARABO_FSM_INIT_FIXED_STATE(OFF, PASSIVE)

            KARABO_FSM_INIT_FIXED_STATE(INSERTED, PASSIVE)

            KARABO_FSM_INIT_FIXED_STATE(STOPPED, PASSIVE)

            KARABO_FSM_INIT_FIXED_STATE(UNLOCKED, PASSIVE)

            KARABO_FSM_INIT_FIXED_STATE(DISENGAGED, PASSIVE)


            KARABO_FSM_INIT_FIXED_STATE(ROTATING, CHANGING)

            KARABO_FSM_INIT_FIXED_STATE(MOVING, CHANGING)

            KARABO_FSM_INIT_FIXED_STATE(SWITCHING, CHANGING)


            KARABO_FSM_INIT_FIXED_STATE(HEATING, INCREASING)

            KARABO_FSM_INIT_FIXED_STATE(MOVING_RIGHT, INCREASING)

            KARABO_FSM_INIT_FIXED_STATE(MOVING_UP, INCREASING)

            KARABO_FSM_INIT_FIXED_STATE(MOVING_FORWARD, INCREASING)

            KARABO_FSM_INIT_FIXED_STATE(ROTATING_CLK, INCREASING)

            KARABO_FSM_INIT_FIXED_STATE(RAMPING_UP, INCREASING)

            KARABO_FSM_INIT_FIXED_STATE(INSERTING, INCREASING)

            KARABO_FSM_INIT_FIXED_STATE(STARTING, INCREASING)

            KARABO_FSM_INIT_FIXED_STATE(FILLING, INCREASING)

            KARABO_FSM_INIT_FIXED_STATE(ENGAGING, INCREASING)

            KARABO_FSM_INIT_FIXED_STATE(SWITCHING_ON, INCREASING)


            KARABO_FSM_INIT_FIXED_STATE(COOLING, DECREASING)

            KARABO_FSM_INIT_FIXED_STATE(MOVING_LEFT, DECREASING)

            KARABO_FSM_INIT_FIXED_STATE(MOVING_DOWN, DECREASING)

            KARABO_FSM_INIT_FIXED_STATE(MOVING_BACK, DECREASING)

            KARABO_FSM_INIT_FIXED_STATE(ROTATING_CNTCLK, DECREASING)

            KARABO_FSM_INIT_FIXED_STATE(RAMPING_DOWN, DECREASING)

            KARABO_FSM_INIT_FIXED_STATE(EXTRACTING, DECREASING)

            KARABO_FSM_INIT_FIXED_STATE(STOPPING, DECREASING)

            KARABO_FSM_INIT_FIXED_STATE(EMPTYING, DECREASING)

            KARABO_FSM_INIT_FIXED_STATE(DISENGAGING, DECREASING)

            KARABO_FSM_INIT_FIXED_STATE(SWITCHING_OFF, DECREASING)

#undef KARABO_FSM_INIT_FIXED_STATE
            
            State::Pointer StateSignifier::returnMostSignificant(const std::vector<State::Pointer>& listOfStates) {
                const vector<State::Pointer>& trumplist = statesRegistrator.stateSignifier->getTrumpList();
                State::Pointer statePtr(new State);
                size_t stateRank = 0;
                for (vector<State::Pointer>::const_iterator ii = listOfStates.begin(); ii != listOfStates.end(); ++ii) {
                    State::Pointer sp = *ii;
                    size_t rank = rankedAt(sp, trumplist);
                    if (rank > stateRank) {
                        statePtr = sp;
                        stateRank = rank;
                    }
                }
                return statePtr;
            }

            size_t StateSignifier::rankedAt(const State::Pointer& sp, const std::vector<State::Pointer>& tl) {
                vector<string> allnames = sp->parents();        // copy parents list
                allnames.insert(allnames.begin(), sp->name());  // insert stateName at the beginning....
                for (vector<string>::const_iterator ii = allnames.begin(); ii != allnames.end(); ii++) {
                    for (size_t i = 0; i < tl.size(); i++) {
                        if (*ii == tl[i]->name()) return i+1;
                    }
                }
                std::cout << "Failed to find " << toString(allnames) << " in trump list" << std::endl;
                return 0;
            }
            
            StateSignifier::StateSignifier(const std::vector<karabo::core::State::Pointer>& trumpList,
                                           const karabo::core::State::Pointer& staticMoreSignificant,
                                           const karabo::core::State::Pointer& changingMoreSignificant)
            : m_trumpList() {

                if (trumpList.empty()) {
                    m_trumpList.push_back(createState("DISABLED"));
                    m_trumpList.push_back(createState("INIT"));

                    if (staticMoreSignificant->name() == "PASSIVE") {
                        m_trumpList.push_back(createState("ACTIVE"));
                        m_trumpList.push_back(createState("PASSIVE"));
                    } else if (staticMoreSignificant->name() == "ACTIVE") {
                        m_trumpList.push_back(createState("PASSIVE"));
                        m_trumpList.push_back(createState("ACTIVE"));
                    }

                    m_trumpList.push_back(createState("STATIC"));

                    if (changingMoreSignificant->name() == "DECREASING") {
                        m_trumpList.push_back(createState("INCREASING"));
                        m_trumpList.push_back(createState("DECREASING"));
                    } else if (changingMoreSignificant->name() == "INCREASING") {
                        m_trumpList.push_back(createState("DECREASING"));
                        m_trumpList.push_back(createState("INCREASING"));
                    }

                    m_trumpList.push_back(createState("CHANGING"));
                    m_trumpList.push_back(createState("ERROR"));
                    m_trumpList.push_back(createState("UNKNOWN"));
                } else {
                    m_trumpList.clear();
                    for (size_t i = 0; i < trumpList.size(); i++) m_trumpList.push_back(trumpList[i]);
                }
            }


            StatesRegistrator::StatesRegistrator() {
#define REGISTER_STATE(X) if (!karabo::util::Factory<karabo::core::State>::has(#X)) karabo::util::Factory<karabo::core::State>::registerClass<X>(#X);
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
                        
                stateSignifier = boost::shared_ptr<StateSignifier>(new StateSignifier);
            }
        
            State::Pointer returnMostSignificant(const std::vector<State::Pointer>& listOfStates) {
                return statesRegistrator.stateSignifier->returnMostSignificant(listOfStates);
            }
        }


        State::Pointer createState(const std::string& stateName) {
            assert(karabo::util::Factory<State>::has(stateName)); // runtime checking
            return karabo::util::Factory<State>::create(stateName);
        }
    }
}
