/* 
 * File:   StateSignifier.hh
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on May 31, 2016, 10:54 AM
 */

#ifndef KARABO_CORE_STATESIGNIFIER_HH
#define	KARABO_CORE_STATESIGNIFIER_HH

#include <iostream>
#include "State.hh"

namespace karabo {
    namespace core {
            

        struct StateSignifier {

            StateSignifier(const std::vector<karabo::core::State>& trumpList = std::vector<karabo::core::State>(),
                    const karabo::core::State& staticMoreSignificant = State::PASSIVE,
                    const karabo::core::State& changingMoreSignificant = State::DECREASING);
            
            StateSignifier(const karabo::core::State& staticMoreSignificant,
                    const karabo::core::State& changingMoreSignificant);

            State returnMostSignificant(const std::vector<State>& listOfStates);

            const std::vector<State>& getTrumpList() const {
                return m_trumpList;
            }

        private:

            size_t rankedAt(const State& sp);
            
            /**
             * Fill full list of ancestors for state, starting from state name itself:
             * state_name, parent_name, grand_parent_name, grand_grand_parent_name, ...
             * @param s    input state 
             * @param all  vector for accumulating of output list of ancestors
             */
            void fillAncestorNames_r(const State& s, std::vector<std::string>& all);
            
            
            const bool inList(const std::vector<State> & list, const State & s) const;
            
        protected:
            std::vector<State> m_trumpList;
        };
            
    }
}


#endif	/* KARABO_CORE_STATESIGNIFIER_HH */

