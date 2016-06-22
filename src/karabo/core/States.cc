#include <assert.h>
#include <karabo/util/StringTools.hh>
#include <karabo/util/Factory.hh>
#include "States.hh"


using namespace std;
using namespace karabo::util;

namespace karabo {
    namespace core {
        namespace states {
            
#define KARABO_INIT_FIXED_STATE(X) const State::Pointer X(new X ## _class);

            KARABO_INIT_FIXED_STATE(UNKNOWN)

            KARABO_INIT_FIXED_STATE(KNOWN)

            KARABO_INIT_FIXED_STATE(INIT)

            KARABO_INIT_FIXED_STATE(DISABLED)

            KARABO_INIT_FIXED_STATE(ERROR)

            KARABO_INIT_FIXED_STATE(NORMAL)

            KARABO_INIT_FIXED_STATE(STATIC)

            KARABO_INIT_FIXED_STATE(CHANGING)

            KARABO_INIT_FIXED_STATE(PASSIVE)

            KARABO_INIT_FIXED_STATE(ACTIVE)

            KARABO_INIT_FIXED_STATE(DECREASING)

            KARABO_INIT_FIXED_STATE(INCREASING)

            /**
             *
             */

            KARABO_INIT_FIXED_STATE(INTERLOCKED)

            KARABO_INIT_FIXED_STATE(COOLED)

            KARABO_INIT_FIXED_STATE(HEATED)

            KARABO_INIT_FIXED_STATE(EVACUATED)

            KARABO_INIT_FIXED_STATE(CLOSED)

            KARABO_INIT_FIXED_STATE(ON)

            KARABO_INIT_FIXED_STATE(EXTRACTED)

            KARABO_INIT_FIXED_STATE(STARTED)

            KARABO_INIT_FIXED_STATE(LOCKED)

            KARABO_INIT_FIXED_STATE(ENGAGED)


            KARABO_INIT_FIXED_STATE(WARM)

            KARABO_INIT_FIXED_STATE(COLD)

            KARABO_INIT_FIXED_STATE(PRESSURIZED)

            KARABO_INIT_FIXED_STATE(OPENED)

            KARABO_INIT_FIXED_STATE(OFF)

            KARABO_INIT_FIXED_STATE(INSERTED)

            KARABO_INIT_FIXED_STATE(STOPPED)

            KARABO_INIT_FIXED_STATE(UNLOCKED)

            KARABO_INIT_FIXED_STATE(DISENGAGED)


            KARABO_INIT_FIXED_STATE(ROTATING)

            KARABO_INIT_FIXED_STATE(MOVING)

            KARABO_INIT_FIXED_STATE(SWITCHING)


            KARABO_INIT_FIXED_STATE(HEATING)

            KARABO_INIT_FIXED_STATE(MOVING_RIGHT)

            KARABO_INIT_FIXED_STATE(MOVING_UP)

            KARABO_INIT_FIXED_STATE(MOVING_FORWARD)

            KARABO_INIT_FIXED_STATE(ROTATING_CLK)

            KARABO_INIT_FIXED_STATE(RAMPING_UP)

            KARABO_INIT_FIXED_STATE(INSERTING)

            KARABO_INIT_FIXED_STATE(STARTING)

            KARABO_INIT_FIXED_STATE(FILLING)

            KARABO_INIT_FIXED_STATE(ENGAGING)

            KARABO_INIT_FIXED_STATE(SWITCHING_ON)


            KARABO_INIT_FIXED_STATE(COOLING)

            KARABO_INIT_FIXED_STATE(MOVING_LEFT)

            KARABO_INIT_FIXED_STATE(MOVING_DOWN)

            KARABO_INIT_FIXED_STATE(MOVING_BACK)

            KARABO_INIT_FIXED_STATE(ROTATING_CNTCLK)

            KARABO_INIT_FIXED_STATE(RAMPING_DOWN)

            KARABO_INIT_FIXED_STATE(EXTRACTING)

            KARABO_INIT_FIXED_STATE(STOPPING)

            KARABO_INIT_FIXED_STATE(EMPTYING)

            KARABO_INIT_FIXED_STATE(DISENGAGING)

            KARABO_INIT_FIXED_STATE(SWITCHING_OFF)

#undef KARABO_INIT_FIXED_STATE
            
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
                    m_trumpList.push_back(DISABLED);
                    m_trumpList.push_back(INIT);

                    // Take care to compare the objects, not the pointers:
                    if (*staticMoreSignificant == *states::PASSIVE) {
                        m_trumpList.push_back(ACTIVE);
                        m_trumpList.push_back(PASSIVE);
                    } else if (*staticMoreSignificant == *states::ACTIVE) {
                        m_trumpList.push_back(PASSIVE);
                        m_trumpList.push_back(ACTIVE);
                    }

                    m_trumpList.push_back(STATIC);

                    if (*changingMoreSignificant == *states::DECREASING) {
                        m_trumpList.push_back(INCREASING);
                        m_trumpList.push_back(DECREASING);
                    } else if (*changingMoreSignificant == *states::INCREASING) {
                        m_trumpList.push_back(DECREASING);
                        m_trumpList.push_back(INCREASING);
                    }

                    m_trumpList.push_back(CHANGING);
                    m_trumpList.push_back(ERROR);
                    m_trumpList.push_back(UNKNOWN);
                } else {
                    m_trumpList = trumpList;
                }
            }


            StatesRegistrator::StatesRegistrator() {
#define REGISTER_STATE(X) if (!karabo::util::Factory<karabo::core::State>::has(#X)) karabo::util::Factory<karabo::core::State>::registerClass<X ## _class>(#X);
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
