/*
 * File:   State.hh
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on June 8, 2016, 2:49 PM
 */

#ifndef _KARABO_UTIL_STATE_HH
#define	_KARABO_UTIL_STATE_HH

#include <boost/shared_ptr.hpp>
#include <vector>
#include <string>
#include <map>
#include "ClassInfo.hh"
#include "Factory.hh"

#define KARABO_INDICATE_STATE_SET "indicateState"

namespace karabo {
    namespace util {
        
        /**
         * @class State
         * @brief A class representing unified states accross the Karabo system.
         * 
         * This class holds all states known to Karabo as static data members.
         * States should always be accessed through this class. It's constructors
         * are fully private, meaning that no additional States can be constructed
         * outside of this class
         */
        class State {

        public:

            KARABO_CLASSINFO(State, "State", "1.0")

            //no inheritance from state!
            ~State() {
            }

            /**
             * Implicit conversion to strings is allowed
             * @return 
             */
            virtual const std::string& operator()() const {
                return m_stateName;
            }

            /**
             * Return the name of the state
             * @return 
             */
            const std::string& name() const {
                return m_stateName;
            }

            /**
             * Return the states parent in the state hierarchy in case it is a 
             * derived state
             * @return 
             */
            const State* parent() const {
                return m_parent;
            }

            /**
             * Check if two states are identical
             * @param state
             * @return 
             */
            bool operator==(const State& state) const {
                // Just compare state names - parents do not matter.
                return m_stateName == state.m_stateName;
            }

            /**
             * Check if two states are not identical
             * @param state
             * @return 
             */
            bool operator!=(const State& state) const {
                return (!this->operator==(state));
            }

            /**
             * Evaluate if this state is derived from another State s
             * @param s
             * @return 
             */
            bool isDerivedFrom(const State& s) const;

            // The base states that have no parent:

            static const State UNKNOWN;
            static const State KNOWN;
            static const State INIT;

            // The derived states with their parents:

            static const State DISABLED;
            static const State ERROR;
            static const State NORMAL;
            static const State STATIC;
            static const State RUNNING;
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
            static const State OPENING;
            static const State CLOSING;
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

            static const State HOMING;
            static const State IGNORING;

            static const State ACQUIRING;
            static const State PROCESSING;

            static const State MONITORING;

            static const State INTERLOCK_BROKEN;
            static const State INTERLOCK_OK;
            static const State SEARCHING;


            /**
             * Create a state from its string representation
             * @param state
             * @return 
             */
            static const State & fromString(const std::string & state);

        private:
            // Private constructor to avoid states not in the predefined set (copy is OK).
            explicit State(const std::string& name, const State* parent = NULL);

            std::string m_stateName;
            const State* m_parent;

            static std::map<std::string, const State &> m_stateFactory;


        };

    }
}

#endif	/* _KARABO_UTIL_STATE_HH */

