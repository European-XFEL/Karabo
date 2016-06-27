/* 
 * File:   States.hh
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on May 31, 2016, 10:54 AM
 */

#ifndef KARABO_CORE_STATES_HH
#define	KARABO_CORE_STATES_HH

#include <iostream>
#include "State.hh"

namespace karabo {
    namespace core {
            

        struct StateSignifier {

            StateSignifier(const std::vector<karabo::core::State>& trumpList = std::vector<karabo::core::State>(),
                    const karabo::core::State& staticMoreSignificant = State::PASSIVE,
                    const karabo::core::State& changingMoreSignificant = State::DECREASING);

            State returnMostSignificant(const std::vector<State>& listOfStates);

            const std::vector<State>& getTrumpList() const {
                return m_trumpList;
            }

        private:

            size_t rankedAt(const State& sp);
            void fillNames_p(const State& s, std::vector<std::string>& all);
            
        protected:
            std::vector<State> m_trumpList;
        };
            
    }
}


#endif	/* KARABO_CORE_STATES_HH */

