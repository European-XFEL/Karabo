/*
 * File:   StateSignifier.hh
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on May 31, 2016, 10:54 AM
 */

#ifndef KARABO_CORE_STATESIGNIFIER_HH
#define	KARABO_CORE_STATESIGNIFIER_HH

#include <iostream>
#include <karabo/util/State.hh>

namespace karabo {
    namespace util {

        struct StateSignifier {

            StateSignifier(const std::vector<karabo::util::State>& trumpList = std::vector<karabo::util::State>(),
                           const karabo::util::State& staticMoreSignificant = karabo::util::State::PASSIVE,
                           const karabo::util::State& changingMoreSignificant = karabo::util::State::DECREASING);

            StateSignifier(const karabo::util::State& staticMoreSignificant,
                           const karabo::util::State& changingMoreSignificant);

            karabo::util::State returnMostSignificant(const std::vector<karabo::util::State>& listOfStates);

            const std::vector<karabo::util::State>& getTrumpList() const {
                return m_trumpList;
            }

        private:

            size_t rankedAt(const karabo::util::State& sp);

            /**
             * Fill full list of ancestors for state, starting from state name itself:
             * state_name, parent_name, grand_parent_name, grand_grand_parent_name, ...
             * @param s    input state
             * @param all  vector for accumulating of output list of ancestors
             */
            void fillAncestorNames_r(const karabo::util::State& s, std::vector<std::string>& all);


            const bool inList(const std::vector<karabo::util::State> & list, const karabo::util::State & s) const;

            void initTrumpList(const std::vector<karabo::util::State>& trumpList,
                               const karabo::util::State& staticMoreSignificant,
                               const karabo::util::State& changingMoreSignificant);

        protected:
            std::vector<karabo::util::State> m_trumpList;
        };

    }
}


#endif	/* KARABO_CORE_STATESIGNIFIER_HH */

