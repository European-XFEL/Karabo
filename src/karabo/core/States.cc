#include "States.hh"


using namespace std;


namespace karabo {
    namespace core {
        namespace states {
//            KARABO_REGISTER_IN_FACTORY(State, UNKNOWN)
//            KARABO_REGISTER_IN_FACTORY(State, KNOWN)
//            KARABO_REGISTER_IN_FACTORY(State, INIT)
//            KARABO_REGISTER_IN_FACTORY(State, DISABLED)
//            KARABO_REGISTER_IN_FACTORY(State, ERROR)
//            KARABO_REGISTER_IN_FACTORY(State, NORMAL)
//            KARABO_REGISTER_IN_FACTORY(State, STATIC)
//            KARABO_REGISTER_IN_FACTORY(State, CHANGING)
//            KARABO_REGISTER_IN_FACTORY(State, PASSIVE)
//            KARABO_REGISTER_IN_FACTORY(State, ACTIVE)
//            KARABO_REGISTER_IN_FACTORY(State, DECREASING)
//            KARABO_REGISTER_IN_FACTORY(State, INCREASING)
//            KARABO_REGISTER_IN_FACTORY(State, INTERLOCKED)
//            KARABO_REGISTER_IN_FACTORY(State, COOLED)
//            KARABO_REGISTER_IN_FACTORY(State, HEATED)
//            KARABO_REGISTER_IN_FACTORY(State, EVACUATED)
//            KARABO_REGISTER_IN_FACTORY(State, CLOSED)
//            KARABO_REGISTER_IN_FACTORY(State, ON)
//            KARABO_REGISTER_IN_FACTORY(State, EXTRACTED)
//            KARABO_REGISTER_IN_FACTORY(State, STARTED)
//            KARABO_REGISTER_IN_FACTORY(State, LOCKED)
//            KARABO_REGISTER_IN_FACTORY(State, ENGAGED)
//            KARABO_REGISTER_IN_FACTORY(State, WARM)
//            KARABO_REGISTER_IN_FACTORY(State, COLD)
//            KARABO_REGISTER_IN_FACTORY(State, PRESSURIZED)
//            KARABO_REGISTER_IN_FACTORY(State, OPENED)
//            KARABO_REGISTER_IN_FACTORY(State, OFF)
//            KARABO_REGISTER_IN_FACTORY(State, INSERTED)
//            KARABO_REGISTER_IN_FACTORY(State, STOPPED)
//            KARABO_REGISTER_IN_FACTORY(State, UNLOCKED)
//            KARABO_REGISTER_IN_FACTORY(State, DISENGAGED)
//            KARABO_REGISTER_IN_FACTORY(State, ROTATING)
//            KARABO_REGISTER_IN_FACTORY(State, MOVING)
//            KARABO_REGISTER_IN_FACTORY(State, SWITCHING)
//            KARABO_REGISTER_IN_FACTORY(State, HEATING)
//            KARABO_REGISTER_IN_FACTORY(State, MOVING_RIGHT)
//            KARABO_REGISTER_IN_FACTORY(State, MOVING_UP)
//            KARABO_REGISTER_IN_FACTORY(State, MOVING_FORWARD)
//            KARABO_REGISTER_IN_FACTORY(State, ROTATING_CLK)
//            KARABO_REGISTER_IN_FACTORY(State, RAMPING_UP)
//            KARABO_REGISTER_IN_FACTORY(State, INSERTING)
//            KARABO_REGISTER_IN_FACTORY(State, STARTING)
//            KARABO_REGISTER_IN_FACTORY(State, FILLING)
//            KARABO_REGISTER_IN_FACTORY(State, ENGAGING)
//            KARABO_REGISTER_IN_FACTORY(State, SWITCHING_ON)
//            KARABO_REGISTER_IN_FACTORY(State, COOLING)
//            KARABO_REGISTER_IN_FACTORY(State, MOVING_LEFT)
//            KARABO_REGISTER_IN_FACTORY(State, MOVING_DOWN)
//            KARABO_REGISTER_IN_FACTORY(State, MOVING_BACK)
//            KARABO_REGISTER_IN_FACTORY(State, ROTATING_CNTCLK)
//            KARABO_REGISTER_IN_FACTORY(State, RAMPING_DOWN)
//            KARABO_REGISTER_IN_FACTORY(State, EXTRACTING)
//            KARABO_REGISTER_IN_FACTORY(State, STOPPING)
//            KARABO_REGISTER_IN_FACTORY(State, EMPTYING)
//            KARABO_REGISTER_IN_FACTORY(State, DISENGAGING)
//            KARABO_REGISTER_IN_FACTORY(State, SWITCHING_OFF)
            
        }
        
        State::State(const std::string& stateName, const std::string& parentName) 
        : m_stateName(stateName)
        , m_parentState(parentName)
        , m_id(0) {
            const std::vector<State::Pointer>& v = states::stateSignifier.getTrumpList();
            for (size_t i = 0; i < v.size(); i++) {
                if (v[i]->name() == m_parentState) {
                    m_id = i + 1;
                    break;
                }
            }
        }
        
        namespace states {
            State StateSignifier::returnMostSignificant(const std::vector<State>& listOfStates) {
                State state;
                for (vector<State>::const_iterator ii = listOfStates.begin(); ii != listOfStates.end(); ++ii) {
                    if (ii->rank() > state.rank()) state = *ii;
                }
                return state;
            }
        }
        
    }
}
