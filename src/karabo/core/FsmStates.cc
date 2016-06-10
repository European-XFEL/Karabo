#include "FsmStates.hh"


using namespace std;


namespace karabo {
    namespace core {
        namespace fsmstates {
            
//            KARABO_REGISTER_IN_FACTORY(MsmState, UNKNOWN)
//            KARABO_REGISTER_IN_FACTORY(MsmState, KNOWN)
//            KARABO_REGISTER_IN_FACTORY(MsmState, INIT)
//            KARABO_REGISTER_IN_FACTORY(MsmState, DISABLED)
//            KARABO_REGISTER_IN_FACTORY(MsmState, ERROR)
//            KARABO_REGISTER_IN_FACTORY(MsmState, NORMAL)
//            KARABO_REGISTER_IN_FACTORY(MsmState, STATIC)
//            KARABO_REGISTER_IN_FACTORY(MsmState, CHANGING)
//            KARABO_REGISTER_IN_FACTORY(MsmState, PASSIVE)
//            KARABO_REGISTER_IN_FACTORY(MsmState, ACTIVE)
//            KARABO_REGISTER_IN_FACTORY(MsmState, DECREASING)
//            KARABO_REGISTER_IN_FACTORY(MsmState, INCREASING)
//            KARABO_REGISTER_IN_FACTORY(MsmState, INTERLOCKED)
//            KARABO_REGISTER_IN_FACTORY(MsmState, COOLED)
//            KARABO_REGISTER_IN_FACTORY(MsmState, HEATED)
//            KARABO_REGISTER_IN_FACTORY(MsmState, EVACUATED)
//            KARABO_REGISTER_IN_FACTORY(MsmState, CLOSED)
//            KARABO_REGISTER_IN_FACTORY(MsmState, ON)
//            KARABO_REGISTER_IN_FACTORY(MsmState, EXTRACTED)
//            KARABO_REGISTER_IN_FACTORY(MsmState, STARTED)
//            KARABO_REGISTER_IN_FACTORY(MsmState, LOCKED)
//            KARABO_REGISTER_IN_FACTORY(MsmState, ENGAGED)
//            KARABO_REGISTER_IN_FACTORY(MsmState, WARM)
//            KARABO_REGISTER_IN_FACTORY(MsmState, COLD)
//            KARABO_REGISTER_IN_FACTORY(MsmState, PRESSURIZED)
//            KARABO_REGISTER_IN_FACTORY(MsmState, OPENED)
//            KARABO_REGISTER_IN_FACTORY(MsmState, OFF)
//            KARABO_REGISTER_IN_FACTORY(MsmState, INSERTED)
//            KARABO_REGISTER_IN_FACTORY(MsmState, STOPPED)
//            KARABO_REGISTER_IN_FACTORY(MsmState, UNLOCKED)
//            KARABO_REGISTER_IN_FACTORY(MsmState, DISENGAGED)
//            KARABO_REGISTER_IN_FACTORY(MsmState, ROTATING)
//            KARABO_REGISTER_IN_FACTORY(MsmState, MOVING)
//            KARABO_REGISTER_IN_FACTORY(MsmState, SWITCHING)
//            KARABO_REGISTER_IN_FACTORY(MsmState, HEATING)
//            KARABO_REGISTER_IN_FACTORY(MsmState, MOVING_RIGHT)
//            KARABO_REGISTER_IN_FACTORY(MsmState, MOVING_UP)
//            KARABO_REGISTER_IN_FACTORY(MsmState, MOVING_FORWARD)
//            KARABO_REGISTER_IN_FACTORY(MsmState, ROTATING_CLK)
//            KARABO_REGISTER_IN_FACTORY(MsmState, RAMPING_UP)
//            KARABO_REGISTER_IN_FACTORY(MsmState, INSERTING)
//            KARABO_REGISTER_IN_FACTORY(MsmState, STARTING)
//            KARABO_REGISTER_IN_FACTORY(MsmState, FILLING)
//            KARABO_REGISTER_IN_FACTORY(MsmState, ENGAGING)
//            KARABO_REGISTER_IN_FACTORY(MsmState, SWITCHING_ON)
//            KARABO_REGISTER_IN_FACTORY(MsmState, COOLING)
//            KARABO_REGISTER_IN_FACTORY(MsmState, MOVING_LEFT)
//            KARABO_REGISTER_IN_FACTORY(MsmState, MOVING_DOWN)
//            KARABO_REGISTER_IN_FACTORY(MsmState, MOVING_BACK)
//            KARABO_REGISTER_IN_FACTORY(MsmState, ROTATING_CNTCLK)
//            KARABO_REGISTER_IN_FACTORY(MsmState, RAMPING_DOWN)
//            KARABO_REGISTER_IN_FACTORY(MsmState, EXTRACTING)
//            KARABO_REGISTER_IN_FACTORY(MsmState, STOPPING)
//            KARABO_REGISTER_IN_FACTORY(MsmState, EMPTYING)
//            KARABO_REGISTER_IN_FACTORY(MsmState, DISENGAGING)
//            KARABO_REGISTER_IN_FACTORY(MsmState, SWITCHING_OFF)
            
            MsmState StateSignifier::returnMostSignificant(const std::vector<MsmState>& listOfStates) {
                MsmState state;
                for (vector<MsmState>::const_iterator ii = listOfStates.begin(); ii != listOfStates.end(); ++ii) {
                    if (ii->rank() > state.rank()) state = *ii;
                }
                return state;
            }
            
        }
    }
}

