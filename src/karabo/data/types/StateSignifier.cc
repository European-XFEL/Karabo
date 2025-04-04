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
#include "StateSignifier.hh"

#include <algorithm>

#include "StringTools.hh"

using namespace std;

namespace karabo {
    namespace data {


        State StateSignifier::returnMostSignificant(const std::vector<State>& listOfStates) {
            if (listOfStates.empty())
                throw KARABO_PARAMETER_EXCEPTION("Empty list of states in StateSignifier::returnMostSignificant");

            const State* state = 0;
            size_t stateRank = 0;
            for (vector<State>::const_iterator ii = listOfStates.begin(); ii != listOfStates.end(); ++ii) {
                size_t rank = rankedAt(*ii);
                if (rank >= stateRank) {
                    state = &(*ii);
                    stateRank = rank;
                }
            }
            if (state) return *state;
            throw KARABO_PARAMETER_EXCEPTION(
                  "Wrong configuration: no states from input list are found in the trumplist!");
        }


        void StateSignifier::fillAncestorNames(const State& s, std::vector<std::string>& all) {
            all.push_back(s.name());
            State sCpy = s;
            while (sCpy.parent()) {
                sCpy = *sCpy.parent();
                all.push_back(sCpy.name());
            }
        }


        size_t StateSignifier::rankedAt(const State& s) {
            vector<string> allnames;
            fillAncestorNames(s, allnames); // fill array of state name and all its parent names
            for (vector<string>::const_iterator ii = allnames.begin(); ii != allnames.end(); ++ii) {
                for (size_t i = 0; i < m_trumpList.size(); ++i) {
                    if (*ii == m_trumpList[i].name()) return i + 1;
                }
            }
            return 0;
        }


        const bool StateSignifier::inList(const std::vector<State>& list, const State& s) const {
            return std::find(list.begin(), list.end(), s) != list.end();
        }


        StateSignifier::StateSignifier(const karabo::data::State& staticMoreSignificant,
                                       const karabo::data::State& changingMoreSignificant)
            : m_trumpList() {
            initTrumpList(std::vector<karabo::data::State>(), staticMoreSignificant, changingMoreSignificant);
        }


        StateSignifier::StateSignifier(const std::vector<karabo::data::State>& trumpList,
                                       const karabo::data::State& staticMoreSignificant,
                                       const karabo::data::State& changingMoreSignificant)
            : m_trumpList() {
            initTrumpList(trumpList, staticMoreSignificant, changingMoreSignificant);
        }


        void StateSignifier::initDefaultTrumpList(const karabo::data::State& staticMoreSignificant,
                                                  const karabo::data::State& changingMoreSignificant) {
            m_trumpList.push_back(State::DISABLED);

            m_trumpList.push_back(State::STATIC);

            // Take care to compare the objects, not the pointers:
            if (staticMoreSignificant == State::PASSIVE) {
                m_trumpList.push_back(State::ACTIVE);
                m_trumpList.push_back(State::PASSIVE);
            } else if (staticMoreSignificant == State::ACTIVE) {
                m_trumpList.push_back(State::PASSIVE);
                m_trumpList.push_back(State::ACTIVE);
            }

            m_trumpList.push_back(State::RUNNING);
            m_trumpList.push_back(State::PAUSED);

            m_trumpList.push_back(State::CHANGING);

            if (changingMoreSignificant == State::DECREASING) {
                m_trumpList.push_back(State::INCREASING);
                m_trumpList.push_back(State::DECREASING);
            } else if (changingMoreSignificant == State::INCREASING) {
                m_trumpList.push_back(State::DECREASING);
                m_trumpList.push_back(State::INCREASING);
            }

            m_trumpList.push_back(State::INTERLOCKED);
            m_trumpList.push_back(State::ERROR);
            m_trumpList.push_back(State::INIT);
            m_trumpList.push_back(State::UNKNOWN);
        }


        void StateSignifier::completeChangingSubstates(const karabo::data::State& changingMoreSignificant) {
            if (inList(m_trumpList, State::CHANGING)) {
                if (!inList(m_trumpList, State::INCREASING) && !inList(m_trumpList, State::DECREASING)) {
                    if (changingMoreSignificant == State::DECREASING) {
                        m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::CHANGING),
                                           State::INCREASING);
                        m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::CHANGING),
                                           State::DECREASING);
                    } else if (changingMoreSignificant == State::INCREASING) {
                        m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::CHANGING),
                                           State::DECREASING);
                        m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::CHANGING),
                                           State::INCREASING);
                    }
                } else if (!inList(m_trumpList, State::INCREASING)) {
                    m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::CHANGING),
                                       State::INCREASING);
                } else if (!inList(m_trumpList, State::DECREASING)) {
                    m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::CHANGING),
                                       State::DECREASING);
                }
            }
        }


        void StateSignifier::completeStaticSubstates(const karabo::data::State& staticMoreSignificant) {
            if (inList(m_trumpList, State::STATIC)) {
                if (!inList(m_trumpList, State::ACTIVE) && !inList(m_trumpList, State::PASSIVE)) {
                    if (staticMoreSignificant == State::PASSIVE) {
                        m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::STATIC),
                                           State::ACTIVE);
                        m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::STATIC),
                                           State::PASSIVE);
                    } else if (staticMoreSignificant == State::ACTIVE) {
                        m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::STATIC),
                                           State::PASSIVE);
                        m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::STATIC),
                                           State::ACTIVE);
                    }
                } else if (!inList(m_trumpList, State::ACTIVE)) {
                    m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::STATIC), State::ACTIVE);
                } else if (!inList(m_trumpList, State::PASSIVE)) {
                    m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::STATIC),
                                       State::PASSIVE);
                }
            }
        }


        void StateSignifier::completeKnownSubstates(const karabo::data::State& staticMoreSignificant,
                                                    const karabo::data::State& changingMoreSignificant) {
            if (inList(m_trumpList, State::KNOWN)) {
                if (!inList(m_trumpList, State::DISABLED)) {
                    m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::KNOWN),
                                       State::DISABLED);
                }

                if (!inList(m_trumpList, State::ACTIVE) && !inList(m_trumpList, State::PASSIVE)) {
                    if (staticMoreSignificant == State::PASSIVE) {
                        m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::KNOWN),
                                           State::ACTIVE);
                        m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::KNOWN),
                                           State::PASSIVE);
                    } else if (staticMoreSignificant == State::ACTIVE) {
                        m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::KNOWN),
                                           State::PASSIVE);
                        m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::KNOWN),
                                           State::ACTIVE);
                    }
                } else if (!inList(m_trumpList, State::ACTIVE)) {
                    m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::KNOWN), State::ACTIVE);
                } else if (!inList(m_trumpList, State::PASSIVE)) {
                    m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::KNOWN), State::PASSIVE);
                }

                if (!inList(m_trumpList, State::STATIC)) {
                    m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::KNOWN), State::STATIC);
                }

                if (!inList(m_trumpList, State::INCREASING) && !inList(m_trumpList, State::DECREASING)) {
                    if (changingMoreSignificant == State::DECREASING) {
                        m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::KNOWN),
                                           State::INCREASING);
                        m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::KNOWN),
                                           State::DECREASING);
                    } else if (changingMoreSignificant == State::INCREASING) {
                        m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::KNOWN),
                                           State::DECREASING);
                        m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::KNOWN),
                                           State::INCREASING);
                    }
                } else if (!inList(m_trumpList, State::INCREASING)) {
                    m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::KNOWN),
                                       State::INCREASING);
                } else if (!inList(m_trumpList, State::DECREASING)) {
                    m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::KNOWN),
                                       State::DECREASING);
                }

                if (!inList(m_trumpList, State::RUNNING)) {
                    m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::KNOWN), State::RUNNING);
                }
                if (!inList(m_trumpList, State::PAUSED)) {
                    m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::RUNNING),
                                       State::PAUSED);
                }
                if (!inList(m_trumpList, State::CHANGING)) {
                    m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::KNOWN),
                                       State::CHANGING);
                }
                if (!inList(m_trumpList, State::INTERLOCKED)) {
                    m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::KNOWN),
                                       State::INTERLOCKED);
                }
                if (!inList(m_trumpList, State::ERROR)) {
                    m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::KNOWN), State::ERROR);
                }
            }
        }


        void StateSignifier::initTrumpList(const std::vector<karabo::data::State>& trumpList,
                                           const karabo::data::State& staticMoreSignificant,
                                           const karabo::data::State& changingMoreSignificant) {
            if (trumpList.empty()) {
                initDefaultTrumpList(staticMoreSignificant, changingMoreSignificant);

            } else {
                m_trumpList = trumpList;
                completeChangingSubstates(changingMoreSignificant);
                completeStaticSubstates(staticMoreSignificant);
                completeKnownSubstates(staticMoreSignificant, changingMoreSignificant);
            }
        }

    } // namespace data
} // namespace karabo
