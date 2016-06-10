#include "States.hh"


using namespace std;


namespace karabo {
    namespace core {
        namespace states {
            
//            KARABO_REGISTER_IN_FACTORY(BaseState, UNKNOWN)
//            KARABO_REGISTER_IN_FACTORY(BaseState, KNOWN)
//            KARABO_REGISTER_IN_FACTORY(BaseState, INIT)
//            KARABO_REGISTER_IN_FACTORY(BaseState, DISABLED)
//            KARABO_REGISTER_IN_FACTORY(BaseState, ERROR)
//            KARABO_REGISTER_IN_FACTORY(BaseState, NORMAL)
//            KARABO_REGISTER_IN_FACTORY(BaseState, STATIC)
//            KARABO_REGISTER_IN_FACTORY(BaseState, CHANGING)
//            KARABO_REGISTER_IN_FACTORY(BaseState, PASSIVE)
//            KARABO_REGISTER_IN_FACTORY(BaseState, ACTIVE)
//            KARABO_REGISTER_IN_FACTORY(BaseState, DECREASING)
//            KARABO_REGISTER_IN_FACTORY(BaseState, INCREASING)
//            KARABO_REGISTER_IN_FACTORY(BaseState, INTERLOCKED)
//            KARABO_REGISTER_IN_FACTORY(BaseState, COOLED)
//            KARABO_REGISTER_IN_FACTORY(BaseState, HEATED)
//            KARABO_REGISTER_IN_FACTORY(BaseState, EVACUATED)
//            KARABO_REGISTER_IN_FACTORY(BaseState, CLOSED)
//            KARABO_REGISTER_IN_FACTORY(BaseState, ON)
//            KARABO_REGISTER_IN_FACTORY(BaseState, EXTRACTED)
//            KARABO_REGISTER_IN_FACTORY(BaseState, STARTED)
//            KARABO_REGISTER_IN_FACTORY(BaseState, LOCKED)
//            KARABO_REGISTER_IN_FACTORY(BaseState, ENGAGED)
//            KARABO_REGISTER_IN_FACTORY(BaseState, WARM)
//            KARABO_REGISTER_IN_FACTORY(BaseState, COLD)
//            KARABO_REGISTER_IN_FACTORY(BaseState, PRESSURIZED)
//            KARABO_REGISTER_IN_FACTORY(BaseState, OPENED)
//            KARABO_REGISTER_IN_FACTORY(BaseState, OFF)
//            KARABO_REGISTER_IN_FACTORY(BaseState, INSERTED)
//            KARABO_REGISTER_IN_FACTORY(BaseState, STOPPED)
//            KARABO_REGISTER_IN_FACTORY(BaseState, UNLOCKED)
//            KARABO_REGISTER_IN_FACTORY(BaseState, DISENGAGED)
//            KARABO_REGISTER_IN_FACTORY(BaseState, ROTATING)
//            KARABO_REGISTER_IN_FACTORY(BaseState, MOVING)
//            KARABO_REGISTER_IN_FACTORY(BaseState, SWITCHING)
//            KARABO_REGISTER_IN_FACTORY(BaseState, HEATING)
//            KARABO_REGISTER_IN_FACTORY(BaseState, MOVING_RIGHT)
//            KARABO_REGISTER_IN_FACTORY(BaseState, MOVING_UP)
//            KARABO_REGISTER_IN_FACTORY(BaseState, MOVING_FORWARD)
//            KARABO_REGISTER_IN_FACTORY(BaseState, ROTATING_CLK)
//            KARABO_REGISTER_IN_FACTORY(BaseState, RAMPING_UP)
//            KARABO_REGISTER_IN_FACTORY(BaseState, INSERTING)
//            KARABO_REGISTER_IN_FACTORY(BaseState, STARTING)
//            KARABO_REGISTER_IN_FACTORY(BaseState, FILLING)
//            KARABO_REGISTER_IN_FACTORY(BaseState, ENGAGING)
//            KARABO_REGISTER_IN_FACTORY(BaseState, SWITCHING_ON)
//            KARABO_REGISTER_IN_FACTORY(BaseState, COOLING)
//            KARABO_REGISTER_IN_FACTORY(BaseState, MOVING_LEFT)
//            KARABO_REGISTER_IN_FACTORY(BaseState, MOVING_DOWN)
//            KARABO_REGISTER_IN_FACTORY(BaseState, MOVING_BACK)
//            KARABO_REGISTER_IN_FACTORY(BaseState, ROTATING_CNTCLK)
//            KARABO_REGISTER_IN_FACTORY(BaseState, RAMPING_DOWN)
//            KARABO_REGISTER_IN_FACTORY(BaseState, EXTRACTING)
//            KARABO_REGISTER_IN_FACTORY(BaseState, STOPPING)
//            KARABO_REGISTER_IN_FACTORY(BaseState, EMPTYING)
//            KARABO_REGISTER_IN_FACTORY(BaseState, DISENGAGING)
//            KARABO_REGISTER_IN_FACTORY(BaseState, SWITCHING_OFF)
            
            BaseState StateSignifier::returnMostSignificant(const std::vector<BaseState>& listOfStates) {
                BaseState state;
                for (vector<BaseState>::const_iterator ii = listOfStates.begin(); ii != listOfStates.end(); ++ii) {
                    if (ii->rank() > state.rank()) state = *ii;
                }
                return state;
            }
            
        }
        
        BaseState::Pointer createState(const std::string& stateName) {
            if (karabo::util::Factory<BaseState>::has(stateName))
                return karabo::util::Factory<BaseState>::create(stateName);
            return BaseState::Pointer();
        }
    }
}
