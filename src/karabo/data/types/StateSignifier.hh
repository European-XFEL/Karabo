/*
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */
/*
 * File:   StateSignifier.hh
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on May 31, 2016, 10:54 AM
 */

#ifndef KARABO_DATA_TYPES_STATESIGNIFIER_HH
#define KARABO_DATA_TYPES_STATESIGNIFIER_HH

#include <iostream>

#include "State.hh"

namespace karabo {
    namespace data {

        /**
         * @class StateSignifier
         * @brief The StateSignifier is used to evaluate the most significant State from a set of States
         */
        struct StateSignifier {
            /**
             * Create a StateSignifier
             * @param trumpList if set use this list to deviate from the standard signification order
             * @param staticMoreSignificant identify if in the State::STATIC regime State::PASSIVE or State::ACTIVE is
             * more significant
             * @param changingMoreSignificant identify if in the State::CHANGING regime State::INCREASING or
             * State::DECREASING is more significant
             */
            StateSignifier(const std::vector<karabo::data::State>& trumpList = std::vector<karabo::data::State>(),
                           const karabo::data::State& staticMoreSignificant = karabo::data::State::PASSIVE,
                           const karabo::data::State& changingMoreSignificant = karabo::data::State::DECREASING);

            /**
             * Create a StateSignifier
             * @param staticMoreSignificant identify if in the State::STATIC regime State::PASSIVE or State::ACTIVE is
             * more
             * @param changingMoreSignificant identify if in the State::CHANGING regime State::INCREASING or
             * State::DECREASING is more significant
             */
            StateSignifier(const karabo::data::State& staticMoreSignificant,
                           const karabo::data::State& changingMoreSignificant);

            /**
             * Return the most significant State from a list of States
             * @param listOfStates
             * @return
             */
            karabo::data::State returnMostSignificant(const std::vector<karabo::data::State>& listOfStates);

           private:
            size_t rankedAt(const karabo::data::State& sp);

            /**
             * Fill full list of ancestors for state, starting from state name itself:
             * state_name, parent_name, grand_parent_name, grand_grand_parent_name, ...
             * @param s    input state
             * @param all  vector for accumulating of output list of ancestors
             */
            void fillAncestorNames(const karabo::data::State& s, std::vector<std::string>& all);


            const bool inList(const std::vector<karabo::data::State>& list, const karabo::data::State& s) const;

            void initTrumpList(const std::vector<karabo::data::State>& trumpList,
                               const karabo::data::State& staticMoreSignificant,
                               const karabo::data::State& changingMoreSignificant);

            void initDefaultTrumpList(const karabo::data::State& staticMoreSignificant,
                                      const karabo::data::State& changingMoreSignificant);

            /**
             * Completes a non default trump list with some substates of CHANGING if that
             * list contains CHANGING.
             *
             * @param changingMoreSignificant which CHANGING substate is more significant between
             * INCREASING and DECREASING?
             */
            void completeChangingSubstates(const karabo::data::State& changingMoreSignificant);

            /**
             * Completes a non default trump list with some substates of STATIC if that
             * list contains STATIC.
             *
             * @param staticMoreSignificant which STATIC substate is more significant between
             * ACTIVE and PASSIVE?
             */
            void completeStaticSubstates(const karabo::data::State& staticMoreSignificant);

            /**
             * Completes a non default trump list with some substates of KNOWN if that
             * list contains KNOWN.
             *
             * @param staticgMoreSignificant which STATIC substate is more significant between
             * ACTIVE and PASSIVE?
             * @param changingMoreSignificant which CHANGING substate is more significant between
             * INCREASING and DECREASING?
             */
            void completeKnownSubstates(const karabo::data::State& staticMoreSignificant,
                                        const karabo::data::State& changingMoreSignificant);

           protected:
            std::vector<karabo::data::State> m_trumpList;
        };

    } // namespace data
} // namespace karabo


#endif /* KARABO_DATA_TYPES_STATESIGNIFIER_HH */
