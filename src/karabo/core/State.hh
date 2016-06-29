/* 
 * File:   State.hh
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on June 8, 2016, 2:49 PM
 */

#ifndef KARABO_CORE_STATE_HH
#define	KARABO_CORE_STATE_HH

#include <boost/shared_ptr.hpp>
#include <vector>
#include <string>
#include <karabo/util/ClassInfo.hh>
#include <karabo/util/Factory.hh>

namespace karabo {
    namespace core {

        /**
         * Base State class
         */
        class State {

        public:
            
            KARABO_CLASSINFO(State, "State", "1.0")

            virtual ~State() {
            }

            virtual const std::string& operator()() const {
                return m_stateName;
            }
            
            const std::string& name() const {
                return m_stateName;
            }
            
            const State* parent() const {
                return m_parent;
            }

            bool operator==(const State& state) const {
                // Just compare state names - parents do not matter.
                return m_stateName == state.m_stateName;
            }

            bool isCompatible(const State& s) const;
            
            // The base states that have no parent:

            static const State UNKNOWN;
            static const State KNOWN;
            static const State INIT;

            // The derived states with their parents:

            static const State DISABLED;
            static const State ERROR;
            static const State NORMAL;
            static const State STATIC;
            static const State CHANGING;
            static const State PASSIVE;
            static const State ACTIVE;
            static const State DECREASING;
            static const State INCREASING;

            /**
             *
             */

            static const State INTERLOCKED;
            static const State COOLED;
            static const State HEATED;
            static const State EVACUATED;
            static const State CLOSED;
            static const State ON;
            static const State EXTRACTED;
            static const State STARTED;
            static const State LOCKED;
            static const State ENGAGED;

            static const State WARM;
            static const State COLD;
            static const State PRESSURIZED;
            static const State OPENED;
            static const State OFF;
            static const State INSERTED;
            static const State STOPPED;
            static const State UNLOCKED;
            static const State DISENGAGED;

            static const State ROTATING;
            static const State MOVING;
            static const State SWITCHING;
            static const State HEATING;
            static const State MOVING_RIGHT;
            static const State MOVING_UP;
            static const State MOVING_FORWARD;
            static const State ROTATING_CLK;
            static const State RAMPING_UP;

            static const State INSERTING;
            static const State STARTING;
            static const State FILLING;
            static const State ENGAGING;
            static const State SWITCHING_ON;
            static const State COOLING;
            static const State MOVING_LEFT;

            static const State MOVING_DOWN;
            static const State MOVING_BACK;
            static const State ROTATING_CNTCLK;
            static const State RAMPING_DOWN;
            static const State EXTRACTING;
            static const State STOPPING;
            static const State EMPTYING;
            static const State DISENGAGING;
            static const State SWITCHING_OFF;

        private:
            // Private constructor to avoid states not in the predefined set (copy is OK).
            explicit State(const std::string& name, const State* parent = NULL);
            
            std::string m_stateName;
            const State* m_parent;

        };

    }
}

#endif	/* KARABO_CORE_STATE_HH */

