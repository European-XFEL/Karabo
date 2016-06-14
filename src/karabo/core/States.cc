#include <assert.h>
#include "States.hh"


using namespace std;
using namespace karabo::util;

namespace karabo {
    namespace core {
        namespace states {


            State::Pointer StateSignifier::returnMostSignificant(const std::vector<State::Pointer>& listOfStates) {
                State::Pointer state(new State);
                for (vector<State::Pointer>::const_iterator ii = listOfStates.begin(); ii != listOfStates.end(); ++ii) {
                    State::Pointer sp = *ii;
                    if (sp->rank() > state->rank()) state = sp;
                }
                return state;
            }


            StateSignifier::StateSignifier(const std::vector<karabo::core::State::Pointer>& trumpList,
                                           const karabo::core::State::Pointer& staticMoreSignificant,
                                           const karabo::core::State::Pointer& changingMoreSignificant)
            : m_trumpList() {

                if (trumpList.empty()) {
                    if (!Factory<State>::has("DISABLED")) Factory<State>::registerClass<DISABLED>("DISABLED");
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
        }


        State::Pointer createState(const std::string& stateName) {
            assert(karabo::util::Factory<State>::has(stateName)); // runtime checking
            return karabo::util::Factory<State>::create(stateName);
        }
    }
}
